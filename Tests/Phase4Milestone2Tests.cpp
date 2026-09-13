#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGraphBuilder.h"
#include "MiliastraPlusPlusGraphIRJson.h"

using namespace MiliastraPlusPlus;

#ifdef MILIASTRA_PHASE4_M2_TEST_ACCESS
namespace MiliastraPlusPlus
{
    struct GraphBuilderPhase4TestAccess final
    {
        static void SetNextExecutionIdentifiers(
            GraphBuilder& Builder,
            std::uint64_t EntryIdentifier,
            std::uint64_t RegionIdentifier
        )
        {
            Builder.m_NextExecutionEntryIdentifier = EntryIdentifier;
            Builder.m_NextExecutionRegionIdentifier = RegionIdentifier;
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

    constexpr NodeDescriptorId EntryId(3001U);
    constexpr NodeDescriptorId SequenceId(3002U);
    constexpr NodeDescriptorId BranchId(3003U);
    constexpr NodeDescriptorId JoinId(3004U);
    constexpr NodeDescriptorId BooleanSourceId(3005U);
    constexpr NodeDescriptorId BooleanExpressionId(3006U);
    constexpr NodeDescriptorId SchemaLessFlowId(3007U);

    PinSchema Flow(const char* Name, PinDirection Direction,
        PinCardinality Cardinality = PinCardinality::Single)
    {
        return PinSchema(Name, TypeDesc::Flow(), Direction, PinCategory::Execution,
            Cardinality);
    }

    NodeDescriptor MakeEntryDescriptor()
    {
        return NodeDescriptor(EntryId, "Entry", {NodeAvailability::Server},
            {Flow("arbitrary root label", PinDirection::Output)},
            EntryControlSchema{PinIndex(0U)});
    }

    NodeDescriptor MakeSequenceDescriptor()
    {
        return NodeDescriptor(SequenceId, "Sequence", {NodeAvailability::Server},
            {Flow("in", PinDirection::Input), Flow("out", PinDirection::Output),
             PinSchema("bool result", TypeDesc::Boolean(), PinDirection::Output,
                PinCategory::Data),
             PinSchema("integer result", TypeDesc::Integer(), PinDirection::Output,
                PinCategory::Data),
             PinSchema("bool input", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true)},
            SequenceControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor MakeBranchDescriptor()
    {
        return NodeDescriptor(BranchId, "Branch", {NodeAvailability::Server},
            {Flow("exec input", PinDirection::Input),
             PinSchema("condition", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true),
             Flow("first unnamed outcome", PinDirection::Output),
             Flow("second unnamed outcome", PinDirection::Output)},
            BranchControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U)});
    }

    NodeDescriptor MakeJoinDescriptor()
    {
        return NodeDescriptor(JoinId, "Join", {NodeAvailability::Server},
            {Flow("many", PinDirection::Input, PinCardinality::Multiple),
             Flow("one", PinDirection::Output)},
            JoinControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor MakeBooleanSourceDescriptor()
    {
        return NodeDescriptor(BooleanSourceId, "BooleanSource", {NodeAvailability::Server},
            {PinSchema("value", TypeDesc::Boolean(), PinDirection::Output,
                PinCategory::Data)});
    }

    NodeDescriptor MakeBooleanExpressionDescriptor()
    {
        return NodeDescriptor(BooleanExpressionId, "BooleanExpression",
            {NodeAvailability::Server},
            {PinSchema("input", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true),
             PinSchema("output", TypeDesc::Boolean(), PinDirection::Output,
                PinCategory::Data)});
    }

    NodeDescriptor MakeSchemaLessFlowDescriptor()
    {
        return NodeDescriptor(SchemaLessFlowId, "LegacyFlow", {NodeAvailability::Server},
            {Flow("legacy input", PinDirection::Input),
             Flow("legacy output", PinDirection::Output)});
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Registry;
        MPP_CHECK(Registry.Register(MakeEntryDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeSequenceDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeBranchDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeJoinDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeBooleanSourceDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeBooleanExpressionDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeSchemaLessFlowDescriptor()).has_value());
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

    bool SameDiagnostics(const DiagnosticCollection& Left, const DiagnosticCollection& Right)
    {
        if (Left.size() != Right.size())
        {
            return false;
        }
        for (std::size_t Index = 0U; Index < Left.size(); ++Index)
        {
            if (Left[Index].Code != Right[Index].Code ||
                Left[Index].Severity != Right[Index].Severity ||
                Left[Index].Message != Right[Index].Message)
            {
                return false;
            }
        }
        return true;
    }

    void ResolveEmptyBranch(GraphBuilder& Builder, BranchStart& Branch)
    {
        auto FalseArm = Builder.BeginArm(Branch.Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());

        auto TrueArm = Builder.BeginArm(Branch.Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());

        auto Outcome = Builder.EndBranch(std::move(Branch.Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        MPP_CHECK(Outcome->GetLiveArmCount() == 0U);
    }

    void TestEntriesAndDeterministicIds(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        MPP_CHECK(Entry->Root.GetDescriptor() == EntryId);
        MPP_CHECK(Entry->Root.GetIdentifier() == NodeInstanceId(1U));
        MPP_CHECK(Entry->RootOutput.GetSourceNode() == Entry->Root.GetIdentifier());
        MPP_CHECK(Entry->RootOutput.GetSourceOutputPin() == PinIndex(0U));

        auto EmptyClose = Builder.EndEntry(std::move(Entry->Scope));
        MPP_CHECK(!EmptyClose.has_value());
        MPP_CHECK(HasCode(EmptyClose.error(), DiagnosticCode::InvalidExecutionReachability));

        auto FirstStep = Builder.AppendExecutionNode(Entry->Scope,
            Entry->RootOutput, SequenceId);
        MPP_CHECK(FirstStep.has_value());
        MPP_CHECK(FirstStep->Node.GetIdentifier() == NodeInstanceId(2U));
        auto SecondStep = Builder.AppendExecutionNode(Entry->Scope,
            FirstStep->Output, SequenceId);
        MPP_CHECK(SecondStep.has_value());
        MPP_CHECK(SecondStep->Node.GetIdentifier() == NodeInstanceId(3U));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());

        auto SecondEntry = Builder.BeginEntry(EntryId);
        MPP_CHECK(SecondEntry.has_value());
        MPP_CHECK(SecondEntry->Root.GetIdentifier() == NodeInstanceId(4U));
        auto SecondEntryStep = Builder.AppendExecutionNode(SecondEntry->Scope,
            SecondEntry->RootOutput, SequenceId);
        MPP_CHECK(SecondEntryStep.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(SecondEntry->Scope)).has_value());

        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(Final->GetExecutionModel() == ExecutionModel::Structured);
        MPP_CHECK(Final->GetExecutionEntries().size() == 2U);
        MPP_CHECK(Final->GetExecutionEntries()[0U].Identifier == ExecutionEntryId(1U));
        MPP_CHECK(Final->GetExecutionEntries()[1U].Identifier == ExecutionEntryId(2U));
        MPP_CHECK(Final->GetExecutionRegions().size() == 2U);
        MPP_CHECK(Final->GetExecutionRegions()[0U].Identifier == ExecutionRegionId(1U));
        MPP_CHECK(Final->GetExecutionRegions()[1U].Identifier == ExecutionRegionId(2U));
        MPP_CHECK(Final->GetNodes()[1U].ExecutionRegion == ExecutionRegionId(1U));
        MPP_CHECK(Final->GetControlEdges().size() == 3U);
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());

        const nlohmann::json Json = GraphIRJson::Serialize(*Final);
        MPP_CHECK(Json["irVersion"] == 3U);
        MPP_CHECK(Json["executionEntries"].size() == 2U);
        const auto RoundTrip = GraphIRJson::Deserialize(Json);
        MPP_CHECK(RoundTrip.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*RoundTrip, Registry).empty());
        GraphBuilder Equivalent(Registry);
        auto E1 = Equivalent.BeginEntry(EntryId);
        MPP_CHECK(E1.has_value());
        auto S1 = Equivalent.AppendExecutionNode(E1->Scope, E1->RootOutput, SequenceId);
        MPP_CHECK(S1.has_value());
        auto S2 = Equivalent.AppendExecutionNode(E1->Scope, S1->Output, SequenceId);
        MPP_CHECK(S2.has_value());
        MPP_CHECK(Equivalent.EndEntry(std::move(E1->Scope)).has_value());
        auto E2 = Equivalent.BeginEntry(EntryId);
        MPP_CHECK(E2.has_value());
        auto S3 = Equivalent.AppendExecutionNode(E2->Scope, E2->RootOutput, SequenceId);
        MPP_CHECK(S3.has_value());
        MPP_CHECK(Equivalent.EndEntry(std::move(E2->Scope)).has_value());
        auto EquivalentFinal = std::move(Equivalent).Finalize();
        MPP_CHECK(EquivalentFinal.has_value());
        MPP_CHECK(GraphIRJson::Serialize(*EquivalentFinal) == Json);
    }

    void TestExecutionIdentifierExhaustion(const NodeDescriptorRegistry& Registry)
    {
        const std::uint64_t Maximum = std::numeric_limits<std::uint64_t>::max();

        GraphBuilder EntryExhaustion(Registry);
        GraphBuilderPhase4TestAccess::SetNextExecutionIdentifiers(
            EntryExhaustion, Maximum, 1U);
        auto Entry = EntryExhaustion.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Step = EntryExhaustion.AppendExecutionNode(Entry->Scope,
            Entry->RootOutput, SequenceId);
        MPP_CHECK(Step.has_value());
        MPP_CHECK(EntryExhaustion.EndEntry(std::move(Entry->Scope)).has_value());
        auto ExhaustedEntry = EntryExhaustion.BeginEntry(EntryId);
        MPP_CHECK(!ExhaustedEntry.has_value());
        MPP_CHECK(HasCode(ExhaustedEntry.error(), DiagnosticCode::InvalidGraphBuilderState));
        auto EntryFinal = std::move(EntryExhaustion).Finalize();
        MPP_CHECK(EntryFinal.has_value());
        MPP_CHECK(EntryFinal->GetExecutionEntries()[0U].Identifier ==
            ExecutionEntryId(Maximum));

        GraphBuilder RegionExhaustion(Registry);
        GraphBuilderPhase4TestAccess::SetNextExecutionIdentifiers(
            RegionExhaustion, 1U, Maximum);
        auto RegionEntry = RegionExhaustion.BeginEntry(EntryId);
        MPP_CHECK(RegionEntry.has_value());
        auto FailedBranch = RegionExhaustion.BeginBranch(RegionEntry->Scope,
            RegionEntry->RootOutput, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(!FailedBranch.has_value());
        MPP_CHECK(HasCode(FailedBranch.error(), DiagnosticCode::InvalidGraphBuilderState));
        auto RegionStep = RegionExhaustion.AppendExecutionNode(RegionEntry->Scope,
            RegionEntry->RootOutput, SequenceId);
        MPP_CHECK(RegionStep.has_value());
        MPP_CHECK(RegionExhaustion.EndEntry(std::move(RegionEntry->Scope)).has_value());
        auto RegionFinal = std::move(RegionExhaustion).Finalize();
        MPP_CHECK(RegionFinal.has_value());
        MPP_CHECK(RegionFinal->GetExecutionRegions().size() == 1U);
        MPP_CHECK(RegionFinal->GetExecutionRegions()[0U].Identifier ==
            ExecutionRegionId(Maximum));
        MPP_CHECK(RegionFinal->GetNodes().size() == 2U);
        MPP_CHECK(GraphIRValidator::Validate(*RegionFinal, Registry).empty());
    }

    void TestHandlesAndBuilderMove(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        const ExecutionHandle CopyA = Entry->RootOutput;
        const ExecutionHandle CopyB = Entry->RootOutput;
        GraphBuilder Moved(std::move(Builder));
        MPP_CHECK(CopyA.IsValid());
        MPP_CHECK(Entry->Scope.IsValid());
        auto MovedFromResult = Builder.BeginEntry(EntryId);
        MPP_CHECK(!MovedFromResult.has_value());

        auto Step = Moved.AppendExecutionNode(Entry->Scope, CopyA, SequenceId);
        MPP_CHECK(Step.has_value());
        auto Reuse = Moved.AppendExecutionNode(Entry->Scope, CopyB, SequenceId);
        MPP_CHECK(!Reuse.has_value());
        MPP_CHECK(HasCode(Reuse.error(), DiagnosticCode::ExecutionEndpointAlreadyConsumed));
        auto Tail = Moved.AppendExecutionNode(Entry->Scope, Step->Output, SequenceId);
        MPP_CHECK(Tail.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Moved).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(!CopyA.IsValid());

        GraphBuilder Left(Registry);
        GraphBuilder Right(Registry);
        auto LeftEntry = Left.BeginEntry(EntryId);
        auto RightEntry = Right.BeginEntry(EntryId);
        MPP_CHECK(LeftEntry.has_value() && RightEntry.has_value());
        auto Foreign = Right.AppendExecutionNode(RightEntry->Scope,
            LeftEntry->RootOutput, SequenceId);
        MPP_CHECK(!Foreign.has_value());
        MPP_CHECK(HasCode(Foreign.error(), DiagnosticCode::ForeignBuilderContext));

        auto ForeignScope = Right.EndEntry(std::move(LeftEntry->Scope));
        MPP_CHECK(!ForeignScope.has_value());
        MPP_CHECK(HasCode(ForeignScope.error(), DiagnosticCode::ForeignBuilderContext));
        auto LeftStep = Left.AppendExecutionNode(LeftEntry->Scope,
            LeftEntry->RootOutput, SequenceId);
        auto RightStep = Right.AppendExecutionNode(RightEntry->Scope,
            RightEntry->RootOutput, SequenceId);
        MPP_CHECK(LeftStep.has_value() && RightStep.has_value());
        MPP_CHECK(Left.EndEntry(std::move(LeftEntry->Scope)).has_value());
        MPP_CHECK(Right.EndEntry(std::move(RightEntry->Scope)).has_value());
    }

    void TestBranchJoinAndEitherArmOrder(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
            BranchId, ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Branch.has_value());

        // False is deliberately constructed first; persisted roles remain descriptor-index based.
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope),
            FalseArm->ArmOutput);
        MPP_CHECK(FalseOutcome.has_value());
        auto FalseAgain = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(!FalseAgain.has_value());
        MPP_CHECK(HasCode(FalseAgain.error(), DiagnosticCode::InvalidBranchArmState));

        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueStep = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueStep.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), TrueStep->Output);
        MPP_CHECK(TrueOutcome.has_value());

