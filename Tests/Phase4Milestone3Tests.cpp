#include <algorithm>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <source_location>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraphBuilder.h"
#include "MiliastraPlusPlusGraphIRJson.h"

using namespace MiliastraPlusPlus;
using namespace MiliastraPlusPlus::GraphIRJson;

#ifdef MILIASTRA_PHASE4_TEST_ACCESS
namespace MiliastraPlusPlus
{
    struct GraphBuilderPhase4TestAccess final
    {
        static void SetNextRegionIdentifier(GraphBuilder& Builder, std::uint64_t Identifier)
        {
            Builder.m_NextExecutionRegionIdentifier = Identifier;
        }
    };
}
#endif

namespace
{
    [[noreturn]] void Fail(const char* Expression, const std::source_location& Location)
    {
        std::fprintf(stderr, "Check failed: %s (%s:%u)\n", Expression,
            Location.file_name(), Location.line());
        std::abort();
    }

    void Check(bool Condition, const char* Expression,
        const std::source_location& Location = std::source_location::current())
    {
        if (!Condition)
        {
            Fail(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr NodeDescriptorId EntryId(5001U);
    constexpr NodeDescriptorId SequenceId(5002U);
    constexpr NodeDescriptorId BranchId(5003U);
    constexpr NodeDescriptorId JoinId(5004U);
    constexpr NodeDescriptorId ConditionalLoopId(5005U);
    constexpr NodeDescriptorId UnconditionalLoopId(5006U);
    constexpr NodeDescriptorId BooleanExpressionId(5007U);

    PinSchema Flow(const char* Name, PinDirection Direction,
        PinCardinality Cardinality = PinCardinality::Single)
    {
        return PinSchema(Name, TypeDesc::Flow(), Direction, PinCategory::Execution,
            Cardinality);
    }

    NodeDescriptor EntryDescriptor()
    {
        return NodeDescriptor(EntryId, "Root", {NodeAvailability::Server},
            {Flow("not a semantic name", PinDirection::Output)},
            EntryControlSchema{PinIndex(0U)});
    }

    NodeDescriptor SequenceDescriptor()
    {
        return NodeDescriptor(SequenceId, "Step", {NodeAvailability::Server},
            {Flow("in", PinDirection::Input),
             Flow("out", PinDirection::Output),
             PinSchema("bool result", TypeDesc::Boolean(), PinDirection::Output,
                 PinCategory::Data),
             PinSchema("bool data", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true)},
            SequenceControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor BranchDescriptor()
    {
        return NodeDescriptor(BranchId, "TwoWay", {NodeAvailability::Server},
            {Flow("in", PinDirection::Input),
             PinSchema("condition", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true),
             Flow("arbitrary true label", PinDirection::Output),
             Flow("arbitrary false label", PinDirection::Output)},
            BranchControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U)});
    }

    NodeDescriptor JoinDescriptor()
    {
        return NodeDescriptor(JoinId, "Join", {NodeAvailability::Server},
            {Flow("many", PinDirection::Input, PinCardinality::Multiple),
             Flow("one", PinDirection::Output)},
            JoinControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor ConditionalLoopDescriptor()
    {
        return NodeDescriptor(ConditionalLoopId, "ConditionalLoop", {NodeAvailability::Server},
            {Flow("execution", PinDirection::Input),
             Flow("body", PinDirection::Output),
             Flow("exit", PinDirection::Output),
             Flow("repeat", PinDirection::Input, PinCardinality::Multiple),
             Flow("break", PinDirection::Input, PinCardinality::Multiple),
             PinSchema("test", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U),
                PinIndex(4U), LoopExitPolicy::Conditional, PinIndex(5U)});
    }

    NodeDescriptor UnconditionalLoopDescriptor()
    {
        return NodeDescriptor(UnconditionalLoopId, "UnconditionalLoop", {NodeAvailability::Server},
            {Flow("execution", PinDirection::Input),
             Flow("body", PinDirection::Output),
             Flow("exit", PinDirection::Output),
             Flow("repeat", PinDirection::Input, PinCardinality::Multiple),
             Flow("break", PinDirection::Input, PinCardinality::Multiple)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U),
                PinIndex(4U), LoopExitPolicy::Unconditional, std::nullopt});
    }

    NodeDescriptor BooleanExpressionDescriptor()
    {
        return NodeDescriptor(BooleanExpressionId, "BooleanExpression", {NodeAvailability::Server},
            {PinSchema("input", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true),
             PinSchema("output", TypeDesc::Boolean(), PinDirection::Output,
                 PinCategory::Data)});
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Registry;
        MPP_CHECK(Registry.Register(EntryDescriptor()).has_value());
        MPP_CHECK(Registry.Register(SequenceDescriptor()).has_value());
        MPP_CHECK(Registry.Register(BranchDescriptor()).has_value());
        MPP_CHECK(Registry.Register(JoinDescriptor()).has_value());
        MPP_CHECK(Registry.Register(ConditionalLoopDescriptor()).has_value());
        MPP_CHECK(Registry.Register(UnconditionalLoopDescriptor()).has_value());
        MPP_CHECK(Registry.Register(BooleanExpressionDescriptor()).has_value());
        return Registry;
    }

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            if (Diagnostic.Code == Code)
            {
                return true;
            }
        }
        return false;
    }

    LiteralValue Boolean(bool Value)
    {
        return LiteralValue(LiteralValue::Data{Value});
    }

    ValueOrExpr<bool> BooleanExpression(bool Value)
    {
        return ValueOrExpr<bool>(Boolean(Value));
    }

    void TestConditionalLoopAndRoundTrip(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Prefix = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Prefix.has_value());
        auto Condition = Builder.GetOutput<bool>(Prefix->Node, PinIndex(2U));
        MPP_CHECK(Condition.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Prefix->Output, ConditionalLoopId,
            ValueOrExpr<bool>(*Condition));
        MPP_CHECK(Loop.has_value());
        MPP_CHECK(Loop->Node.GetIdentifier() == NodeInstanceId(3U));
        MPP_CHECK(Loop->BodyOutput.GetSourceOutputPin() == PinIndex(1U));
        MPP_CHECK(Loop->BodyOutput.GetRegion() == ExecutionRegionId(2U));
        auto Body = Builder.AppendExecutionNode(Loop->Scope, Loop->BodyOutput, SequenceId);
        MPP_CHECK(Body.has_value());
        auto ClosedLoop = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(ClosedLoop.has_value());
        MPP_CHECK(ClosedLoop->ExitOutput.has_value());
        MPP_CHECK(ClosedLoop->ExitOutput->GetSourceNode() == Loop->Node.GetIdentifier());
        MPP_CHECK(ClosedLoop->ExitOutput->GetSourceOutputPin() == PinIndex(2U));
        MPP_CHECK(ClosedLoop->ExitOutput->GetRegion() == ExecutionRegionId(1U));
        auto Suffix = Builder.AppendExecutionNode(Entry->Scope,
            *ClosedLoop->ExitOutput, SequenceId);
        MPP_CHECK(Suffix.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        MPP_CHECK(Graph->GetExecutionRegions().size() == 2U);
        MPP_CHECK(Graph->GetExecutionRegions()[1U].Kind == ExecutionRegionKind::LoopBody);
        MPP_CHECK(Graph->GetExecutionRegions()[1U].OwnerNode == Loop->Node.GetIdentifier());
        MPP_CHECK(Graph->GetExecutionRegions()[1U].OwnerOutputPin == PinIndex(1U));
        MPP_CHECK(Graph->GetControlEdges().size() == 4U);
        MPP_CHECK(Graph->GetInputBinding(Loop->Node.GetIdentifier(), PinIndex(5U)) != nullptr);

        const nlohmann::json First = Serialize(*Graph);
        const nlohmann::json Second = Serialize(*Graph);
        MPP_CHECK(First == Second);
        MPP_CHECK(First["irVersion"] == 3U);
        const auto RoundTrip = Deserialize(First);
        MPP_CHECK(RoundTrip.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*RoundTrip, Registry).empty());
        MPP_CHECK(Serialize(*RoundTrip) == First);
    }

    void TestExplicitContinueAndBreak(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder ConditionalBuilder(Registry);
        auto ConditionalEntry = ConditionalBuilder.BeginEntry(EntryId);
        MPP_CHECK(ConditionalEntry.has_value());
        auto ConditionalLoop = ConditionalBuilder.BeginLoop(ConditionalEntry->Scope,
            ConditionalEntry->RootOutput, ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(ConditionalLoop.has_value());
        auto ConditionalBody = ConditionalBuilder.AppendExecutionNode(ConditionalLoop->Scope,
            ConditionalLoop->BodyOutput, SequenceId);
        MPP_CHECK(ConditionalBody.has_value());
        MPP_CHECK(ConditionalBuilder.Continue(ConditionalLoop->Scope,
            ConditionalBody->Output).has_value());
        auto ConditionalResult = ConditionalBuilder.EndLoop(std::move(ConditionalLoop->Scope));
        MPP_CHECK(ConditionalResult.has_value() && ConditionalResult->ExitOutput.has_value());
        MPP_CHECK(ConditionalBuilder.EndEntry(std::move(ConditionalEntry->Scope)).has_value());
        auto ConditionalGraph = std::move(ConditionalBuilder).Finalize();
        MPP_CHECK(ConditionalGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*ConditionalGraph, Registry).empty());
        MPP_CHECK(ConditionalGraph->GetControlEdges().back().DestinationInputPin == PinIndex(3U));

        GraphBuilder UnconditionalBuilder(Registry);
        auto UnconditionalEntry = UnconditionalBuilder.BeginEntry(EntryId);
        MPP_CHECK(UnconditionalEntry.has_value());
        auto Prefix = UnconditionalBuilder.AppendExecutionNode(UnconditionalEntry->Scope,
            UnconditionalEntry->RootOutput, SequenceId);
        MPP_CHECK(Prefix.has_value());
        auto PrefixValue = UnconditionalBuilder.GetOutput<bool>(Prefix->Node, PinIndex(2U));
        MPP_CHECK(PrefixValue.has_value());
        auto UnconditionalLoop = UnconditionalBuilder.BeginLoop(UnconditionalEntry->Scope,
            Prefix->Output, UnconditionalLoopId);
        MPP_CHECK(UnconditionalLoop.has_value());
        auto UnconditionalBody = UnconditionalBuilder.AppendExecutionNode(
            UnconditionalLoop->Scope, UnconditionalLoop->BodyOutput, SequenceId);
        MPP_CHECK(UnconditionalBody.has_value());
        MPP_CHECK(UnconditionalBuilder.Break(UnconditionalLoop->Scope,
            UnconditionalBody->Output).has_value());
        auto ReusedBreak = UnconditionalBuilder.Break(UnconditionalLoop->Scope,
            UnconditionalBody->Output);
        MPP_CHECK(!ReusedBreak.has_value());
        MPP_CHECK(HasCode(ReusedBreak.error(), DiagnosticCode::ExecutionEndpointAlreadyConsumed));
        auto UnconditionalResult = UnconditionalBuilder.EndLoop(std::move(UnconditionalLoop->Scope));
        MPP_CHECK(UnconditionalResult.has_value() && UnconditionalResult->ExitOutput.has_value());
        auto After = UnconditionalBuilder.AppendExecutionNode(UnconditionalEntry->Scope,
            *UnconditionalResult->ExitOutput, SequenceId);
        MPP_CHECK(After.has_value());
        MPP_CHECK(UnconditionalBuilder.BindInput(After->Node, PinIndex(3U),
            ValueOrExpr<bool>(*PrefixValue)).has_value());
        MPP_CHECK(UnconditionalBuilder.EndEntry(std::move(UnconditionalEntry->Scope)).has_value());
        auto UnconditionalGraph = std::move(UnconditionalBuilder).Finalize();
        MPP_CHECK(UnconditionalGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*UnconditionalGraph, Registry).empty());
        bool HasBreakEdge = false;
        for (const ControlEdge& Edge : UnconditionalGraph->GetControlEdges())
        {
            HasBreakEdge = HasBreakEdge || (Edge.DestinationInputPin == PinIndex(4U) &&
                Edge.DestinationNode == UnconditionalLoop->Node.GetIdentifier());
        }
        MPP_CHECK(HasBreakEdge);
    }

