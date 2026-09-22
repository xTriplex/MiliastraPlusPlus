#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <source_location>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraphBuilder.h"
#include "MiliastraPlusPlusGraphIRJson.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;
using namespace MiliastraPlusPlus::GraphIRJson;

#ifdef MILIASTRA_PHASE4_TEST_ACCESS
namespace MiliastraPlusPlus
{
    struct GraphBuilderPhase4TestAccess final
    {
        static void SetNextNodeIdentifier(GraphBuilder& Builder, std::uint64_t Identifier)
        {
            Builder.m_NextNodeIdentifier = Identifier;
        }

        static std::uint64_t GetNextNodeIdentifier(const GraphBuilder& Builder)
        {
            return Builder.m_NextNodeIdentifier;
        }

        static std::size_t GetNodeCount(const GraphBuilder& Builder)
        {
            return Builder.m_Graph.GetNodes().size();
        }

        static std::size_t GetControlEdgeCount(const GraphBuilder& Builder)
        {
            return Builder.m_Graph.GetControlEdges().size();
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
        std::exit(EXIT_FAILURE);
    }

    void Check(bool Condition, const char* Expression, const std::source_location& Location = std::source_location::current())
    {
        if (!Condition)
        {
            Fail(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr NodeDescriptorId EntryId(5401U);
    constexpr NodeDescriptorId SequenceId(5402U);
    constexpr NodeDescriptorId BranchId(5403U);
    constexpr NodeDescriptorId JoinId(5404U);
    constexpr NodeDescriptorId ConditionalLoopId(5405U);
    constexpr NodeDescriptorId UnconditionalLoopId(5406U);
    constexpr NodeDescriptorId ReturnId(5407U);

    PinSchema Flow(const char* Name, PinDirection Direction, PinCardinality Cardinality = PinCardinality::Single)
    {
        return PinSchema(Name, TypeDesc::Flow(), Direction, PinCategory::Execution,
            Cardinality);
    }

    NodeDescriptor EntryDescriptor()
    {
        return NodeDescriptor(EntryId, "Entry", {NodeAvailability::Server},
            {Flow("role output", PinDirection::Output)},
            EntryControlSchema{PinIndex(0U)});
    }

    NodeDescriptor SequenceDescriptor()
    {
        return NodeDescriptor(SequenceId, "Sequence", {NodeAvailability::Server},
            {Flow("role input", PinDirection::Input),
             Flow("role output", PinDirection::Output),
             PinSchema("condition value", TypeDesc::Boolean(), PinDirection::Output,
                 PinCategory::Data),
             PinSchema("data input", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true)},
            SequenceControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor BranchDescriptor()
    {
        return NodeDescriptor(BranchId, "Branch", {NodeAvailability::Server},
            {Flow("role input", PinDirection::Input),
             PinSchema("condition", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true),
             Flow("true role", PinDirection::Output),
             Flow("false role", PinDirection::Output)},
            BranchControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U)});
    }

    NodeDescriptor JoinDescriptor()
    {
        return NodeDescriptor(JoinId, "Join", {NodeAvailability::Server},
            {Flow("role input", PinDirection::Input, PinCardinality::Multiple),
             Flow("role output", PinDirection::Output)},
            JoinControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor ConditionalLoopDescriptor()
    {
        return NodeDescriptor(ConditionalLoopId, "ConditionalLoop", {NodeAvailability::Server},
            {Flow("role input", PinDirection::Input),
             Flow("body role", PinDirection::Output),
             Flow("exit role", PinDirection::Output),
             Flow("repeat role", PinDirection::Input, PinCardinality::Multiple),
             Flow("break role", PinDirection::Input, PinCardinality::Multiple),
             PinSchema("condition", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data, PinCardinality::Single, true)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U),
                PinIndex(4U), LoopExitPolicy::Conditional, PinIndex(5U)});
    }

    NodeDescriptor UnconditionalLoopDescriptor()
    {
        return NodeDescriptor(UnconditionalLoopId, "UnconditionalLoop", {NodeAvailability::Server},
            {Flow("role input", PinDirection::Input),
             Flow("body role", PinDirection::Output),
             Flow("exit role", PinDirection::Output),
             Flow("repeat role", PinDirection::Input, PinCardinality::Multiple),
             Flow("break role", PinDirection::Input, PinCardinality::Multiple)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U),
                PinIndex(4U), LoopExitPolicy::Unconditional, std::nullopt});
    }

    NodeDescriptor ReturnDescriptor()
    {
        return NodeDescriptor(ReturnId, "Return", {NodeAvailability::Server},
            {Flow("execution input", PinDirection::Input)},
            ReturnControlSchema{PinIndex(0U)});
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
        MPP_CHECK(Registry.Register(ReturnDescriptor()).has_value());
        return Registry;
    }

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return std::any_of(Diagnostics.begin(), Diagnostics.end(),
            [Code](const Diagnostic& Diagnostic)
            {
                return Diagnostic.Code == Code;
            });
    }

    std::vector<DiagnosticCode> DiagnosticCodes(const DiagnosticCollection& Diagnostics)
    {
        std::vector<DiagnosticCode> Result;
        Result.reserve(Diagnostics.size());
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            Result.push_back(Diagnostic.Code);
        }
        return Result;
    }

    ValueOrExpr<bool> BooleanExpression(bool Value)
    {
        return ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{Value}));
    }

