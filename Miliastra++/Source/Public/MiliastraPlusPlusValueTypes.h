#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace MiliastraPlusPlus
{
    template<typename Tag>
    class ValueIdentifier
    {
    public:
        constexpr ValueIdentifier() = default;

        explicit constexpr ValueIdentifier(std::uint64_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const
        {
            return m_Value != 0U;
        }

        [[nodiscard]] constexpr std::uint64_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const ValueIdentifier&) const = default;

    private:
        std::uint64_t m_Value = 0U;
    };

    struct GraphVariableIdTag;
    struct NodeInstanceIdTag;

    using GraphVariableId = ValueIdentifier<GraphVariableIdTag>;
    using NodeInstanceId = ValueIdentifier<NodeInstanceIdTag>;

    class PinIndex
    {
    public:
        constexpr PinIndex() = default;

        explicit constexpr PinIndex(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const
        {
            return m_Value != InvalidValue;
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const PinIndex&) const = default;

    private:
        static constexpr std::uint32_t InvalidValue = 0xFFFFFFFFU;
        std::uint32_t m_Value = InvalidValue;
    };

    struct GuidValue
    {
        std::uint64_t Value = 0U;
        auto operator<=>(const GuidValue&) const = default;
    };

    struct Vector3Value
    {
        float X = 0.0F;
        float Y = 0.0F;
        float Z = 0.0F;
        auto operator<=>(const Vector3Value&) const = default;
    };

    struct PrefabIdValue
    {
        std::uint64_t Value = 0U;
        auto operator<=>(const PrefabIdValue&) const = default;
    };

    struct ConfigIdValue
    {
        std::uint64_t Value = 0U;
        auto operator<=>(const ConfigIdValue&) const = default;
    };

    struct FactionValue
    {
        std::uint64_t Value = 0U;
        auto operator<=>(const FactionValue&) const = default;
    };

    // Compile-time graph type token for Entity pins and variables. It carries
    // no runtime entity identity and is intentionally not a LiteralValue.
    struct EntityTypeTag
    {
    };

    class LiteralValue
    {
    public:
        using Data = std::variant<
            std::monostate,
            bool,
            std::int64_t,
            double,
            std::string,
            GuidValue,
            Vector3Value,
            PrefabIdValue,
            ConfigIdValue,
            FactionValue
        >;

        LiteralValue() = default;

        explicit LiteralValue(Data Value)
            : m_Data(std::move(Value))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return !std::holds_alternative<std::monostate>(m_Data);
        }

        template<typename Type>
        [[nodiscard]] bool Is() const
        {
            return std::holds_alternative<Type>(m_Data);
        }

        template<typename Type>
        [[nodiscard]] const Type* TryGet() const
        {
            return std::get_if<Type>(&m_Data);
        }

        [[nodiscard]] const Data& GetData() const
        {
            return m_Data;
        }

        auto operator<=>(const LiteralValue&) const = default;

    private:
        Data m_Data;
    };

    struct OutputReference
    {
        NodeInstanceId SourceNode;
        PinIndex SourceOutputPin;

        [[nodiscard]] bool IsValid() const
        {
            return SourceNode.IsValid() && SourceOutputPin.IsValid();
        }

        auto operator<=>(const OutputReference&) const = default;
    };

    struct GraphVariableReference
    {
        GraphVariableId Variable;

        [[nodiscard]] bool IsValid() const
        {
            return Variable.IsValid();
        }

        auto operator<=>(const GraphVariableReference&) const = default;
    };

    using InputBinding = std::variant<LiteralValue, OutputReference, GraphVariableReference>;
}
