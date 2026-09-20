#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <utility>

namespace MiliastraPlusPlus
{
    /// Exact opaque semantic identity for an enum family.
    class EnumTypeIdentity final
    {
    public:
        EnumTypeIdentity() = default;

        explicit EnumTypeIdentity(std::string Value)
            : m_Value(std::move(Value))
        {
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return !m_Value.empty();
        }

        [[nodiscard]] const std::string& GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const EnumTypeIdentity&) const = default;

    private:
        std::string m_Value;
    };

    /// An enum value is meaningful only together with its exact enum-family identity.
    class EnumLiteralValue final
    {
    public:
        EnumLiteralValue() = default;

        explicit EnumLiteralValue(EnumTypeIdentity EnumType, std::int64_t Value)
            : m_EnumTypeIdentity(std::move(EnumType))
            , m_Value(Value)
        {
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_EnumTypeIdentity.IsValid();
        }

        [[nodiscard]] const EnumTypeIdentity& GetEnumTypeIdentity() const noexcept
        {
            return m_EnumTypeIdentity;
        }

        [[nodiscard]] std::int64_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const EnumLiteralValue&) const = default;

    private:
        EnumTypeIdentity m_EnumTypeIdentity;
        std::int64_t m_Value = 0;
    };
}
