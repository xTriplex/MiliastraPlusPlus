#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusIdentifiers.h"

namespace MiliastraPlusPlus
{
    enum class EPinCategory
    {
        Execution,
        Data
    };

    enum class EPinType
    {
        Flow,
        Boolean,
        Integer,
        Float,
        String,
        Entity,
        GUID,
        Vector3,
        PrefabID,
        ConfigID,
        Faction,
        Structure,
        List,
        Object,
        LocalVariable,
        Generic
    };

    enum class EPinKind
    {
        Input,
        Output
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(MiliastraPlusPlus::EPinCategory, {
        { MiliastraPlusPlus::EPinCategory::Execution, "Execution" },
        { MiliastraPlusPlus::EPinCategory::Data, "Data" }
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(MiliastraPlusPlus::EPinType, {
        { MiliastraPlusPlus::EPinType::Flow, "Flow" },
        { MiliastraPlusPlus::EPinType::Boolean, "Boolean" },
        { MiliastraPlusPlus::EPinType::Integer, "Integer" },
        { MiliastraPlusPlus::EPinType::Float, "Floating Point Numbers" },
        { MiliastraPlusPlus::EPinType::String, "String" },
        { MiliastraPlusPlus::EPinType::Entity, "Entity" },
        { MiliastraPlusPlus::EPinType::GUID, "GUID" },
        { MiliastraPlusPlus::EPinType::Vector3, "3D Vector" },
        { MiliastraPlusPlus::EPinType::PrefabID, "Prefab ID" },
        { MiliastraPlusPlus::EPinType::ConfigID, "Configuration ID" },
        { MiliastraPlusPlus::EPinType::Faction, "Faction" },
        { MiliastraPlusPlus::EPinType::Structure, "Structure" },
        { MiliastraPlusPlus::EPinType::List, "List" },
        { MiliastraPlusPlus::EPinType::Object, "Object" },
        { MiliastraPlusPlus::EPinType::LocalVariable, "Local Variable" },
        { MiliastraPlusPlus::EPinType::Generic, "Generic" }
    })

    struct PinTypeSignature
    {
        EPinType PinType;
        std::optional<EPinType> ElementType;

        auto operator<=>(const PinTypeSignature&) const = default;
    };

    class Pin
    {
    public:
        Pin(
            PinIdentifier Identifier,
            std::string Name,
            EPinCategory Category,
            EPinType Type,
            EPinKind Kind,
            std::optional<EPinType> ElementType = std::nullopt,
            std::int32_t TypeGroupIdentifier = -1
        )
            : m_Identifier(Identifier)
            , m_Name(std::move(Name))
            , m_Category(Category)
            , m_Type(Type)
            , m_Kind(Kind)
            , m_ElementType(ElementType)
            , m_TypeGroupIdentifier(TypeGroupIdentifier)
        {
        }

        virtual ~Pin() = default;

        [[nodiscard]] PinIdentifier GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] EPinCategory GetPinCategory() const
        {
            return m_Category;
        }

        [[nodiscard]] EPinType GetPinType() const
        {
            return m_Type;
        }

        [[nodiscard]] EPinKind GetPinKind() const
        {
            return m_Kind;
        }

        [[nodiscard]] std::optional<EPinType> GetElementType() const
        {
            return m_ElementType;
        }

        [[nodiscard]] std::int32_t GetTypeGroupIdentifier() const
        {
            return m_TypeGroupIdentifier;
        }

        void SetTypeGroupIdentifier(std::int32_t TypeGroupIdentifier)
        {
            m_TypeGroupIdentifier = TypeGroupIdentifier;
        }

        [[nodiscard]] bool IsGeneric() const
        {
            return m_Type == EPinType::Generic;
        }

        [[nodiscard]] bool IsExecution() const
        {
            return m_Category == EPinCategory::Execution;
        }

        [[nodiscard]] PinTypeSignature GetDeclaredTypeSignature() const
        {
            return { m_Type, m_ElementType };
        }

        [[nodiscard]] PinTypeSignature GetEffectiveTypeSignature() const
        {
            if (IsGeneric() && m_ResolvedType.has_value())
            {
                return { m_ResolvedType.value(), m_ResolvedElementType };
            }

            return GetDeclaredTypeSignature();
        }

        [[nodiscard]] EPinType GetEffectiveType() const
        {
            return GetEffectiveTypeSignature().PinType;
        }

        void ResolveType(PinTypeSignature TypeSignature)
        {
            if (IsGeneric())
            {
                m_ResolvedType = TypeSignature.PinType;
                m_ResolvedElementType = TypeSignature.ElementType;
            }
        }

        void ClearResolvedType()
        {
            m_ResolvedType.reset();
            m_ResolvedElementType.reset();
        }

        [[nodiscard]] bool IsCompatibleWith(const Pin& OtherPin) const
        {
            const PinTypeSignature ThisTypeSignature = GetEffectiveTypeSignature();
            const PinTypeSignature OtherTypeSignature = OtherPin.GetEffectiveTypeSignature();

            if (
                ThisTypeSignature.PinType == EPinType::Generic ||
                OtherTypeSignature.PinType == EPinType::Generic
            )
            {
                return true;
            }

            return ThisTypeSignature == OtherTypeSignature;
        }

        [[nodiscard]] nlohmann::json Serialize() const
        {
            const PinTypeSignature EffectiveTypeSignature = GetEffectiveTypeSignature();
            nlohmann::json SerializedPin = {
                { "Id", m_Identifier.GetValue() },
                { "Name", m_Name },
                { "Pin Category", m_Category },
                { "Pin Type", EffectiveTypeSignature.PinType },
                { "Pin Kind", m_Kind }
            };

            if (EffectiveTypeSignature.ElementType.has_value())
            {
                SerializedPin["Element Type"] = EffectiveTypeSignature.ElementType.value();
            }

            return SerializedPin;
        }

    private:
        PinIdentifier m_Identifier;
        std::string m_Name;
        EPinCategory m_Category;
        EPinType m_Type;
        EPinKind m_Kind;
        std::optional<EPinType> m_ElementType;
        std::int32_t m_TypeGroupIdentifier;
        std::optional<EPinType> m_ResolvedType;
        std::optional<EPinType> m_ResolvedElementType;
    };
}