    void TestNaturalCompletionAndRetry(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            UnconditionalLoopId);
        MPP_CHECK(Loop.has_value());
        auto EmptyEnd = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(!EmptyEnd.has_value());
        MPP_CHECK(Loop->Scope.IsValid());
        auto Body = Builder.AppendExecutionNode(Loop->Scope, Loop->BodyOutput, SequenceId);
        MPP_CHECK(Body.has_value());
        auto NaturalEnd = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(NaturalEnd.has_value());
        MPP_CHECK(!NaturalEnd->ExitOutput.has_value());
        auto StaleTransfer = Builder.Continue(Loop->Scope, Body->Output);
        MPP_CHECK(!StaleTransfer.has_value());
        MPP_CHECK(HasCode(StaleTransfer.error(), DiagnosticCode::InvalidExecutionScope));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        MPP_CHECK(Graph->GetControlEdges().size() == 2U);
    }

    void TestTransferOnlyLoopBodies(const NodeDescriptorRegistry& Registry)
    {
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            auto EmptyEnd = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(!EmptyEnd.has_value());
            MPP_CHECK(Loop->Scope.IsValid());
            MPP_CHECK(Builder.Break(Loop->Scope, Loop->BodyOutput).has_value());
            auto RepairedEnd = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(RepairedEnd.has_value() && RepairedEnd->ExitOutput.has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(Graph->GetNodes().size() == 2U);
            MPP_CHECK(Graph->GetExecutionRegions().size() == 2U);
            MPP_CHECK(Graph->GetControlEdges().size() == 2U);
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        }

        const auto VerifyTransferOnly = [&Registry](
            NodeDescriptorId LoopDescriptor,
            bool IsConditional,
            bool IsBreak,
            bool ExpectedExit
        )
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = IsConditional
                ? Builder.BeginLoop(Entry->Scope, Entry->RootOutput, LoopDescriptor,
                    BooleanExpression(true))
                : Builder.BeginLoop(Entry->Scope, Entry->RootOutput, LoopDescriptor);
            MPP_CHECK(Loop.has_value());

            const auto Transfer = IsBreak
                ? Builder.Break(Loop->Scope, Loop->BodyOutput)
                : Builder.Continue(Loop->Scope, Loop->BodyOutput);
            MPP_CHECK(Transfer.has_value());

            auto Closed = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(Closed.has_value());
            MPP_CHECK(Closed->ExitOutput.has_value() == ExpectedExit);
            if (!IsConditional && IsBreak)
            {
                MPP_CHECK(Builder.AppendExecutionNode(Entry->Scope,
                    *Closed->ExitOutput, SequenceId).has_value());
            }
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            MPP_CHECK(Builder.Validate().empty());

            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(Graph->GetNodes().size() == (!IsConditional && IsBreak ? 3U : 2U));
            MPP_CHECK(Graph->GetExecutionRegions().size() == 2U);
            MPP_CHECK(Graph->GetControlEdges().size() == (!IsConditional && IsBreak ? 3U : 2U));
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

            const NodeInstanceId LoopNode = Graph->GetExecutionRegions()[1U].OwnerNode.value();
            const ExecutionRegionId BodyRegion = Graph->GetExecutionRegions()[1U].Identifier;
            const std::size_t OwnedBodyNodes = static_cast<std::size_t>(std::count_if(
                Graph->GetNodes().begin(), Graph->GetNodes().end(), [BodyRegion](const NodeInstance& Node)
                {
                    return Node.ExecutionRegion == BodyRegion;
                }));
            MPP_CHECK(OwnedBodyNodes == 0U);
            const PinIndex ExpectedInput = IsBreak ? PinIndex(4U) : PinIndex(3U);
            bool FoundDirectTransfer = false;
            for (const ControlEdge& Edge : Graph->GetControlEdges())
            {
                FoundDirectTransfer = FoundDirectTransfer ||
                    (Edge.SourceNode == LoopNode && Edge.SourceOutputPin == PinIndex(1U) &&
                        Edge.DestinationNode == LoopNode &&
                        Edge.DestinationInputPin == ExpectedInput);
            }
            MPP_CHECK(FoundDirectTransfer);

            const nlohmann::json Serialized = Serialize(*Graph);
            MPP_CHECK(Serialized["irVersion"] == 3U);
            MPP_CHECK(Serialize(*Graph) == Serialized);
            const auto RoundTrip = Deserialize(Serialized);
            MPP_CHECK(RoundTrip.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*RoundTrip, Registry).empty());
            MPP_CHECK(Serialize(*RoundTrip) == Serialized);
            return Serialized;
        };

        const nlohmann::json ConditionalBreak = VerifyTransferOnly(
            ConditionalLoopId, true, true, true);
        const nlohmann::json ConditionalContinue = VerifyTransferOnly(
            ConditionalLoopId, true, false, true);
        const nlohmann::json UnconditionalBreak = VerifyTransferOnly(
            UnconditionalLoopId, false, true, true);
        const nlohmann::json UnconditionalContinue = VerifyTransferOnly(
            UnconditionalLoopId, false, false, false);

        const auto IsRejected = [&Registry](const nlohmann::json& Json)
        {
            const auto Graph = Deserialize(Json);
            return !Graph.has_value() || !GraphIRValidator::Validate(*Graph, Registry).empty();
        };
        nlohmann::json WrongBodyRole = ConditionalBreak;
        for (nlohmann::json& Edge : WrongBodyRole["controlEdges"])
        {
            if (Edge["sourcePin"] == 1U && Edge["destinationPin"] == 4U)
            {
                Edge["destinationPin"] = 0U;
            }
        }
        MPP_CHECK(IsRejected(WrongBodyRole));

        nlohmann::json MissingBodyAction = ConditionalContinue;
        auto& Edges = MissingBodyAction["controlEdges"];
        Edges.erase(std::remove_if(Edges.begin(), Edges.end(), [](const nlohmann::json& Edge)
        {
            return Edge["sourcePin"] == 1U;
        }), Edges.end());
        MPP_CHECK(IsRejected(MissingBodyAction));
    }

    void TestVariableDataConditionAndRepeatFanIn(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Variable = Builder.DeclareVariable<bool>("loop-condition", Boolean(true));
        MPP_CHECK(Variable.has_value());
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto DataNode = Builder.AddNode(BooleanExpressionId);
        MPP_CHECK(DataNode.has_value());
        MPP_CHECK(Builder.BindInput(*DataNode, PinIndex(0U), BooleanExpression(false)).has_value());
        auto DataOutput = Builder.GetOutput<bool>(*DataNode, PinIndex(1U));
        MPP_CHECK(DataOutput.has_value());
        auto DataLoop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, ValueOrExpr<bool>(*DataOutput));
        MPP_CHECK(DataLoop.has_value());
        auto DataBody = Builder.AppendExecutionNode(DataLoop->Scope,
            DataLoop->BodyOutput, SequenceId);
        MPP_CHECK(DataBody.has_value());
        auto DataResult = Builder.EndLoop(std::move(DataLoop->Scope));
        MPP_CHECK(DataResult.has_value() && DataResult->ExitOutput.has_value());
        auto VariableLoop = Builder.BeginLoop(Entry->Scope, *DataResult->ExitOutput,
            ConditionalLoopId, Variable->AsInput());
        MPP_CHECK(VariableLoop.has_value());
        auto Branch = Builder.BeginBranch(VariableLoop->Scope,
            VariableLoop->BodyOutput, BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());
        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueTail = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueTail.has_value());
        MPP_CHECK(Builder.Continue(VariableLoop->Scope, TrueTail->Output).has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseTail = Builder.AppendExecutionNode(FalseArm->Scope,
            FalseArm->ArmOutput, SequenceId);
        MPP_CHECK(FalseTail.has_value());
        MPP_CHECK(Builder.Continue(VariableLoop->Scope, FalseTail->Output).has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto ZeroLive = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(ZeroLive.has_value() && ZeroLive->GetLiveArmCount() == 0U);
        auto VariableResult = Builder.EndLoop(std::move(VariableLoop->Scope));
        MPP_CHECK(VariableResult.has_value() && VariableResult->ExitOutput.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

        std::size_t RepeatIncoming = 0U;
        for (const ControlEdge& Edge : Graph->GetControlEdges())
        {
            if (Edge.DestinationInputPin == PinIndex(3U))
            {
                ++RepeatIncoming;
            }
        }
        MPP_CHECK(RepeatIncoming == 2U);
    }

    void TestLoopBodyDataCannotEscape(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Loop.has_value());
        auto Body = Builder.AppendExecutionNode(Loop->Scope, Loop->BodyOutput, SequenceId);
        MPP_CHECK(Body.has_value());
        auto BodyValue = Builder.GetOutput<bool>(Body->Node, PinIndex(2U));
        MPP_CHECK(BodyValue.has_value());
        auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResultValue.has_value() && LoopResultValue->ExitOutput.has_value());
        auto After = Builder.AppendExecutionNode(Entry->Scope,
            *LoopResultValue->ExitOutput, SequenceId);
        MPP_CHECK(After.has_value());
        MPP_CHECK(Builder.BindInput(After->Node, PinIndex(3U),
            ValueOrExpr<bool>(*BodyValue)).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(!Graph.has_value());
        MPP_CHECK(HasCode(Graph.error(), DiagnosticCode::ExecutionDataNotDominated));
    }

    void TestLoopTransferDataBoundaries(const NodeDescriptorRegistry& Registry)
    {
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            auto Body = Builder.AppendExecutionNode(Loop->Scope,
                Loop->BodyOutput, SequenceId);
            MPP_CHECK(Body.has_value());
            auto BodyValue = Builder.GetOutput<bool>(Body->Node, PinIndex(2U));
            MPP_CHECK(BodyValue.has_value());
            MPP_CHECK(Builder.Break(Loop->Scope, Body->Output).has_value());
            auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResultValue.has_value() && LoopResultValue->ExitOutput.has_value());
            auto After = Builder.AppendExecutionNode(Entry->Scope,
                *LoopResultValue->ExitOutput, SequenceId);
            MPP_CHECK(After.has_value());
            MPP_CHECK(Builder.BindInput(After->Node, PinIndex(3U),
                ValueOrExpr<bool>(*BodyValue)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(!Graph.has_value());
            MPP_CHECK(HasCode(Graph.error(), DiagnosticCode::ExecutionDataNotDominated));
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            auto Consumer = Builder.AppendExecutionNode(Loop->Scope,
                Loop->BodyOutput, SequenceId);
            MPP_CHECK(Consumer.has_value());
            auto Producer = Builder.AppendExecutionNode(Loop->Scope,
                Consumer->Output, SequenceId);
            MPP_CHECK(Producer.has_value());
            auto CarriedValue = Builder.GetOutput<bool>(Producer->Node, PinIndex(2U));
            MPP_CHECK(CarriedValue.has_value());
            MPP_CHECK(Builder.BindInput(Consumer->Node, PinIndex(3U),
                ValueOrExpr<bool>(*CarriedValue)).has_value());
            MPP_CHECK(Builder.Continue(Loop->Scope, Producer->Output).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(Loop->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(!Graph.has_value());
            MPP_CHECK(HasCode(Graph.error(), DiagnosticCode::ExecutionDataNotDominated));
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Outer.has_value());
            auto OuterPrefix = Builder.AppendExecutionNode(Outer->Scope,
                Outer->BodyOutput, SequenceId);
            MPP_CHECK(OuterPrefix.has_value());
            auto InnerCondition = Builder.GetOutput<bool>(OuterPrefix->Node, PinIndex(2U));
            MPP_CHECK(InnerCondition.has_value());
            auto Inner = Builder.BeginLoop(Outer->Scope, OuterPrefix->Output,
                ConditionalLoopId, ValueOrExpr<bool>(*InnerCondition));
            MPP_CHECK(Inner.has_value());
            auto InnerBody = Builder.AppendExecutionNode(Inner->Scope,
                Inner->BodyOutput, SequenceId);
            MPP_CHECK(InnerBody.has_value());
            auto InnerValue = Builder.GetOutput<bool>(InnerBody->Node, PinIndex(2U));
            MPP_CHECK(InnerValue.has_value());
            auto InnerResult = Builder.EndLoop(std::move(Inner->Scope));
            MPP_CHECK(InnerResult.has_value() && InnerResult->ExitOutput.has_value());
            auto AfterInner = Builder.AppendExecutionNode(Outer->Scope,
                *InnerResult->ExitOutput, SequenceId);
            MPP_CHECK(AfterInner.has_value());
            MPP_CHECK(Builder.BindInput(AfterInner->Node, PinIndex(3U),
                ValueOrExpr<bool>(*InnerValue)).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(Outer->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(!Graph.has_value());
            MPP_CHECK(HasCode(Graph.error(), DiagnosticCode::ExecutionDataNotDominated));
        }
    }

    void TestNestedNearestLoopAndBuilderMove(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            UnconditionalLoopId);
        MPP_CHECK(Outer.has_value());

        GraphBuilder Moved(std::move(Builder));
        MPP_CHECK(!Builder.Validate().empty());
        auto OuterPrefix = Moved.AppendExecutionNode(Outer->Scope, Outer->BodyOutput,
            SequenceId);
        MPP_CHECK(OuterPrefix.has_value());
        auto OuterCondition = Moved.GetOutput<bool>(OuterPrefix->Node, PinIndex(2U));
        MPP_CHECK(OuterCondition.has_value());
        auto Inner = Moved.BeginLoop(Outer->Scope, OuterPrefix->Output,
            ConditionalLoopId, ValueOrExpr<bool>(*OuterCondition));
        MPP_CHECK(Inner.has_value());
        auto InnerBody = Moved.AppendExecutionNode(Inner->Scope, Inner->BodyOutput,
            SequenceId);
        MPP_CHECK(InnerBody.has_value());

        auto WrongOuterBreak = Moved.Break(Outer->Scope, InnerBody->Output);
        MPP_CHECK(!WrongOuterBreak.has_value());
        MPP_CHECK(HasCode(WrongOuterBreak.error(), DiagnosticCode::InvalidLoopTransfer));
        auto WrongOuterContinue = Moved.Continue(Outer->Scope, InnerBody->Output);
        MPP_CHECK(!WrongOuterContinue.has_value());
        MPP_CHECK(HasCode(WrongOuterContinue.error(), DiagnosticCode::InvalidLoopTransfer));
        auto WrongInnerScopeTail = Moved.Break(Inner->Scope, OuterPrefix->Output);
        MPP_CHECK(!WrongInnerScopeTail.has_value());
        MPP_CHECK(Moved.Continue(Inner->Scope, InnerBody->Output).has_value());
        auto InnerResult = Moved.EndLoop(std::move(Inner->Scope));
        MPP_CHECK(InnerResult.has_value() && InnerResult->ExitOutput.has_value());
        auto OuterTail = Moved.AppendExecutionNode(Outer->Scope, *InnerResult->ExitOutput,
            SequenceId);
        MPP_CHECK(OuterTail.has_value());
        MPP_CHECK(Moved.Break(Outer->Scope, OuterTail->Output).has_value());
        auto OuterResult = Moved.EndLoop(std::move(Outer->Scope));
        MPP_CHECK(OuterResult.has_value() && OuterResult->ExitOutput.has_value());
        auto After = Moved.AppendExecutionNode(Entry->Scope, *OuterResult->ExitOutput,
            SequenceId);
        MPP_CHECK(After.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Moved).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        const nlohmann::json Serialized = Serialize(*Graph);
        const auto RoundTrip = Deserialize(Serialized);
        MPP_CHECK(RoundTrip.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*RoundTrip, Registry).empty());
        MPP_CHECK(Serialize(*RoundTrip) == Serialized);
    }

    void TestNestedLoopScopesSurviveBuilderMove(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Outer.has_value());
        auto OuterPrefix = Builder.AppendExecutionNode(Outer->Scope,
            Outer->BodyOutput, SequenceId);
        MPP_CHECK(OuterPrefix.has_value());
        auto InnerCondition = Builder.GetOutput<bool>(OuterPrefix->Node, PinIndex(2U));
        MPP_CHECK(InnerCondition.has_value());
        auto Inner = Builder.BeginLoop(Outer->Scope, OuterPrefix->Output,
            ConditionalLoopId, ValueOrExpr<bool>(*InnerCondition));
        MPP_CHECK(Inner.has_value());

        GraphBuilder Moved(std::move(Builder));
        MPP_CHECK(!Builder.Validate().empty());
        MPP_CHECK(Moved.Continue(Inner->Scope, Inner->BodyOutput).has_value());
        auto InnerResult = Moved.EndLoop(std::move(Inner->Scope));
        MPP_CHECK(InnerResult.has_value() && InnerResult->ExitOutput.has_value());
        auto OuterTail = Moved.AppendExecutionNode(Outer->Scope,
            *InnerResult->ExitOutput, SequenceId);
        MPP_CHECK(OuterTail.has_value());
        MPP_CHECK(Moved.Continue(Outer->Scope, OuterTail->Output).has_value());
        auto OuterResult = Moved.EndLoop(std::move(Outer->Scope));
        MPP_CHECK(OuterResult.has_value() && OuterResult->ExitOutput.has_value());
        auto After = Moved.AppendExecutionNode(Entry->Scope,
            *OuterResult->ExitOutput, SequenceId);
        MPP_CHECK(After.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Moved).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestBranchTransferPairs(const NodeDescriptorRegistry& Registry)
    {
        const auto VerifyPair = [&Registry](bool TrueBreak, bool FalseBreak,
            std::size_t ExpectedBreaks, std::size_t ExpectedRepeats)
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                UnconditionalLoopId);
            MPP_CHECK(Loop.has_value());
            auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
                BranchId, BooleanExpression(true));
            MPP_CHECK(Branch.has_value());

            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            const auto TrueTransfer = TrueBreak
                ? Builder.Break(Loop->Scope, TrueArm->ArmOutput)
                : Builder.Continue(Loop->Scope, TrueArm->ArmOutput);
            MPP_CHECK(TrueTransfer.has_value());
            auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
            MPP_CHECK(TrueOutcome.has_value());

            auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            const auto FalseTransfer = FalseBreak
                ? Builder.Break(Loop->Scope, FalseArm->ArmOutput)
                : Builder.Continue(Loop->Scope, FalseArm->ArmOutput);
            MPP_CHECK(FalseTransfer.has_value());
            auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
            MPP_CHECK(FalseOutcome.has_value());

            auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 0U);
            auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResultValue.has_value());
            MPP_CHECK(LoopResultValue->ExitOutput.has_value() == (ExpectedBreaks != 0U));
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

            std::size_t BreakCount = 0U;
            std::size_t RepeatCount = 0U;
            for (const ControlEdge& Edge : Graph->GetControlEdges())
            {
                if (Edge.DestinationNode == Loop->Node.GetIdentifier())
                {
                    BreakCount += Edge.DestinationInputPin == PinIndex(4U) ? 1U : 0U;
                    RepeatCount += Edge.DestinationInputPin == PinIndex(3U) ? 1U : 0U;
                }
            }
            MPP_CHECK(BreakCount == ExpectedBreaks);
            MPP_CHECK(RepeatCount == ExpectedRepeats);
        };

        VerifyPair(true, true, 2U, 0U);
        VerifyPair(false, false, 0U, 2U);
        VerifyPair(true, false, 1U, 1U);
    }

    void TestLoopInBranchArmAndBranchTransfer(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
            BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());
        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto Loop = Builder.BeginLoop(TrueArm->Scope, TrueArm->ArmOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Loop.has_value());
        auto Body = Builder.AppendExecutionNode(Loop->Scope, Loop->BodyOutput, SequenceId);
        MPP_CHECK(Body.has_value());
        MPP_CHECK(Builder.Continue(Loop->Scope, Body->Output).has_value());
        auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResultValue.has_value() && LoopResultValue->ExitOutput.has_value());
        auto ArmTail = Builder.AppendExecutionNode(TrueArm->Scope,
            *LoopResultValue->ExitOutput, SequenceId);
        MPP_CHECK(ArmTail.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), ArmTail->Output);
        MPP_CHECK(TrueOutcome.has_value());
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto BranchOutcomeValue = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(BranchOutcomeValue.has_value());
        MPP_CHECK(BranchOutcomeValue->GetLiveArmCount() == 1U);
        auto ParentTail = Builder.ContinueWith(Entry->Scope,
            std::move(*BranchOutcomeValue), SequenceId);
        MPP_CHECK(ParentTail.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

        GraphBuilder TransferBuilder(Registry);
        auto TransferEntry = TransferBuilder.BeginEntry(EntryId);
        MPP_CHECK(TransferEntry.has_value());
        auto Outer = TransferBuilder.BeginLoop(TransferEntry->Scope,
            TransferEntry->RootOutput, UnconditionalLoopId);
        MPP_CHECK(Outer.has_value());
        auto InnerBranch = TransferBuilder.BeginBranch(Outer->Scope,
            Outer->BodyOutput, BranchId, BooleanExpression(true));
        MPP_CHECK(InnerBranch.has_value());
        auto BreakArm = TransferBuilder.BeginArm(InnerBranch->Scope, BranchArm::True);
        MPP_CHECK(BreakArm.has_value());
        auto BreakTail = TransferBuilder.AppendExecutionNode(BreakArm->Scope,
            BreakArm->ArmOutput, SequenceId);
        MPP_CHECK(BreakTail.has_value());
        MPP_CHECK(TransferBuilder.Break(Outer->Scope, BreakTail->Output).has_value());
        auto BreakOutcome = TransferBuilder.EndArm(std::move(BreakArm->Scope),
            NoContinuation{});
        MPP_CHECK(BreakOutcome.has_value());
        auto NaturalArm = TransferBuilder.BeginArm(InnerBranch->Scope, BranchArm::False);
        MPP_CHECK(NaturalArm.has_value());
        auto NaturalOutcome = TransferBuilder.EndArm(std::move(NaturalArm->Scope),
            NoContinuation{});
        MPP_CHECK(NaturalOutcome.has_value());
        auto ZeroLive = TransferBuilder.EndBranch(std::move(InnerBranch->Scope),
            std::move(*BreakOutcome), std::move(*NaturalOutcome));
        MPP_CHECK(ZeroLive.has_value() && ZeroLive->GetLiveArmCount() == 0U);
        auto TransferredLoop = TransferBuilder.EndLoop(std::move(Outer->Scope));
        MPP_CHECK(TransferredLoop.has_value() && TransferredLoop->ExitOutput.has_value());
        MPP_CHECK(TransferBuilder.EndEntry(std::move(TransferEntry->Scope)).has_value());
        auto TransferGraph = std::move(TransferBuilder).Finalize();
        MPP_CHECK(TransferGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*TransferGraph, Registry).empty());
    }

    void TestBranchTransferWithOneLiveArm(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            UnconditionalLoopId);
        MPP_CHECK(Loop.has_value());
        auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
            BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());

        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        MPP_CHECK(Builder.Break(Loop->Scope, TrueArm->ArmOutput).has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        GraphBuilder Moved(std::move(Builder));
        MPP_CHECK(!Builder.Validate().empty());
        auto FalseOutcome = Moved.EndArm(std::move(FalseArm->Scope),
            FalseArm->ArmOutput);
        MPP_CHECK(FalseOutcome.has_value());

        auto Outcome = Moved.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 1U);
        auto Continued = Moved.ContinueWith(Loop->Scope,
            std::move(*Outcome), SequenceId);
        MPP_CHECK(Continued.has_value());
        MPP_CHECK(Moved.Break(Loop->Scope, Continued->Output).has_value());

        auto LoopResultValue = Moved.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResultValue.has_value() && LoopResultValue->ExitOutput.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Moved).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestJoinInsideLoop(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Loop.has_value());
        const ExecutionRegionId LoopBodyRegion = Loop->BodyOutput.GetRegion();
        auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
            BranchId, BooleanExpression(false));
        MPP_CHECK(Branch.has_value());

        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueTail = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueTail.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), TrueTail->Output);
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseTail = Builder.AppendExecutionNode(FalseArm->Scope,
            FalseArm->ArmOutput, SequenceId);
        MPP_CHECK(FalseTail.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), FalseTail->Output);
        MPP_CHECK(FalseOutcome.has_value());

        auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 2U);
        auto Joined = Builder.Join(Loop->Scope, std::move(*Outcome), JoinId);
        MPP_CHECK(Joined.has_value());
        const NodeInstanceId JoinNode = Joined->Node.GetIdentifier();
        MPP_CHECK(Builder.Continue(Loop->Scope, Joined->Output).has_value());
        MPP_CHECK(Builder.EndLoop(std::move(Loop->Scope)).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        const NodeInstance* JoinNodeRecord = Graph->FindNode(JoinNode);
        MPP_CHECK(JoinNodeRecord != nullptr && JoinNodeRecord->ExecutionRegion == LoopBodyRegion);
        std::size_t JoinIncoming = 0U;
        for (const ControlEdge& Edge : Graph->GetControlEdges())
        {
            if (Edge.DestinationNode == JoinNode && Edge.DestinationInputPin == PinIndex(0U))
            {
                ++JoinIncoming;
            }
        }
        MPP_CHECK(JoinIncoming == 2U);
    }

    void TestLoopScopeFailureLifecycle(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Loop.has_value());
        auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
            BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());
        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto InvalidClose = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(!InvalidClose.has_value());
        MPP_CHECK(Loop->Scope.IsValid());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto ZeroLive = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(ZeroLive.has_value() && ZeroLive->GetLiveArmCount() == 0U);
        MPP_CHECK(Builder.EndLoop(std::move(Loop->Scope)).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

        GraphBuilder Abandoned(Registry);
        auto AbandonedEntry = Abandoned.BeginEntry(EntryId);
        MPP_CHECK(AbandonedEntry.has_value());
        {
            auto AbandonedLoop = Abandoned.BeginLoop(AbandonedEntry->Scope,
                AbandonedEntry->RootOutput, UnconditionalLoopId);
            MPP_CHECK(AbandonedLoop.has_value());
            LoopScope Dropped(std::move(AbandonedLoop->Scope));
            MPP_CHECK(Dropped.IsValid());
        }
        const DiagnosticCollection OpenDiagnostics = Abandoned.Validate();
        MPP_CHECK(HasCode(OpenDiagnostics, DiagnosticCode::OpenExecutionScope));
        auto FailedFinalize = std::move(Abandoned).Finalize();
        MPP_CHECK(!FailedFinalize.has_value());
        MPP_CHECK(HasCode(FailedFinalize.error(), DiagnosticCode::OpenExecutionScope));
    }

    void TestCrossEntryLoopConditionRejected(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto FirstEntry = Builder.BeginEntry(EntryId);
        MPP_CHECK(FirstEntry.has_value());
        auto FirstStep = Builder.AppendExecutionNode(FirstEntry->Scope,
            FirstEntry->RootOutput, SequenceId);
        MPP_CHECK(FirstStep.has_value());
        auto FirstValue = Builder.GetOutput<bool>(FirstStep->Node, PinIndex(2U));
        MPP_CHECK(FirstValue.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(FirstEntry->Scope)).has_value());

        auto SecondEntry = Builder.BeginEntry(EntryId);
        MPP_CHECK(SecondEntry.has_value());
        auto ForeignCondition = Builder.BeginLoop(SecondEntry->Scope,
            SecondEntry->RootOutput, ConditionalLoopId,
            ValueOrExpr<bool>(*FirstValue));
        MPP_CHECK(!ForeignCondition.has_value());
        MPP_CHECK(HasCode(ForeignCondition.error(), DiagnosticCode::ExecutionDataNotDominated));
        auto SecondStep = Builder.AppendExecutionNode(SecondEntry->Scope,
            SecondEntry->RootOutput, SequenceId);
        MPP_CHECK(SecondStep.has_value());
        MPP_CHECK(SecondStep->Node.GetIdentifier() == NodeInstanceId(4U));
        MPP_CHECK(Builder.EndEntry(std::move(SecondEntry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestFailuresAreAtomic(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto MissingCondition = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId);
        MPP_CHECK(!MissingCondition.has_value());
        auto ForbiddenCondition = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            UnconditionalLoopId, BooleanExpression(true));
        MPP_CHECK(!ForbiddenCondition.has_value());
        auto Step = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Step.has_value());
        MPP_CHECK(Step->Node.GetIdentifier() == NodeInstanceId(2U));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(Graph->GetExecutionRegions().size() == 1U);

#ifdef MILIASTRA_PHASE4_TEST_ACCESS
        GraphBuilder MaximumRegion(Registry);
        auto MaximumEntry = MaximumRegion.BeginEntry(EntryId);
        MPP_CHECK(MaximumEntry.has_value());
        GraphBuilderPhase4TestAccess::SetNextRegionIdentifier(MaximumRegion,
            std::numeric_limits<std::uint64_t>::max());
        auto MaximumLoop = MaximumRegion.BeginLoop(MaximumEntry->Scope,
            MaximumEntry->RootOutput, ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(MaximumLoop.has_value());
        MPP_CHECK(MaximumLoop->Scope.IsValid());
        MPP_CHECK(MaximumRegion.AppendExecutionNode(MaximumLoop->Scope,
            MaximumLoop->BodyOutput, SequenceId).has_value());
        MPP_CHECK(MaximumRegion.EndLoop(std::move(MaximumLoop->Scope)).has_value());
        MPP_CHECK(MaximumRegion.EndEntry(std::move(MaximumEntry->Scope)).has_value());
        auto MaximumGraph = std::move(MaximumRegion).Finalize();
        MPP_CHECK(MaximumGraph.has_value());
        MPP_CHECK(MaximumGraph->GetExecutionRegions().back().Identifier ==
            ExecutionRegionId(std::numeric_limits<std::uint64_t>::max()));
        MPP_CHECK(GraphIRValidator::Validate(*MaximumGraph, Registry).empty());
#endif

#ifdef MILIASTRA_PHASE4_TEST_ACCESS
        GraphBuilder Exhausted(Registry);
        auto ExhaustedEntry = Exhausted.BeginEntry(EntryId);
        MPP_CHECK(ExhaustedEntry.has_value());
        GraphBuilderPhase4TestAccess::SetNextRegionIdentifier(Exhausted, 0U);
        auto FailedRegion = Exhausted.BeginLoop(ExhaustedEntry->Scope,
            ExhaustedEntry->RootOutput, UnconditionalLoopId);
        MPP_CHECK(!FailedRegion.has_value());
        auto AfterFailure = Exhausted.AppendExecutionNode(ExhaustedEntry->Scope,
            ExhaustedEntry->RootOutput, SequenceId);
        MPP_CHECK(AfterFailure.has_value());
        MPP_CHECK(AfterFailure->Node.GetIdentifier() == NodeInstanceId(2U));
#endif
    }

    void TestRawLoopTransferBoundaries(const NodeDescriptorRegistry& Registry)
    {
        const auto RejectsJson = [&Registry](const nlohmann::json& Json)
        {
            const auto Graph = Deserialize(Json);
            return !Graph.has_value() || !GraphIRValidator::Validate(*Graph, Registry).empty();
        };

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Outer.has_value());
            const NodeInstanceId OuterNode = Outer->Node.GetIdentifier();
            auto Prefix = Builder.AppendExecutionNode(Outer->Scope,
                Outer->BodyOutput, SequenceId);
            MPP_CHECK(Prefix.has_value());
            auto Condition = Builder.GetOutput<bool>(Prefix->Node, PinIndex(2U));
            MPP_CHECK(Condition.has_value());
            auto Inner = Builder.BeginLoop(Outer->Scope, Prefix->Output,
                ConditionalLoopId, ValueOrExpr<bool>(*Condition));
            MPP_CHECK(Inner.has_value());
            const NodeInstanceId InnerNode = Inner->Node.GetIdentifier();
            MPP_CHECK(Builder.Continue(Inner->Scope, Inner->BodyOutput).has_value());
            auto InnerResult = Builder.EndLoop(std::move(Inner->Scope));
            MPP_CHECK(InnerResult.has_value() && InnerResult->ExitOutput.has_value());
            auto OuterTail = Builder.AppendExecutionNode(Outer->Scope,
                *InnerResult->ExitOutput, SequenceId);
            MPP_CHECK(OuterTail.has_value());
            MPP_CHECK(Builder.Continue(Outer->Scope, OuterTail->Output).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(Outer->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

            nlohmann::json WrongNearest = Serialize(*Graph);
            for (nlohmann::json& Edge : WrongNearest["controlEdges"])
            {
                if (Edge["sourceNode"] == InnerNode.GetValue() &&
                    Edge["sourcePin"] == 1U && Edge["destinationPin"] == 3U)
                {
                    Edge["destinationNode"] = OuterNode.GetValue();
                }
            }
            MPP_CHECK(RejectsJson(WrongNearest));
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto First = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(First.has_value());
            const NodeInstanceId FirstNode = First->Node.GetIdentifier();
            auto FirstBody = Builder.AppendExecutionNode(First->Scope,
                First->BodyOutput, SequenceId);
            MPP_CHECK(FirstBody.has_value());
            auto FirstResult = Builder.EndLoop(std::move(First->Scope));
            MPP_CHECK(FirstResult.has_value() && FirstResult->ExitOutput.has_value());
            auto Second = Builder.BeginLoop(Entry->Scope, *FirstResult->ExitOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Second.has_value());
            const NodeInstanceId SecondNode = Second->Node.GetIdentifier();
            MPP_CHECK(Builder.Continue(Second->Scope, Second->BodyOutput).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(Second->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

            nlohmann::json SiblingTransfer = Serialize(*Graph);
            for (nlohmann::json& Edge : SiblingTransfer["controlEdges"])
            {
                if (Edge["sourceNode"] == SecondNode.GetValue() &&
                    Edge["sourcePin"] == 1U && Edge["destinationPin"] == 3U)
                {
                    Edge["destinationNode"] = FirstNode.GetValue();
                }
            }
            MPP_CHECK(RejectsJson(SiblingTransfer));
        }

        {
            GraphBuilder Builder(Registry);
            auto FirstEntry = Builder.BeginEntry(EntryId);
            MPP_CHECK(FirstEntry.has_value());
            auto FirstLoop = Builder.BeginLoop(FirstEntry->Scope,
                FirstEntry->RootOutput, ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(FirstLoop.has_value());
            const NodeInstanceId FirstLoopNode = FirstLoop->Node.GetIdentifier();
            MPP_CHECK(Builder.Continue(FirstLoop->Scope, FirstLoop->BodyOutput).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(FirstLoop->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(FirstEntry->Scope)).has_value());
            auto SecondEntry = Builder.BeginEntry(EntryId);
            MPP_CHECK(SecondEntry.has_value());
            auto SecondLoop = Builder.BeginLoop(SecondEntry->Scope,
                SecondEntry->RootOutput, ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(SecondLoop.has_value());
            const NodeInstanceId SecondLoopNode = SecondLoop->Node.GetIdentifier();
            MPP_CHECK(Builder.Continue(SecondLoop->Scope, SecondLoop->BodyOutput).has_value());
            MPP_CHECK(Builder.EndLoop(std::move(SecondLoop->Scope)).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(SecondEntry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

            nlohmann::json CrossEntry = Serialize(*Graph);
            for (nlohmann::json& Edge : CrossEntry["controlEdges"])
            {
                if (Edge["sourceNode"] == FirstLoopNode.GetValue() &&
                    Edge["sourcePin"] == 1U && Edge["destinationPin"] == 3U)
                {
                    Edge["destinationNode"] = SecondLoopNode.GetValue();
                }
            }
            MPP_CHECK(RejectsJson(CrossEntry));
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            const NodeInstanceId LoopNode = Loop->Node.GetIdentifier();
            auto Body = Builder.AppendExecutionNode(Loop->Scope,
                Loop->BodyOutput, SequenceId);
            MPP_CHECK(Body.has_value());
            MPP_CHECK(Builder.Break(Loop->Scope, Body->Output).has_value());
            auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResultValue.has_value() && LoopResultValue->ExitOutput.has_value());
            auto ExitTail = Builder.AppendExecutionNode(Entry->Scope,
                *LoopResultValue->ExitOutput, SequenceId);
            MPP_CHECK(ExitTail.has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            Graph->AddControlEdge(ControlEdge{
                ExitTail->Node.GetIdentifier(), PinIndex(1U), LoopNode, PinIndex(4U)
            });
            const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(*Graph, Registry);
            MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidLoopTransfer));
        }
    }

    void TestRawCycleRejection(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Loop.has_value());
        auto Body = Builder.AppendExecutionNode(Loop->Scope, Loop->BodyOutput, SequenceId);
        MPP_CHECK(Body.has_value());
        auto LoopResultValue = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResultValue.has_value());
        MPP_CHECK(LoopResultValue->ExitOutput.has_value());
        auto ExitSequence = Builder.AppendExecutionNode(Entry->Scope,
            *LoopResultValue->ExitOutput, SequenceId);
        MPP_CHECK(ExitSequence.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        const nlohmann::json Serialized = Serialize(*Graph);
        const NodeInstanceId LoopNode = *Graph->GetExecutionRegions()[1U].OwnerNode;
        const NodeInstanceId EntryRoot = Graph->GetExecutionEntries()[0U].RootNode;
        const auto IsRejectedAfterRead = [&Registry](const nlohmann::json& Json)
        {
            const auto Parsed = Deserialize(Json);
            return !Parsed.has_value() ||
                !GraphIRValidator::Validate(*Parsed, Registry).empty();
        };

        nlohmann::json WrongOwnerPin = Serialized;
        WrongOwnerPin["executionRegions"][1U]["ownerOutputPin"] = 2U;
        MPP_CHECK(IsRejectedAfterRead(WrongOwnerPin));
        nlohmann::json WrongOwnerNode = Serialized;
        WrongOwnerNode["executionRegions"][1U]["ownerNode"] = EntryRoot.GetValue();
        MPP_CHECK(IsRejectedAfterRead(WrongOwnerNode));
        nlohmann::json UnconditionalExit = Serialized;
        for (nlohmann::json& Node : UnconditionalExit["nodes"])
        {
            if (Node["id"] == LoopNode.GetValue())
            {
                Node["descriptor"] = UnconditionalLoopId.GetValue();
            }
        }
        auto& InputBindings = UnconditionalExit["inputBindings"];
        InputBindings.erase(std::remove_if(InputBindings.begin(), InputBindings.end(),
            [&LoopNode](const nlohmann::json& Binding)
            {
                return Binding["destinationNode"] == LoopNode.GetValue() &&
                    Binding["destinationPin"] == 5U;
            }), InputBindings.end());
        MPP_CHECK(IsRejectedAfterRead(UnconditionalExit));
        nlohmann::json MissingParent = Serialized;
        MissingParent["executionRegions"][1U]["parent"] = nullptr;
        MPP_CHECK(IsRejectedAfterRead(MissingParent));
        nlohmann::json WrongBodyEntry = Serialized;
        for (nlohmann::json& Edge : WrongBodyEntry["controlEdges"])
        {
            if (Edge["sourceNode"] == LoopNode.GetValue() && Edge["sourcePin"] == 1U)
            {
                Edge["destinationNode"] = EntryRoot.GetValue();
                break;
            }
        }
        MPP_CHECK(IsRejectedAfterRead(WrongBodyEntry));
        nlohmann::json OutsideRepeat = Serialized;
        OutsideRepeat["controlEdges"].push_back({
            {"sourceNode", EntryRoot.GetValue()}, {"sourcePin", 0U},
            {"destinationNode", LoopNode.GetValue()}, {"destinationPin", 3U}
        });
        MPP_CHECK(IsRejectedAfterRead(OutsideRepeat));

        Graph->AddControlEdge(ControlEdge{
            Body->Node.GetIdentifier(), PinIndex(1U),
            Body->Node.GetIdentifier(), PinIndex(0U)
        });
        Graph->AddControlEdge(ControlEdge{
            EntryRoot, PinIndex(0U),
            LoopNode, PinIndex(3U)
        });
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(*Graph, Registry);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidExecutionReachability));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidControlEdge));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidLoopTransfer));
    }
}

