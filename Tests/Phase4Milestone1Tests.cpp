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

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraphIRJson.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;
using namespace MiliastraPlusPlus::GraphIRJson;

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
        if (!Condition) Fail(Expression, Location);
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr NodeDescriptorId EntryId(1001U);
    constexpr NodeDescriptorId SequenceId(1002U);
    constexpr NodeDescriptorId BranchId(1003U);
    constexpr NodeDescriptorId JoinId(1004U);
    constexpr NodeDescriptorId LoopId(1005U);
    constexpr NodeDescriptorId InfiniteLoopId(1006U);
    constexpr NodeDescriptorId ReturnId(1007U);
    constexpr NodeDescriptorId DataId(1008U);
    constexpr NodeDescriptorId LegacyFlowSourceId(1009U);
    constexpr NodeDescriptorId LegacyFlowDestinationId(1010U);

    PinSchema FlowPin(const char* Name, PinDirection Direction,
        PinCardinality Cardinality = PinCardinality::Single)
    {
        return PinSchema(Name, TypeDesc::Flow(), Direction, PinCategory::Execution,
            Cardinality);
    }

    NodeDescriptor EntryDescriptor()
    {
        return NodeDescriptor(EntryId, "Root", {NodeAvailability::Server},
            {FlowPin("role label is ignored", PinDirection::Output)},
            EntryControlSchema{PinIndex(0U)});
    }

    NodeDescriptor SequenceDescriptor()
    {
        return NodeDescriptor(SequenceId, "Step", {NodeAvailability::Server},
            {FlowPin("arbitrary input label", PinDirection::Input),
             FlowPin("arbitrary output label", PinDirection::Output)},
            SequenceControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor BranchDescriptor()
    {
        return NodeDescriptor(BranchId, "TwoWay", {NodeAvailability::Server},
            {FlowPin("p0", PinDirection::Input),
             PinSchema("p1", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true),
             FlowPin("not named true", PinDirection::Output),
             FlowPin("not named false", PinDirection::Output)},
            BranchControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U)});
    }

    NodeDescriptor JoinDescriptor()
    {
        return NodeDescriptor(JoinId, "Reconverge", {NodeAvailability::Server},
            {FlowPin("p0", PinDirection::Input, PinCardinality::Multiple),
             FlowPin("p1", PinDirection::Output)},
            JoinControlSchema{PinIndex(0U), PinIndex(1U)});
    }

    NodeDescriptor LoopDescriptor()
    {
        return NodeDescriptor(LoopId, "ConditionalLoop", {NodeAvailability::Server},
            {FlowPin("p0", PinDirection::Input),
             FlowPin("p1", PinDirection::Output),
             FlowPin("p2", PinDirection::Output),
             FlowPin("p3", PinDirection::Input, PinCardinality::Multiple),
             FlowPin("p4", PinDirection::Input, PinCardinality::Multiple),
             PinSchema("p5", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U),
                PinIndex(3U), PinIndex(4U), LoopExitPolicy::Conditional, PinIndex(5U)});
    }

    NodeDescriptor InfiniteLoopDescriptor()
    {
        return NodeDescriptor(InfiniteLoopId, "InfiniteLoop", {NodeAvailability::Server},
            {FlowPin("p0", PinDirection::Input),
             FlowPin("p1", PinDirection::Output),
             FlowPin("p2", PinDirection::Output),
             FlowPin("p3", PinDirection::Input, PinCardinality::Multiple),
             FlowPin("p4", PinDirection::Input, PinCardinality::Multiple)},
            LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U),
                PinIndex(3U), PinIndex(4U), LoopExitPolicy::Unconditional, std::nullopt});
    }

    NodeDescriptor ReturnDescriptor()
    {
        return NodeDescriptor(ReturnId, "Finish", {NodeAvailability::Server},
            {FlowPin("p0", PinDirection::Input)},
            ReturnControlSchema{PinIndex(0U)});
    }

    NodeDescriptor DataDescriptor()
    {
        return NodeDescriptor(DataId, "ConditionData", {NodeAvailability::Server},
            {PinSchema("value", TypeDesc::Boolean(), PinDirection::Output,
                PinCategory::Data),
             PinSchema("input", TypeDesc::Boolean(), PinDirection::Input,
                PinCategory::Data, PinCardinality::Single, true)});
    }

    NodeDescriptor LegacyFlowSourceDescriptor()
    {
        return NodeDescriptor(LegacyFlowSourceId, "LegacyStart", {NodeAvailability::Server},
            {FlowPin("output", PinDirection::Output)});
    }

    NodeDescriptor LegacyFlowDestinationDescriptor()
    {
        return NodeDescriptor(LegacyFlowDestinationId, "LegacyStep", {NodeAvailability::Server},
            {FlowPin("input", PinDirection::Input)});
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Registry;
        MPP_CHECK(Registry.Register(EntryDescriptor()).has_value());
        MPP_CHECK(Registry.Register(SequenceDescriptor()).has_value());
        MPP_CHECK(Registry.Register(BranchDescriptor()).has_value());
        MPP_CHECK(Registry.Register(JoinDescriptor()).has_value());
        MPP_CHECK(Registry.Register(LoopDescriptor()).has_value());
        MPP_CHECK(Registry.Register(InfiniteLoopDescriptor()).has_value());
        MPP_CHECK(Registry.Register(ReturnDescriptor()).has_value());
        MPP_CHECK(Registry.Register(DataDescriptor()).has_value());
        MPP_CHECK(Registry.Register(LegacyFlowSourceDescriptor()).has_value());
        MPP_CHECK(Registry.Register(LegacyFlowDestinationDescriptor()).has_value());
        return Registry;
    }

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            if (Diagnostic.Code == Code) return true;
        }
        return false;
    }

    void CheckDeserializationRejects(const nlohmann::json& Malformed)
    {
        MPP_CHECK(!Deserialize(Malformed).has_value());
    }

    GraphIR MakeStraightLineGraph()
    {
        GraphIR Graph;
        Graph.SetExecutionModel(ExecutionModel::Structured);
        Graph.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
        Graph.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
            ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
        Graph.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
        Graph.AddNode({NodeInstanceId(2U), SequenceId, ExecutionRegionId(1U)});
        Graph.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(0U)});
        return Graph;
    }

    GraphIR MakeBranchGraph()
    {
        GraphIR Graph = MakeStraightLineGraph();
        Graph.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
            ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(3U), PinIndex(2U)});
        Graph.AddExecutionRegion({ExecutionRegionId(3U), ExecutionEntryId(1U),
            ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(3U), PinIndex(3U)});
        Graph.AddNode({NodeInstanceId(3U), BranchId, ExecutionRegionId(1U)});
        Graph.AddNode({NodeInstanceId(4U), SequenceId, ExecutionRegionId(2U)});
        Graph.AddNode({NodeInstanceId(5U), SequenceId, ExecutionRegionId(3U)});
        return Graph;
    }

    GraphIR MakeLoopGraph()
    {
        GraphIR Graph = MakeStraightLineGraph();
        Graph.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
            ExecutionRegionKind::LoopBody, ExecutionRegionId(1U), NodeInstanceId(3U), PinIndex(1U)});
        Graph.AddNode({NodeInstanceId(3U), LoopId, ExecutionRegionId(1U)});
        Graph.AddNode({NodeInstanceId(4U), SequenceId, ExecutionRegionId(2U)});
        return Graph;
    }
}

