#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <optional>
#include <source_location>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGraphBuilder.h"
#include "MiliastraPlusPlusGraphIRJson.h"

using namespace MiliastraPlusPlus;

namespace
{
    [[noreturn]] void ReportFailure(
        const char* Expression,
        const std::source_location& Location
    )
    {
        std::fprintf(
            stderr,
            "Check failed: %s (%s:%u, %s)\n",
            Expression,
            Location.file_name(),
            Location.line(),
            Location.function_name()
        );
        std::abort();
    }

    void Check(
        bool Condition,
        const char* Expression,
        const std::source_location& Location = std::source_location::current()
    )
    {
        if (!Condition)
        {
            ReportFailure(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr NodeDescriptorId ConcreteDescriptorId(2001U);
    constexpr NodeDescriptorId GenericDescriptorId(2002U);
    constexpr NodeDescriptorId MultiGenericDescriptorId(2003U);
    constexpr NodeDescriptorId GenericExecutionDescriptorId(2004U);

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

    std::size_t CountCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        std::size_t Count = 0U;
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            if (Diagnostic.Code == Code)
            {
                ++Count;
            }
        }
        return Count;
    }

    bool HasMessageContaining(const DiagnosticCollection& Diagnostics, const char* Text)
    {
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            if (Diagnostic.Message.find(Text) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    NodeDescriptor MakeConcreteDescriptor()
    {
        return NodeDescriptor(
            ConcreteDescriptorId,
            "ConcreteNode",
            {NodeAvailability::Server},
            {
                PinSchema("IntOutput", TypeDesc::Integer(), PinDirection::Output, PinCategory::Data),
                PinSchema("FloatOutput", TypeDesc::Float(), PinDirection::Output, PinCategory::Data),
                PinSchema("IntInput", TypeDesc::Integer(), PinDirection::Input, PinCategory::Data),
                PinSchema("FloatInput", TypeDesc::Float(), PinDirection::Input, PinCategory::Data),
                PinSchema("IntListInput", TypeDesc::List(TypeDesc::Integer()),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("IntStringDictionaryInput",
                    TypeDesc::Dictionary(TypeDesc::Integer(), TypeDesc::String()),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("FloatStringDictionaryInput",
                    TypeDesc::Dictionary(TypeDesc::Float(), TypeDesc::String()),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("Struct42Input", TypeDesc::StructObject(StructTypeId(42U)),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("Struct43Input", TypeDesc::StructObject(StructTypeId(43U)),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("NestedIntListInput",
                    TypeDesc::List(TypeDesc::List(TypeDesc::Integer())),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("Struct42Output", TypeDesc::StructObject(StructTypeId(42U)),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("Struct43Output", TypeDesc::StructObject(StructTypeId(43U)),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("FloatListInput", TypeDesc::List(TypeDesc::Float()),
                    PinDirection::Input, PinCategory::Data)
            }
        );
    }

    NodeDescriptor MakeGenericDescriptor()
    {
        const TypeDesc Generic = TypeDesc::Generic(GenericParameterId(1U));
        return NodeDescriptor(
            GenericDescriptorId,
            "GenericNode",
            {NodeAvailability::Server},
            {
                PinSchema("GenericInput", Generic, PinDirection::Input, PinCategory::Data,
                    PinCardinality::Single, true),
                PinSchema("GenericOutput", Generic, PinDirection::Output, PinCategory::Data),
                PinSchema("GenericListOutput", TypeDesc::List(Generic),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("GenericListInput", TypeDesc::List(Generic),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("GenericDictionaryOutput",
                    TypeDesc::Dictionary(Generic, TypeDesc::String()),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("GenericDictionaryInput",
                    TypeDesc::Dictionary(Generic, TypeDesc::String()),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("GenericNestedListOutput",
                    TypeDesc::List(TypeDesc::List(Generic)),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("GenericNestedListInput",
                    TypeDesc::List(TypeDesc::List(Generic)),
                    PinDirection::Input, PinCategory::Data)
            }
        );
    }

    NodeDescriptor MakeMultiGenericDescriptor()
    {
        return NodeDescriptor(
            MultiGenericDescriptorId,
            "MultiGenericNode",
            {NodeAvailability::Server},
            {
                PinSchema("AOutput", TypeDesc::Generic(GenericParameterId(1U)),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("BOutput", TypeDesc::Generic(GenericParameterId(2U)),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("AInput", TypeDesc::Generic(GenericParameterId(1U)),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("BInput", TypeDesc::Generic(GenericParameterId(2U)),
                    PinDirection::Input, PinCategory::Data)
            }
        );
    }

    NodeDescriptor MakeGenericExecutionDescriptor()
    {
        const TypeDesc Generic = TypeDesc::Generic(GenericParameterId(1U));
        return NodeDescriptor(
            GenericExecutionDescriptorId,
            "GenericExecutionNode",
            {NodeAvailability::Server},
            {
                PinSchema("ExecutionOutput", Generic, PinDirection::Output,
                    PinCategory::Execution),
                PinSchema("ExecutionInput", Generic, PinDirection::Input,
                    PinCategory::Execution)
            }
        );
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Registry;
        MPP_CHECK(Registry.Register(MakeConcreteDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeGenericDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeMultiGenericDescriptor()).has_value());
        MPP_CHECK(Registry.Register(MakeGenericExecutionDescriptor()).has_value());
        return Registry;
    }

    void AddNode(GraphIR& Graph, NodeInstanceId Identifier, NodeDescriptorId Descriptor)
    {
        Graph.AddNode(NodeInstance{Identifier, Descriptor});
    }

    void AddOutputBinding(
        GraphIR& Graph,
        NodeInstanceId DestinationNode,
        PinIndex DestinationPin,
        NodeInstanceId SourceNode,
        PinIndex SourcePin,
        std::optional<TypeDesc> OutputTypeConstraint = std::nullopt
    )
    {
        Graph.BindInput(
            DestinationNode,
            DestinationPin,
            OutputReference{SourceNode, SourcePin},
            std::move(OutputTypeConstraint)
        );
    }

    void AddVariableBinding(
        GraphIR& Graph,
        NodeInstanceId DestinationNode,
        PinIndex DestinationPin,
        GraphVariableId Variable
    )
    {
        Graph.BindInput(
            DestinationNode,
            DestinationPin,
            GraphVariableReference{Variable}
        );
    }

    bool SameDiagnostics(const DiagnosticCollection& Left, const DiagnosticCollection& Right)
    {
        if (Left.size() != Right.size())
        {
            return false;
        }
        for (std::size_t Index = 0U; Index < Left.size(); ++Index)
        {
            if (Left[Index].Severity != Right[Index].Severity ||
                Left[Index].Code != Right[Index].Code ||
                Left[Index].Message != Right[Index].Message)
            {
                return false;
            }
        }
        return true;
    }

    GraphIR MakeConflictGraph(bool ReverseBindings)
    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        if (ReverseBindings)
        {
            AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(3U),
                NodeInstanceId(1U), PinIndex(1U));
            AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
                NodeInstanceId(1U), PinIndex(1U));
        }
        else
        {
            AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
                NodeInstanceId(1U), PinIndex(1U));
            AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(3U),
                NodeInstanceId(1U), PinIndex(1U));
        }
        return Graph;
    }

    GraphIR MakeMultipleConflictGraph(bool ReverseBindings)
    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(4U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(5U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(6U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(7U), ConcreteDescriptorId);

        const auto AddConstraints = [&Graph](bool Reverse)
        {
            if (Reverse)
            {
                AddOutputBinding(Graph, NodeInstanceId(7U), PinIndex(3U),
                    NodeInstanceId(2U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(6U), PinIndex(2U),
                    NodeInstanceId(2U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(5U), PinIndex(3U),
                    NodeInstanceId(1U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(4U), PinIndex(2U),
                    NodeInstanceId(1U), PinIndex(1U));
            }
            else
            {
                AddOutputBinding(Graph, NodeInstanceId(4U), PinIndex(2U),
                    NodeInstanceId(1U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(5U), PinIndex(3U),
                    NodeInstanceId(1U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(6U), PinIndex(2U),
                    NodeInstanceId(2U), PinIndex(1U));
                AddOutputBinding(Graph, NodeInstanceId(7U), PinIndex(3U),
                    NodeInstanceId(2U), PinIndex(1U));
            }
        };
        AddConstraints(ReverseBindings);
        return Graph;
    }

    GraphIR MakeOutputIntentConflictGraph(bool ReverseBindings)
    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        const auto AddIntegerUse = [&Graph]
        {
            AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
                NodeInstanceId(1U), PinIndex(1U), TypeDesc::Integer());
        };
        const auto AddFloatUse = [&Graph]
        {
            AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(3U),
                NodeInstanceId(1U), PinIndex(1U), TypeDesc::Float());
        };
        if (ReverseBindings)
        {
            AddFloatUse();
            AddIntegerUse();
        }
        else
        {
            AddIntegerUse();
            AddFloatUse();
        }
        return Graph;
    }

    GraphIR MakeMixedStructuralAndGenericConflictGraph(bool ReverseBindings)
    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);

        const auto AddMalformedReference = [&Graph]
        {
            AddOutputBinding(
                Graph,
                NodeInstanceId(2U),
                PinIndex(2U),
                NodeInstanceId(99U),
                PinIndex(1U),
                TypeDesc::Float()
            );
        };
        const auto AddIndependentGenericConflict = [&Graph]
        {
            AddOutputBinding(
                Graph,
                NodeInstanceId(3U),
                PinIndex(3U),
                NodeInstanceId(1U),
                PinIndex(1U),
                TypeDesc::Integer()
            );
        };

        if (ReverseBindings)
        {
            AddIndependentGenericConflict();
            AddMalformedReference();
        }
        else
        {
            AddMalformedReference();
            AddIndependentGenericConflict();
        }
        return Graph;
    }
}

int main()
{
    NodeDescriptorRegistry Descriptors = MakeRegistry();

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(0U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        const DiagnosticCollection Diagnostics =
            GraphIRValidator::Validate(MakeConflictGraph(false), Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 1U);
        MPP_CHECK(HasMessageContaining(Diagnostics, "Integer vs Float"));

        const DiagnosticCollection ReversedDiagnostics =
            GraphIRValidator::Validate(MakeConflictGraph(true), Descriptors);
        MPP_CHECK(SameDiagnostics(Diagnostics, ReversedDiagnostics));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(4U),
            NodeInstanceId(1U), PinIndex(2U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(12U),
            NodeInstanceId(1U), PinIndex(2U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 1U);
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(4U),
            NodeInstanceId(1U), PinIndex(2U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(9U),
            NodeInstanceId(1U), PinIndex(6U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(5U),
            NodeInstanceId(1U), PinIndex(4U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(5U),
            NodeInstanceId(1U), PinIndex(4U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(6U),
            NodeInstanceId(1U), PinIndex(4U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(4U), ConcreteDescriptorId);
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U),
            "ListOrInteger",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        AddVariableBinding(Graph, NodeInstanceId(2U), PinIndex(3U), GraphVariableId(1U));
        AddVariableBinding(Graph, NodeInstanceId(3U), PinIndex(0U), GraphVariableId(1U));
        AddOutputBinding(Graph, NodeInstanceId(4U), PinIndex(2U),
            NodeInstanceId(3U), PinIndex(1U));

        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
        MPP_CHECK(HasMessageContaining(Diagnostics, "Integer"));
        MPP_CHECK(HasMessageContaining(Diagnostics, "List<Generic<1>>"));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(4U), ConcreteDescriptorId);
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U),
            "CompatibleList",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        AddVariableBinding(Graph, NodeInstanceId(2U), PinIndex(3U), GraphVariableId(1U));
        AddVariableBinding(Graph, NodeInstanceId(3U), PinIndex(0U), GraphVariableId(1U));
        AddOutputBinding(Graph, NodeInstanceId(4U), PinIndex(4U),
            NodeInstanceId(3U), PinIndex(1U));

        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U),
            "GenericValue",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        AddVariableBinding(Graph, NodeInstanceId(2U), PinIndex(2U), GraphVariableId(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(HasMessageContaining(Diagnostics, "node 2, parameter 1"));
        MPP_CHECK(!HasMessageContaining(Diagnostics, "node 1, parameter 1"));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), ConcreteDescriptorId);
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U),
            "Constrained",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        Graph.AddVariable(GraphVariable{
            GraphVariableId(2U),
            "Independent",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        AddVariableBinding(Graph, NodeInstanceId(1U), PinIndex(2U), GraphVariableId(1U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(HasMessageContaining(Diagnostics, "graph variable 2, parameter 1"));
        MPP_CHECK(!HasMessageContaining(Diagnostics, "graph variable 1, parameter 1"));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U),
            "SameNumericParameter",
            TypeDesc::Generic(GenericParameterId(1U)),
            std::nullopt
        });
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(HasMessageContaining(Diagnostics, "graph variable 1, parameter 1"));
        MPP_CHECK(!HasMessageContaining(Diagnostics, "node 1, parameter 1"));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(7U),
            NodeInstanceId(1U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(7U),
            NodeInstanceId(1U), PinIndex(10U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(Diagnostics.empty());
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(8U),
            NodeInstanceId(1U), PinIndex(10U));
        const DiagnosticCollection MismatchDiagnostics =
            GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(MismatchDiagnostics, DiagnosticCode::IncompatibleGraphIRTypes));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        Graph.BindInput(
            NodeInstanceId(99U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(2U), PinIndex(0U)}
        );
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(99U),
            OutputReference{NodeInstanceId(2U), PinIndex(0U)}
        );
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(99U), PinIndex(0U)}
        );
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(2U), PinIndex(2U)}
        );
        Graph.BindInput(
            NodeInstanceId(2U),
            PinIndex(2U),
            GraphVariableReference{GraphVariableId(99U)}
        );
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidInputBinding));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidGraphIRPinReference));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::MissingGraphVariable));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), MultiGenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), MultiGenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(4U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(5U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(1U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(2U),
            NodeInstanceId(2U), PinIndex(0U));
        AddOutputBinding(Graph, NodeInstanceId(4U), PinIndex(3U),
            NodeInstanceId(1U), PinIndex(0U));
        AddOutputBinding(Graph, NodeInstanceId(5U), PinIndex(3U),
            NodeInstanceId(2U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), MultiGenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(0U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(3U),
            NodeInstanceId(1U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(3U),
            NodeInstanceId(1U), PinIndex(1U));
        AddOutputBinding(Graph, NodeInstanceId(3U), PinIndex(2U),
            NodeInstanceId(2U), PinIndex(1U));
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(1U), PinIndex(1U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(!HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), MultiGenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), MultiGenericDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(3U),
            NodeInstanceId(1U), PinIndex(0U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::UnresolvedGenericType));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(2U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(Graph, NodeInstanceId(2U), PinIndex(4U),
            NodeInstanceId(1U), PinIndex(4U));
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes));
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericExecutionDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericExecutionDescriptorId);
        Graph.AddControlEdge(ControlEdge{
            NodeInstanceId(1U), PinIndex(0U),
            NodeInstanceId(2U), PinIndex(1U)
        });
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::UnresolvedGenericType));
    }

    {
        GraphIR ConcreteGraph;
        AddNode(ConcreteGraph, NodeInstanceId(1U), ConcreteDescriptorId);
        AddNode(ConcreteGraph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(ConcreteGraph, NodeInstanceId(2U), PinIndex(2U),
            NodeInstanceId(1U), PinIndex(0U));
        MPP_CHECK(GraphIRValidator::Validate(ConcreteGraph, Descriptors).empty());

        GraphBuilder Builder(Descriptors);
        const auto Source = Builder.AddNode(ConcreteDescriptorId);
        const auto Target = Builder.AddNode(ConcreteDescriptorId);
        MPP_CHECK(Source.has_value());
        MPP_CHECK(Target.has_value());
        const auto Output = Builder.GetOutput<int>(*Source, PinIndex(0U));
        MPP_CHECK(Output.has_value());
        MPP_CHECK(Builder.BindInput(
            *Target,
            PinIndex(2U),
            ValueOrExpr<int>(*Output)
        ).has_value());
        const auto Variable = Builder.DeclareVariable<int>("Score");
        MPP_CHECK(Variable.has_value());
        const auto VariableTarget = Builder.AddNode(ConcreteDescriptorId);
        MPP_CHECK(VariableTarget.has_value());
        MPP_CHECK(Builder.BindInput(
            *VariableTarget,
            PinIndex(2U),
            Variable->AsInput()
        ).has_value());
        MPP_CHECK(std::move(Builder).Finalize().has_value());
    }

    {
        GraphBuilder Builder(Descriptors);
        const auto GenericNode = Builder.AddNode(GenericDescriptorId);
        MPP_CHECK(GenericNode.has_value());
        const auto LiteralBinding = Builder.BindInput(
            *GenericNode,
            PinIndex(0U),
            ValueOrExpr<int>(LiteralValue(LiteralValue::Data{std::int64_t{7}}))
        );
        MPP_CHECK(!LiteralBinding.has_value());
        MPP_CHECK(HasCode(LiteralBinding.error(), DiagnosticCode::IncompatibleGraphIRTypes));
    }

    {
        NodeDescriptorRegistry MutableRegistry = MakeRegistry();
        GraphBuilder Builder(MutableRegistry);
        MPP_CHECK(Builder.AddNode(ConcreteDescriptorId).has_value());
        MutableRegistry = NodeDescriptorRegistry{};
        const auto FailedFinalization = std::move(Builder).Finalize();
        MPP_CHECK(!FailedFinalization.has_value());
        MPP_CHECK(HasCode(FailedFinalization.error(), DiagnosticCode::MissingDescriptor));
        const auto ClosedAddNode = Builder.AddNode(ConcreteDescriptorId);
        MPP_CHECK(!ClosedAddNode.has_value());
        MPP_CHECK(HasCode(ClosedAddNode.error(), DiagnosticCode::InvalidGraphBuilderState));
    }

    {
        const GraphIR ConflictGraph = MakeConflictGraph(false);
        const DiagnosticCollection First = GraphIRValidator::Validate(ConflictGraph, Descriptors);
        const DiagnosticCollection Second = GraphIRValidator::Validate(ConflictGraph, Descriptors);
        MPP_CHECK(SameDiagnostics(First, Second));
    }

    {
        const DiagnosticCollection ForwardDiagnostics = GraphIRValidator::Validate(
            MakeMultipleConflictGraph(false),
            Descriptors
        );
        const DiagnosticCollection ReversedDiagnostics = GraphIRValidator::Validate(
            MakeMultipleConflictGraph(true),
            Descriptors
        );
        MPP_CHECK(CountCode(ForwardDiagnostics, DiagnosticCode::GenericConstraintConflict) == 2U);
        MPP_CHECK(CountCode(ForwardDiagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(SameDiagnostics(ForwardDiagnostics, ReversedDiagnostics));
        MPP_CHECK(ForwardDiagnostics.size() == 3U);
        MPP_CHECK(ForwardDiagnostics[0U].Message.find("node 1, parameter 1") != std::string::npos);
        MPP_CHECK(ForwardDiagnostics[1U].Message.find("node 2, parameter 1") != std::string::npos);
        MPP_CHECK(ForwardDiagnostics[2U].Message.find("node 3, parameter 1") != std::string::npos);
        MPP_CHECK(HasMessageContaining(ForwardDiagnostics, "node 1, parameter 1"));
        MPP_CHECK(HasMessageContaining(ForwardDiagnostics, "node 2, parameter 1"));
        MPP_CHECK(HasMessageContaining(ForwardDiagnostics, "node 3, parameter 1"));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(0U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Integer()
        );
        MPP_CHECK(Graph.GetInputBindings().front().OutputTypeConstraint ==
            TypeDesc::Integer());
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
        MPP_CHECK(Descriptors.Find(GenericDescriptorId)->GetPins()[1U].GetType() ==
            TypeDesc::Generic(GenericParameterId(1U)));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Integer()
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(3U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Integer()
        );
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(3U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Float()
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        const DiagnosticCollection ForwardDiagnostics = GraphIRValidator::Validate(
            MakeOutputIntentConflictGraph(false),
            Descriptors
        );
        const DiagnosticCollection ReversedDiagnostics = GraphIRValidator::Validate(
            MakeOutputIntentConflictGraph(true),
            Descriptors
        );
        MPP_CHECK(CountCode(
            ForwardDiagnostics,
            DiagnosticCode::GenericConstraintConflict
        ) == 1U);
        MPP_CHECK(SameDiagnostics(ForwardDiagnostics, ReversedDiagnostics));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(0U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Integer()
        );
        AddOutputBinding(
            Graph,
            NodeInstanceId(3U),
            PinIndex(3U),
            NodeInstanceId(2U),
            PinIndex(1U)
        );
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(4U),
            NodeInstanceId(1U),
            PinIndex(2U),
            TypeDesc::List(TypeDesc::Integer())
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(9U),
            NodeInstanceId(1U),
            PinIndex(6U),
            TypeDesc::List(TypeDesc::List(TypeDesc::Integer()))
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(5U),
            NodeInstanceId(1U),
            PinIndex(4U),
            TypeDesc::Dictionary(TypeDesc::Integer(), TypeDesc::String())
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(6U),
            NodeInstanceId(1U),
            PinIndex(4U),
            TypeDesc::Dictionary(TypeDesc::Integer(), TypeDesc::String())
        );
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddNode(Graph, NodeInstanceId(3U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(4U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Integer()
        );
        AddOutputBinding(
            Graph,
            NodeInstanceId(4U),
            PinIndex(3U),
            NodeInstanceId(3U),
            PinIndex(1U),
            TypeDesc::Float()
        );
        MPP_CHECK(GraphIRValidator::Validate(Graph, Descriptors).empty());
    }

    {
        GraphIR LiteralConstraint;
        AddNode(LiteralConstraint, NodeInstanceId(1U), ConcreteDescriptorId);
        LiteralConstraint.BindInput(
            NodeInstanceId(1U),
            PinIndex(2U),
            LiteralValue(LiteralValue::Data{std::int64_t{1}}),
            TypeDesc::Integer()
        );
        MPP_CHECK(HasCode(
            GraphIRValidator::Validate(LiteralConstraint, Descriptors),
            DiagnosticCode::InvalidInputBinding
        ));

        GraphIR VariableConstraint;
        AddNode(VariableConstraint, NodeInstanceId(1U), ConcreteDescriptorId);
        VariableConstraint.AddVariable(GraphVariable{
            GraphVariableId(1U), "Value", TypeDesc::Integer(), std::nullopt
        });
        VariableConstraint.BindInput(
            NodeInstanceId(1U),
            PinIndex(2U),
            GraphVariableReference{GraphVariableId(1U)},
            TypeDesc::Integer()
        );
        MPP_CHECK(HasCode(
            GraphIRValidator::Validate(VariableConstraint, Descriptors),
            DiagnosticCode::InvalidInputBinding
        ));
    }

    {
        GraphIR InvalidConstraint;
        AddNode(InvalidConstraint, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(InvalidConstraint, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            InvalidConstraint,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc{}
        );
        MPP_CHECK(HasCode(
            GraphIRValidator::Validate(InvalidConstraint, Descriptors),
            DiagnosticCode::IncompatibleGraphIRTypes
        ));

        GraphIR GenericConstraint;
        AddNode(GenericConstraint, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(GenericConstraint, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            GenericConstraint,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::List(TypeDesc::Generic(GenericParameterId(7U)))
        );
        MPP_CHECK(HasCode(
            GraphIRValidator::Validate(GenericConstraint, Descriptors),
            DiagnosticCode::IncompatibleGraphIRTypes
        ));
    }

    {
        GraphIR ConcreteMismatch;
        AddNode(ConcreteMismatch, NodeInstanceId(1U), ConcreteDescriptorId);
        AddNode(ConcreteMismatch, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            ConcreteMismatch,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(0U),
            TypeDesc::Float()
        );
        MPP_CHECK(HasCode(
            GraphIRValidator::Validate(ConcreteMismatch, Descriptors),
            DiagnosticCode::IncompatibleGraphIRTypes
        ));
    }

    {
        const GraphIR SourceGraph = MakeOutputIntentConflictGraph(false);
        const auto Parsed = GraphIRJson::Deserialize(GraphIRJson::Serialize(SourceGraph));
        MPP_CHECK(Parsed.has_value());
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(*Parsed, Descriptors);
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::GenericConstraintConflict));
    }

    {
        GraphIR Graph;
        AddNode(Graph, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(Graph, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            Graph,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(1U),
            PinIndex(99U),
            TypeDesc::Integer()
        );

        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(Graph, Descriptors);
        MPP_CHECK(Diagnostics.size() == 2U);
        MPP_CHECK(Diagnostics[0U].Code == DiagnosticCode::InvalidGraphIRPinReference);
        MPP_CHECK(Diagnostics[1U].Code == DiagnosticCode::UnresolvedGenericType);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::InvalidGraphIRPinReference) == 1U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 1U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 0U);
    }

    {
        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(
            MakeMixedStructuralAndGenericConflictGraph(false),
            Descriptors
        );
        MPP_CHECK(Diagnostics.size() == 2U);
        MPP_CHECK(Diagnostics[0U].Code == DiagnosticCode::InvalidGraphIRPinReference);
        MPP_CHECK(Diagnostics[1U].Code == DiagnosticCode::GenericConstraintConflict);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::InvalidGraphIRPinReference) == 1U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 1U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 0U);

        const DiagnosticCollection ReversedDiagnostics = GraphIRValidator::Validate(
            MakeMixedStructuralAndGenericConflictGraph(true),
            Descriptors
        );
        MPP_CHECK(SameDiagnostics(Diagnostics, ReversedDiagnostics));
    }

    {
        GraphIR MissingSource;
        AddNode(MissingSource, NodeInstanceId(2U), ConcreteDescriptorId);
        AddOutputBinding(
            MissingSource,
            NodeInstanceId(2U),
            PinIndex(2U),
            NodeInstanceId(99U),
            PinIndex(1U),
            TypeDesc::Integer()
        );

        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(
            MissingSource,
            Descriptors
        );
        MPP_CHECK(Diagnostics.size() == 1U);
        MPP_CHECK(Diagnostics[0U].Code == DiagnosticCode::InvalidGraphIRPinReference);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 0U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) == 0U);
    }

    {
        constexpr NodeDescriptorId EntityDictionarySourceId(2005U);
        constexpr NodeDescriptorId EntityDictionaryTargetId(2006U);
        const TypeDesc GenericEntityDictionary = TypeDesc::Dictionary(
            TypeDesc::Generic(GenericParameterId(1U)),
            TypeDesc::Entity()
        );
        const TypeDesc IntegerEntityDictionary = TypeDesc::Dictionary(
            TypeDesc::Integer(),
            TypeDesc::Entity()
        );
        NodeDescriptorRegistry EntityDictionaryDescriptors;
        MPP_CHECK(EntityDictionaryDescriptors.Register(NodeDescriptor(
            EntityDictionarySourceId,
            "EntityDictionarySource",
            {NodeAvailability::Server},
            {PinSchema("Output", GenericEntityDictionary, PinDirection::Output,
                PinCategory::Data)}
        )).has_value());
        MPP_CHECK(EntityDictionaryDescriptors.Register(NodeDescriptor(
            EntityDictionaryTargetId,
            "EntityDictionaryTarget",
            {NodeAvailability::Server},
            {PinSchema("Input", IntegerEntityDictionary, PinDirection::Input,
                PinCategory::Data)}
        )).has_value());

        GraphIR EntityDictionaryGraph;
        AddNode(EntityDictionaryGraph, NodeInstanceId(1U), EntityDictionarySourceId);
        AddNode(EntityDictionaryGraph, NodeInstanceId(2U), EntityDictionaryTargetId);
        EntityDictionaryGraph.BindInput(
            NodeInstanceId(2U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)},
            IntegerEntityDictionary
        );
        MPP_CHECK(GraphIRValidator::Validate(
            EntityDictionaryGraph,
            EntityDictionaryDescriptors
        ).empty());

        const auto RoundTrip = GraphIRJson::Deserialize(
            GraphIRJson::Serialize(EntityDictionaryGraph)
        );
        MPP_CHECK(RoundTrip.has_value());
        MPP_CHECK(RoundTrip->GetInputBindings().front().OutputTypeConstraint ==
            IntegerEntityDictionary);
        MPP_CHECK(GraphIRValidator::Validate(
            *RoundTrip,
            EntityDictionaryDescriptors
        ).empty());
    }

    {
        GraphIR FlowTypeConstraint;
        AddNode(FlowTypeConstraint, NodeInstanceId(1U), GenericDescriptorId);
        AddNode(FlowTypeConstraint, NodeInstanceId(2U), GenericDescriptorId);
        AddOutputBinding(
            FlowTypeConstraint,
            NodeInstanceId(2U),
            PinIndex(0U),
            NodeInstanceId(1U),
            PinIndex(1U),
            TypeDesc::Flow()
        );

        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(
            FlowTypeConstraint,
            Descriptors
        );
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes));
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 0U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) > 0U);
    }

    {
        GraphIR FlowGraphVariable;
        AddNode(FlowGraphVariable, NodeInstanceId(1U), GenericDescriptorId);
        FlowGraphVariable.AddVariable(GraphVariable{
            GraphVariableId(1U), "Control", TypeDesc::Flow(), std::nullopt
        });
        AddVariableBinding(
            FlowGraphVariable,
            NodeInstanceId(1U),
            PinIndex(0U),
            GraphVariableId(1U)
        );

        const DiagnosticCollection Diagnostics = GraphIRValidator::Validate(
            FlowGraphVariable,
            Descriptors
        );
        MPP_CHECK(HasCode(Diagnostics, DiagnosticCode::InvalidGraphVariable));
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::GenericConstraintConflict) == 0U);
        MPP_CHECK(CountCode(Diagnostics, DiagnosticCode::UnresolvedGenericType) > 0U);
    }

    return 0;
}

#undef MPP_CHECK
