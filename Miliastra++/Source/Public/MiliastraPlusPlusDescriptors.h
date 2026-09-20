#pragma once

#include <algorithm>
#include <cstddef>
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusTypeDesc.h"
#include "MiliastraPlusPlusValueTypes.h"

namespace MiliastraPlusPlus
{
    /// Registry identity for a reusable node schema, separate from graph-local
    /// node identity.
    class NodeDescriptorId
    {
    public:
        constexpr NodeDescriptorId() = default;

        explicit constexpr NodeDescriptorId(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const
        {
            return m_Value != 0U;
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const NodeDescriptorId&) const = default;

    private:
        std::uint32_t m_Value = 0U;
    };

    enum class PinDirection
    {
        Input,
        Output
    };

    enum class PinCategory
    {
        Data,
        Execution
    };

    enum class PinCardinality
    {
        Single,
        Optional,
        Multiple
    };

    enum class NodeAvailability
    {
        Server,
        Client
    };

    enum class LoopExitPolicy
    {
        Conditional,
        Unconditional
    };

    struct EntryControlSchema
    {
        PinIndex ExecutionOutput;
    };

    struct SequenceControlSchema
    {
        PinIndex ExecutionInput;
        PinIndex ExecutionOutput;
    };

    struct BranchControlSchema
    {
        PinIndex ExecutionInput;
        PinIndex ConditionInput;
        PinIndex TrueOutput;
        PinIndex FalseOutput;
    };

    struct JoinControlSchema
    {
        PinIndex ExecutionInput;
        PinIndex ExecutionOutput;
    };

    struct LoopControlSchema
    {
        PinIndex ExecutionInput;
        PinIndex BodyOutput;
        PinIndex ExitOutput;
        PinIndex RepeatInput;
        PinIndex BreakInput;
        LoopExitPolicy ExitPolicy = LoopExitPolicy::Conditional;
        std::optional<PinIndex> ConditionInput;
    };

    struct ReturnControlSchema
    {
        // Return is execution-only and has no continuation pin.
        PinIndex ExecutionInput;
    };

    /// Trusted control roles refer to pin indices; pin labels do not define
    /// execution semantics.
    using ExecutionControlSchema = std::variant<
        EntryControlSchema,
        SequenceControlSchema,
        BranchControlSchema,
        JoinControlSchema,
        LoopControlSchema,
        ReturnControlSchema>;

    /// Pin type, direction, cardinality, and literal policy used to validate
    /// node instances.
    class PinSchema
    {
    public:
        PinSchema(
            std::string Name,
            TypeDesc Type,
            PinDirection Direction,
            PinCategory Category,
            PinCardinality Cardinality = PinCardinality::Single,
            bool AllowsLiteral = false,
            std::optional<LiteralValue> DefaultValue = std::nullopt
        )
            : m_Name(std::move(Name))
            , m_Type(std::move(Type))
            , m_Direction(Direction)
            , m_Category(Category)
            , m_Cardinality(Cardinality)
            , m_AllowsLiteral(AllowsLiteral)
            , m_DefaultValue(std::move(DefaultValue))
        {
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] const TypeDesc& GetType() const
        {
            return m_Type;
        }

        [[nodiscard]] PinDirection GetDirection() const
        {
            return m_Direction;
        }

        [[nodiscard]] PinCategory GetCategory() const
        {
            return m_Category;
        }

        [[nodiscard]] PinCardinality GetCardinality() const
        {
            return m_Cardinality;
        }

        [[nodiscard]] bool AllowsLiteral() const
        {
            return m_AllowsLiteral;
        }

        [[nodiscard]] const std::optional<LiteralValue>& GetDefaultValue() const
        {
            return m_DefaultValue;
        }

    private:
        std::string m_Name;
        TypeDesc m_Type;
        PinDirection m_Direction;
        PinCategory m_Category;
        PinCardinality m_Cardinality;
        bool m_AllowsLiteral;
        std::optional<LiteralValue> m_DefaultValue;
    };

    /// A trusted node schema. Structured control meaning comes from its control schema.
    class NodeDescriptor
    {
    public:
        NodeDescriptor(
            NodeDescriptorId Identifier,
            std::string Name,
            std::vector<NodeAvailability> Availability,
            std::vector<PinSchema> Pins,
            std::optional<ExecutionControlSchema> ControlSchema = std::nullopt
        )
            : m_Identifier(Identifier)
            , m_Name(std::move(Name))
            , m_Availability(std::move(Availability))
            , m_Pins(std::move(Pins))
            , m_ControlSchema(std::move(ControlSchema))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            if (!m_Identifier.IsValid() || m_Name.empty())
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < m_Availability.size(); ++Index)
            {
                if (!IsValidAvailability(m_Availability[Index]))
                {
                    return false;
                }
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (m_Availability[Prior] == m_Availability[Index])
                    {
                        return false;
                    }
                }
            }