        auto BranchOutcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(BranchOutcome.has_value());
        MPP_CHECK(BranchOutcome->GetLiveArmCount() == 2U);
        auto ImplicitContinuation = Builder.ContinueWith(Entry->Scope,
            std::move(*BranchOutcome), SequenceId);
        MPP_CHECK(!ImplicitContinuation.has_value());
        MPP_CHECK(HasCode(ImplicitContinuation.error(), DiagnosticCode::InvalidBranchOutcome));
        auto Join = Builder.Join(Entry->Scope, std::move(*BranchOutcome), JoinId);
        MPP_CHECK(Join.has_value());
        auto AfterJoin = Builder.AppendExecutionNode(Entry->Scope, Join->Output, SequenceId);
        MPP_CHECK(AfterJoin.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(Final->GetExecutionRegions().size() == 3U);
        MPP_CHECK(Final->GetExecutionRegions()[1U].Kind == ExecutionRegionKind::BranchArm);
        MPP_CHECK(Final->GetExecutionRegions()[1U].OwnerOutputPin == PinIndex(2U));
        MPP_CHECK(Final->GetExecutionRegions()[2U].OwnerOutputPin == PinIndex(3U));
        MPP_CHECK(Join->Node.GetIdentifier() == NodeInstanceId(4U));
        MPP_CHECK(Final->FindNode(Join->Node.GetIdentifier())->ExecutionRegion ==
            Final->GetExecutionRegions()[0U].Identifier);
        MPP_CHECK(Final->GetControlEdges().size() == 5U);
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());
        MPP_CHECK(!BranchOutcome->IsValid());
        auto Reuse = Builder.Join(Entry->Scope, std::move(*BranchOutcome), JoinId);
        MPP_CHECK(!Reuse.has_value());
        MPP_CHECK(HasCode(Reuse.error(), DiagnosticCode::InvalidBranchOutcome));
        const auto RoundTrip = GraphIRJson::Deserialize(GraphIRJson::Serialize(*Final));
        MPP_CHECK(RoundTrip.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*RoundTrip, Registry).empty());
        nlohmann::json Malformed = GraphIRJson::Serialize(*Final);
        auto& Edges = Malformed["controlEdges"];
        for (std::size_t Index = 0U; Index < Edges.size(); ++Index)
        {
            if (Edges[Index]["sourceNode"] == 2U && Edges[Index]["sourcePin"] == 2U)
            {
                Edges.erase(Edges.begin() + static_cast<std::ptrdiff_t>(Index));
                break;
            }
        }
        const auto ParsedMalformed = GraphIRJson::Deserialize(Malformed);
        MPP_CHECK(ParsedMalformed.has_value());
        MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedMalformed, Registry),
            DiagnosticCode::InvalidExecutionReachability));
    }

    void TestOneAndZeroLiveOutcomes(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder OneLive(Registry);
        auto Entry = OneLive.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = OneLive.BeginBranch(Entry->Scope, Entry->RootOutput, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Branch.has_value());
        auto TrueArm = OneLive.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueOutcome = OneLive.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto FalseArm = OneLive.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = OneLive.EndArm(std::move(FalseArm->Scope), FalseArm->ArmOutput);
        MPP_CHECK(FalseOutcome.has_value());
        auto BranchOutcome = OneLive.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(BranchOutcome.has_value());
        MPP_CHECK(BranchOutcome->GetLiveArmCount() == 1U);
        auto WrongJoin = OneLive.Join(Entry->Scope,
            std::move(*BranchOutcome), JoinId);
        MPP_CHECK(!WrongJoin.has_value());
        MPP_CHECK(HasCode(WrongJoin.error(), DiagnosticCode::InvalidBranchOutcome));
        auto Continued = OneLive.ContinueWith(Entry->Scope,
            std::move(*BranchOutcome), SequenceId);
        MPP_CHECK(Continued.has_value());
        MPP_CHECK(Continued->Node.GetIdentifier() == NodeInstanceId(3U));
        MPP_CHECK(OneLive.EndEntry(std::move(Entry->Scope)).has_value());
        auto OneLiveFinal = std::move(OneLive).Finalize();
        MPP_CHECK(OneLiveFinal.has_value());
        MPP_CHECK(OneLiveFinal->GetExecutionRegions().size() == 3U);
        MPP_CHECK(OneLiveFinal->FindNode(Continued->Node.GetIdentifier())->ExecutionRegion ==
            OneLiveFinal->GetExecutionRegions()[0U].Identifier);
        MPP_CHECK(GraphIRValidator::Validate(*OneLiveFinal, Registry).empty());

    }

    void TestNoLiveBranchAndOpenScopeDiagnostics(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{false})));
        MPP_CHECK(Branch.has_value());
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        MPP_CHECK(Outcome->GetLiveArmCount() == 0U);
        auto Join = Builder.Join(Entry->Scope, std::move(*Outcome), JoinId);
        MPP_CHECK(!Join.has_value());
        MPP_CHECK(HasCode(Join.error(), DiagnosticCode::InvalidBranchOutcome));
        auto Continued = Builder.ContinueWith(Entry->Scope, std::move(*Outcome), SequenceId);
        MPP_CHECK(!Continued.has_value());
        MPP_CHECK(HasCode(Continued.error(), DiagnosticCode::InvalidBranchOutcome));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(Final->GetNodes().size() == 2U);
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());

        GraphBuilder Open(Registry);
        auto OpenEntry = Open.BeginEntry(EntryId);
        MPP_CHECK(OpenEntry.has_value());
        const DiagnosticCollection ValidateDiagnostics = Open.Validate();
        MPP_CHECK(HasCode(ValidateDiagnostics, DiagnosticCode::OpenExecutionScope));
        auto FailedFinalize = std::move(Open).Finalize();
        MPP_CHECK(!FailedFinalize.has_value());
        MPP_CHECK(HasCode(FailedFinalize.error(), DiagnosticCode::OpenExecutionScope));
        MPP_CHECK(!OpenEntry->RootOutput.IsValid());
    }

    void TestBuilderMoveWithBranchArm(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput,
            BranchId, ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Branch.has_value());
        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), FalseArm->ArmOutput);
        MPP_CHECK(FalseOutcome.has_value());
        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        GraphBuilder Moved(std::move(Builder));
        MPP_CHECK(TrueArm->ArmOutput.IsValid());
        auto Step = Moved.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(Step.has_value());
        auto TrueOutcome = Moved.EndArm(std::move(TrueArm->Scope), Step->Output);
        MPP_CHECK(TrueOutcome.has_value());
        auto Outcome = Moved.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        auto Join = Moved.Join(Entry->Scope, std::move(*Outcome), JoinId);
        MPP_CHECK(Join.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Moved).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());
    }

    void TestConditionBindingAndDataDominance(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Valid(Registry);
        auto Source = Valid.AddNode(BooleanSourceId);
        MPP_CHECK(Source.has_value());
        auto BooleanOutput = Valid.GetOutput<bool>(*Source, PinIndex(0U));
        MPP_CHECK(BooleanOutput.has_value());
        auto Entry = Valid.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Valid.BeginBranch(Entry->Scope, Entry->RootOutput, BranchId,
            ValueOrExpr<bool>(std::move(*BooleanOutput)));
        MPP_CHECK(Branch.has_value());
        ResolveEmptyBranch(Valid, *Branch);
        MPP_CHECK(Valid.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Valid).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(Final->GetInputBindings().size() == 1U);
        MPP_CHECK(Final->GetInputBindings()[0U].OutputTypeConstraint == TypeDesc::Boolean());
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());

        GraphBuilder VariableCondition(Registry);
        auto BoolVariable = VariableCondition.DeclareVariable<bool>("condition");
        MPP_CHECK(BoolVariable.has_value());
        auto VariableEntry = VariableCondition.BeginEntry(EntryId);
        MPP_CHECK(VariableEntry.has_value());
        auto VariableBranch = VariableCondition.BeginBranch(VariableEntry->Scope,
            VariableEntry->RootOutput, BranchId,
            BoolVariable->AsInput());
        MPP_CHECK(VariableBranch.has_value());
        ResolveEmptyBranch(VariableCondition, *VariableBranch);
        MPP_CHECK(VariableCondition.EndEntry(std::move(VariableEntry->Scope)).has_value());
        auto VariableFinal = std::move(VariableCondition).Finalize();
        MPP_CHECK(VariableFinal.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*VariableFinal, Registry).empty());

        GraphBuilder SharedVariable(Registry);
        auto EntryIndependent = SharedVariable.DeclareVariable<bool>("shared_condition");
        MPP_CHECK(EntryIndependent.has_value());
        auto FirstEntry = SharedVariable.BeginEntry(EntryId);
        MPP_CHECK(FirstEntry.has_value());
        auto FirstBranch = SharedVariable.BeginBranch(FirstEntry->Scope,
            FirstEntry->RootOutput, BranchId, EntryIndependent->AsInput());
        MPP_CHECK(FirstBranch.has_value());
        ResolveEmptyBranch(SharedVariable, *FirstBranch);
        MPP_CHECK(SharedVariable.EndEntry(std::move(FirstEntry->Scope)).has_value());
        auto SecondEntry = SharedVariable.BeginEntry(EntryId);
        MPP_CHECK(SecondEntry.has_value());
        auto SecondBranch = SharedVariable.BeginBranch(SecondEntry->Scope,
            SecondEntry->RootOutput, BranchId, EntryIndependent->AsInput());
        MPP_CHECK(SecondBranch.has_value());
        ResolveEmptyBranch(SharedVariable, *SecondBranch);
        MPP_CHECK(SharedVariable.EndEntry(std::move(SecondEntry->Scope)).has_value());
        auto SharedVariableFinal = std::move(SharedVariable).Finalize();
        MPP_CHECK(SharedVariableFinal.has_value());
        MPP_CHECK(SharedVariableFinal->GetExecutionEntries().size() == 2U);
        MPP_CHECK(GraphIRValidator::Validate(*SharedVariableFinal, Registry).empty());

        GraphBuilder CrossEntryProducer(Registry);
        auto ProducerEntry = CrossEntryProducer.BeginEntry(EntryId);
        MPP_CHECK(ProducerEntry.has_value());
        auto Producer = CrossEntryProducer.AppendExecutionNode(ProducerEntry->Scope,
            ProducerEntry->RootOutput, SequenceId);
        MPP_CHECK(Producer.has_value());
        auto EntryBoundValue = CrossEntryProducer.GetOutput<bool>(Producer->Node, PinIndex(2U));
        MPP_CHECK(EntryBoundValue.has_value());
        MPP_CHECK(CrossEntryProducer.EndEntry(std::move(ProducerEntry->Scope)).has_value());
        auto ConsumerEntry = CrossEntryProducer.BeginEntry(EntryId);
        MPP_CHECK(ConsumerEntry.has_value());
        auto CrossEntryBranch = CrossEntryProducer.BeginBranch(ConsumerEntry->Scope,
            ConsumerEntry->RootOutput, BranchId,
            ValueOrExpr<bool>(std::move(*EntryBoundValue)));
        MPP_CHECK(CrossEntryBranch.has_value());
        ResolveEmptyBranch(CrossEntryProducer, *CrossEntryBranch);
        MPP_CHECK(CrossEntryProducer.EndEntry(std::move(ConsumerEntry->Scope)).has_value());
        auto CrossEntryFinal = std::move(CrossEntryProducer).Finalize();
        MPP_CHECK(!CrossEntryFinal.has_value());
        MPP_CHECK(HasCode(CrossEntryFinal.error(), DiagnosticCode::ExecutionDataNotDominated));

        GraphBuilder BadLiteral(Registry);
        auto BadEntry = BadLiteral.BeginEntry(EntryId);
        MPP_CHECK(BadEntry.has_value());
        const LiteralValue IntegerLiteral(LiteralValue::Data{std::int64_t{7}});
        auto BadBranch = BadLiteral.BeginBranch(BadEntry->Scope,
            BadEntry->RootOutput, BranchId, ValueOrExpr<bool>(IntegerLiteral));
        MPP_CHECK(!BadBranch.has_value());
        MPP_CHECK(HasCode(BadBranch.error(), DiagnosticCode::IncompatibleGraphIRTypes));
    }

    void TestPreBranchDataDominatesBothArmsAndJoin(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Prefix = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Prefix.has_value());
        auto SharedValue = Builder.GetOutput<bool>(Prefix->Node, PinIndex(2U));
        MPP_CHECK(SharedValue.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Prefix->Output, BranchId,
            ValueOrExpr<bool>(*SharedValue));
        MPP_CHECK(Branch.has_value());

        auto TrueArm = Builder.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueStep = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueStep.has_value());
        MPP_CHECK(Builder.BindInput(TrueStep->Node, PinIndex(4U),
            ValueOrExpr<bool>(*SharedValue)).has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), TrueStep->Output);
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseStep = Builder.AppendExecutionNode(FalseArm->Scope,
            FalseArm->ArmOutput, SequenceId);
        MPP_CHECK(FalseStep.has_value());
        MPP_CHECK(Builder.BindInput(FalseStep->Node, PinIndex(4U),
            ValueOrExpr<bool>(*SharedValue)).has_value());
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), FalseStep->Output);
        MPP_CHECK(FalseOutcome.has_value());

        auto Outcome = Builder.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        auto Join = Builder.Join(Entry->Scope, std::move(*Outcome), JoinId);
        MPP_CHECK(Join.has_value());
        auto AfterJoin = Builder.AppendExecutionNode(Entry->Scope, Join->Output, SequenceId);
        MPP_CHECK(AfterJoin.has_value());
        MPP_CHECK(Builder.BindInput(AfterJoin->Node, PinIndex(4U),
            ValueOrExpr<bool>(*SharedValue)).has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(Final->GetInputBindings().size() == 4U);
        for (const InputBindingRecord& Binding : Final->GetInputBindings())
        {
            MPP_CHECK(Binding.OutputTypeConstraint == TypeDesc::Boolean());
        }
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());
    }

    void TestEntryRootOnlyRawGraphAndIndependentExpression(const NodeDescriptorRegistry& Registry)
    {
        GraphIR RootOnly;
        RootOnly.SetExecutionModel(ExecutionModel::Structured);
        RootOnly.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
        RootOnly.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
            ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
        RootOnly.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
        MPP_CHECK(HasCode(GraphIRValidator::Validate(RootOnly, Registry),
            DiagnosticCode::InvalidExecutionReachability));

        GraphBuilder Builder(Registry);
        auto Expression = Builder.AddNode(BooleanExpressionId);
        MPP_CHECK(Expression.has_value());
        MPP_CHECK(Builder.BindInput(*Expression, PinIndex(0U),
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true}))).has_value());
        auto ExpressionOutput = Builder.GetOutput<bool>(*Expression, PinIndex(1U));
        MPP_CHECK(ExpressionOutput.has_value());
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput, BranchId,
            ValueOrExpr<bool>(*ExpressionOutput));
        MPP_CHECK(Branch.has_value());
        ResolveEmptyBranch(Builder, *Branch);
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());
        MPP_CHECK(GraphIRValidator::Validate(*Final, Registry).empty());
    }

    void TestMalformedTopologyIsRejected(const NodeDescriptorRegistry& Registry)
    {
        GraphIR FanOut;
        FanOut.SetExecutionModel(ExecutionModel::Structured);
        FanOut.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
        FanOut.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
            ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
        FanOut.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
        FanOut.AddNode({NodeInstanceId(2U), SequenceId, ExecutionRegionId(1U)});
        FanOut.AddNode({NodeInstanceId(3U), SequenceId, ExecutionRegionId(1U)});
        FanOut.AddNode({NodeInstanceId(4U), SequenceId, ExecutionRegionId(1U)});
        FanOut.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(0U)});
        FanOut.AddControlEdge({NodeInstanceId(2U), PinIndex(1U),
            NodeInstanceId(3U), PinIndex(0U)});
        FanOut.AddControlEdge({NodeInstanceId(2U), PinIndex(1U),
            NodeInstanceId(4U), PinIndex(0U)});
        const DiagnosticCollection FanOutDiagnostics = GraphIRValidator::Validate(FanOut, Registry);
        MPP_CHECK(HasCode(FanOutDiagnostics, DiagnosticCode::ExecutionEndpointAlreadyConsumed));
        MPP_CHECK(SameDiagnostics(FanOutDiagnostics,
            GraphIRValidator::Validate(FanOut, Registry)));

        GraphIR ImplicitJoin;
        ImplicitJoin.SetExecutionModel(ExecutionModel::Structured);
        ImplicitJoin.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
        ImplicitJoin.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
            ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
        ImplicitJoin.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
            ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U),
            PinIndex(2U)});
        ImplicitJoin.AddExecutionRegion({ExecutionRegionId(3U), ExecutionEntryId(1U),
            ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U),
            PinIndex(3U)});
        ImplicitJoin.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
        ImplicitJoin.AddNode({NodeInstanceId(2U), BranchId, ExecutionRegionId(1U)});
        ImplicitJoin.AddNode({NodeInstanceId(3U), SequenceId, ExecutionRegionId(2U)});
        ImplicitJoin.AddNode({NodeInstanceId(4U), SequenceId, ExecutionRegionId(3U)});
        ImplicitJoin.AddNode({NodeInstanceId(5U), SequenceId, ExecutionRegionId(1U)});
        ImplicitJoin.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(0U)});
        ImplicitJoin.AddControlEdge({NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(3U), PinIndex(0U)});
        ImplicitJoin.AddControlEdge({NodeInstanceId(2U), PinIndex(3U),
            NodeInstanceId(4U), PinIndex(0U)});
        ImplicitJoin.AddControlEdge({NodeInstanceId(3U), PinIndex(1U),
            NodeInstanceId(5U), PinIndex(0U)});
        ImplicitJoin.AddControlEdge({NodeInstanceId(4U), PinIndex(1U),
            NodeInstanceId(5U), PinIndex(0U)});
        MPP_CHECK(HasCode(GraphIRValidator::Validate(ImplicitJoin, Registry),
            DiagnosticCode::MissingExplicitJoin));

        GraphIR Cycle;
        Cycle.SetExecutionModel(ExecutionModel::Structured);
        Cycle.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
        Cycle.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
            ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
        Cycle.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
        Cycle.AddNode({NodeInstanceId(2U), SequenceId, ExecutionRegionId(1U)});
        Cycle.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(0U)});
        Cycle.AddControlEdge({NodeInstanceId(2U), PinIndex(1U),
            NodeInstanceId(2U), PinIndex(0U)});
        MPP_CHECK(HasCode(GraphIRValidator::Validate(Cycle, Registry),
            DiagnosticCode::InvalidExecutionReachability));
    }

    void TestArmLocalDataCannotCrossBranch(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Prefix = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Prefix.has_value());
        auto Outer = Builder.BeginBranch(Entry->Scope, Prefix->Output, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Outer.has_value());

        auto TrueArm = Builder.BeginArm(Outer->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueProducer = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueProducer.has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Outer->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto CrossArmCondition = Builder.GetOutput<bool>(TrueProducer->Node, PinIndex(2U));
        MPP_CHECK(CrossArmCondition.has_value());
        auto Nested = Builder.BeginBranch(FalseArm->Scope, FalseArm->ArmOutput,
            BranchId, ValueOrExpr<bool>(std::move(*CrossArmCondition)));
        MPP_CHECK(Nested.has_value());
        ResolveEmptyBranch(Builder, *Nested);
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());

        auto OuterOutcome = Builder.EndBranch(std::move(Outer->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(OuterOutcome.has_value());
        MPP_CHECK(OuterOutcome->GetLiveArmCount() == 0U);
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(!Final.has_value());
        MPP_CHECK(HasCode(Final.error(), DiagnosticCode::ExecutionDataNotDominated));
    }

    void TestTransitiveDataOnlyProvenance(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Prefix = Builder.AppendExecutionNode(Entry->Scope, Entry->RootOutput, SequenceId);
        MPP_CHECK(Prefix.has_value());
        auto Outer = Builder.BeginBranch(Entry->Scope, Prefix->Output, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Outer.has_value());

        auto TrueArm = Builder.BeginArm(Outer->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueProducer = Builder.AppendExecutionNode(TrueArm->Scope,
            TrueArm->ArmOutput, SequenceId);
        MPP_CHECK(TrueProducer.has_value());
        auto Expression = Builder.AddNode(BooleanExpressionId);
        MPP_CHECK(Expression.has_value());
        auto TrueValue = Builder.GetOutput<bool>(TrueProducer->Node, PinIndex(2U));
        MPP_CHECK(TrueValue.has_value());
        MPP_CHECK(Builder.BindInput(*Expression, PinIndex(0U),
            ValueOrExpr<bool>(std::move(*TrueValue))).has_value());
        auto TrueOutcome = Builder.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());

        auto FalseArm = Builder.BeginArm(Outer->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto ExpressionOutput = Builder.GetOutput<bool>(*Expression, PinIndex(1U));
        MPP_CHECK(ExpressionOutput.has_value());
        auto Nested = Builder.BeginBranch(FalseArm->Scope, FalseArm->ArmOutput,
            BranchId, ValueOrExpr<bool>(std::move(*ExpressionOutput)));
        MPP_CHECK(Nested.has_value());
        ResolveEmptyBranch(Builder, *Nested);
        auto FalseOutcome = Builder.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());

        auto Outcome = Builder.EndBranch(std::move(Outer->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(!Final.has_value());
        MPP_CHECK(HasCode(Final.error(), DiagnosticCode::ExecutionDataNotDominated));
    }

    void TestConstructionFailuresAndScopeLifetime(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto WrongEntry = Builder.BeginEntry(SequenceId);
        MPP_CHECK(!WrongEntry.has_value());
        MPP_CHECK(HasCode(WrongEntry.error(), DiagnosticCode::InvalidExecutionControlRole));
        auto Missing = Builder.BeginEntry(NodeDescriptorId(9999U));
        MPP_CHECK(!Missing.has_value());
        MPP_CHECK(HasCode(Missing.error(), DiagnosticCode::MissingDescriptor));
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Concurrent = Builder.BeginEntry(EntryId);
        MPP_CHECK(!Concurrent.has_value());

        auto BadSequence = Builder.AppendExecutionNode(Entry->Scope,
            Entry->RootOutput, BranchId);
        MPP_CHECK(!BadSequence.has_value());
        MPP_CHECK(HasCode(BadSequence.error(), DiagnosticCode::InvalidExecutionControlRole));
        auto GoodSequence = Builder.AppendExecutionNode(Entry->Scope,
            Entry->RootOutput, SequenceId);
        MPP_CHECK(GoodSequence.has_value());
        auto BadBranch = Builder.BeginBranch(Entry->Scope, GoodSequence->Output,
            SequenceId, ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(!BadBranch.has_value());
        MPP_CHECK(HasCode(BadBranch.error(), DiagnosticCode::InvalidExecutionControlRole));
        MPP_CHECK(Builder.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Builder).Finalize();
        MPP_CHECK(Final.has_value());

        GraphBuilder LegacyFlow(Registry);
        MPP_CHECK(LegacyFlow.AddNode(SchemaLessFlowId).has_value());
        auto LegacyGraph = std::move(LegacyFlow).Finalize();
        MPP_CHECK(LegacyGraph.has_value());
        MPP_CHECK(LegacyGraph->GetExecutionModel() == ExecutionModel::Unstructured);

        GraphBuilder StructuredFlow(Registry);
        auto StructuredEntry = StructuredFlow.BeginEntry(EntryId);
        MPP_CHECK(StructuredEntry.has_value());
        auto SchemaLess = StructuredFlow.AddNode(SchemaLessFlowId);
        MPP_CHECK(!SchemaLess.has_value());
        MPP_CHECK(HasCode(SchemaLess.error(), DiagnosticCode::InvalidExecutionOwnership));

        GraphBuilder OpenChild(Registry);
        auto OpenEntry = OpenChild.BeginEntry(EntryId);
        MPP_CHECK(OpenEntry.has_value());
        auto OpenBranch = OpenChild.BeginBranch(OpenEntry->Scope,
            OpenEntry->RootOutput, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(OpenBranch.has_value());
        auto CloseParentEarly = OpenChild.EndEntry(std::move(OpenEntry->Scope));
        MPP_CHECK(!CloseParentEarly.has_value());
        MPP_CHECK(HasCode(CloseParentEarly.error(), DiagnosticCode::InvalidExecutionScope));
        MPP_CHECK(OpenEntry->Scope.IsValid());
        ResolveEmptyBranch(OpenChild, *OpenBranch);
        MPP_CHECK(OpenChild.EndEntry(std::move(OpenEntry->Scope)).has_value());
        MPP_CHECK(std::move(OpenChild).Finalize().has_value());

        GraphBuilder DroppedScope(Registry);
        {
            auto DroppedEntry = DroppedScope.BeginEntry(EntryId);
            MPP_CHECK(DroppedEntry.has_value());
        }
        MPP_CHECK(HasCode(DroppedScope.Validate(), DiagnosticCode::OpenExecutionScope));
        MPP_CHECK(!std::move(DroppedScope).Finalize().has_value());
    }

    void TestBuilderMoveWithActiveBranchScope(const NodeDescriptorRegistry& Registry)
    {
        GraphBuilder Builder(Registry);
        auto Entry = Builder.BeginEntry(EntryId);
        MPP_CHECK(Entry.has_value());
        auto Branch = Builder.BeginBranch(Entry->Scope, Entry->RootOutput, BranchId,
            ValueOrExpr<bool>(LiteralValue(LiteralValue::Data{true})));
        MPP_CHECK(Branch.has_value());
        GraphBuilder Moved(std::move(Builder));
        auto FalseArm = Moved.BeginArm(Branch->Scope, BranchArm::False);
        MPP_CHECK(FalseArm.has_value());
        auto FalseOutcome = Moved.EndArm(std::move(FalseArm->Scope), NoContinuation{});
        MPP_CHECK(FalseOutcome.has_value());
        auto TrueArm = Moved.BeginArm(Branch->Scope, BranchArm::True);
        MPP_CHECK(TrueArm.has_value());
        auto TrueOutcome = Moved.EndArm(std::move(TrueArm->Scope), NoContinuation{});
        MPP_CHECK(TrueOutcome.has_value());
        auto Outcome = Moved.EndBranch(std::move(Branch->Scope),
            std::move(*TrueOutcome), std::move(*FalseOutcome));
        MPP_CHECK(Outcome.has_value());
        MPP_CHECK(Moved.EndEntry(std::move(Entry->Scope)).has_value());
        auto Final = std::move(Moved).Finalize();
        MPP_CHECK(Final.has_value());
    }
}

int main()
{
    static_assert(std::is_copy_constructible_v<ExecutionHandle>);
    static_assert(std::is_copy_assignable_v<ExecutionHandle>);
    static_assert(std::is_move_constructible_v<EntryScope>);
    static_assert(!std::is_move_assignable_v<EntryScope>);
    static_assert(!std::is_copy_constructible_v<EntryScope>);
    static_assert(std::is_move_constructible_v<BranchScope>);
    static_assert(!std::is_move_assignable_v<BranchScope>);
    static_assert(!std::is_copy_constructible_v<BranchScope>);
    static_assert(std::is_move_constructible_v<BranchArmScope>);
    static_assert(!std::is_move_assignable_v<BranchArmScope>);
    static_assert(!std::is_copy_constructible_v<BranchArmScope>);
    static_assert(std::is_move_constructible_v<BranchArmOutcome>);
    static_assert(!std::is_move_assignable_v<BranchArmOutcome>);
    static_assert(!std::is_copy_constructible_v<BranchArmOutcome>);
    static_assert(std::is_move_constructible_v<BranchOutcome>);
    static_assert(!std::is_move_assignable_v<BranchOutcome>);
    static_assert(!std::is_copy_constructible_v<BranchOutcome>);
    static_assert(!std::is_default_constructible_v<EntryScope>);
    static_assert(!std::is_default_constructible_v<BranchScope>);
    static_assert(!std::is_default_constructible_v<BranchArmScope>);

    const NodeDescriptorRegistry Registry = MakeRegistry();
    TestEntriesAndDeterministicIds(Registry);
    TestExecutionIdentifierExhaustion(Registry);
    TestHandlesAndBuilderMove(Registry);
    TestBranchJoinAndEitherArmOrder(Registry);
    TestOneAndZeroLiveOutcomes(Registry);
    TestNoLiveBranchAndOpenScopeDiagnostics(Registry);
    TestBuilderMoveWithBranchArm(Registry);
    TestConditionBindingAndDataDominance(Registry);
    TestPreBranchDataDominatesBothArmsAndJoin(Registry);
    TestEntryRootOnlyRawGraphAndIndependentExpression(Registry);
    TestMalformedTopologyIsRejected(Registry);
    TestArmLocalDataCannotCrossBranch(Registry);
    TestTransitiveDataOnlyProvenance(Registry);
    TestConstructionFailuresAndScopeLifetime(Registry);
    TestBuilderMoveWithActiveBranchScope(Registry);
    return 0;
}

#undef MPP_CHECK
