#pragma once

#include <cstdint>
#include <cstddef>
#include <iterator>
#include <string>
#include <variant>

#include "MiliastraPlusPlusGraphIR.h"
#include "MiliastraPlusPlusGraphIRGenericValidation.h"

namespace MiliastraPlusPlus
{
    class GraphIRValidator
    {
    public:
        [[nodiscard]] static DiagnosticCollection Validate(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            DiagnosticCollection Diagnostics;

            for (std::size_t Index = 0U; Index < Graph.GetNodes().size(); ++Index)
            {
                const NodeInstance& Node = Graph.GetNodes()[Index];
                if (!Node.Identifier.IsValid() || !Node.Descriptor.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphIRNode,
                        "GraphIR contains a node with an invalid instance or descriptor identifier.");
                }
                if (FindPriorNode(Graph, Node.Identifier, Index) != nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateGraphIRNodeIdentifier,
                        "GraphIR contains duplicate node instance identifiers.");
                }
                if (Node.Descriptor.IsValid() && Descriptors.Find(Node.Descriptor) == nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::MissingDescriptor,
                        "GraphIR references a node descriptor that is not registered.");
                }
            }

            for (std::size_t Index = 0U; Index < Graph.GetVariables().size(); ++Index)
            {
                const GraphVariable& Variable = Graph.GetVariables()[Index];
                if (!Variable.IsValid() || ContainsFlowType(Variable.Type))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphVariable,
                        "GraphIR contains an invalid graph variable or a control Flow type used as data.");
                }
                if (FindPriorVariable(Graph, Variable.Identifier, Index) != nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateGraphVariableIdentifier,
                        "GraphIR contains duplicate graph variable identifiers.");
                }
                if (Variable.DefaultValue.has_value() &&
                    !IsLiteralCompatible(*Variable.DefaultValue, Variable.Type))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::IncompatibleGraphIRTypes,
                        "A graph variable default literal is incompatible with its declared type.");
                }
            }

            for (const InputBindingRecord& Record : Graph.GetInputBindings())
            {
                const NodeInstance* DestinationNode = Graph.FindNode(Record.DestinationNode);
                const PinSchema* DestinationPin = FindPin(
                    DestinationNode, Record.DestinationInputPin, Descriptors);
                if (DestinationNode == nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidInputBinding,
                        "An input binding references a missing destination node.");
                    continue;
                }
                if (DestinationPin == nullptr || !Record.DestinationInputPin.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphIRPinReference,
                        "An input binding references an invalid destination pin.");
                    continue;
                }
                if (DestinationPin->GetDirection() != PinDirection::Input ||
                    DestinationPin->GetCategory() != PinCategory::Data)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidInputBinding,
                        "An input binding destination must be a data input pin.");
                }
                if (DestinationPin->GetCardinality() != PinCardinality::Multiple &&
                    CountBindings(Graph, Record.DestinationNode, Record.DestinationInputPin) > 1U)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateInputBinding,
                        "A Single or Optional input pin has multiple bindings.");
                }
                ValidateBinding(Graph, Descriptors, Record, *DestinationPin, Diagnostics);
            }

            for (std::size_t Index = 0U; Index < Graph.GetControlEdges().size(); ++Index)
            {
                const ControlEdge& Edge = Graph.GetControlEdges()[Index];
                const NodeInstance* SourceNode = Graph.FindNode(Edge.SourceNode);
                const NodeInstance* DestinationNode = Graph.FindNode(Edge.DestinationNode);
                const PinSchema* SourcePin = FindPin(SourceNode, Edge.SourceOutputPin, Descriptors);
                const PinSchema* DestinationPin = FindPin(
                    DestinationNode, Edge.DestinationInputPin, Descriptors);
                if (SourceNode == nullptr || DestinationNode == nullptr ||
                    SourcePin == nullptr || DestinationPin == nullptr ||
                    !Edge.SourceOutputPin.IsValid() || !Edge.DestinationInputPin.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "A control edge contains a missing or invalid node or pin reference.");
                    continue;
                }
                if (SourcePin->GetDirection() != PinDirection::Output ||
                    DestinationPin->GetDirection() != PinDirection::Input ||
                    SourcePin->GetCategory() != PinCategory::Execution ||
                    DestinationPin->GetCategory() != PinCategory::Execution ||
                    SourcePin->GetType() != TypeDesc::Flow() ||
                    DestinationPin->GetType() != TypeDesc::Flow())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "A control edge must connect Flow-typed execution output and input pins.");
                }
                if (HasPriorControlEdge(Graph, Edge, Index))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "GraphIR contains a duplicate control edge.");
                }
            }

            DiagnosticCollection GenericDiagnostics =
                GraphIRGenericValidationDetail::ValidateGenericTypes(Graph, Descriptors);
            Diagnostics.insert(
                Diagnostics.end(),
                std::make_move_iterator(GenericDiagnostics.begin()),
                std::make_move_iterator(GenericDiagnostics.end())
            );

            return Diagnostics;
        }

    private:
        static void Add(
            DiagnosticCollection& Diagnostics,
            DiagnosticCode Code,
            const char* Message
        )
        {
            Diagnostics.push_back(Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = Message
            });
        }

        static const NodeInstance* FindPriorNode(
            const GraphIR& Graph,
            NodeInstanceId Identifier,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                if (Graph.GetNodes()[Index].Identifier == Identifier)
                {
                    return &Graph.GetNodes()[Index];
                }
            }
            return nullptr;
        }

        static const GraphVariable* FindPriorVariable(
            const GraphIR& Graph,
            GraphVariableId Identifier,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                if (Graph.GetVariables()[Index].Identifier == Identifier)
                {
                    return &Graph.GetVariables()[Index];
                }
            }
            return nullptr;
        }

        static bool HasPriorControlEdge(
            const GraphIR& Graph,
            const ControlEdge& Edge,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                const ControlEdge& Prior = Graph.GetControlEdges()[Index];
                if (Prior.SourceNode == Edge.SourceNode &&
                    Prior.SourceOutputPin == Edge.SourceOutputPin &&
                    Prior.DestinationNode == Edge.DestinationNode &&
                    Prior.DestinationInputPin == Edge.DestinationInputPin)
                {
                    return true;
                }
            }
            return false;
        }

        static const PinSchema* FindPin(
            const NodeInstance* Node,
            PinIndex Index,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            if (Node == nullptr || !Node->Descriptor.IsValid() || !Index.IsValid())
            {
                return nullptr;
            }
            const NodeDescriptor* Descriptor = Descriptors.Find(Node->Descriptor);
            if (Descriptor == nullptr || Index.GetValue() >= Descriptor->GetPins().size())
            {
                return nullptr;
            }
            return &Descriptor->GetPins()[Index.GetValue()];
        }

        static std::size_t CountBindings(
            const GraphIR& Graph,
            NodeInstanceId Node,
            PinIndex Pin
        )
        {
            std::size_t Count = 0U;
            for (const InputBindingRecord& Record : Graph.GetInputBindings())
            {
                if (Record.DestinationNode == Node && Record.DestinationInputPin == Pin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static void ValidateBinding(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const InputBindingRecord& Record,
            const PinSchema& DestinationPin,
            DiagnosticCollection& Diagnostics)
        {
            const InputBinding& Binding = Record.Binding;
            const bool HasValidOutputTypeConstraint =
                Record.OutputTypeConstraint.has_value() &&
                Record.OutputTypeConstraint->IsValid() &&
                !ContainsGenericType(*Record.OutputTypeConstraint) &&
                !ContainsFlowType(*Record.OutputTypeConstraint);
            if (Record.OutputTypeConstraint.has_value())
            {
                if (!std::holds_alternative<OutputReference>(Binding))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidInputBinding,
                        "An output type constraint can only be attached to an output binding.");
                }
                if (!HasValidOutputTypeConstraint)
                {
                    Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                        "An output type constraint must be a valid concrete data type.");
                }
            }

            if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Binding))
            {
                if (!DestinationPin.AllowsLiteral())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidInputBinding,
                        "A literal is not allowed by the destination pin schema.");
                }
                else if (!IsLiteralCompatible(*Literal, DestinationPin.GetType()))
                {
                    Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                        "A literal binding is incompatible with the destination pin type.");
                }
                return;
            }
            if (const OutputReference* Output = std::get_if<OutputReference>(&Binding))
            {
                const NodeInstance* SourceNode = Graph.FindNode(Output->SourceNode);
                const PinSchema* SourcePin = FindPin(
                    SourceNode, Output->SourceOutputPin, Descriptors);
                if (!Output->IsValid() || SourcePin == nullptr)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRPinReference,
                        "An output binding references a missing or invalid source pin.");
                }
                else
                {
                    if (SourcePin->GetDirection() != PinDirection::Output ||
                        SourcePin->GetCategory() != PinCategory::Data ||
                        ContainsFlowType(SourcePin->GetType()) ||
                        ContainsFlowType(DestinationPin.GetType()) ||
                        !SourcePin->GetType().IsCompatibleWith(DestinationPin.GetType()))
                    {
                        Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                            "An output binding is incompatible with its destination pin.");
                    }
                    if (HasValidOutputTypeConstraint)
                    {
                        if (!SourcePin->GetType().IsCompatibleWith(
                            *Record.OutputTypeConstraint))
                        {
                            Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                                "An output type constraint is structurally incompatible with its source pin.");
                        }
                        else if (!ContainsGenericType(SourcePin->GetType()) &&
                            SourcePin->GetType() != *Record.OutputTypeConstraint)
                        {
                            Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                                "A concrete output type must equal its typed-use constraint.");
                        }
                    }
                }
                return;
            }
            const GraphVariableReference& Variable = std::get<GraphVariableReference>(Binding);
            const GraphVariable* SourceVariable = Graph.FindVariable(Variable.Variable);
            if (!Variable.IsValid() || SourceVariable == nullptr)
            {
                Add(Diagnostics, DiagnosticCode::MissingGraphVariable,
                    "An input binding references a missing or invalid graph variable.");
            }
            else if (!ContainsFlowType(SourceVariable->Type) &&
                !SourceVariable->Type.IsCompatibleWith(DestinationPin.GetType()))
            {
                Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                    "A graph variable binding is incompatible with the destination pin type.");
            }
        }

        static bool IsLiteralCompatible(const LiteralValue& Literal, const TypeDesc& Type)
        {
            if (!Literal.IsValid() || !Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Boolean: return Literal.Is<bool>();
            case TypeDesc::Kind::Integer: return Literal.Is<std::int64_t>();
            case TypeDesc::Kind::Float: return Literal.Is<double>();
            case TypeDesc::Kind::String: return Literal.Is<std::string>();
            case TypeDesc::Kind::GUID: return Literal.Is<GuidValue>();
            case TypeDesc::Kind::Vector3: return Literal.Is<Vector3Value>();
            case TypeDesc::Kind::PrefabId: return Literal.Is<PrefabIdValue>();
            case TypeDesc::Kind::ConfigId: return Literal.Is<ConfigIdValue>();
            case TypeDesc::Kind::Faction: return Literal.Is<FactionValue>();
            default: return false;
            }
        }

        static bool ContainsGenericType(const TypeDesc& Type)
        {
            if (!Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Generic:
                return true;
            case TypeDesc::Kind::List:
                return ContainsGenericType(*Type.GetElementType());
            case TypeDesc::Kind::Dictionary:
                return ContainsGenericType(*Type.GetKeyType()) ||
                    ContainsGenericType(*Type.GetValueType());
            default:
                return false;
            }
        }

        static bool ContainsFlowType(const TypeDesc& Type)
        {
            if (!Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Flow:
                return true;
            case TypeDesc::Kind::List:
                return ContainsFlowType(*Type.GetElementType());
            case TypeDesc::Kind::Dictionary:
                return ContainsFlowType(*Type.GetKeyType()) ||
                    ContainsFlowType(*Type.GetValueType());
            default:
                return false;
            }
        }
    };
}
