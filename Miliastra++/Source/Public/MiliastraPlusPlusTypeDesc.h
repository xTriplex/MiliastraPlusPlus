#pragma once

#include <compare>
#include <cstdint>
#include <expected>
#include <memory>
#include <utility>

#include "MiliastraPlusPlusDiagnostics.h"

namespace MiliastraPlusPlus
{
    class GenericParameterId
    {
    public:
        constexpr GenericParameterId() = default;

        explicit constexpr GenericParameterId(std::uint32_t Value)
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

        auto operator<=>(const GenericParameterId&) const = default;

    private:
        std::uint32_t m_Value = 0U;
    };

    class StructTypeId
    {
    public:
        constexpr StructTypeId() = default;

        explicit constexpr StructTypeId(std::uint32_t Value)
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

        auto operator<=>(const StructTypeId&) const = default;

    private:
        std::uint32_t m_Value = 0U;
    };

    class TypeDesc
    {
    public:
        enum class Kind
        {
            Invalid,
            Boolean,
            Integer,
            Float,
            String,
            Flow,
            Entity,
            GUID,
            Vector3,
            PrefabId,
            ConfigId,
            Faction,
            Generic,
            List,
            Dictionary,
            StructObject
        };

        TypeDesc() = default;

        [[nodiscard]] static TypeDesc Boolean()
        {
            return TypeDesc(Kind::Boolean);
        }

        [[nodiscard]] static TypeDesc Integer()
        {
            return TypeDesc(Kind::Integer);
        }

        [[nodiscard]] static TypeDesc Float()
        {
            return TypeDesc(Kind::Float);
        }

        [[nodiscard]] static TypeDesc String()
        {
            return TypeDesc(Kind::String);
        }

        [[nodiscard]] static TypeDesc Flow()
        {
            return TypeDesc(Kind::Flow);
        }

        [[nodiscard]] static TypeDesc Entity()
        {
            return TypeDesc(Kind::Entity);
        }

        [[nodiscard]] static TypeDesc GUID()
        {
            return TypeDesc(Kind::GUID);
        }

        [[nodiscard]] static TypeDesc Vector3()
        {
            return TypeDesc(Kind::Vector3);
        }

        [[nodiscard]] static TypeDesc PrefabId()
        {
            return TypeDesc(Kind::PrefabId);
        }

        [[nodiscard]] static TypeDesc ConfigId()
        {
            return TypeDesc(Kind::ConfigId);
        }

        [[nodiscard]] static TypeDesc Faction()
        {
            return TypeDesc(Kind::Faction);
        }

        [[nodiscard]] static TypeDesc Generic(GenericParameterId Parameter)
        {
            TypeDesc Result(Kind::Generic);
            Result.m_GenericParameter = Parameter;
            return Result;
        }

        [[nodiscard]] static TypeDesc List(TypeDesc ElementType)
        {
            TypeDesc Result(Kind::List);
            Result.m_ElementType = std::make_shared<const TypeDesc>(std::move(ElementType));
            return Result;
        }

        [[nodiscard]] static TypeDesc Dictionary(TypeDesc KeyType, TypeDesc ValueType)
        {
            TypeDesc Result(Kind::Dictionary);
            Result.m_KeyType = std::make_shared<const TypeDesc>(std::move(KeyType));
            Result.m_ValueType = std::make_shared<const TypeDesc>(std::move(ValueType));
            return Result;
        }

        [[nodiscard]] static TypeDesc StructObject(StructTypeId Identifier)
        {
            TypeDesc Result(Kind::StructObject);
            Result.m_StructType = Identifier;
            return Result;
        }

        [[nodiscard]] bool IsValid() const
        {
            switch (m_Kind)
            {
            case Kind::Invalid:
                return false;
            case Kind::Generic:
                return m_GenericParameter.IsValid();
            case Kind::List:
                return m_ElementType != nullptr && m_ElementType->IsValid();
            case Kind::Dictionary:
                return m_KeyType != nullptr && m_KeyType->IsValid() &&
                    m_ValueType != nullptr && m_ValueType->IsValid();
            case Kind::StructObject:
                return m_StructType.IsValid();
            default:
                return true;
            }
        }

        [[nodiscard]] Kind GetKind() const
        {
            return m_Kind;
        }

        [[nodiscard]] GenericParameterId GetGenericParameter() const
        {
            return m_GenericParameter;
        }

        [[nodiscard]] const TypeDesc* GetElementType() const
        {
            return m_ElementType.get();
        }

        [[nodiscard]] const TypeDesc* GetKeyType() const
        {
            return m_KeyType.get();
        }

