#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;

namespace
{
    void Check(bool Condition)
    {
        if (!Condition)
        {
            std::abort();
        }
    }

    NodeDescriptor MakeDescriptor(
        NodeDescriptorId Id,
        bool Execution = false,
        TypeDesc ExecutionType = TypeDesc::Flow())
    {
        const TypeDesc PinType = Execution ? std::move(ExecutionType) : TypeDesc::Integer();
        return NodeDescriptor(
            Id,
            "TestNode",
            {NodeAvailability::Server},
            {
                PinSchema(
                    "Input",
                    PinType,
                    PinDirection::Input,
                    Execution ? PinCategory::Execution : PinCategory::Data,
                    PinCardinality::Single,
                    !Execution
                ),
                PinSchema(
                    "Output",
                    PinType,
                    PinDirection::Output,
                    Execution ? PinCategory::Execution : PinCategory::Data
                ),
                PinSchema(
                    "Many",
                    TypeDesc::Integer(),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Multiple,
                    true
                )
            }
        );
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
    NodeDescriptorRegistry Descriptors;
    Check(
        Descriptors.Register(MakeDescriptor(NodeDescriptorId(1U))).has_value()
    );
    Check(
        Descriptors.Register(MakeDescriptor(NodeDescriptorId(2U), true)).has_value()
    );
    Check(
        Descriptors.Register(MakeDescriptor(
            NodeDescriptorId(3U), true, TypeDesc::Integer())).has_value()
    );
    Check(
        Descriptors.Register(MakeDescriptor(
            NodeDescriptorId(4U), true, TypeDesc::Generic(GenericParameterId(1U)))).has_value()
    );

    GraphIR ValidGraph;
    const NodeInstance Source{NodeInstanceId(1U), NodeDescriptorId(1U)};
    const NodeInstance Destination{NodeInstanceId(2U), NodeDescriptorId(1U)};
    ValidGraph.AddNode(Source);
    ValidGraph.AddNode(Destination);
    ValidGraph.AddVariable(GraphVariable{
        GraphVariableId(1U), "Value", TypeDesc::Integer(),
        LiteralValue(LiteralValue::Data{std::int64_t{3}})
    });
    ValidGraph.BindInput(
        Source.Identifier,
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{4}}));
    ValidGraph.BindInput(
        Destination.Identifier,
        PinIndex(0U),
        OutputReference{Source.Identifier, PinIndex(1U)});
    ValidGraph.BindInput(
        Destination.Identifier,
        PinIndex(2U),
        GraphVariableReference{GraphVariableId(1U)});
    Check(GraphIRValidator::Validate(ValidGraph, Descriptors).empty());

    GraphIR ControlGraph;
    ControlGraph.AddNode(NodeInstance{NodeInstanceId(3U), NodeDescriptorId(2U)});
    ControlGraph.AddNode(NodeInstance{NodeInstanceId(4U), NodeDescriptorId(2U)});
    ControlGraph.AddControlEdge(ControlEdge{
        NodeInstanceId(3U), PinIndex(1U), NodeInstanceId(4U), PinIndex(0U)
    });
    Check(GraphIRValidator::Validate(ControlGraph, Descriptors).empty());

    GraphIR DuplicateControlGraph;
    DuplicateControlGraph.AddNode(NodeInstance{NodeInstanceId(10U), NodeDescriptorId(2U)});
    DuplicateControlGraph.AddNode(NodeInstance{NodeInstanceId(11U), NodeDescriptorId(2U)});
    const ControlEdge FlowEdge{
        NodeInstanceId(10U), PinIndex(1U), NodeInstanceId(11U), PinIndex(0U)
    };
    DuplicateControlGraph.AddControlEdge(FlowEdge);
    DuplicateControlGraph.AddControlEdge(FlowEdge);
    Check(HasCode(
        GraphIRValidator::Validate(DuplicateControlGraph, Descriptors),
        DiagnosticCode::InvalidControlEdge
    ));

    GraphIR SharedEndpointControlGraph;
    SharedEndpointControlGraph.AddNode(NodeInstance{NodeInstanceId(12U), NodeDescriptorId(2U)});
    SharedEndpointControlGraph.AddNode(NodeInstance{NodeInstanceId(13U), NodeDescriptorId(2U)});
    SharedEndpointControlGraph.AddNode(NodeInstance{NodeInstanceId(14U), NodeDescriptorId(2U)});
    SharedEndpointControlGraph.AddControlEdge(ControlEdge{
        NodeInstanceId(12U), PinIndex(1U), NodeInstanceId(13U), PinIndex(0U)
    });
    SharedEndpointControlGraph.AddControlEdge(ControlEdge{
        NodeInstanceId(12U), PinIndex(1U), NodeInstanceId(14U), PinIndex(0U)
    });
    Check(GraphIRValidator::Validate(SharedEndpointControlGraph, Descriptors).empty());

    const auto CheckInvalidExecutionType = [&Descriptors](
        NodeDescriptorId SourceDescriptor,
        NodeDescriptorId DestinationDescriptor,
        NodeInstanceId SourceId,
        NodeInstanceId DestinationId)
    {
        GraphIR Graph;
        Graph.AddNode(NodeInstance{SourceId, SourceDescriptor});
        Graph.AddNode(NodeInstance{DestinationId, DestinationDescriptor});
        Graph.AddControlEdge(ControlEdge{
            SourceId, PinIndex(1U), DestinationId, PinIndex(0U)
        });
        Check(HasCode(
            GraphIRValidator::Validate(Graph, Descriptors),
            DiagnosticCode::InvalidControlEdge
        ));
    };
    CheckInvalidExecutionType(
        NodeDescriptorId(3U), NodeDescriptorId(2U),
        NodeInstanceId(15U), NodeInstanceId(16U));
    CheckInvalidExecutionType(
        NodeDescriptorId(2U), NodeDescriptorId(3U),
        NodeInstanceId(17U), NodeInstanceId(18U));
    CheckInvalidExecutionType(
        NodeDescriptorId(4U), NodeDescriptorId(2U),
        NodeInstanceId(19U), NodeInstanceId(20U));
    CheckInvalidExecutionType(
        NodeDescriptorId(2U), NodeDescriptorId(4U),
        NodeInstanceId(21U), NodeInstanceId(22U));

    GraphIR InvalidGraph;
    InvalidGraph.AddNode(NodeInstance{NodeInstanceId{}, NodeDescriptorId(99U)});
    InvalidGraph.AddNode(NodeInstance{NodeInstanceId(5U), NodeDescriptorId(1U)});
    InvalidGraph.AddNode(NodeInstance{NodeInstanceId(5U), NodeDescriptorId(1U)});
    InvalidGraph.AddVariable(GraphVariable{GraphVariableId{}, "", TypeDesc{}, std::nullopt});
    InvalidGraph.AddVariable(GraphVariable{
        GraphVariableId(2U), "Bad", TypeDesc::Float(),
        LiteralValue(LiteralValue::Data{std::int64_t{1}})
    });
    InvalidGraph.BindInput(
        NodeInstanceId(77U),
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{1}}));
    InvalidGraph.BindInput(
        NodeInstanceId(5U),
        PinIndex(99U),
        LiteralValue(LiteralValue::Data{std::int64_t{1}}));
    InvalidGraph.BindInput(
        NodeInstanceId(5U),
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{1}}));
    InvalidGraph.BindInput(
        NodeInstanceId(5U),
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{2}}));
    InvalidGraph.BindInput(
        NodeInstanceId(5U),
        PinIndex(0U),
        OutputReference{NodeInstanceId(5U), PinIndex(0U)});
    InvalidGraph.AddControlEdge(ControlEdge{
        NodeInstanceId(5U), PinIndex(0U), NodeInstanceId(5U), PinIndex(1U)
    });
    const DiagnosticCollection InvalidDiagnostics =
        GraphIRValidator::Validate(InvalidGraph, Descriptors);
    Check(InvalidDiagnostics.size() >= 6U);
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::InvalidGraphIRNode));
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::DuplicateGraphIRNodeIdentifier));
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::MissingDescriptor));
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::InvalidGraphVariable));
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::IncompatibleGraphIRTypes));
    Check(HasCode(InvalidDiagnostics, DiagnosticCode::InvalidControlEdge));

    GraphIR TypeGraph;
    TypeGraph.AddNode(NodeInstance{NodeInstanceId(6U), NodeDescriptorId(1U)});
    TypeGraph.AddVariable(GraphVariable{
        GraphVariableId(3U), "Wrong", TypeDesc::Float(), std::nullopt
    });
    TypeGraph.BindInput(
        NodeInstanceId(6U),
        PinIndex(0U),
        GraphVariableReference{GraphVariableId(3U)});
    Check(HasCode(GraphIRValidator::Validate(TypeGraph, Descriptors),
        DiagnosticCode::IncompatibleGraphIRTypes));

    GraphIR LiteralGraph;
    LiteralGraph.AddNode(NodeInstance{NodeInstanceId(7U), NodeDescriptorId(1U)});
    LiteralGraph.BindInput(
        NodeInstanceId(7U),
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::string("wrong")}));
    Check(HasCode(GraphIRValidator::Validate(LiteralGraph, Descriptors),
        DiagnosticCode::IncompatibleGraphIRTypes));

    GraphIR MultipleGraph;
    MultipleGraph.AddNode(NodeInstance{NodeInstanceId(8U), NodeDescriptorId(1U)});
    MultipleGraph.BindInput(
        NodeInstanceId(8U),
        PinIndex(2U),
        LiteralValue(LiteralValue::Data{std::int64_t{1}}));
    MultipleGraph.BindInput(
        NodeInstanceId(8U),
        PinIndex(2U),
        LiteralValue(LiteralValue::Data{std::int64_t{2}}));
    Check(GraphIRValidator::Validate(MultipleGraph, Descriptors).empty());

    GraphIR FlowVariableGraph;
    FlowVariableGraph.AddVariable(GraphVariable{
        GraphVariableId(4U), "Control", TypeDesc::Flow(), std::nullopt
    });
    Check(HasCode(
        GraphIRValidator::Validate(FlowVariableGraph, Descriptors),
        DiagnosticCode::InvalidGraphVariable
    ));

    return 0;
}
