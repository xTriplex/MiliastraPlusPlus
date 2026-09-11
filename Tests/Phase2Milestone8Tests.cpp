#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;

namespace
{
    void Check(bool Condition, const char* Expression, int Line)
    {
        if (!Condition)
        {
            std::fprintf(stderr, "CHECK FAILED: %s (line %d)\n", Expression, Line);
            std::abort();
        }
    }

    #define Check(Condition) Check((Condition), #Condition, __LINE__)

    constexpr NodeDescriptorId SourceDescriptorId(801U);
    constexpr NodeDescriptorId DestinationDescriptorId(802U);
    constexpr NodeInstanceId SourceNodeId(1001U);
    constexpr NodeInstanceId DestinationNodeId(1002U);
    constexpr PinIndex SourceFlowOutput(1U);
    constexpr PinIndex SourceDataOutput(2U);
    constexpr PinIndex DestinationFlowInput(0U);
    constexpr PinIndex DestinationDataInput(2U);

    NodeDescriptor MakeSourceDescriptor()
    {
        return NodeDescriptor(
            SourceDescriptorId,
            "FixtureSource",
            {NodeAvailability::Server},
            {
                PinSchema("Execute In", TypeDesc::Flow(), PinDirection::Input,
                    PinCategory::Execution),
                PinSchema("Execute Out", TypeDesc::Flow(), PinDirection::Output,
                    PinCategory::Execution),
                PinSchema("Value", TypeDesc::Integer(), PinDirection::Output,
                    PinCategory::Data)
            }
        );
    }

    NodeDescriptor MakeDestinationDescriptor()
    {
        return NodeDescriptor(
            DestinationDescriptorId,
            "FixtureDestination",
            {NodeAvailability::Server},
            {
                PinSchema("Execute In", TypeDesc::Flow(), PinDirection::Input,
                    PinCategory::Execution),
                PinSchema("Execute Out", TypeDesc::Flow(), PinDirection::Output,
                    PinCategory::Execution),
                PinSchema("Value", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data),
                PinSchema("Optional Value", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Optional, true,
                    LiteralValue(LiteralValue::Data(std::int64_t(0))))
            }
        );
    }

    NodeDescriptorRegistry MakeDescriptors()
    {
        NodeDescriptorRegistry Descriptors;
        Check(Descriptors.Register(MakeSourceDescriptor()).has_value());
        Check(Descriptors.Register(MakeDestinationDescriptor()).has_value());
        return Descriptors;
    }

    GraphIR MakeRepresentativeGraph()
    {
        GraphIR Graph;
        Graph.AddNode(NodeInstance{SourceNodeId, SourceDescriptorId});
        Graph.AddNode(NodeInstance{DestinationNodeId, DestinationDescriptorId});
        Graph.BindInput(
            DestinationNodeId,
            DestinationDataInput,
            OutputReference{SourceNodeId, SourceDataOutput}
        );
        Graph.AddControlEdge(ControlEdge{
            SourceNodeId,
            SourceFlowOutput,
            DestinationNodeId,
            DestinationFlowInput
        });
        return Graph;
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
}

int main()
{
    NodeDescriptorRegistry Descriptors = MakeDescriptors();
    Check(Descriptors.Size() == 2U);
    Check(Descriptors.Find(SourceDescriptorId) != nullptr);
    Check(Descriptors.Find(DestinationDescriptorId) != nullptr);

    const GraphIR RepresentativeGraph = MakeRepresentativeGraph();
    Check(RepresentativeGraph.GetNodeCount() == 2U);
    Check(RepresentativeGraph.GetInputBindingCount() == 1U);
    Check(RepresentativeGraph.GetControlEdgeCount() == 1U);
    Check(std::get<OutputReference>(
        RepresentativeGraph.GetInputBindings()[0].Binding).SourceNode == SourceNodeId);
    Check(std::get<OutputReference>(
        RepresentativeGraph.GetInputBindings()[0].Binding).SourceOutputPin == SourceDataOutput);
    Check(RepresentativeGraph.GetControlEdges()[0].SourceOutputPin == SourceFlowOutput);
    Check(RepresentativeGraph.GetControlEdges()[0].DestinationInputPin == DestinationFlowInput);
    Check(GraphIRValidator::Validate(RepresentativeGraph, Descriptors).empty());

    GraphIR InvalidDescriptorGraph = RepresentativeGraph;
    InvalidDescriptorGraph.AddNode(NodeInstance{
        NodeInstanceId(1003U), NodeDescriptorId(999U)
    });
    const DiagnosticCollection InvalidDescriptorDiagnostics =
        GraphIRValidator::Validate(InvalidDescriptorGraph, Descriptors);
    Check(HasCode(InvalidDescriptorDiagnostics, DiagnosticCode::MissingDescriptor));

    GraphIR IncompatibleBindingGraph = RepresentativeGraph;
    IncompatibleBindingGraph.BindInput(
        DestinationNodeId,
        DestinationDataInput,
        OutputReference{SourceNodeId, SourceFlowOutput}
    );
    const DiagnosticCollection IncompatibleBindingDiagnostics =
        GraphIRValidator::Validate(IncompatibleBindingGraph, Descriptors);
    Check(HasCode(IncompatibleBindingDiagnostics, DiagnosticCode::DuplicateInputBinding));
    Check(HasCode(IncompatibleBindingDiagnostics, DiagnosticCode::IncompatibleGraphIRTypes));

    GraphIR InvalidPinGraph = RepresentativeGraph;
    InvalidPinGraph.BindInput(
        DestinationNodeId,
        DestinationDataInput,
        OutputReference{SourceNodeId, PinIndex(99U)}
    );
    const DiagnosticCollection InvalidPinDiagnostics =
        GraphIRValidator::Validate(InvalidPinGraph, Descriptors);
    Check(HasCode(InvalidPinDiagnostics, DiagnosticCode::InvalidGraphIRPinReference));

    GraphIR InvalidControlEdgeGraph = RepresentativeGraph;
    InvalidControlEdgeGraph.AddControlEdge(ControlEdge{
        SourceNodeId,
        SourceDataOutput,
        DestinationNodeId,
        DestinationFlowInput
    });
    const DiagnosticCollection InvalidControlEdgeDiagnostics =
        GraphIRValidator::Validate(InvalidControlEdgeGraph, Descriptors);
    Check(HasCode(InvalidControlEdgeDiagnostics, DiagnosticCode::InvalidControlEdge));

    return 0;
}
