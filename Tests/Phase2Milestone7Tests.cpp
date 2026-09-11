#include <cstdlib>
#include <memory>

#include "MiliastraPlusPlusGraphIRAdapter.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;

namespace
{
    void Check(bool Condition, const char* Expression, int Line)
    {
        if (!Condition)
        {
            std::fprintf(
                stderr,
                "CHECK FAILED: %s (line %d)\n",
                Expression,
                Line
            );
            std::abort();
        }
    }

    #define Check(Condition) Check((Condition), #Condition, __LINE__)

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

    GraphIRNodeMapping MakeMapping(NodeIdentifier Node)
    {
        return GraphIRNodeMapping{
            Node,
            NodeDescriptorId(100U),
            {
                {PinIdentifier(10U), PinIndex(0U)},
                {PinIdentifier(11U), PinIndex(1U)},
                {PinIdentifier(12U), PinIndex(2U)},
                {PinIdentifier(13U), PinIndex(3U)}
            }
        };
    }

    NodeDescriptor MakeDescriptor()
    {
        return NodeDescriptor(
            NodeDescriptorId(100U),
            "MappedNode",
            {NodeAvailability::Server},
            {
                PinSchema("Input", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, false),
                PinSchema("Output", TypeDesc::Integer(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("FlowInput", TypeDesc::Flow(), PinDirection::Input,
                    PinCategory::Execution),
                PinSchema("FlowOutput", TypeDesc::Flow(), PinDirection::Output,
                    PinCategory::Execution)
            }
        );
    }

    void CheckEquivalent(const GraphIR& Left, const GraphIR& Right)
    {
        Check(Left.GetNodes().size() == Right.GetNodes().size());
        for (std::size_t Index = 0U; Index < Left.GetNodes().size(); ++Index)
        {
            Check(Left.GetNodes()[Index].Identifier == Right.GetNodes()[Index].Identifier);
            Check(Left.GetNodes()[Index].Descriptor == Right.GetNodes()[Index].Descriptor);
        }
        Check(Left.GetVariables().size() == Right.GetVariables().size());
        Check(Left.GetInputBindings().size() == Right.GetInputBindings().size());
        for (std::size_t Index = 0U; Index < Left.GetInputBindings().size(); ++Index)
        {
            const InputBindingRecord& LeftRecord = Left.GetInputBindings()[Index];
            const InputBindingRecord& RightRecord = Right.GetInputBindings()[Index];
            Check(LeftRecord.DestinationNode == RightRecord.DestinationNode);
            Check(LeftRecord.DestinationInputPin == RightRecord.DestinationInputPin);
            Check(LeftRecord.Binding == RightRecord.Binding);
        }
        Check(Left.GetControlEdges().size() == Right.GetControlEdges().size());
        for (std::size_t Index = 0U; Index < Left.GetControlEdges().size(); ++Index)
        {
            const ControlEdge& LeftEdge = Left.GetControlEdges()[Index];
            const ControlEdge& RightEdge = Right.GetControlEdges()[Index];
            Check(LeftEdge.SourceNode == RightEdge.SourceNode);
            Check(LeftEdge.SourceOutputPin == RightEdge.SourceOutputPin);
            Check(LeftEdge.DestinationNode == RightEdge.DestinationNode);
            Check(LeftEdge.DestinationInputPin == RightEdge.DestinationInputPin);
        }
    }
}

int main()
{
    Graph SourceGraph(GraphIdentifier(1U), "Source");
    auto FirstNode = std::make_unique<Node>(NodeIdentifier(1U), "First");
    auto SecondNode = std::make_unique<Node>(NodeIdentifier(2U), "Second");
    Check(FirstNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(10U), "Input", EPinCategory::Data, EPinType::Integer, EPinKind::Input
    )).has_value());
    Check(FirstNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(11U), "Output", EPinCategory::Data, EPinType::Integer, EPinKind::Output
    )).has_value());
    Check(FirstNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(12U), "FlowInput", EPinCategory::Execution, EPinType::Flow, EPinKind::Input
    )).has_value());
    Check(FirstNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(13U), "FlowOutput", EPinCategory::Execution, EPinType::Flow, EPinKind::Output
    )).has_value());
    Check(SecondNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(10U), "Input", EPinCategory::Data, EPinType::Integer, EPinKind::Input
    )).has_value());
    Check(SecondNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(11U), "Output", EPinCategory::Data, EPinType::Integer, EPinKind::Output
    )).has_value());
    Check(SecondNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(12U), "FlowInput", EPinCategory::Execution, EPinType::Flow, EPinKind::Input
    )).has_value());
    Check(SecondNode->AddPin(std::make_unique<Pin>(
        PinIdentifier(13U), "FlowOutput", EPinCategory::Execution, EPinType::Flow, EPinKind::Output
    )).has_value());
    Check(SourceGraph.AddNode(std::move(FirstNode)).has_value());
    Check(SourceGraph.AddNode(std::move(SecondNode)).has_value());
    Check(SourceGraph.AddLink(Link(
        LinkIdentifier(1U),
        PinReference{NodeIdentifier(1U), PinIdentifier(11U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(10U)}
    )).has_value());

    Check(!SourceGraph.AddLink(Link(
        LinkIdentifier(3U),
        PinReference{NodeIdentifier(1U), PinIdentifier(11U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(12U)}
    )).has_value());
    Check(!SourceGraph.AddLink(Link(
        LinkIdentifier(4U),
        PinReference{NodeIdentifier(1U), PinIdentifier(13U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(10U)}
    )).has_value());
    Check(!SourceGraph.AddLink(Link(
        LinkIdentifier(5U),
        PinReference{NodeIdentifier(1U), PinIdentifier(10U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(10U)}
    )).has_value());
    Check(!SourceGraph.AddLink(Link(
        LinkIdentifier(6U),
        PinReference{NodeIdentifier(1U), PinIdentifier(11U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(11U)}
    )).has_value());
    Check(SourceGraph.AddLink(Link(
        LinkIdentifier(2U),
        PinReference{NodeIdentifier(1U), PinIdentifier(13U)},
        PinReference{NodeIdentifier(2U), PinIdentifier(12U)}
    )).has_value());

    const GraphIRAdapterMapping Mapping{{MakeMapping(NodeIdentifier(1U)), MakeMapping(NodeIdentifier(2U))}};
    const auto Converted = GraphIRAdapter::Convert(SourceGraph, Mapping);
    Check(Converted.has_value());
    Check(Converted->GetNodeCount() == 2U);
    Check(Converted->GetInputBindingCount() == 1U);
    Check(Converted->GetControlEdgeCount() == 1U);
    Check(Converted->GetNodes()[0].Identifier == NodeInstanceId(1U));
    Check(Converted->GetNodes()[1].Descriptor == NodeDescriptorId(100U));
    Check(Converted->GetInputBindings()[0].DestinationNode == NodeInstanceId(2U));
    Check(Converted->GetInputBindings()[0].DestinationInputPin == PinIndex(0U));
    Check(std::get<OutputReference>(Converted->GetInputBindings()[0].Binding).SourceOutputPin == PinIndex(1U));
    Check(Converted->GetControlEdges()[0].SourceOutputPin == PinIndex(3U));
    Check(Converted->GetControlEdges()[0].DestinationInputPin == PinIndex(2U));

    NodeDescriptorRegistry Descriptors;
    Check(Descriptors.Register(MakeDescriptor()).has_value());
    Check(GraphIRValidator::Validate(*Converted, Descriptors).empty());
    CheckEquivalent(*Converted, *GraphIRAdapter::Convert(SourceGraph, Mapping));

    GraphIRAdapterMapping MissingMapping{{MakeMapping(NodeIdentifier(1U))}};
    const auto Failed = GraphIRAdapter::Convert(SourceGraph, MissingMapping);
    Check(!Failed.has_value());
    Check(!Failed.error().empty());
    Check(HasCode(Failed.error(), DiagnosticCode::MissingAdapterNodeMapping));

    GraphIRAdapterMapping InvalidDescriptor = Mapping;
    InvalidDescriptor.Nodes[0].Descriptor = NodeDescriptorId();
    const auto InvalidDescriptorResult = GraphIRAdapter::Convert(SourceGraph, InvalidDescriptor);
    Check(!InvalidDescriptorResult.has_value());
    Check(HasCode(InvalidDescriptorResult.error(), DiagnosticCode::InvalidGraphIRAdapterMapping));

    GraphIRAdapterMapping MissingPin = Mapping;
    MissingPin.Nodes[0].Pins.erase(MissingPin.Nodes[0].Pins.begin() + 1);
    const auto MissingPinResult = GraphIRAdapter::Convert(SourceGraph, MissingPin);
    Check(!MissingPinResult.has_value());
    Check(HasCode(MissingPinResult.error(), DiagnosticCode::InvalidGraphIRAdapterMapping));

    Graph SourceEmpty(GraphIdentifier(2U), "Empty");
    const auto EmptyResult = GraphIRAdapter::Convert(SourceEmpty, {});
    Check(EmptyResult.has_value());
    Check(EmptyResult->GetNodeCount() == 0U);
    Check(GraphIRAdapter::Convert(SourceGraph, Mapping).has_value());

    GraphIRAdapterMapping InvalidPinMapping = Mapping;
    InvalidPinMapping.Nodes[0].Pins[0].GraphIRPin = PinIndex();
    Check(!GraphIRAdapter::Convert(SourceGraph, InvalidPinMapping).has_value());

    GraphIRAdapterMapping DuplicateMapping = Mapping;
    DuplicateMapping.Nodes.push_back(MakeMapping(NodeIdentifier(1U)));
    Check(!GraphIRAdapter::Convert(SourceGraph, DuplicateMapping).has_value());

    GraphIRAdapterMapping DuplicatePinMapping = Mapping;
    DuplicatePinMapping.Nodes[0].Pins.push_back(
        {PinIdentifier(11U), PinIndex(4U)});
    Check(!GraphIRAdapter::Convert(SourceGraph, DuplicatePinMapping).has_value());

    GraphIRAdapterMapping DuplicateGraphIRPinMapping = Mapping;
    DuplicateGraphIRPinMapping.Nodes[0].Pins.push_back(
        {PinIdentifier(10U), PinIndex(1U)});
    Check(!GraphIRAdapter::Convert(SourceGraph, DuplicateGraphIRPinMapping).has_value());

    GraphIRAdapterMapping AggregatedErrors = Mapping;
    AggregatedErrors.Nodes[0].Descriptor = NodeDescriptorId();
    AggregatedErrors.Nodes[1].Pins.clear();
    AggregatedErrors.Nodes.push_back(GraphIRNodeMapping{
        NodeIdentifier(99U), NodeDescriptorId(), {}
    });
    const auto AggregatedResult = GraphIRAdapter::Convert(SourceGraph, AggregatedErrors);
    Check(!AggregatedResult.has_value());
    Check(AggregatedResult.error().size() >= 3U);
    Check(HasCode(AggregatedResult.error(), DiagnosticCode::InvalidGraphIRAdapterMapping));
    Check(!HasCode(AggregatedResult.error(), DiagnosticCode::InvalidGraphIRAdapterLink));

    Graph IndependentGraph = SourceGraph;
    const auto IndependentResult = GraphIRAdapter::Convert(IndependentGraph, Mapping);
    Check(IndependentResult.has_value());
    CheckEquivalent(*Converted, *IndependentResult);

    return 0;
}
