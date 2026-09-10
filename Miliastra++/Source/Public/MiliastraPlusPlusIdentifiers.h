#pragma once

#include <compare>
#include <cstdint>
#include <functional>

namespace MiliastraPlusPlus
{
    class GraphIdentifier
    {
    public:
        explicit constexpr GraphIdentifier(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const GraphIdentifier&) const = default;

    private:
        std::uint32_t m_Value;
    };

    class NodeIdentifier
    {
    public:
        explicit constexpr NodeIdentifier(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const NodeIdentifier&) const = default;

    private:
        std::uint32_t m_Value;
    };

    class PinIdentifier
    {
    public:
        explicit constexpr PinIdentifier(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const PinIdentifier&) const = default;

    private:
        std::uint32_t m_Value;
    };

    class LinkIdentifier
    {
    public:
        explicit constexpr LinkIdentifier(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const LinkIdentifier&) const = default;

    private:
        std::uint32_t m_Value;
    };

    struct PinReference
    {
        NodeIdentifier OwningNodeIdentifier;
        PinIdentifier LocalPinIdentifier;

        auto operator<=>(const PinReference&) const = default;
    };
}

template<>
struct std::hash<MiliastraPlusPlus::NodeIdentifier>
{
    std::size_t operator()(const MiliastraPlusPlus::NodeIdentifier& NodeIdentifier) const noexcept
    {
        return std::hash<std::uint32_t>{}(NodeIdentifier.GetValue());
    }
};

template<>
struct std::hash<MiliastraPlusPlus::PinReference>
{
    std::size_t operator()(const MiliastraPlusPlus::PinReference& PinReference) const noexcept
    {
        const std::size_t NodeHash = std::hash<MiliastraPlusPlus::NodeIdentifier>{}(
            PinReference.OwningNodeIdentifier
        );
        const std::size_t PinHash = std::hash<std::uint32_t>{}(
            PinReference.LocalPinIdentifier.GetValue()
        );
        return NodeHash ^ (PinHash + 0x9e3779b9U + (NodeHash << 6U) + (NodeHash >> 2U));
    }
};