    GraphIR BuildDirectReturn(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        MPP_CHECK(Builder.Return(Entry->Scope, Entry->RootOutput, ReturnId).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        return std::move(*Graph);
    }

    GraphIR ParseValid(const nlohmann::json& Json)
    {
        auto Parsed = Deserialize(Json);
        MPP_CHECK(Parsed.has_value());
        return std::move(*Parsed);
    }

    void TestDirectEntryReturnAndV3RoundTrip(const NodeDescriptorRegistry& Registry)
    {
        GraphIR Graph = BuildDirectReturn(Registry);
        MPP_CHECK(Graph.GetExecutionModel() == ExecutionModel::Structured);
        MPP_CHECK(Graph.GetNodes().size() == 2U);
        MPP_CHECK(Graph.GetControlEdges().size() == 1U);
        MPP_CHECK(Graph.GetNodes()[1U].Descriptor == ReturnId);
        MPP_CHECK(Graph.GetNodes()[1U].ExecutionRegion == ExecutionRegionId(1U));
        MPP_CHECK(Graph.GetControlEdges()[0U].SourceNode == NodeInstanceId(1U));
        MPP_CHECK(Graph.GetControlEdges()[0U].SourceOutputPin == PinIndex(0U));
        MPP_CHECK(Graph.GetControlEdges()[0U].DestinationNode == NodeInstanceId(2U));
        MPP_CHECK(Graph.GetControlEdges()[0U].DestinationInputPin == PinIndex(0U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Registry).empty());

        const nlohmann::json First = Serialize(Graph);
        const nlohmann::json Second = Serialize(Graph);
        MPP_CHECK(First == Second);
        MPP_CHECK(First["irVersion"] == 3U);
        const GraphIR RoundTrip = ParseValid(First);
        MPP_CHECK(RoundTrip.GetExecutionModel() == ExecutionModel::Structured);
        MPP_CHECK(GraphIRValidator::Validate(RoundTrip, Registry).empty());
        MPP_CHECK(Serialize(RoundTrip) == First);
    }

    void TestSequenceToReturnConsumesTail(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Step = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Step.has_value());
        const ExecutionHandle StaleTail = Step->Output;
        MPP_CHECK(Builder.Return(Entry->Scope, Step->Output, ReturnId).has_value());
        auto Reuse = Builder.AppendExecutionNode(Entry->Scope, StaleTail, SequenceId);
        MPP_CHECK(!Reuse.has_value());
        MPP_CHECK(HasCode(Reuse.error(), DiagnosticCode::ExecutionEndpointAlreadyConsumed));
        auto RepeatedReturn = Builder.Return(Entry->Scope, StaleTail, ReturnId);
        MPP_CHECK(!RepeatedReturn.has_value());
        MPP_CHECK(HasCode(RepeatedReturn.error(),
            DiagnosticCode::ExecutionEndpointAlreadyConsumed));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(Graph->GetNodes().size() == 3U);
        MPP_CHECK(Graph->GetControlEdges().size() == 2U);
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestReturnReturnBranchHasZeroLiveArms(const NodeDescriptorRegistry& Registry)
    {
        // Each arm has its own Return because a Return input is single-cardinality.
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
            BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());

        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        MPP_CHECK(Builder.Return(TrueArm->Scope, TrueArm->ArmOutput, ReturnId).has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        MPP_CHECK(Builder.Return(FalseArm->Scope, FalseArm->ArmOutput, ReturnId).has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());

        auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        MPP_CHECK(Outcome->GetLiveArmCount() == 0U);
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

        std::size_t ReturnCount = 0U;
        for (const NodeInstance& Node : Graph->GetNodes())
        {
            ReturnCount += static_cast<std::size_t>(Node.Descriptor == ReturnId);
            if (Node.Descriptor == ReturnId)
            {
                MPP_CHECK(Node.ExecutionRegion.has_value());
                const ExecutionRegion* Region = Graph->FindExecutionRegion(*Node.ExecutionRegion);
                MPP_CHECK(Region != nullptr && Region->Kind == ExecutionRegionKind::BranchArm);
            }
        }
        MPP_CHECK(ReturnCount == 2U);
        MPP_CHECK(Serialize(ParseValid(Serialize(*Graph))) == Serialize(*Graph));
    }

    void TestReturnLiveAndLiveLiveBranchComposition(const NodeDescriptorRegistry& Registry)
    {
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
                BranchId, BooleanExpression(true));
            MPP_CHECK(Branch.has_value());

            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            MPP_CHECK(Builder.Return(TrueArm->Scope, TrueArm->ArmOutput, ReturnId).has_value());
            auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
            MPP_CHECK(TrueOutcome.has_value());

            auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            auto Work = Builder.AppendExecutionNode(FalseArm->Scope,
                FalseArm->ArmOutput, SequenceId);
            MPP_CHECK(Work.has_value());
            auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), Work->Output);
            MPP_CHECK(FalseOutcome.has_value());

            auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 1U);
            auto Continuation = Builder.ContinueWith(Entry->Scope,
                std::move(*Outcome), SequenceId);
            MPP_CHECK(Continuation.has_value());
            MPP_CHECK(Builder.Return(Entry->Scope, Continuation->Output, ReturnId).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
                BranchId, BooleanExpression(true));
            MPP_CHECK(Branch.has_value());
            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            auto TrueWork = Builder.AppendExecutionNode(TrueArm->Scope,
                TrueArm->ArmOutput, SequenceId);
            MPP_CHECK(TrueWork.has_value());
            auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), TrueWork->Output);
            MPP_CHECK(TrueOutcome.has_value());
            auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            auto FalseWork = Builder.AppendExecutionNode(FalseArm->Scope,
                FalseArm->ArmOutput, SequenceId);
            MPP_CHECK(FalseWork.has_value());
            auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), FalseWork->Output);
            MPP_CHECK(FalseOutcome.has_value());
            auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 2U);
            auto Joined = Builder.Join(Entry->Scope, std::move(*Outcome), JoinId);
            MPP_CHECK(Joined.has_value());
            MPP_CHECK(Builder.Return(Entry->Scope, Joined->Output, ReturnId).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        }
    }

    void TestConditionalAndUnconditionalReturnLoops(const NodeDescriptorRegistry& Registry)
    {
        // A conditional loop can still exit when its condition is false; Return is not a Break.
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            MPP_CHECK(Builder.Return(Loop->Scope, Loop->BodyOutput, ReturnId).has_value());
            auto LoopResult = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResult.has_value() && LoopResult->ExitOutput.has_value());
            auto After = Builder.AppendExecutionNode(Entry->Scope,
                *LoopResult->ExitOutput, SequenceId);
            MPP_CHECK(After.has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                UnconditionalLoopId);
            MPP_CHECK(Loop.has_value());
            MPP_CHECK(Builder.Return(Loop->Scope, Loop->BodyOutput, ReturnId).has_value());
            auto LoopResult = Builder.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResult.has_value() && !LoopResult->ExitOutput.has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
        }
    }

    void BuildLoopReturnTransferPair(const NodeDescriptorRegistry& Registry, bool UseBreak, bool IsConditional, bool ExpectedExit)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = IsConditional
            ? Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true))
            : Builder.BeginLoop(Entry->Scope, Entry->RootOutput, UnconditionalLoopId);
        MPP_CHECK(Loop.has_value());
        auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
            BranchId, BooleanExpression(false));
        MPP_CHECK(Branch.has_value());
        auto ReturningArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(ReturningArm.has_value());
        MPP_CHECK(Builder.Return(ReturningArm->Scope,
            ReturningArm->ArmOutput, ReturnId).has_value());
        auto ReturnOutcome = Builder.EndArm(std::move(ReturningArm->Scope), NoContinuation{});
        MPP_CHECK(ReturnOutcome.has_value());

        auto TransferArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(TransferArm.has_value());
        const auto Transfer = UseBreak
            ? Builder.Break(Loop->Scope, TransferArm->ArmOutput)
            : Builder.Continue(Loop->Scope, TransferArm->ArmOutput);
        MPP_CHECK(Transfer.has_value());
        auto TransferOutcome = Builder.EndArm(std::move(TransferArm->Scope), NoContinuation{});
        MPP_CHECK(TransferOutcome.has_value());
        auto BranchOutcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*ReturnOutcome), std::move(*TransferOutcome));
        MPP_CHECK(BranchOutcome.has_value() && BranchOutcome->GetLiveArmCount() == 0U);
        auto LoopResult = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResult.has_value());
        MPP_CHECK(LoopResult->ExitOutput.has_value() == ExpectedExit);
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestReturnWithLoopTransfers(const NodeDescriptorRegistry& Registry)
    {
        BuildLoopReturnTransferPair(Registry, false, false, false);
        BuildLoopReturnTransferPair(Registry, true, false, true);
        BuildLoopReturnTransferPair(Registry, false, true, true);
    }

    void TestNestedReturnAndPostInnerExit(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Outer.has_value());
        auto Inner = Builder.BeginLoop(Outer->Scope, Outer->BodyOutput,
            ConditionalLoopId, BooleanExpression(false));
        MPP_CHECK(Inner.has_value());
        MPP_CHECK(Builder.Return(Inner->Scope, Inner->BodyOutput, ReturnId).has_value());
        auto InnerResult = Builder.EndLoop(std::move(Inner->Scope));
        MPP_CHECK(InnerResult.has_value() && InnerResult->ExitOutput.has_value());
        MPP_CHECK(Builder.Return(Outer->Scope, *InnerResult->ExitOutput, ReturnId).has_value());
        auto OuterResult = Builder.EndLoop(std::move(Outer->Scope));
        MPP_CHECK(OuterResult.has_value() && OuterResult->ExitOutput.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());

        GraphBuilder NonExitingBuilder(Registry);
        auto NonExitingEntry = NonExitingBuilder.BeginEntry(EntryId);
        MPP_CHECK(NonExitingEntry.has_value());
        auto OuterLoop = NonExitingBuilder.BeginLoop(NonExitingEntry->Scope,
            NonExitingEntry->RootOutput, ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(OuterLoop.has_value());
        auto Infinite = NonExitingBuilder.BeginLoop(OuterLoop->Scope,
            OuterLoop->BodyOutput, UnconditionalLoopId);
        MPP_CHECK(Infinite.has_value());
        MPP_CHECK(NonExitingBuilder.Continue(Infinite->Scope, Infinite->BodyOutput).has_value());
        auto InfiniteResult = NonExitingBuilder.EndLoop(std::move(Infinite->Scope));
        MPP_CHECK(InfiniteResult.has_value() && !InfiniteResult->ExitOutput.has_value());
        auto NonExitingOuterResult = NonExitingBuilder.EndLoop(std::move(OuterLoop->Scope));
        MPP_CHECK(NonExitingOuterResult.has_value() &&
            NonExitingOuterResult->ExitOutput.has_value());
        MPP_CHECK(NonExitingBuilder.EndEntry(std::move(NonExitingEntry->Scope)).has_value());
        auto NonExitingGraph = std::move(NonExitingBuilder).Finalize();
        MPP_CHECK(NonExitingGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*NonExitingGraph, Registry).empty());

        nlohmann::json Fabricated = Serialize(*NonExitingGraph);
        Fabricated["nodes"].push_back({
            {"id", 5U}, {"descriptor", ReturnId.GetValue()}, {"executionRegion", 2U}
        });
        Fabricated["controlEdges"].push_back({
            {"sourceNode", 4U}, {"sourcePin", 2U},
            {"destinationNode", 5U}, {"destinationPin", 0U}
        });
        const GraphIR FabricatedGraph = ParseValid(Fabricated);
        MPP_CHECK(HasCode(GraphIRValidator::Validate(FabricatedGraph, Registry),
            DiagnosticCode::InvalidExecutionReachability));
    }

    void TestEntryCompletionAndMultipleEntries(const NodeDescriptorRegistry& Registry)
    {
        // Root-only is invalid, while Return or an ordinary first action may close
        // an entry.
        {
            GraphBuilder Builder(Registry);
            auto Empty = Builder.BeginEntry(EntryId);
            MPP_CHECK(Empty.has_value());
            auto EmptyEnd = Builder.EndEntry(std::move(Empty->Scope));
            MPP_CHECK(!EmptyEnd.has_value());
            MPP_CHECK(HasCode(EmptyEnd.error(), DiagnosticCode::InvalidExecutionReachability));
            MPP_CHECK(Empty->Scope.IsValid());
            MPP_CHECK(Builder.Return(Empty->Scope, Empty->RootOutput, ReturnId).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Empty->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
        }

        GraphBuilder Builder(Registry);
        auto First = Builder.BeginEntry(EntryId);
        MPP_CHECK(First.has_value());
        MPP_CHECK(Builder.Return(First->Scope, First->RootOutput, ReturnId).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(First->Scope)).has_value());
        auto Second = Builder.BeginEntry(EntryId);
        MPP_CHECK(Second.has_value());
        auto Natural = Builder.AppendExecutionNode(Second->Scope,
            Second->RootOutput, SequenceId);
        MPP_CHECK(Natural.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Second->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(Graph->GetExecutionEntries().size() == 2U);
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestConstructionFailuresAreAtomic(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Missing = Builder.Return(Entry->Scope, Entry->RootOutput, NodeDescriptorId(9999U));
        MPP_CHECK(!Missing.has_value());
        MPP_CHECK(HasCode(Missing.error(), DiagnosticCode::MissingDescriptor));
        auto WrongRole = Builder.Return(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(!WrongRole.has_value());
        MPP_CHECK(HasCode(WrongRole.error(), DiagnosticCode::InvalidExecutionControlRole));
#ifdef MILIASTRA_PHASE4_TEST_ACCESS
        MPP_CHECK(GraphBuilderPhase4TestAccess::GetNodeCount(Builder) == 1U);
        MPP_CHECK(GraphBuilderPhase4TestAccess::GetControlEdgeCount(Builder) == 0U);
        MPP_CHECK(GraphBuilderPhase4TestAccess::GetNextNodeIdentifier(Builder) == 2U);
#endif
        auto Step = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Step.has_value() && Step->Node.GetIdentifier() == NodeInstanceId(2U));
        MPP_CHECK(Builder.Return(Entry->Scope, Step->Output, ReturnId).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());

        GraphBuilder ScopeBuilder(Registry);
        auto ScopeEntry = ScopeBuilder.BeginEntry(EntryId);
        MPP_CHECK(ScopeEntry.has_value());
        auto Branch = ScopeBuilder.BeginBranch(ScopeEntry->Scope,
            ScopeEntry->RootOutput, BranchId, BooleanExpression(true));
        MPP_CHECK(Branch.has_value());
        auto WrongActiveScope = ScopeBuilder.Return(ScopeEntry->Scope,
            ScopeEntry->RootOutput, ReturnId);
        MPP_CHECK(!WrongActiveScope.has_value());
        MPP_CHECK(HasCode(WrongActiveScope.error(), DiagnosticCode::InvalidExecutionScope));
        auto TrueArm = ScopeBuilder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        const ExecutionHandle WrongRegionTail = TrueArm->ArmOutput;
        auto TrueOutcome = ScopeBuilder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto FalseArm = ScopeBuilder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = ScopeBuilder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto BranchOutcome = ScopeBuilder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(BranchOutcome.has_value() && BranchOutcome->GetLiveArmCount() == 0U);
        auto WrongRegion = ScopeBuilder.Return(ScopeEntry->Scope, WrongRegionTail, ReturnId);
        MPP_CHECK(!WrongRegion.has_value());
        MPP_CHECK(HasCode(WrongRegion.error(), DiagnosticCode::InvalidExecutionHandle));
        MPP_CHECK(ScopeBuilder.EndEntry(std::move(ScopeEntry->Scope)).has_value());
        auto ScopeGraph = std::move(ScopeBuilder).Finalize();
        MPP_CHECK(ScopeGraph.has_value());

        GraphBuilder MultiEntryBuilder(Registry);
        auto FirstEntry = MultiEntryBuilder.BeginEntry(EntryId);
        MPP_CHECK(FirstEntry.has_value());
        auto FirstStep = MultiEntryBuilder.AppendExecutionNode(FirstEntry->Scope,
            FirstEntry->RootOutput, SequenceId);
        MPP_CHECK(FirstStep.has_value());
        const ExecutionHandle FirstEntryTail = FirstStep->Output;
        MPP_CHECK(MultiEntryBuilder.EndEntry(std::move(FirstEntry->Scope)).has_value());
        auto SecondEntry = MultiEntryBuilder.BeginEntry(EntryId);
        MPP_CHECK(SecondEntry.has_value());
        auto CrossEntryTail = MultiEntryBuilder.Return(SecondEntry->Scope,
            FirstEntryTail, ReturnId);
        MPP_CHECK(!CrossEntryTail.has_value());
        MPP_CHECK(HasCode(CrossEntryTail.error(), DiagnosticCode::InvalidExecutionHandle));
        MPP_CHECK(MultiEntryBuilder.Return(SecondEntry->Scope,
            SecondEntry->RootOutput, ReturnId).has_value());
        MPP_CHECK(MultiEntryBuilder.EndEntry(std::move(SecondEntry->Scope)).has_value());
        auto MultiEntryGraph = std::move(MultiEntryBuilder).Finalize();
        MPP_CHECK(MultiEntryGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*MultiEntryGraph, Registry).empty());
    }

    void TestForeignStaleAndMovedScopes(const NodeDescriptorRegistry& Registry)
    {
        {
            GraphBuilder FirstBuilder(Registry);
            auto First = FirstBuilder.BeginEntry(EntryId);
            MPP_CHECK(First.has_value());
            GraphBuilder SecondBuilder(Registry);
            auto Second = SecondBuilder.BeginEntry(EntryId);
            MPP_CHECK(Second.has_value());
            auto ForeignTail = FirstBuilder.Return(First->Scope, Second->RootOutput, ReturnId);
            MPP_CHECK(!ForeignTail.has_value());
            MPP_CHECK(HasCode(ForeignTail.error(), DiagnosticCode::ForeignBuilderContext));
            auto ForeignScope = FirstBuilder.Return(Second->Scope, First->RootOutput, ReturnId);
            MPP_CHECK(!ForeignScope.has_value());
            MPP_CHECK(HasCode(ForeignScope.error(), DiagnosticCode::ForeignBuilderContext));
            MPP_CHECK(FirstBuilder.Return(First->Scope, First->RootOutput, ReturnId).has_value());
            MPP_CHECK(FirstBuilder.EndEntry(std::move(First->Scope)).has_value());
            auto FirstGraph = std::move(FirstBuilder).Finalize();
            MPP_CHECK(FirstGraph.has_value());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            const ExecutionHandle StaleTail = Entry->RootOutput;
            MPP_CHECK(Builder.Return(Entry->Scope, Entry->RootOutput, ReturnId).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Stale = Builder.Return(Entry->Scope, StaleTail, ReturnId);
            MPP_CHECK(!Stale.has_value());
            MPP_CHECK(HasCode(Stale.error(), DiagnosticCode::InvalidExecutionScope));
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            GraphBuilder Moved(std::move(Builder));
            MPP_CHECK(Moved.Return(Entry->Scope, Entry->RootOutput, ReturnId).has_value());
            auto MovedFromUse = Builder.Return(Entry->Scope, Entry->RootOutput, ReturnId);
            MPP_CHECK(!MovedFromUse.has_value());
            MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Moved).Finalize();
            MPP_CHECK(Graph.has_value());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
                BranchId, BooleanExpression(true));
            MPP_CHECK(Branch.has_value());
            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            GraphBuilder Moved(std::move(Builder));
            MPP_CHECK(Moved.Return(TrueArm->Scope, TrueArm->ArmOutput, ReturnId).has_value());
            auto TrueOutcome = Moved.EndArm(std::move(TrueArm->Scope), NoContinuation{});
            MPP_CHECK(TrueOutcome.has_value());
            auto FalseArm = Moved.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            MPP_CHECK(Moved.Return(FalseArm->Scope, FalseArm->ArmOutput, ReturnId).has_value());
            auto FalseOutcome = Moved.EndArm(std::move(FalseArm->Scope), NoContinuation{});
            MPP_CHECK(FalseOutcome.has_value());
            auto Outcome = Moved.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 0U);
            MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Moved).Finalize();
            MPP_CHECK(Graph.has_value());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            GraphBuilder Moved(std::move(Builder));
            MPP_CHECK(Moved.Return(Loop->Scope, Loop->BodyOutput, ReturnId).has_value());
            auto LoopResult = Moved.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResult.has_value() && LoopResult->ExitOutput.has_value());
            MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Moved).Finalize();
            MPP_CHECK(Graph.has_value());
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
                ConditionalLoopId, BooleanExpression(true));
            MPP_CHECK(Loop.has_value());
            auto Branch = Builder.BeginBranch(Loop->Scope, Loop->BodyOutput,
                BranchId, BooleanExpression(false));
            MPP_CHECK(Branch.has_value());
            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            GraphBuilder Moved(std::move(Builder));
            MPP_CHECK(Moved.Return(TrueArm->Scope, TrueArm->ArmOutput, ReturnId).has_value());
            auto TrueOutcome = Moved.EndArm(std::move(TrueArm->Scope), NoContinuation{});
            MPP_CHECK(TrueOutcome.has_value());
            auto FalseArm = Moved.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            MPP_CHECK(Moved.Return(FalseArm->Scope, FalseArm->ArmOutput, ReturnId).has_value());
            auto FalseOutcome = Moved.EndArm(std::move(FalseArm->Scope), NoContinuation{});
            MPP_CHECK(FalseOutcome.has_value());
            auto Outcome = Moved.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 0U);
            auto LoopResult = Moved.EndLoop(std::move(Loop->Scope));
            MPP_CHECK(LoopResult.has_value() && LoopResult->ExitOutput.has_value());
            MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Moved).Finalize();
            MPP_CHECK(Graph.has_value());
        }
    }

    void TestReturnIdFailureAtomicity(const NodeDescriptorRegistry& Registry)
    {
#ifdef MILIASTRA_PHASE4_TEST_ACCESS
        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            GraphBuilderPhase4TestAccess::SetNextNodeIdentifier(Builder, 0U);
            auto Failed = Builder.Return(Entry->Scope, Entry->RootOutput, ReturnId);
            MPP_CHECK(!Failed.has_value());
            MPP_CHECK(HasCode(Failed.error(), DiagnosticCode::InvalidGraphBuilderState));
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNodeCount(Builder) == 1U);
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetControlEdgeCount(Builder) == 0U);
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNextNodeIdentifier(Builder) == 0U);
            GraphBuilderPhase4TestAccess::SetNextNodeIdentifier(Builder, 2U);
            MPP_CHECK(Builder.Return(Entry->Scope, Entry->RootOutput, ReturnId).has_value());
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(Graph->GetNodes().back().Identifier == NodeInstanceId(2U));
        }

        {
            GraphBuilder Builder(Registry);
            auto Entry = Builder.BeginEntry(EntryId);
            MPP_CHECK(Entry.has_value());
            auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
                BranchId, BooleanExpression(true));
            MPP_CHECK(Branch.has_value());
            auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
            MPP_CHECK(TrueArm.has_value());
            GraphBuilderPhase4TestAccess::SetNextNodeIdentifier(
                Builder, std::numeric_limits<std::uint64_t>::max());
            MPP_CHECK(Builder.Return(TrueArm->Scope, TrueArm->ArmOutput, ReturnId).has_value());
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNextNodeIdentifier(Builder) == 0U);
            auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
            MPP_CHECK(TrueOutcome.has_value());

            auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
            MPP_CHECK(FalseArm.has_value());
            const std::size_t NodesBeforeFailure =
                GraphBuilderPhase4TestAccess::GetNodeCount(Builder);
            const std::size_t EdgesBeforeFailure =
                GraphBuilderPhase4TestAccess::GetControlEdgeCount(Builder);
            auto Exhausted = Builder.Return(
                FalseArm->Scope, FalseArm->ArmOutput, ReturnId);
            MPP_CHECK(!Exhausted.has_value());
            MPP_CHECK(HasCode(Exhausted.error(), DiagnosticCode::InvalidGraphBuilderState));
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNodeCount(Builder) ==
                NodesBeforeFailure);
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetControlEdgeCount(Builder) ==
                EdgesBeforeFailure);
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNextNodeIdentifier(Builder) == 0U);

            GraphBuilderPhase4TestAccess::SetNextNodeIdentifier(Builder, 3U);
            MPP_CHECK(Builder.Return(FalseArm->Scope, FalseArm->ArmOutput,
                ReturnId).has_value());
            MPP_CHECK(GraphBuilderPhase4TestAccess::GetNodeCount(Builder) ==
                NodesBeforeFailure + 1U);
            auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
            MPP_CHECK(FalseOutcome.has_value());
            auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
                std::move(*TrueOutcome), std::move(*FalseOutcome));
            MPP_CHECK(Outcome.has_value() && Outcome->GetLiveArmCount() == 0U);
            MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
            auto Graph = std::move(Builder).Finalize();
            MPP_CHECK(Graph.has_value());
            MPP_CHECK(Graph->GetNodes()[2U].Identifier ==
                NodeInstanceId(std::numeric_limits<std::uint64_t>::max()));
            MPP_CHECK(Graph->GetNodes()[3U].Identifier == NodeInstanceId(3U));
        }