            for (std::size_t Index = 0U; Index < m_Pins.size(); ++Index)
            {
                const PinSchema& Pin = m_Pins[Index];
                if (Pin.GetName().empty() || !Pin.GetType().IsValid() ||
                    !IsValidDirection(Pin.GetDirection()) ||
                    !IsValidCategory(Pin.GetCategory()) ||
                    !IsValidCardinality(Pin.GetCardinality()) ||
                    (Pin.GetCategory() == PinCategory::Data &&
                        ContainsFlowType(Pin.GetType())) ||
                    (Pin.GetCategory() == PinCategory::Execution &&
                        Pin.GetType() != TypeDesc::Flow()) ||
                    (Pin.GetCategory() == PinCategory::Execution &&
                        (Pin.AllowsLiteral() || Pin.GetDefaultValue().has_value())) ||
                    (Pin.GetDirection() == PinDirection::Output &&
                        Pin.GetDefaultValue().has_value()))
                {
                    return false;
                }
                if (Pin.GetDefaultValue().has_value() &&
                    (Pin.GetCategory() != PinCategory::Data ||
                        Pin.GetDirection() != PinDirection::Input ||
                        !IsLiteralCompatible(*Pin.GetDefaultValue(), Pin.GetType())))
                {
                    return false;
                }
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (m_Pins[Prior].GetName() == Pin.GetName())
                    {
                        return false;
                    }
                }
            }

            if (m_ControlSchema.has_value() && !IsControlSchemaValid(*m_ControlSchema))
            {
                return false;
            }
            return true;
        }

        [[nodiscard]] NodeDescriptorId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] const std::vector<NodeAvailability>& GetAvailability() const
        {
            return m_Availability;
        }

        [[nodiscard]] bool IsAvailableOn(NodeAvailability Domain) const
        {
            return std::find(m_Availability.begin(), m_Availability.end(), Domain) !=
                m_Availability.end();
        }

        [[nodiscard]] const std::vector<PinSchema>& GetPins() const
        {
            return m_Pins;
        }

        [[nodiscard]] const std::optional<ExecutionControlSchema>& GetExecutionControlSchema() const
        {
            return m_ControlSchema;
        }

        [[nodiscard]] const PinSchema* FindPinByName(const std::string& Name) const
        {
            for (const PinSchema& Pin : m_Pins)
            {
                if (Pin.GetName() == Name)
                {
                    return &Pin;
                }
            }
            return nullptr;
        }

    private:
        [[nodiscard]] static bool IsValidDirection(PinDirection Direction)
        {
            switch (Direction)
            {
            case PinDirection::Input:
            case PinDirection::Output:
                return true;
            }
            return false;
        }

        [[nodiscard]] static bool IsValidCategory(PinCategory Category)
        {
            switch (Category)
            {
            case PinCategory::Data:
            case PinCategory::Execution:
                return true;
            }
            return false;
        }

        [[nodiscard]] static bool IsValidCardinality(PinCardinality Cardinality)
        {
            switch (Cardinality)
            {
            case PinCardinality::Single:
            case PinCardinality::Optional:
            case PinCardinality::Multiple:
                return true;
            }
            return false;
        }

        [[nodiscard]] static bool IsValidAvailability(NodeAvailability Availability)
        {
            switch (Availability)
            {
            case NodeAvailability::Server:
            case NodeAvailability::Client:
                return true;
            }
            return false;
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
            case TypeDesc::Kind::Enum:
                return Literal.Is<EnumLiteralValue>() &&
                    Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                    Type.GetEnumTypeIdentity();
            default: return false;
            }
        }

        [[nodiscard]] bool IsControlSchemaValid(const ExecutionControlSchema& Schema) const
        {
            std::vector<PinIndex> DeclaredRoles;
            const auto AddRole = [this, &DeclaredRoles](
                PinIndex Index,
                PinDirection Direction,
                PinCategory Category,
                const TypeDesc& Type,
                PinCardinality Cardinality) -> bool
            {
                if (!Index.IsValid() || Index.GetValue() >= m_Pins.size())
                {
                    return false;
                }
                for (const PinIndex Prior : DeclaredRoles)
                {
                    if (Prior == Index)
                    {
                        return false;
                    }
                }
                const PinSchema& Pin = m_Pins[Index.GetValue()];
                if (Pin.GetDirection() != Direction || Pin.GetCategory() != Category ||
                    Pin.GetType() != Type || Pin.GetCardinality() != Cardinality)
                {
                    return false;
                }
                DeclaredRoles.push_back(Index);
                return true;
            };
            const auto AddFlowRole = [&AddRole](PinIndex Index, PinDirection Direction,
                PinCardinality Cardinality = PinCardinality::Single) -> bool
            {
                return AddRole(Index, Direction, PinCategory::Execution,
                    TypeDesc::Flow(), Cardinality);
            };

            bool RolesValid = std::visit([&](const auto& Control)
            {
                using Schema = std::decay_t<decltype(Control)>;
                if constexpr (std::is_same_v<Schema, EntryControlSchema>)
                {
                    return AddFlowRole(Control.ExecutionOutput, PinDirection::Output);
                }
                else if constexpr (std::is_same_v<Schema, SequenceControlSchema>)
                {
                    return AddFlowRole(Control.ExecutionInput, PinDirection::Input) &&
                        AddFlowRole(Control.ExecutionOutput, PinDirection::Output);
                }
                else if constexpr (std::is_same_v<Schema, BranchControlSchema>)
                {
                    return AddFlowRole(Control.ExecutionInput, PinDirection::Input) &&
                        AddRole(Control.ConditionInput, PinDirection::Input,
                            PinCategory::Data, TypeDesc::Boolean(), PinCardinality::Single) &&
                        AddFlowRole(Control.TrueOutput, PinDirection::Output) &&
                        AddFlowRole(Control.FalseOutput, PinDirection::Output);
                }
                else if constexpr (std::is_same_v<Schema, JoinControlSchema>)
                {
                    return AddFlowRole(Control.ExecutionInput, PinDirection::Input,
                            PinCardinality::Multiple) &&
                        AddFlowRole(Control.ExecutionOutput, PinDirection::Output);
                }
                else if constexpr (std::is_same_v<Schema, LoopControlSchema>)
                {
                    const bool PolicyValid =
                        Control.ExitPolicy == LoopExitPolicy::Conditional ||
                        Control.ExitPolicy == LoopExitPolicy::Unconditional;
                    if (!PolicyValid)
                    {
                        return false;
                    }
                    bool FlowRolesValid =
                        AddFlowRole(Control.ExecutionInput, PinDirection::Input) &&
                        AddFlowRole(Control.BodyOutput, PinDirection::Output) &&
                        AddFlowRole(Control.ExitOutput, PinDirection::Output) &&
                        AddFlowRole(Control.RepeatInput, PinDirection::Input,
                            PinCardinality::Multiple) &&
                        AddFlowRole(Control.BreakInput, PinDirection::Input,
                            PinCardinality::Multiple);
                    if (!FlowRolesValid)
                    {
                        return false;
                    }
                    if (Control.ExitPolicy == LoopExitPolicy::Conditional)
                    {
                        return Control.ConditionInput.has_value() &&
                            AddRole(*Control.ConditionInput, PinDirection::Input,
                                PinCategory::Data, TypeDesc::Boolean(),
                                PinCardinality::Single);
                    }
                    return !Control.ConditionInput.has_value();
                }
                else if constexpr (std::is_same_v<Schema, ReturnControlSchema>)
                {
                    return AddFlowRole(Control.ExecutionInput, PinDirection::Input) &&
                        std::none_of(m_Pins.begin(), m_Pins.end(),
                            [](const PinSchema& Pin)
                            {
                                return Pin.GetCategory() == PinCategory::Data;
                            });
                }
                else
                {
                    return AddFlowRole(Control.ExecutionInput, PinDirection::Input);
                }
            }, Schema);

            if (!RolesValid)
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < m_Pins.size(); ++Index)
            {
                if (m_Pins[Index].GetCategory() == PinCategory::Execution)
                {
                    bool IsDeclared = false;
                    for (const PinIndex Role : DeclaredRoles)
                    {
                        if (Role.GetValue() == Index)
                        {
                            IsDeclared = true;
                            break;
                        }
                    }
                    if (!IsDeclared)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]] static bool ContainsFlowType(const TypeDesc& Type)
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

        NodeDescriptorId m_Identifier;
        std::string m_Name;
        std::vector<NodeAvailability> m_Availability;
        std::vector<PinSchema> m_Pins;
        std::optional<ExecutionControlSchema> m_ControlSchema;
    };

    /// Owns validated descriptors and resolves them by registry-local identifier.
    class NodeDescriptorRegistry
    {
    public:
        [[nodiscard]] std::expected<void, Diagnostic> Register(NodeDescriptor Descriptor)
        {
            if (!Descriptor.IsValid())
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::InvalidNodeDescriptor,
                    .Message = "An invalid node descriptor cannot be registered."
                });
            }

            if (Find(Descriptor.GetIdentifier()) != nullptr)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::DuplicateDescriptor,
                    .Message = "A node descriptor with this identifier is already registered."
                });
            }

            m_Descriptors.push_back(std::move(Descriptor));
            return {};
        }

        [[nodiscard]] const NodeDescriptor* Find(NodeDescriptorId Identifier) const
        {
            for (const NodeDescriptor& Descriptor : m_Descriptors)
            {
                if (Descriptor.GetIdentifier() == Identifier)
                {
                    return &Descriptor;
                }
            }
            return nullptr;
        }

        [[nodiscard]] std::expected<const NodeDescriptor*, Diagnostic> Get(
            NodeDescriptorId Identifier
        ) const
        {
            const NodeDescriptor* Descriptor = Find(Identifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::MissingDescriptor,
                    .Message = "The requested node descriptor is not registered."
                });
            }
            return Descriptor;
        }

        [[nodiscard]] std::size_t Size() const
        {
            return m_Descriptors.size();
        }

    private:
        std::vector<NodeDescriptor> m_Descriptors;
    };
}
