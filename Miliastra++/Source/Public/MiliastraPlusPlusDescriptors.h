#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusTypeDesc.h"
#include "MiliastraPlusPlusValueTypes.h"

namespace MiliastraPlusPlus
{
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

    class NodeDescriptor
    {
    public:
        NodeDescriptor(
            NodeDescriptorId Identifier,
            std::string Name,
            std::vector<NodeAvailability> Availability,
            std::vector<PinSchema> Pins
        )
            : m_Identifier(Identifier)
            , m_Name(std::move(Name))
            , m_Availability(std::move(Availability))
            , m_Pins(std::move(Pins))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            if (!m_Identifier.IsValid() || m_Name.empty())
            {
                return false;
            }

            for (const PinSchema& Pin : m_Pins)
            {
                if (!Pin.GetType().IsValid() ||
                    (Pin.GetCategory() == PinCategory::Data &&
                        ContainsFlowType(Pin.GetType())))
                {
                    return false;
                }
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
    };

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