#else
        (void)Registry;
#endif
    }

    void TestReturnDescriptorHardening()
    {
        const auto IsRejected = [](NodeDescriptor Descriptor)
        {
            NodeDescriptorRegistry Registry;
            const auto Result = Registry.Register(std::move(Descriptor));
            return !Result.has_value() && Result.error().Code == DiagnosticCode::InvalidNodeDescriptor;
        };
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5501U), "ReturnWithValue",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input),
             PinSchema("value", TypeDesc::Boolean(), PinDirection::Input,
                 PinCategory::Data)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5502U), "ReturnWithFlowOutput",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input), Flow("out", PinDirection::Output)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5503U), "ReturnWithMultipleInput",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input, PinCardinality::Multiple)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5504U), "ReturnWrongDirection",
            {NodeAvailability::Server},
            {Flow("out", PinDirection::Output)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5505U), "ReturnBadRoleIndex",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input)},
            ReturnControlSchema{PinIndex(1U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5506U), "ReturnExtraFlowInput",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input), Flow("extra", PinDirection::Input)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5507U), "ReturnDataOutput",
            {NodeAvailability::Server},
            {Flow("in", PinDirection::Input),
             PinSchema("value", TypeDesc::Boolean(), PinDirection::Output, PinCategory::Data)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(IsRejected(NodeDescriptor(NodeDescriptorId(5508U), "ReturnWrongFlowType",
            {NodeAvailability::Server},
            {PinSchema("in", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Execution)},
            ReturnControlSchema{PinIndex(0U)})));
        MPP_CHECK(ReturnDescriptor().IsValid());
    }

    void TestRawReturnMalformedTopology(const NodeDescriptorRegistry& Registry)
    {
        const GraphIR Valid = BuildDirectReturn(Registry);
        const nlohmann::json Serialized = Serialize(Valid);

        nlohmann::json NoPredecessor = Serialized;
        NoPredecessor["controlEdges"].clear();
        const GraphIR NoPredecessorGraph = ParseValid(NoPredecessor);
        const DiagnosticCollection MissingInputDiagnostics =
            GraphIRValidator::Validate(NoPredecessorGraph, Registry);
        MPP_CHECK(HasCode(MissingInputDiagnostics, DiagnosticCode::InvalidExecutionReachability));

        nlohmann::json PostReturn = Serialized;
        PostReturn["nodes"].push_back({
            {"id", 3U}, {"descriptor", SequenceId.GetValue()}, {"executionRegion", 1U}
        });
        PostReturn["controlEdges"].push_back({
            {"sourceNode", 2U}, {"sourcePin", 0U},
            {"destinationNode", 3U}, {"destinationPin", 0U}
        });
        const GraphIR PostReturnGraph = ParseValid(PostReturn);
        const DiagnosticCollection PostReturnDiagnostics =
            GraphIRValidator::Validate(PostReturnGraph, Registry);
        MPP_CHECK(HasCode(PostReturnDiagnostics, DiagnosticCode::InvalidControlEdge));
        MPP_CHECK(HasCode(PostReturnDiagnostics, DiagnosticCode::InvalidExecutionReachability));
        MPP_CHECK(DiagnosticCodes(PostReturnDiagnostics) ==
            DiagnosticCodes(GraphIRValidator::Validate(PostReturnGraph, Registry)));

        nlohmann::json FanIn = Serialized;
        FanIn["nodes"].push_back({
            {"id", 3U}, {"descriptor", SequenceId.GetValue()}, {"executionRegion", 1U}
        });
        FanIn["controlEdges"].push_back({
            {"sourceNode", 1U}, {"sourcePin", 0U},
            {"destinationNode", 3U}, {"destinationPin", 0U}
        });
        FanIn["controlEdges"].push_back({
            {"sourceNode", 3U}, {"sourcePin", 1U},
            {"destinationNode", 2U}, {"destinationPin", 0U}
        });
        const DiagnosticCollection FanInDiagnostics = GraphIRValidator::Validate(
            ParseValid(FanIn), Registry);
        MPP_CHECK(HasCode(FanInDiagnostics, DiagnosticCode::InvalidControlEdge));

        nlohmann::json WrongRegion = Serialize(Valid);
        WrongRegion["executionRegions"].push_back({
            {"id", 2U}, {"entry", 1U}, {"kind", "BranchArm"},
            {"parent", 1U}, {"ownerNode", 1U}, {"ownerOutputPin", 0U}
        });
        WrongRegion["nodes"][1U]["executionRegion"] = 2U;
        const DiagnosticCollection RegionDiagnostics = GraphIRValidator::Validate(
            ParseValid(WrongRegion), Registry);
        MPP_CHECK(HasCode(RegionDiagnostics, DiagnosticCode::InvalidExecutionOwnership) ||
            HasCode(RegionDiagnostics, DiagnosticCode::InvalidExecutionRegion));

        nlohmann::json MissingRegion = Serialized;
        MissingRegion["nodes"][1U]["executionRegion"] = 999U;
        const DiagnosticCollection MissingRegionDiagnostics = GraphIRValidator::Validate(
            ParseValid(MissingRegion), Registry);
        MPP_CHECK(HasCode(MissingRegionDiagnostics, DiagnosticCode::InvalidExecutionOwnership));
        MPP_CHECK(!HasCode(MissingRegionDiagnostics,
            DiagnosticCode::InvalidExecutionReachability));

        NodeDescriptorRegistry WithoutReturn;
        MPP_CHECK(WithoutReturn.Register(EntryDescriptor()).has_value());
        MPP_CHECK(WithoutReturn.Register(SequenceDescriptor()).has_value());
        MPP_CHECK(WithoutReturn.Register(BranchDescriptor()).has_value());
        MPP_CHECK(WithoutReturn.Register(JoinDescriptor()).has_value());
        MPP_CHECK(WithoutReturn.Register(ConditionalLoopDescriptor()).has_value());
        MPP_CHECK(WithoutReturn.Register(UnconditionalLoopDescriptor()).has_value());
        const DiagnosticCollection MissingDescriptorDiagnostics =
            GraphIRValidator::Validate(Valid, WithoutReturn);
        MPP_CHECK(HasCode(MissingDescriptorDiagnostics, DiagnosticCode::MissingDescriptor));
        MPP_CHECK(!HasCode(MissingDescriptorDiagnostics,
            DiagnosticCode::InvalidExecutionReachability));
    }

    void TestRawReturnAfterNonExitingLoopAndMultiEntry(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Outer = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            ConditionalLoopId, BooleanExpression(true));
        MPP_CHECK(Outer.has_value());
        auto Infinite = Builder.BeginLoop(Outer->Scope, Outer->BodyOutput,
            UnconditionalLoopId);
        MPP_CHECK(Infinite.has_value());
        MPP_CHECK(Builder.Continue(Infinite->Scope, Infinite->BodyOutput).has_value());
        auto InfiniteResult = Builder.EndLoop(std::move(Infinite->Scope));
        MPP_CHECK(InfiniteResult.has_value() && !InfiniteResult->ExitOutput.has_value());
        auto OuterResult = Builder.EndLoop(std::move(Outer->Scope));
        MPP_CHECK(OuterResult.has_value() && OuterResult->ExitOutput.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());

        nlohmann::json Fabricated = Serialize(*Graph);
        Fabricated["nodes"].push_back({
            {"id", 4U}, {"descriptor", ReturnId.GetValue()}, {"executionRegion", 2U}
        });
        Fabricated["controlEdges"].push_back({
            {"sourceNode", 3U}, {"sourcePin", 2U},
            {"destinationNode", 4U}, {"destinationPin", 0U}
        });
        const DiagnosticCollection FabricatedDiagnostics = GraphIRValidator::Validate(
            ParseValid(Fabricated), Registry);
        MPP_CHECK(HasCode(FabricatedDiagnostics, DiagnosticCode::InvalidExecutionReachability));

        GraphBuilder MultiBuilder(Registry);
        auto First = MultiBuilder.BeginEntry(EntryId);
        MPP_CHECK(First.has_value());
        MPP_CHECK(MultiBuilder.Return(First->Scope, First->RootOutput, ReturnId).has_value());
        MPP_CHECK(MultiBuilder.EndEntry(std::move(First->Scope)).has_value());
        auto Second = MultiBuilder.BeginEntry(EntryId);
        MPP_CHECK(Second.has_value());
        MPP_CHECK(MultiBuilder.Return(Second->Scope, Second->RootOutput, ReturnId).has_value());
        MPP_CHECK(MultiBuilder.EndEntry(std::move(Second->Scope)).has_value());
        auto MultiGraph = std::move(MultiBuilder).Finalize();
        MPP_CHECK(MultiGraph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*MultiGraph, Registry).empty());
    }

    void TestRawUnreachableReturnAndCrossEntryIsolation(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto NaturalPath = Builder.AppendExecutionNode(
            Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(NaturalPath.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto NaturalGraph = std::move(Builder).Finalize();
        MPP_CHECK(NaturalGraph.has_value());

        nlohmann::json Unreachable = Serialize(*NaturalGraph);
        Unreachable["nodes"].push_back({
            {"id", 3U}, {"descriptor", SequenceId.GetValue()}, {"executionRegion", 1U}
        });
        Unreachable["nodes"].push_back({
            {"id", 4U}, {"descriptor", ReturnId.GetValue()}, {"executionRegion", 1U}
        });
        Unreachable["controlEdges"].push_back({
            {"sourceNode", 3U}, {"sourcePin", 1U},
            {"destinationNode", 4U}, {"destinationPin", 0U}
        });
        const DiagnosticCollection UnreachableDiagnostics = GraphIRValidator::Validate(
            ParseValid(Unreachable), Registry);
        MPP_CHECK(HasCode(UnreachableDiagnostics,
            DiagnosticCode::InvalidExecutionReachability));

        GraphBuilder MultiEntryBuilder(Registry);
        auto First = MultiEntryBuilder.BeginEntry(EntryId);
        MPP_CHECK(First.has_value());
        MPP_CHECK(MultiEntryBuilder.Return(First->Scope, First->RootOutput,
            ReturnId).has_value());
        MPP_CHECK(MultiEntryBuilder.EndEntry(std::move(First->Scope)).has_value());
        auto Second = MultiEntryBuilder.BeginEntry(EntryId);
        MPP_CHECK(Second.has_value());
        MPP_CHECK(MultiEntryBuilder.Return(Second->Scope, Second->RootOutput,
            ReturnId).has_value());
        MPP_CHECK(MultiEntryBuilder.EndEntry(std::move(Second->Scope)).has_value());
        auto MultiEntryGraph = std::move(MultiEntryBuilder).Finalize();
        MPP_CHECK(MultiEntryGraph.has_value());
        nlohmann::json CrossEntry = Serialize(*MultiEntryGraph);
        CrossEntry["controlEdges"][1U]["sourceNode"] = 1U;
        const DiagnosticCollection CrossEntryDiagnostics = GraphIRValidator::Validate(
            ParseValid(CrossEntry), Registry);
        MPP_CHECK(HasCode(CrossEntryDiagnostics, DiagnosticCode::InvalidExecutionOwnership));
    }

    void TestRawReturnCannotTransferOrAuthorizeLoopExit(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Loop = Builder.BeginLoop(Entry->Scope, Entry->RootOutput,
            UnconditionalLoopId);
        MPP_CHECK(Loop.has_value());
        MPP_CHECK(Builder.Return(Loop->Scope, Loop->BodyOutput, ReturnId).has_value());
        auto LoopResult = Builder.EndLoop(std::move(Loop->Scope));
        MPP_CHECK(LoopResult.has_value() && !LoopResult->ExitOutput.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());

        for (const std::uint32_t LoopInputPin : {3U, 4U})
        {
            nlohmann::json Transfer = Serialize(*Graph);
            Transfer["controlEdges"].push_back({
                {"sourceNode", 3U}, {"sourcePin", 0U},
                {"destinationNode", 2U}, {"destinationPin", LoopInputPin}
            });
            const DiagnosticCollection TransferDiagnostics = GraphIRValidator::Validate(
                ParseValid(Transfer), Registry);
            MPP_CHECK(HasCode(TransferDiagnostics, DiagnosticCode::InvalidControlEdge));
            MPP_CHECK(HasCode(TransferDiagnostics,
                DiagnosticCode::InvalidExecutionReachability));
        }

        nlohmann::json FakeBreakExit = Serialize(*Graph);
        FakeBreakExit["nodes"].push_back({
            {"id", 4U}, {"descriptor", SequenceId.GetValue()}, {"executionRegion", 1U}
        });
        FakeBreakExit["controlEdges"].push_back({
            {"sourceNode", 2U}, {"sourcePin", 2U},
            {"destinationNode", 4U}, {"destinationPin", 0U}
        });
        const DiagnosticCollection FakeExitDiagnostics = GraphIRValidator::Validate(
            ParseValid(FakeBreakExit), Registry);
        MPP_CHECK(HasCode(FakeExitDiagnostics,
            DiagnosticCode::InvalidExecutionReachability));
    }

    void TestUnstructuredLegacyAndExecutionModeBoundary(const NodeDescriptorRegistry& Registry)
    {
        for (const std::uint32_t Version : {1U, 2U})
        {
            nlohmann::json Legacy = {
                {"irVersion", Version},
                {"nodes", nlohmann::json::array({
                    {{"id", 1U}, {"descriptor", ReturnId.GetValue()}}
                })},
                {"variables", nlohmann::json::array()},
                {"inputBindings", nlohmann::json::array()},
                {"controlEdges", nlohmann::json::array()}
            };
            const GraphIR Graph = ParseValid(Legacy);
            MPP_CHECK(Graph.GetExecutionModel() == ExecutionModel::Unstructured);
            MPP_CHECK(Graph.GetExecutionEntries().empty());
            MPP_CHECK(Graph.GetExecutionRegions().empty());
            MPP_CHECK(GraphIRValidator::Validate(Graph, Registry).empty());
            const nlohmann::json Upgraded = Serialize(Graph);
            MPP_CHECK(Upgraded["irVersion"] == 3U);
            MPP_CHECK(Upgraded["executionModel"] == "Unstructured");
        }

        nlohmann::json Mixed = Serialize(BuildDirectReturn(Registry));
        Mixed["executionModel"] = "Unstructured";
        const DiagnosticCollection MixedDiagnostics = GraphIRValidator::Validate(
            ParseValid(Mixed), Registry);
        MPP_CHECK(HasCode(MixedDiagnostics, DiagnosticCode::InvalidExecutionModel));
    }

    void TestReturnWithUnusedDataProducer(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Compute = Builder.AppendExecutionNode(Entry->Scope,
            Entry->RootOutput, SequenceId);
        MPP_CHECK(Compute.has_value());
        auto Unused = Builder.GetOutput<bool>(Compute->Node, PinIndex(2U));
        MPP_CHECK(Unused.has_value());
        MPP_CHECK(Builder.Return(Entry->Scope, Compute->Output, ReturnId).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Graph = std::move(Builder).Finalize();
        MPP_CHECK(Graph.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Graph, Registry).empty());
    }

    void TestDirectReturnDescriptorDoesNotChangeLegacyContract()
    {
        MPP_CHECK(ReturnDescriptor().GetPins().size() == 1U);
        MPP_CHECK(ReturnDescriptor().GetPins()[0U].GetCategory() == PinCategory::Execution);
        MPP_CHECK(ReturnDescriptor().GetPins()[0U].GetDirection() == PinDirection::Input);
        MPP_CHECK(ReturnDescriptor().GetPins()[0U].GetCardinality() == PinCardinality::Single);
        MPP_CHECK(std::holds_alternative<ReturnControlSchema>(
            *ReturnDescriptor().GetExecutionControlSchema()));
    }
}

int main()
{
    static_assert(!std::is_copy_constructible_v<EntryScope>);
    static_assert(std::is_move_constructible_v<EntryScope>);
    static_assert(!std::is_move_assignable_v<EntryScope>);
    static_assert(!std::is_copy_constructible_v<BranchArmScope>);
    static_assert(std::is_move_constructible_v<BranchArmScope>);
    static_assert(!std::is_copy_constructible_v<LoopScope>);
    static_assert(std::is_move_constructible_v<LoopScope>);
    static_assert(!std::is_move_assignable_v<LoopScope>);

    const NodeDescriptorRegistry Registry = MakeRegistry();
    TestDirectReturnDescriptorDoesNotChangeLegacyContract();
    TestReturnDescriptorHardening();
    TestDirectEntryReturnAndV3RoundTrip(Registry);
    TestSequenceToReturnConsumesTail(Registry);
    TestReturnReturnBranchHasZeroLiveArms(Registry);
    TestReturnLiveAndLiveLiveBranchComposition(Registry);
    TestConditionalAndUnconditionalReturnLoops(Registry);
    TestReturnWithLoopTransfers(Registry);
    TestNestedReturnAndPostInnerExit(Registry);
    TestEntryCompletionAndMultipleEntries(Registry);
    TestConstructionFailuresAreAtomic(Registry);
    TestForeignStaleAndMovedScopes(Registry);
    TestReturnIdFailureAtomicity(Registry);
    TestRawReturnMalformedTopology(Registry);
    TestRawReturnAfterNonExitingLoopAndMultiEntry(Registry);
    TestRawUnreachableReturnAndCrossEntryIsolation(Registry);
    TestRawReturnCannotTransferOrAuthorizeLoopExit(Registry);
    TestUnstructuredLegacyAndExecutionModeBoundary(Registry);
    TestReturnWithUnusedDataProducer(Registry);
    return 0;
}