        [[nodiscard]] const TypeDesc* GetValueType() const
        {
            return m_ValueType.get();
        }

        [[nodiscard]] StructTypeId GetStructType() const
        {
            return m_StructType;
        }

        [[nodiscard]] bool IsCompatibleWith(const TypeDesc& OtherType) const
        {
            if (!IsValid() || !OtherType.IsValid())
            {
                return false;
            }

            if (*this == OtherType || m_Kind == Kind::Generic || OtherType.m_Kind == Kind::Generic)
            {
                return true;
            }

            if (m_Kind != OtherType.m_Kind)
            {
                return false;
            }

            if (m_Kind == Kind::List)
            {
                return m_ElementType->IsCompatibleWith(*OtherType.m_ElementType);
            }

            if (m_Kind == Kind::Dictionary)
            {
                return m_KeyType->IsCompatibleWith(*OtherType.m_KeyType) &&
                    m_ValueType->IsCompatibleWith(*OtherType.m_ValueType);
            }

            if (m_Kind == Kind::StructObject)
            {
                return m_StructType == OtherType.m_StructType;
            }

            return true;
        }

        [[nodiscard]] std::expected<TypeDesc, Diagnostic> Unify(const TypeDesc& OtherType) const
        {
            if (!IsValid() || !OtherType.IsValid())
            {
                return std::unexpected(MakeTypeDiagnostic(
                    "Invalid types cannot be unified."
                ));
            }

            if (*this == OtherType)
            {
                return *this;
            }

            if (m_Kind == Kind::Generic)
            {
                return OtherType.m_Kind == Kind::Generic
                    ? std::unexpected(MakeTypeDiagnostic(
                        "Different generic parameters cannot be unified without a binding context."
                    ))
                    : std::expected<TypeDesc, Diagnostic>(OtherType);
            }

            if (OtherType.m_Kind == Kind::Generic)
            {
                return *this;
            }

            if (m_Kind != OtherType.m_Kind)
            {
                return std::unexpected(MakeTypeDiagnostic(
                    "Types with different kinds cannot be unified."
                ));
            }

            if (m_Kind == Kind::List)
            {
                const auto ElementResult = m_ElementType->Unify(*OtherType.m_ElementType);
                if (!ElementResult.has_value())
                {
                    return std::unexpected(ElementResult.error());
                }
                return List(*ElementResult);
            }

            if (m_Kind == Kind::Dictionary)
            {
                const auto KeyResult = m_KeyType->Unify(*OtherType.m_KeyType);
                const auto ValueResult = m_ValueType->Unify(*OtherType.m_ValueType);
                if (!KeyResult.has_value())
                {
                    return std::unexpected(KeyResult.error());
                }
                if (!ValueResult.has_value())
                {
                    return std::unexpected(ValueResult.error());
                }
                return Dictionary(*KeyResult, *ValueResult);
            }

            return std::unexpected(MakeTypeDiagnostic(
                "Types are incompatible and cannot be unified."
            ));
        }

        auto operator<=>(const TypeDesc& OtherType) const
        {
            if (m_Kind != OtherType.m_Kind)
            {
                return m_Kind <=> OtherType.m_Kind;
            }
            if (m_GenericParameter != OtherType.m_GenericParameter)
            {
                return m_GenericParameter <=> OtherType.m_GenericParameter;
            }
            if (m_StructType != OtherType.m_StructType)
            {
                return m_StructType <=> OtherType.m_StructType;
            }
            if (m_Kind == Kind::List)
            {
                return *m_ElementType <=> *OtherType.m_ElementType;
            }
            if (m_Kind == Kind::Dictionary)
            {
                if (const auto KeyComparison = *m_KeyType <=> *OtherType.m_KeyType;
                    KeyComparison != 0)
                {
                    return KeyComparison;
                }
                return *m_ValueType <=> *OtherType.m_ValueType;
            }
            return std::strong_ordering::equal;
        }

        bool operator==(const TypeDesc& OtherType) const
        {
            return (*this <=> OtherType) == 0;
        }

    private:
        explicit TypeDesc(Kind TypeKind)
            : m_Kind(TypeKind)
        {
        }

        [[nodiscard]] static Diagnostic MakeTypeDiagnostic(const char* Message)
        {
            return {
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::IncompatibleDataPinTypes,
                .Message = Message
            };
        }

        Kind m_Kind = Kind::Invalid;
        GenericParameterId m_GenericParameter;
        StructTypeId m_StructType;
        std::shared_ptr<const TypeDesc> m_ElementType;
        std::shared_ptr<const TypeDesc> m_KeyType;
        std::shared_ptr<const TypeDesc> m_ValueType;
    };
}
