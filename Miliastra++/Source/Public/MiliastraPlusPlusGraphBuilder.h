#pragma once

#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusGraphIR.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

namespace MiliastraPlusPlus
{
    class GraphBuilder;

    class NodeHandle
    {
    public:
        struct Context;

        NodeHandle() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Identifier.IsValid() && m_Descriptor.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] NodeDescriptorId GetDescriptor() const
        {
            return m_Descriptor;
        }

    private:
        NodeHandle(
            NodeInstanceId Identifier,
            NodeDescriptorId Descriptor,
            const std::shared_ptr<const Context>& ContextToken
        )
            : m_Identifier(Identifier)
            , m_Descriptor(Descriptor)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Identifier;
        NodeDescriptorId m_Descriptor;
        std::weak_ptr<const Context> m_Context;
    };

    struct NodeHandle::Context
    {
    };

    template<typename T>
    struct CppTypeDesc
    {
        static constexpr bool IsSupported = false;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return std::unexpected(DiagnosticCollection{
                Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::UnsupportedCppType,
                    .Message = "The requested C++ type is not supported by the graph type bridge."
                }
            });
        }
    };

    template<>
    struct CppTypeDesc<bool>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Boolean();
        }
    };

    template<typename T>
        requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    struct CppTypeDesc<T>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Integer();
        }
    };

    template<typename T>
        requires std::is_floating_point_v<T>
    struct CppTypeDesc<T>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Float();
        }
    };

    template<>
    struct CppTypeDesc<std::string>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::String();
        }
    };

    template<>
    struct CppTypeDesc<GuidValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::GUID();
        }
    };

    template<>
    struct CppTypeDesc<Vector3Value>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Vector3();
        }
    };

    template<>
    struct CppTypeDesc<PrefabIdValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::PrefabId();
        }
    };

    template<>
    struct CppTypeDesc<ConfigIdValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::ConfigId();
        }
    };

    template<>
    struct CppTypeDesc<FactionValue>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Faction();
        }
    };

    template<>
    struct CppTypeDesc<EntityTypeTag>
    {
        static constexpr bool IsSupported = true;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            return TypeDesc::Entity();
        }
    };

    template<typename T>
    struct CppTypeDesc<std::vector<T>>
    {
        static constexpr bool IsSupported = CppTypeDesc<T>::IsSupported;

        [[nodiscard]] static std::expected<TypeDesc, DiagnosticCollection> Get()
        {
            auto ElementResult = CppTypeDesc<T>::Get();
            if (!ElementResult.has_value())
            {
                return std::unexpected(std::move(ElementResult.error()));
            }
            return TypeDesc::List(std::move(*ElementResult));
        }
    };

    template<typename T>
    [[nodiscard]] std::expected<TypeDesc, DiagnosticCollection> GetCppTypeDesc()
    {
        return CppTypeDesc<std::remove_cvref_t<T>>::Get();
    }

    template<typename T>
    [[nodiscard]] std::expected<LiteralValue, DiagnosticCollection> MakeLiteralValue(T Value)
    {
        using ValueType = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<ValueType, bool>)
        {
            return LiteralValue(LiteralValue::Data{Value});
        }
        else if constexpr (std::is_integral_v<ValueType>)
        {
            if constexpr (std::is_unsigned_v<ValueType>)
            {
                if (Value > static_cast<ValueType>(std::numeric_limits<std::int64_t>::max()))
                {
                    return std::unexpected(DiagnosticCollection{
                        Diagnostic{
                            .Severity = DiagnosticSeverity::Error,
                            .Code = DiagnosticCode::IncompatibleGraphIRTypes,
                            .Message = "The integer literal cannot be represented by graph integer storage."
                        }
                    });
                }
            }
            return LiteralValue(LiteralValue::Data{static_cast<std::int64_t>(Value)});
        }
        else if constexpr (std::is_floating_point_v<ValueType>)
        {
            return LiteralValue(LiteralValue::Data{static_cast<double>(Value)});
        }
        else if constexpr (std::is_same_v<ValueType, std::string> ||
            std::is_same_v<ValueType, GuidValue> ||
            std::is_same_v<ValueType, Vector3Value> ||
            std::is_same_v<ValueType, PrefabIdValue> ||
            std::is_same_v<ValueType, ConfigIdValue> ||
            std::is_same_v<ValueType, FactionValue>)
        {
            return LiteralValue(LiteralValue::Data{std::move(Value)});
        }
        else
        {
            return std::unexpected(DiagnosticCollection{
                Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::UnsupportedCppType,
                    .Message = "The requested C++ type has no supported graph literal representation."
                }
            });
        }
    }

    template<typename T>
    class Output
    {
    public:
        Output() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_SourceNode.IsValid() && m_SourceOutputPin.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetIdentifier() const
        {
            return m_SourceNode;
        }

        [[nodiscard]] PinIndex GetPin() const
        {
            return m_SourceOutputPin;
        }

    private:
        Output(
            NodeInstanceId SourceNode,
            PinIndex SourceOutputPin,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_SourceNode(SourceNode)
            , m_SourceOutputPin(SourceOutputPin)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_SourceNode;
        PinIndex m_SourceOutputPin;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    template<typename T>
    class ValueOrExpr;

    template<typename T>
    class Variable
    {
    public:
        Variable() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Identifier.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] GraphVariableId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] ValueOrExpr<T> AsInput() const;

    private:
        Variable(
            GraphVariableId Identifier,
            const std::shared_ptr<const NodeHandle::Context>& ContextToken
        )
            : m_Identifier(Identifier)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        GraphVariableId m_Identifier;
        std::weak_ptr<const NodeHandle::Context> m_Context;
    };

    template<typename T>
    class ValueOrExpr
    {
    public:
        using Variant = std::variant<LiteralValue, Output<T>, Variable<T>>;

        explicit ValueOrExpr(LiteralValue Literal)
            : m_Value(std::move(Literal))
        {
        }

        explicit ValueOrExpr(Output<T> OutputValue)
            : m_Value(std::move(OutputValue))
        {
        }

        explicit ValueOrExpr(Variable<T> VariableValue)
            : m_Value(std::move(VariableValue))
        {
        }

        [[nodiscard]] const Variant& GetValue() const
        {
            return m_Value;
        }

    private:
        Variant m_Value;
    };

    template<typename T>
    [[nodiscard]] ValueOrExpr<T> Variable<T>::AsInput() const
    {
        return ValueOrExpr<T>(*this);
    }

    class GraphBuilder
    {
    public:
        explicit GraphBuilder(const NodeDescriptorRegistry& Descriptors)
            : m_Descriptors(Descriptors)
            , m_Context(std::make_shared<const NodeHandle::Context>())
        {
        }

        GraphBuilder(const GraphBuilder&) = delete;
        GraphBuilder& operator=(const GraphBuilder&) = delete;

        GraphBuilder(GraphBuilder&& Other) noexcept
            : m_Descriptors(Other.m_Descriptors)
            , m_Graph(std::move(Other.m_Graph))
            , m_NextNodeIdentifier(Other.m_NextNodeIdentifier)
            , m_NextGraphVariableIdentifier(Other.m_NextGraphVariableIdentifier)
            , m_Context(std::move(Other.m_Context))
            , m_IsClosed(Other.m_IsClosed)
        {
            Other.m_IsClosed = true;
            Other.m_Context.reset();
        }

        GraphBuilder& operator=(GraphBuilder&&) = delete;

        [[nodiscard]] std::expected<NodeHandle, DiagnosticCollection> AddNode(
            NodeDescriptorId DescriptorIdentifier
        )
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot add a node.")
                });
            }

            if (!DescriptorIdentifier.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("An invalid node descriptor identifier was supplied.")
                });
            }

            const NodeDescriptor* Descriptor = m_Descriptors.Find(DescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic("The requested node descriptor is not registered.")
                });
            }

            if (!Descriptor->IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("The requested node descriptor is invalid.")
                });
            }

            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId Identifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{Identifier, DescriptorIdentifier});

            if (m_NextNodeIdentifier == std::numeric_limits<std::uint64_t>::max())
            {
                m_NextNodeIdentifier = 0U;
            }
            else
            {
                ++m_NextNodeIdentifier;
            }

            return NodeHandle(Identifier, DescriptorIdentifier, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<Output<T>, DiagnosticCollection> GetOutput(
            const NodeHandle& Node,
            PinIndex OutputPin
        ) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (!IsHandleUsable(Node))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("The output source handle is invalid or belongs to another builder.")
                });
            }

            const NodeInstance* SourceNode = m_Graph.FindNode(Node.GetIdentifier());
            const NodeDescriptor* Descriptor = m_Descriptors.Find(SourceNode->Descriptor);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic(
                        "The output source node descriptor is no longer registered.")
                });
            }
            if (!OutputPin.IsValid() || OutputPin.GetValue() >= Descriptor->GetPins().size())
            {
                return std::unexpected(DiagnosticCollection{
                    MakePinDiagnostic("The requested output pin does not exist.")
                });
            }

            const PinSchema& Pin = Descriptor->GetPins()[OutputPin.GetValue()];
            if (Pin.GetDirection() != PinDirection::Output || Pin.GetCategory() != PinCategory::Data)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("A typed output must reference a data output pin.")
                });
            }
            if (!Pin.GetType().IsCompatibleWith(*TypeResult))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeTypeDiagnostic("The requested C++ output type is incompatible with the descriptor pin type.")
                });
            }

            return Output<T>(Node.GetIdentifier(), OutputPin, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<Variable<T>, DiagnosticCollection> DeclareVariable(
            std::string Name,
            std::optional<LiteralValue> DefaultValue = std::nullopt
        )
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot declare a variable.")
                });
            }

            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (Name.empty())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeVariableDiagnostic("A graph variable name cannot be empty.")
                });
            }
            for (const GraphVariable& VariableRecord : m_Graph.GetVariables())
            {
                if (VariableRecord.Name == Name)
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeVariableNameDiagnostic("A graph variable with this name already exists.")
                    });
                }
            }
            if (DefaultValue.has_value() && !IsLiteralCompatible(*DefaultValue, *TypeResult))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeTypeDiagnostic("The graph variable default is incompatible with its declared C++ type.")
                });
            }
            if (m_NextGraphVariableIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its graph variable identifier range.")
                });
            }

            const GraphVariableId Identifier(m_NextGraphVariableIdentifier);
            m_Graph.AddVariable(GraphVariable{
                Identifier,
                std::move(Name),
                *TypeResult,
                std::move(DefaultValue)
            });
            AdvanceGraphVariableIdentifier();
            return Variable<T>(Identifier, m_Context);
        }

        template<typename T>
        [[nodiscard]] std::expected<void, DiagnosticCollection> BindInput(
            const NodeHandle& DestinationNode,
            PinIndex DestinationPin,
            const ValueOrExpr<T>& Expression
        )
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot bind an input.")
                });
            }
            if (!IsHandleUsable(DestinationNode))
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("The destination node handle is invalid or belongs to another builder.")
                });
            }

            const NodeInstance* Destination = m_Graph.FindNode(DestinationNode.GetIdentifier());
            const NodeDescriptor* Descriptor = m_Descriptors.Find(Destination->Descriptor);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic(
                        "The input destination node descriptor is no longer registered.")
                });
            }
            if (!DestinationPin.IsValid() || DestinationPin.GetValue() >= Descriptor->GetPins().size())
            {
                return std::unexpected(DiagnosticCollection{
                    MakePinDiagnostic("The requested destination pin does not exist.")
                });
            }
            const PinSchema& Pin = Descriptor->GetPins()[DestinationPin.GetValue()];
            if (Pin.GetDirection() != PinDirection::Input || Pin.GetCategory() != PinCategory::Data)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeBindingDiagnostic("An input binding destination must be a data input pin.")
                });
            }
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return std::unexpected(TypeResult.error());
            }
            if (Pin.GetCardinality() != PinCardinality::Multiple &&
                CountBindings(DestinationNode.GetIdentifier(), DestinationPin) != 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    Diagnostic{
                        .Severity = DiagnosticSeverity::Error,
                        .Code = DiagnosticCode::DuplicateInputBinding,
                        .Message = "A Single or Optional input pin already has a binding."
                    }
                });
            }

            InputBinding Binding;
            std::optional<TypeDesc> OutputTypeConstraint;
            const auto& Value = Expression.GetValue();
            if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Value))
            {
                if (!Pin.AllowsLiteral())
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeBindingDiagnostic("The destination pin does not allow literal bindings.")
                    });
                }
                if (!IsLiteralCompatible(*Literal, *TypeResult))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeTypeDiagnostic(
                            "The literal is incompatible with the ValueOrExpr C++ type.")
                    });
                }
                if (!IsLiteralCompatible(*Literal, Pin.GetType()))
                {
                    return std::unexpected(DiagnosticCollection{
                        MakeTypeDiagnostic("The literal is incompatible with the destination pin type.")
                    });
                }
                Binding = *Literal;
            }
            else if (const Output<T>* OutputValue = std::get_if<Output<T>>(&Value))
            {
                const auto OutputValidation = ValidateOutput(*OutputValue, Pin.GetType());
                if (!OutputValidation.empty())
                {
                    return std::unexpected(OutputValidation);
                }
                Binding = OutputReference{
                    OutputValue->GetIdentifier(),
                    OutputValue->GetPin()
                };
                OutputTypeConstraint = *TypeResult;
            }
            else
            {
                const Variable<T>& VariableValue = std::get<Variable<T>>(Value);
                const auto VariableValidation = ValidateVariable(VariableValue, Pin.GetType());
                if (!VariableValidation.empty())
                {
                    return std::unexpected(VariableValidation);
                }
                Binding = GraphVariableReference{VariableValue.GetIdentifier()};
            }

            m_Graph.BindInput(
                DestinationNode.GetIdentifier(),
                DestinationPin,
                std::move(Binding),
                std::move(OutputTypeConstraint)
            );
            return {};
        }

        [[nodiscard]] bool IsHandleUsable(const NodeHandle& Handle) const
        {
            if (m_IsClosed || !Handle.IsValid() || !HasSameContext(Handle))
            {
                return false;
            }

            const NodeInstance* Node = m_Graph.FindNode(Handle.GetIdentifier());
            return Node != nullptr && Node->Descriptor == Handle.GetDescriptor();
        }

        [[nodiscard]] DiagnosticCollection Validate() const
        {
            if (m_IsClosed)
            {
                return DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be validated.")
                };
            }

            return GraphIRValidator::Validate(m_Graph, m_Descriptors);
        }

        [[nodiscard]] std::expected<GraphIR, DiagnosticCollection> Finalize() &&
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be finalized.")
                });
            }

            m_IsClosed = true;
            m_Context.reset();

            DiagnosticCollection Diagnostics = GraphIRValidator::Validate(m_Graph, m_Descriptors);
            if (ContainsError(Diagnostics))
            {
                return std::unexpected(std::move(Diagnostics));
            }

            return std::move(m_Graph);
        }

    private:
        [[nodiscard]] std::size_t CountBindings(
            NodeInstanceId DestinationNode,
            PinIndex DestinationPin
        ) const
        {
            std::size_t Count = 0U;
            for (const InputBindingRecord& Record : m_Graph.GetInputBindings())
            {
                if (Record.DestinationNode == DestinationNode &&
                    Record.DestinationInputPin == DestinationPin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        template<typename T>
        [[nodiscard]] DiagnosticCollection ValidateOutput(
            const Output<T>& OutputValue,
            const TypeDesc& DestinationType
        ) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return TypeResult.error();
            }
            if (!OutputValue.IsValid() || !HasSameContext(OutputValue))
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("The output handle is invalid or belongs to another builder.")
                };
            }
            const NodeInstance* SourceNode = m_Graph.FindNode(OutputValue.GetIdentifier());
            if (SourceNode == nullptr)
            {
                return DiagnosticCollection{
                    MakePinDiagnostic("The output handle references a missing node.")
                };
            }
            const NodeDescriptor* Descriptor = m_Descriptors.Find(SourceNode->Descriptor);
            if (Descriptor == nullptr)
            {
                return DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic(
                        "The output source node descriptor is no longer registered.")
                };
            }
            if (OutputValue.GetPin().GetValue() >= Descriptor->GetPins().size())
            {
                return DiagnosticCollection{
                    MakePinDiagnostic("The output handle references a missing pin.")
                };
            }
            const PinSchema& SourcePin = Descriptor->GetPins()[OutputValue.GetPin().GetValue()];
            if (SourcePin.GetDirection() != PinDirection::Output ||
                SourcePin.GetCategory() != PinCategory::Data)
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("An output binding must reference a data output pin.")
                };
            }
            if (!SourcePin.GetType().IsCompatibleWith(*TypeResult) ||
                !SourcePin.GetType().IsCompatibleWith(DestinationType))
            {
                return DiagnosticCollection{
                    MakeTypeDiagnostic("The output type is incompatible with the destination pin type.")
                };
            }
            return {};
        }

        template<typename T>
        [[nodiscard]] DiagnosticCollection ValidateVariable(
            const Variable<T>& VariableValue,
            const TypeDesc& DestinationType
        ) const
        {
            const auto TypeResult = GetCppTypeDesc<T>();
            if (!TypeResult.has_value())
            {
                return TypeResult.error();
            }
            if (!VariableValue.IsValid() || !HasSameContext(VariableValue))
            {
                return DiagnosticCollection{
                    MakeBindingDiagnostic("The graph variable handle is invalid or belongs to another builder.")
                };
            }
            const GraphVariable* SourceVariable = m_Graph.FindVariable(VariableValue.GetIdentifier());
            if (SourceVariable == nullptr)
            {
                return DiagnosticCollection{
                    MakeVariableDiagnostic("The graph variable handle references a missing variable.")
                };
            }
            if (!SourceVariable->Type.IsCompatibleWith(*TypeResult) ||
                !SourceVariable->Type.IsCompatibleWith(DestinationType))
            {
                return DiagnosticCollection{
                    MakeTypeDiagnostic("The graph variable type is incompatible with the destination pin type.")
                };
            }
            return {};
        }

        template<typename T>
        [[nodiscard]] bool HasSameContext(const Output<T>& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }
            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            const std::weak_ptr<const NodeHandle::Context> HandleContext = Handle.m_Context;
            return !BuilderContext.owner_before(HandleContext) &&
                !HandleContext.owner_before(BuilderContext);
        }

        template<typename T>
        [[nodiscard]] bool HasSameContext(const Variable<T>& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }
            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            const std::weak_ptr<const NodeHandle::Context> HandleContext = Handle.m_Context;
            return !BuilderContext.owner_before(HandleContext) &&
                !HandleContext.owner_before(BuilderContext);
        }

        [[nodiscard]] static bool IsLiteralCompatible(
            const LiteralValue& Literal,
            const TypeDesc& Type
        )
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

        [[nodiscard]] static Diagnostic MakeBindingDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidInputBinding,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakePinDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphIRPinReference,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeTypeDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::IncompatibleGraphIRTypes,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeVariableDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphVariable,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeVariableNameDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::DuplicateGraphVariableName,
                .Message = Message
            };
        }

        void AdvanceGraphVariableIdentifier()
        {
            if (m_NextGraphVariableIdentifier == std::numeric_limits<std::uint64_t>::max())
            {
                m_NextGraphVariableIdentifier = 0U;
            }
            else
            {
                ++m_NextGraphVariableIdentifier;
            }
        }

        [[nodiscard]] bool HasSameContext(const NodeHandle& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }

            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            return !BuilderContext.owner_before(Handle.m_Context) &&
                !Handle.m_Context.owner_before(BuilderContext);
        }

        [[nodiscard]] static Diagnostic MakeDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidNodeDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeMissingDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::MissingDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeLifecycleDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphBuilderState,
                .Message = Message
            };
        }

        const NodeDescriptorRegistry& m_Descriptors;
        GraphIR m_Graph;
        std::uint64_t m_NextNodeIdentifier = 1U;
        std::uint64_t m_NextGraphVariableIdentifier = 1U;
        std::shared_ptr<const NodeHandle::Context> m_Context;
        bool m_IsClosed = false;
    };
}