int main()
{
    NodeDescriptorRegistry Registry = MakeRegistry();

    // Typed IDs are graph-local, nonzero, and do not alias other ID domains.
    static_assert(!std::is_convertible_v<ExecutionEntryId, ExecutionRegionId>);
    MPP_CHECK(!ExecutionEntryId{}.IsValid());
    MPP_CHECK(ExecutionEntryId(1U).IsValid());
    MPP_CHECK(!ExecutionRegionId{}.IsValid());
    MPP_CHECK(ExecutionRegionId(1U).IsValid());
    MPP_CHECK(GraphIR{}.GetExecutionModel() == ExecutionModel::Unstructured);

    GraphIR EmptyUnstructured;
    const nlohmann::json EmptyUnstructuredJson = Serialize(EmptyUnstructured);
    MPP_CHECK(EmptyUnstructuredJson["irVersion"] == 3U);
    MPP_CHECK(EmptyUnstructuredJson["executionModel"] == "Unstructured");
    MPP_CHECK(EmptyUnstructuredJson["executionEntries"].empty());
    MPP_CHECK(EmptyUnstructuredJson["executionRegions"].empty());
    const auto EmptyUnstructuredRoundTrip = Deserialize(EmptyUnstructuredJson);
    MPP_CHECK(EmptyUnstructuredRoundTrip.has_value());
    MPP_CHECK(EmptyUnstructuredRoundTrip->GetExecutionModel() == ExecutionModel::Unstructured);
    MPP_CHECK(GraphIRValidator::Validate(*EmptyUnstructuredRoundTrip, Registry).empty());

    // Trusted descriptor schemas use indices; deliberately non-semantic names validate.
    MPP_CHECK(EntryDescriptor().IsValid());
    MPP_CHECK(SequenceDescriptor().IsValid());
    MPP_CHECK(BranchDescriptor().IsValid());
    MPP_CHECK(JoinDescriptor().IsValid());
    MPP_CHECK(LoopDescriptor().IsValid());
    MPP_CHECK(InfiniteLoopDescriptor().IsValid());
    MPP_CHECK(ReturnDescriptor().IsValid());

    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1100U), "bad pins", {}, {
        PinSchema("", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1101U), "duplicate names", {}, {
        PinSchema("same", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data),
        PinSchema("same", TypeDesc::Boolean(), PinDirection::Output, PinCategory::Data)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1102U), "bad direction", {}, {
        PinSchema("p", TypeDesc::Boolean(), static_cast<PinDirection>(77), PinCategory::Data)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1103U), "bad category", {}, {
        PinSchema("p", TypeDesc::Boolean(), PinDirection::Input, static_cast<PinCategory>(77))
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1104U), "bad cardinality", {}, {
        PinSchema("p", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data,
            static_cast<PinCardinality>(77))
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1105U), "bad availability",
        {static_cast<NodeAvailability>(77)}, {}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1106U), "duplicate availability",
        {NodeAvailability::Server, NodeAvailability::Server}, {}).IsValid());
    MPP_CHECK(NodeDescriptor(NodeDescriptorId(1107U), "legacy Flow", {}, {
        FlowPin("not data", PinDirection::Input)
    }).IsValid()); // Legacy Flow descriptors remain valid without a role schema.
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1108U), "wrong Flow type", {}, {
        PinSchema("flow", TypeDesc::Integer(), PinDirection::Input, PinCategory::Execution)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1118U), "Flow marked as Data", {}, {
        PinSchema("flow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Data)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1119U), "Generic marked as Execution", {}, {
        PinSchema("flow", TypeDesc::Generic(GenericParameterId(1U)),
            PinDirection::Input, PinCategory::Execution)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1109U), "literal Flow", {}, {
        PinSchema("flow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution,
            PinCardinality::Single, true)
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1120U), "Flow default", {}, {
        PinSchema("flow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution,
            PinCardinality::Single, false,
            LiteralValue(LiteralValue::Data{true}))
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1110U), "output default", {}, {
        PinSchema("p", TypeDesc::Boolean(), PinDirection::Output, PinCategory::Data,
            PinCardinality::Single, true, LiteralValue(LiteralValue::Data{true}))
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1111U), "bad default", {}, {
        PinSchema("p", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data,
            PinCardinality::Single, true,
            LiteralValue(LiteralValue::Data{std::int64_t{1}}))
    }).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1112U), "bad Entry role", {}, {
        FlowPin("p0", PinDirection::Input)
    }, EntryControlSchema{PinIndex(0U)}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1113U), "duplicate Sequence role", {}, {
        FlowPin("p0", PinDirection::Input), FlowPin("p1", PinDirection::Output)
    }, SequenceControlSchema{PinIndex(0U), PinIndex(0U)}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1114U), "undeclared Flow role", {}, {
        FlowPin("p0", PinDirection::Input), FlowPin("p1", PinDirection::Output),
        FlowPin("p2", PinDirection::Output)
    }, SequenceControlSchema{PinIndex(0U), PinIndex(1U)}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1115U), "bad Branch condition", {}, {
        FlowPin("p0", PinDirection::Input),
        PinSchema("p1", TypeDesc::Integer(), PinDirection::Input, PinCategory::Data),
        FlowPin("p2", PinDirection::Output), FlowPin("p3", PinDirection::Output)
    }, BranchControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U)}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1116U), "bad Join cardinality", {}, {
        FlowPin("p0", PinDirection::Input), FlowPin("p1", PinDirection::Output)
    }, JoinControlSchema{PinIndex(0U), PinIndex(1U)}).IsValid());
    MPP_CHECK(!NodeDescriptor(NodeDescriptorId(1117U), "bad Loop policy", {}, {
        FlowPin("p0", PinDirection::Input), FlowPin("p1", PinDirection::Output),
        FlowPin("p2", PinDirection::Output),
        FlowPin("p3", PinDirection::Input, PinCardinality::Multiple),
        FlowPin("p4", PinDirection::Input, PinCardinality::Multiple)
    }, LoopControlSchema{PinIndex(0U), PinIndex(1U), PinIndex(2U), PinIndex(3U),
        PinIndex(4U), LoopExitPolicy::Conditional, std::nullopt}).IsValid());

    // Structured entry and straight-line records validate and survive v3 round trips.
    GraphIR StraightLine = MakeStraightLineGraph();
    MPP_CHECK(GraphIRValidator::Validate(StraightLine, Registry).empty());
    const nlohmann::json StraightJson = Serialize(StraightLine);
    MPP_CHECK(StraightJson["irVersion"] == 3U);
    MPP_CHECK(StraightJson["executionModel"] == "Structured");
    MPP_CHECK(StraightJson["executionEntries"].size() == 1U);
    MPP_CHECK(StraightJson["executionRegions"].size() == 1U);
    MPP_CHECK(StraightJson["nodes"][0U]["executionRegion"] == 1U);
    MPP_CHECK(!StraightJson.contains("controlRoles"));
    MPP_CHECK(!StraightJson.contains("executionEdges"));
    MPP_CHECK(!StraightJson.contains("successors"));
    MPP_CHECK(!StraightJson.contains("executionTerminators"));
    MPP_CHECK(StraightJson.dump() == Serialize(StraightLine).dump());
    const auto StraightRoundTrip = Deserialize(StraightJson);
    MPP_CHECK(StraightRoundTrip.has_value());
    MPP_CHECK(GraphIRValidator::Validate(*StraightRoundTrip, Registry).empty());
    MPP_CHECK(Serialize(*StraightRoundTrip).dump() == StraightJson.dump());

    // Multiple roots own disjoint Entry regions; vector order does not infer connectivity.
    GraphIR MultipleEntries;
    MultipleEntries.SetExecutionModel(ExecutionModel::Structured);
    MultipleEntries.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    MultipleEntries.AddExecutionEntry({ExecutionEntryId(2U), NodeInstanceId(3U)});
    MultipleEntries.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    MultipleEntries.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(2U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    MultipleEntries.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
    MultipleEntries.AddNode({NodeInstanceId(2U), SequenceId, ExecutionRegionId(1U)});
    MultipleEntries.AddNode({NodeInstanceId(3U), EntryId, ExecutionRegionId(2U)});
    MultipleEntries.AddNode({NodeInstanceId(4U), SequenceId, ExecutionRegionId(2U)});
    MultipleEntries.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
        NodeInstanceId(2U), PinIndex(0U)});
    MultipleEntries.AddControlEdge({NodeInstanceId(3U), PinIndex(0U),
        NodeInstanceId(4U), PinIndex(0U)});
    MPP_CHECK(GraphIRValidator::Validate(MultipleEntries, Registry).empty());
    const auto MultipleRoundTrip = Deserialize(Serialize(MultipleEntries));
    MPP_CHECK(MultipleRoundTrip.has_value());
    MPP_CHECK(MultipleRoundTrip->GetExecutionEntries().size() == 2U);
    MPP_CHECK(GraphIRValidator::Validate(*MultipleRoundTrip, Registry).empty());

    // Branch arm owner output indices and LoopBody output indices are role metadata, not names.
    GraphIR Branch = MakeBranchGraph();
    MPP_CHECK(GraphIRValidator::Validate(Branch, Registry).empty());
    const auto BranchRoundTrip = Deserialize(Serialize(Branch));
    MPP_CHECK(BranchRoundTrip.has_value());
    MPP_CHECK(BranchRoundTrip->GetExecutionRegions()[1U].OwnerOutputPin == PinIndex(2U));
    MPP_CHECK(BranchRoundTrip->GetExecutionRegions()[2U].OwnerOutputPin == PinIndex(3U));
    MPP_CHECK(GraphIRValidator::Validate(*BranchRoundTrip, Registry).empty());

    GraphIR Loop = MakeLoopGraph();
    MPP_CHECK(GraphIRValidator::Validate(Loop, Registry).empty());
    const auto LoopRoundTrip = Deserialize(Serialize(Loop));
    MPP_CHECK(LoopRoundTrip.has_value());
    MPP_CHECK(LoopRoundTrip->GetExecutionRegions()[1U].Kind == ExecutionRegionKind::LoopBody);
    MPP_CHECK(GraphIRValidator::Validate(*LoopRoundTrip, Registry).empty());

    // Data-only nodes remain outside execution ownership and encode an explicit null membership.
    GraphIR WithData = MakeStraightLineGraph();
    WithData.AddNode({NodeInstanceId(3U), DataId, std::nullopt});
    MPP_CHECK(GraphIRValidator::Validate(WithData, Registry).empty());
    const auto DataRoundTrip = Deserialize(Serialize(WithData));
    MPP_CHECK(DataRoundTrip.has_value());
    MPP_CHECK(DataRoundTrip->GetNodes().back().ExecutionRegion == std::nullopt);
    MPP_CHECK(Serialize(WithData)["nodes"].back()["executionRegion"].is_null());

    // Raw v3 malformed ownership is not inferred or repaired by validation.
    GraphIR MissingMembership = StraightLine;
    MissingMembership.AddNode({NodeInstanceId(3U), SequenceId, std::nullopt});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(MissingMembership, Registry),
        DiagnosticCode::InvalidExecutionOwnership));
    const auto ParsedMissingMembership = Deserialize(Serialize(MissingMembership));
    MPP_CHECK(ParsedMissingMembership.has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedMissingMembership, Registry),
        DiagnosticCode::InvalidExecutionOwnership));
    GraphIR DuplicateEntryId = StraightLine;
    DuplicateEntryId.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(DuplicateEntryId, Registry),
        DiagnosticCode::DuplicateExecutionEntryIdentifier));
    GraphIR DuplicateRegionId = StraightLine;
    DuplicateRegionId.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(DuplicateRegionId, Registry),
        DiagnosticCode::DuplicateExecutionRegionIdentifier));

    GraphIR BadParent;
    BadParent.SetExecutionModel(ExecutionModel::Structured);
    BadParent.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    BadParent.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    BadParent.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(99U), NodeInstanceId(2U), PinIndex(2U)});
    BadParent.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
    BadParent.AddNode({NodeInstanceId(2U), BranchId, ExecutionRegionId(1U)});
    BadParent.AddExecutionRegion({ExecutionRegionId(3U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U), PinIndex(3U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(BadParent, Registry),
        DiagnosticCode::InvalidExecutionRegion));
    const auto ParsedBadParent = Deserialize(Serialize(BadParent));
    MPP_CHECK(ParsedBadParent.has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedBadParent, Registry),
        DiagnosticCode::InvalidExecutionRegion));

    GraphIR MissingOwner;
    MissingOwner.SetExecutionModel(ExecutionModel::Structured);
    MissingOwner.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    MissingOwner.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    MissingOwner.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(99U), PinIndex(2U)});
    MissingOwner.AddExecutionRegion({ExecutionRegionId(3U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U), PinIndex(3U)});
    MissingOwner.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
    MissingOwner.AddNode({NodeInstanceId(2U), BranchId, ExecutionRegionId(1U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(MissingOwner, Registry),
        DiagnosticCode::InvalidExecutionRegion));
    const auto ParsedMissingOwner = Deserialize(Serialize(MissingOwner));
    MPP_CHECK(ParsedMissingOwner.has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedMissingOwner, Registry),
        DiagnosticCode::InvalidExecutionRegion));

    GraphIR WrongOwnerPin;
    WrongOwnerPin.SetExecutionModel(ExecutionModel::Structured);
    WrongOwnerPin.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    WrongOwnerPin.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    WrongOwnerPin.AddExecutionRegion({ExecutionRegionId(2U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U), PinIndex(17U)});
    WrongOwnerPin.AddExecutionRegion({ExecutionRegionId(3U), ExecutionEntryId(1U),
        ExecutionRegionKind::BranchArm, ExecutionRegionId(1U), NodeInstanceId(2U), PinIndex(3U)});
    WrongOwnerPin.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
    WrongOwnerPin.AddNode({NodeInstanceId(2U), BranchId, ExecutionRegionId(1U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(WrongOwnerPin, Registry),
        DiagnosticCode::InvalidExecutionRegion));
    const auto ParsedWrongOwnerPin = Deserialize(Serialize(WrongOwnerPin));
    MPP_CHECK(ParsedWrongOwnerPin.has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedWrongOwnerPin, Registry),
        DiagnosticCode::InvalidExecutionRegion));

    GraphIR NoEntries;
    NoEntries.SetExecutionModel(ExecutionModel::Structured);
    MPP_CHECK(HasCode(GraphIRValidator::Validate(NoEntries, Registry),
        DiagnosticCode::InvalidExecutionModel));
    GraphIR MixedMode;
    MixedMode.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(MixedMode, Registry),
        DiagnosticCode::InvalidExecutionModel));
    nlohmann::json V3MixedMode = Serialize(MixedMode);
    MPP_CHECK(Deserialize(V3MixedMode).has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*Deserialize(V3MixedMode), Registry),
        DiagnosticCode::InvalidExecutionModel));
    GraphIR InvalidMode;
    InvalidMode.SetExecutionModel(static_cast<ExecutionModel>(88));
    MPP_CHECK(HasCode(GraphIRValidator::Validate(InvalidMode, Registry),
        DiagnosticCode::InvalidExecutionModel));

    GraphIR UnschematizedFlow;
    UnschematizedFlow.SetExecutionModel(ExecutionModel::Structured);
    UnschematizedFlow.AddExecutionEntry({ExecutionEntryId(1U), NodeInstanceId(1U)});
    UnschematizedFlow.AddExecutionRegion({ExecutionRegionId(1U), ExecutionEntryId(1U),
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    UnschematizedFlow.AddNode({NodeInstanceId(1U), EntryId, ExecutionRegionId(1U)});
    UnschematizedFlow.AddNode({NodeInstanceId(2U), LegacyFlowSourceId, ExecutionRegionId(1U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(UnschematizedFlow, Registry),
        DiagnosticCode::InvalidExecutionOwnership));

    GraphIR CrossEntry = MultipleEntries;
    CrossEntry.AddControlEdge({NodeInstanceId(1U), PinIndex(0U),
        NodeInstanceId(4U), PinIndex(0U)});
    MPP_CHECK(HasCode(GraphIRValidator::Validate(CrossEntry, Registry),
        DiagnosticCode::InvalidExecutionOwnership));

    // Legacy v1 and v2 remain Unstructured, keep Flow edges, and upgrade only on serialization.
    nlohmann::json V1 = {
        {"irVersion", 1U},
        {"nodes", nlohmann::json::array({
            {{"id", 1U}, {"descriptor", LegacyFlowSourceId.GetValue()}},
            {{"id", 2U}, {"descriptor", LegacyFlowDestinationId.GetValue()}}
        })},
        {"variables", nlohmann::json::array()},
        {"inputBindings", nlohmann::json::array()},
        {"controlEdges", nlohmann::json::array({{
            {"sourceNode", 1U}, {"sourcePin", 0U},
            {"destinationNode", 2U}, {"destinationPin", 0U}
        }})}
    };
    const auto V1Graph = Deserialize(V1);
    MPP_CHECK(V1Graph.has_value());
    MPP_CHECK(V1Graph->GetExecutionModel() == ExecutionModel::Unstructured);
    MPP_CHECK(V1Graph->GetExecutionEntries().empty());
    MPP_CHECK(GraphIRValidator::Validate(*V1Graph, Registry).empty());
    const nlohmann::json V1Upgraded = Serialize(*V1Graph);
    MPP_CHECK(V1Upgraded["irVersion"] == 3U);
    MPP_CHECK(V1Upgraded["executionModel"] == "Unstructured");
    MPP_CHECK(V1Upgraded["nodes"][0U]["executionRegion"].is_null());
    MPP_CHECK(V1Upgraded["controlEdges"].size() == 1U);

    GraphIR TypedLegacy;
    TypedLegacy.AddNode({NodeInstanceId(1U), DataId});
    TypedLegacy.AddNode({NodeInstanceId(2U), DataId});
    TypedLegacy.BindInput(NodeInstanceId(2U), PinIndex(1U),
        OutputReference{NodeInstanceId(1U), PinIndex(0U)}, TypeDesc::Boolean());
    nlohmann::json V2 = Serialize(TypedLegacy);
    V2["irVersion"] = 2U;
    V2.erase("executionModel");
    V2.erase("executionEntries");
    V2.erase("executionRegions");
    for (nlohmann::json& Node : V2["nodes"]) Node.erase("executionRegion");
    const auto V2Graph = Deserialize(V2);
    MPP_CHECK(V2Graph.has_value());
    MPP_CHECK(V2Graph->GetExecutionModel() == ExecutionModel::Unstructured);
    MPP_CHECK(V2Graph->GetInputBindings()[0U].OutputTypeConstraint == TypeDesc::Boolean());
    MPP_CHECK(GraphIRValidator::Validate(*V2Graph, Registry).empty());
    const nlohmann::json V2Upgraded = Serialize(*V2Graph);
    MPP_CHECK(V2Upgraded["irVersion"] == 3U);
    MPP_CHECK(V2Upgraded["inputBindings"][0U]["outputTypeConstraint"]["kind"] == "Boolean");

    // Every v3 strong-ID field rejects 2^64, which is outside ValueIdentifier's uint64_t range.
    const nlohmann::json OversizedIdentifier =
        nlohmann::json::parse("18446744073709551616");
    MPP_CHECK(OversizedIdentifier.is_number_float());

    nlohmann::json OversizedEntryId = Serialize(StraightLine);
    OversizedEntryId["executionEntries"][0U]["id"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedEntryId);

    nlohmann::json OversizedRootNode = Serialize(StraightLine);
    OversizedRootNode["executionEntries"][0U]["rootNode"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedRootNode);

    nlohmann::json OversizedRegionId = Serialize(StraightLine);
    OversizedRegionId["executionRegions"][0U]["id"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedRegionId);

    nlohmann::json OversizedRegionEntry = Serialize(StraightLine);
    OversizedRegionEntry["executionRegions"][0U]["entry"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedRegionEntry);

    nlohmann::json OversizedParent = Serialize(Branch);
    OversizedParent["executionRegions"][1U]["parent"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedParent);

    nlohmann::json OversizedOwnerNode = Serialize(Branch);
    OversizedOwnerNode["executionRegions"][1U]["ownerNode"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedOwnerNode);

    nlohmann::json OversizedNodeRegion = Serialize(StraightLine);
    OversizedNodeRegion["nodes"][0U]["executionRegion"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedNodeRegion);

    GraphIR VariableIdGraph;
    VariableIdGraph.AddVariable({GraphVariableId(1U), "persisted", TypeDesc::Boolean(),
        std::nullopt});
    nlohmann::json OversizedVariableId = Serialize(VariableIdGraph);
    OversizedVariableId["variables"][0U]["id"] = OversizedIdentifier;
    CheckDeserializationRejects(OversizedVariableId);

    // The complete uint64_t maximum remains a valid, losslessly round-tripped strong ID.
    const std::uint64_t MaximumIdentifier = std::numeric_limits<std::uint64_t>::max();
    const NodeInstanceId MaximumNodeId(MaximumIdentifier);
    const ExecutionEntryId MaximumEntryId(MaximumIdentifier);
    const ExecutionRegionId MaximumRegionId(MaximumIdentifier);
    GraphIR MaximumIdentifiers;
    MaximumIdentifiers.SetExecutionModel(ExecutionModel::Structured);
    MaximumIdentifiers.AddExecutionEntry({MaximumEntryId, MaximumNodeId});
    MaximumIdentifiers.AddExecutionRegion({MaximumRegionId, MaximumEntryId,
        ExecutionRegionKind::Entry, std::nullopt, std::nullopt, std::nullopt});
    MaximumIdentifiers.AddNode({MaximumNodeId, EntryId, MaximumRegionId});
    MaximumIdentifiers.AddNode({NodeInstanceId(MaximumIdentifier - 1U), SequenceId,
        MaximumRegionId});
    MaximumIdentifiers.AddControlEdge({MaximumNodeId, PinIndex(0U),
        NodeInstanceId(MaximumIdentifier - 1U), PinIndex(0U)});
    MPP_CHECK(GraphIRValidator::Validate(MaximumIdentifiers, Registry).empty());
    const auto MaximumIdentifiersRoundTrip = Deserialize(Serialize(MaximumIdentifiers));
    MPP_CHECK(MaximumIdentifiersRoundTrip.has_value());
    MPP_CHECK(MaximumIdentifiersRoundTrip->GetNodes()[0U].Identifier.GetValue() ==
        MaximumIdentifier);
    MPP_CHECK(MaximumIdentifiersRoundTrip->GetExecutionEntries()[0U].Identifier.GetValue() ==
        MaximumIdentifier);
    MPP_CHECK(MaximumIdentifiersRoundTrip->GetExecutionRegions()[0U].Identifier.GetValue() ==
        MaximumIdentifier);
    MPP_CHECK(GraphIRValidator::Validate(*MaximumIdentifiersRoundTrip, Registry).empty());

    // The same bounded path protects pin indices, which use the project's uint32_t PinIndex.
    nlohmann::json OversizedOwnerPin = Serialize(Branch);
    OversizedOwnerPin["executionRegions"][1U]["ownerOutputPin"] = MaximumIdentifier;
    CheckDeserializationRejects(OversizedOwnerPin);

    nlohmann::json SignedIdentifier = Serialize(StraightLine);
    SignedIdentifier["executionEntries"][0U]["id"] = -1;
    CheckDeserializationRejects(SignedIdentifier);

    // Strict v3 fields reject malformed records; no malformed execution data is ignored.
    nlohmann::json BadModelJson = Serialize(StraightLine);
    BadModelJson["executionModel"] = "GuessFromEdges";
    MPP_CHECK(!Deserialize(BadModelJson).has_value());
    nlohmann::json MissingArrays = Serialize(StraightLine);
    MissingArrays.erase("executionEntries");
    MPP_CHECK(!Deserialize(MissingArrays).has_value());
    nlohmann::json MissingNodeRegion = Serialize(StraightLine);
    MissingNodeRegion["nodes"][1U].erase("executionRegion");
    MPP_CHECK(!Deserialize(MissingNodeRegion).has_value());
    nlohmann::json WrongNodeRegion = Serialize(StraightLine);
    WrongNodeRegion["nodes"][1U]["executionRegion"] = "1";
    MPP_CHECK(!Deserialize(WrongNodeRegion).has_value());
    nlohmann::json MissingRegionParent = Serialize(Branch);
    MissingRegionParent["executionRegions"][1U].erase("parent");
    MPP_CHECK(!Deserialize(MissingRegionParent).has_value());
    nlohmann::json BadRegionKind = Serialize(Branch);
    BadRegionKind["executionRegions"][1U]["kind"] = "FutureArm";
    MPP_CHECK(!Deserialize(BadRegionKind).has_value());
    nlohmann::json BadOwnerPinJson = Serialize(Branch);
    BadOwnerPinJson["executionRegions"][1U]["ownerOutputPin"] = "2";
    MPP_CHECK(!Deserialize(BadOwnerPinJson).has_value());
    nlohmann::json MissingEntryRoot = Serialize(StraightLine);
    MissingEntryRoot["executionEntries"][0U].erase("rootNode");
    MPP_CHECK(!Deserialize(MissingEntryRoot).has_value());

    // JSON parsing preserves malformed-but-shaped structured graphs for the sole validator to reject.
    nlohmann::json StructuredWithoutEntry = Serialize(StraightLine);
    StructuredWithoutEntry["executionEntries"] = nlohmann::json::array();
    const auto ParsedWithoutEntry = Deserialize(StructuredWithoutEntry);
    MPP_CHECK(ParsedWithoutEntry.has_value());
    MPP_CHECK(HasCode(GraphIRValidator::Validate(*ParsedWithoutEntry, Registry),
        DiagnosticCode::InvalidExecutionModel));

    nlohmann::json V2WithNewFields = V1;
    V2WithNewFields["irVersion"] = 2U;
    V2WithNewFields["nodes"][0U]["executionRegion"] = nullptr;
    MPP_CHECK(!Deserialize(V2WithNewFields).has_value());
    nlohmann::json Unsupported = Serialize(GraphIR{});
    Unsupported["irVersion"] = 4U;
    MPP_CHECK(!Deserialize(Unsupported).has_value());

    return 0;
}

#undef MPP_CHECK
