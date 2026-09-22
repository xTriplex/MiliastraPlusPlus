#pragma once

#include <cmath>
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <utility>

#include "MiliastraPlusPlusDiagnostics.h"

namespace MiliastraPlusPlus
{
    enum class GiaExportTargetProfile
    {
        ClientBooleanFilter
    };

    enum class GiaExportMode
    {
        Beyond
    };

    namespace GiaExportConfigurationDetail
    {
        [[nodiscard]] inline Diagnostic MakeDiagnostic(DiagnosticCode Code,
            std::string Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = std::move(Message)
            };
        }

        [[nodiscard]] constexpr bool IsSupportedTargetProfile(GiaExportTargetProfile Profile)
        {
            switch (Profile)
            {
            case GiaExportTargetProfile::ClientBooleanFilter:
                return true;
            }
            return false;
        }

        [[nodiscard]] constexpr bool IsSupportedMode(GiaExportMode Mode)
        {
            switch (Mode)
            {
            case GiaExportMode::Beyond:
                return true;
            }
            return false;
        }
    }

    class GiaGraphIdentifier final
    {
    public:
        constexpr GiaGraphIdentifier() = default;

        explicit constexpr GiaGraphIdentifier(std::int64_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value > 0;
        }

        [[nodiscard]] constexpr std::int64_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaGraphIdentifier&) const = default;

    private:
        std::int64_t m_Value = 0;
    };

    class GiaUniqueIdentifier final
    {
    public:
        constexpr GiaUniqueIdentifier() = default;

        explicit constexpr GiaUniqueIdentifier(std::int64_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value > 0;
        }

        [[nodiscard]] constexpr std::int64_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaUniqueIdentifier&) const = default;

    private:
        std::int64_t m_Value = 0;
    };

    class GiaExportConfiguration final
    {
    public:
        GiaExportConfiguration(const GiaExportConfiguration&) = default;
        GiaExportConfiguration(GiaExportConfiguration&&) = default;
        GiaExportConfiguration& operator=(const GiaExportConfiguration&) = default;
        GiaExportConfiguration& operator=(GiaExportConfiguration&&) = default;

        [[nodiscard]] static std::expected<GiaExportConfiguration, DiagnosticCollection> Create(
            GiaExportTargetProfile TargetProfile,
            GiaExportMode Mode,
            GiaGraphIdentifier GraphIdentifier,
            std::string GraphName,
            GiaUniqueIdentifier UniqueIdentifier,
            std::optional<double> EvaluationInterval = std::nullopt)
        {
            DiagnosticCollection Diagnostics;

            if (!GiaExportConfigurationDetail::IsSupportedTargetProfile(TargetProfile) ||
                !GiaExportConfigurationDetail::IsSupportedMode(Mode))
            {
                Diagnostics.push_back(GiaExportConfigurationDetail::MakeDiagnostic(
                    DiagnosticCode::UnsupportedGiaExportTarget,
                    "The requested GIA export target or mode is unsupported."
                ));
            }

            if (!GraphIdentifier.IsValid())
            {
                Diagnostics.push_back(GiaExportConfigurationDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidGiaExportConfiguration,
                    "The GIA graph identifier must be greater than zero."
                ));
            }

            if (GraphName.empty())
            {
                Diagnostics.push_back(GiaExportConfigurationDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidGiaExportConfiguration,
                    "The GIA graph name must not be empty."
                ));
            }

            if (!UniqueIdentifier.IsValid())
            {
                Diagnostics.push_back(GiaExportConfigurationDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidGiaExportConfiguration,
                    "The GIA unique identifier must be greater than zero."
                ));
            }

            const double ResolvedEvaluationInterval =
                EvaluationInterval.value_or(0.3);
            if (!std::isfinite(ResolvedEvaluationInterval) ||
                ResolvedEvaluationInterval < 0.0)
            {
                Diagnostics.push_back(GiaExportConfigurationDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidGiaExportConfiguration,
                    "The GIA evaluation interval must be finite and non-negative."
                ));
            }

            if (!Diagnostics.empty())
            {
                return std::unexpected(std::move(Diagnostics));
            }

            return GiaExportConfiguration(
                TargetProfile,
                Mode,
                GraphIdentifier,
                std::move(GraphName),
                UniqueIdentifier,
                ResolvedEvaluationInterval
            );
        }

        [[nodiscard]] GiaExportTargetProfile GetTargetProfile() const noexcept
        {
            return m_TargetProfile;
        }

        [[nodiscard]] GiaExportMode GetMode() const noexcept
        {
            return m_Mode;
        }

        [[nodiscard]] GiaGraphIdentifier GetGraphIdentifier() const noexcept
        {
            return m_GraphIdentifier;
        }

        [[nodiscard]] const std::string& GetGraphName() const noexcept
        {
            return m_GraphName;
        }

        [[nodiscard]] GiaUniqueIdentifier GetUniqueIdentifier() const noexcept
        {
            return m_UniqueIdentifier;
        }

        [[nodiscard]] double GetEvaluationInterval() const noexcept
        {
            return m_EvaluationInterval;
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return GiaExportConfigurationDetail::IsSupportedTargetProfile(
                       m_TargetProfile
                   ) &&
                GiaExportConfigurationDetail::IsSupportedMode(m_Mode) &&
                m_GraphIdentifier.IsValid() &&
                !m_GraphName.empty() &&
                m_UniqueIdentifier.IsValid() &&
                std::isfinite(m_EvaluationInterval) &&
                m_EvaluationInterval >= 0.0;
        }

        bool operator==(const GiaExportConfiguration&) const = default;

    private:
        GiaExportConfiguration(GiaExportTargetProfile TargetProfile,
            GiaExportMode Mode,
            GiaGraphIdentifier GraphIdentifier,
            std::string GraphName,
            GiaUniqueIdentifier UniqueIdentifier,
            double EvaluationInterval)
            : m_TargetProfile(TargetProfile)
            , m_Mode(Mode)
            , m_GraphIdentifier(GraphIdentifier)
            , m_GraphName(std::move(GraphName))
            , m_UniqueIdentifier(UniqueIdentifier)
            , m_EvaluationInterval(EvaluationInterval)
        {
        }

        GiaExportTargetProfile m_TargetProfile;
        GiaExportMode m_Mode;
        GiaGraphIdentifier m_GraphIdentifier;
        std::string m_GraphName;
        GiaUniqueIdentifier m_UniqueIdentifier;
        double m_EvaluationInterval;
    };
}