int main()
{
    static_assert(!std::is_default_constructible_v<LoopScope>);
    static_assert(!std::is_copy_constructible_v<LoopScope>);
    static_assert(!std::is_copy_assignable_v<LoopScope>);
    static_assert(std::is_move_constructible_v<LoopScope>);
    static_assert(!std::is_move_assignable_v<LoopScope>);

    const NodeDescriptorRegistry Registry = MakeRegistry();
    TestConditionalLoopAndRoundTrip(Registry);
    TestExplicitContinueAndBreak(Registry);
    TestNaturalCompletionAndRetry(Registry);
    TestTransferOnlyLoopBodies(Registry);
    TestVariableDataConditionAndRepeatFanIn(Registry);
    TestLoopBodyDataCannotEscape(Registry);
    TestLoopTransferDataBoundaries(Registry);
    TestNestedNearestLoopAndBuilderMove(Registry);
    TestNestedLoopScopesSurviveBuilderMove(Registry);
    TestBranchTransferPairs(Registry);
    TestLoopInBranchArmAndBranchTransfer(Registry);
    TestBranchTransferWithOneLiveArm(Registry);
    TestJoinInsideLoop(Registry);
    TestLoopScopeFailureLifecycle(Registry);
    TestCrossEntryLoopConditionRejected(Registry);
    TestFailuresAreAtomic(Registry);
    TestRawLoopTransferBoundaries(Registry);
    TestRawCycleRejection(Registry);
    return 0;
}
