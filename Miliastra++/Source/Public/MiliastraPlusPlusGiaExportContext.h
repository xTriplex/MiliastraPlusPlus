#pragma once

#include <expected>
#include <utility>

#include "MiliastraPlusPlusDescriptorCatalogueSnapshot.h"
#include "MiliastraPlusPlusGiaBackendMapping.h"

namespace MiliastraPlusPlus
{
    namespace GiaExportContextDetail
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
    }

    class GiaExportContext final
    {
    public:
        GiaExportContext(const GiaExportContext&) = default;
        GiaExportContext(GiaExportContext&&) = default;
        GiaExportContext& operator=(const GiaExportContext&) = default;
        GiaExportContext& operator=(GiaExportContext&&) = default;

        [[nodiscard]] static std::expected<GiaExportContext, DiagnosticCollection> Create(
            DescriptorCatalogueBinding Binding,
            DescriptorCatalogueRegistryContext RegistryContext,
            GiaExportConfiguration Configuration,
            GiaBackendMappingPackage MappingPackage)
        {
            if (!RegistryContext.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    GiaExportContextDetail::MakeDiagnostic(
                        DiagnosticCode::InvalidGiaExportContext,
                        "The GIA export context contains an invalid value."
                    )
                });
            }

            const auto CatalogueCompatibility =
                ValidateDescriptorCatalogueCompatibility(
                    Binding,
                    RegistryContext.GetCatalogueIdentity()
                );
            if (!CatalogueCompatibility.has_value())
            {
                return std::unexpected(CatalogueCompatibility.error());
            }

            const GiaBackendMappingIdentity& MappingIdentity =
                MappingPackage.GetIdentity();
            if (MappingIdentity.GetCatalogueIdentity() !=
                RegistryContext.GetCatalogueIdentity())
            {
                return std::unexpected(DiagnosticCollection{
                    GiaExportContextDetail::MakeDiagnostic(
                        DiagnosticCode::IncompatibleGiaBackendMappingPackage,
                        "The GIA backend mapping package catalogue identity does not match the registry context."
                    )
                });
            }

            if (MappingIdentity.GetTargetProfile() !=
                Configuration.GetTargetProfile() ||
                MappingIdentity.GetMode() != Configuration.GetMode())
            {
                return std::unexpected(DiagnosticCollection{
                    GiaExportContextDetail::MakeDiagnostic(
                        DiagnosticCode::IncompatibleGiaBackendMappingPackage,
                        "The GIA backend mapping package target does not match the export configuration."
                    )
                });
            }

            if (MappingIdentity.GetSchemaVersion().GetValue() != 1U)
            {
                return std::unexpected(DiagnosticCollection{
                    GiaExportContextDetail::MakeDiagnostic(
                        DiagnosticCode::IncompatibleGiaBackendMappingPackage,
                        "The GIA backend mapping package schema version is unsupported by the export context."
                    )
                });
            }

            if (!Configuration.IsValid() || !MappingPackage.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    GiaExportContextDetail::MakeDiagnostic(
                        DiagnosticCode::InvalidGiaExportContext,
                        "The GIA export context contains an invalid value."
                    )
                });
            }

            return GiaExportContext(
                std::move(Binding),
                std::move(RegistryContext),
                std::move(Configuration),
                std::move(MappingPackage)
            );
        }

        [[nodiscard]] const DescriptorCatalogueBinding& GetCatalogueBinding() const noexcept
        {
            return m_Binding;
        }

        [[nodiscard]] const DescriptorCatalogueRegistryContext& GetRegistryContext() const noexcept
        {
            return m_RegistryContext;
        }

        [[nodiscard]] const GiaExportConfiguration& GetConfiguration() const noexcept
        {
            return m_Configuration;
        }

        [[nodiscard]] const GiaBackendMappingPackage& GetMappingPackage() const noexcept
        {
            return m_MappingPackage;
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_RegistryContext.IsValid() &&
                m_Configuration.IsValid() &&
                m_MappingPackage.IsValid() &&
                m_Binding.IsValid() &&
                m_Binding.GetIdentity() ==
                    m_RegistryContext.GetCatalogueIdentity() &&
                m_MappingPackage.GetIdentity().GetCatalogueIdentity() ==
                    m_RegistryContext.GetCatalogueIdentity() &&
                m_MappingPackage.GetIdentity().GetTargetProfile() ==
                    m_Configuration.GetTargetProfile() &&
                m_MappingPackage.GetIdentity().GetMode() ==
                    m_Configuration.GetMode();
        }

    private:
        GiaExportContext(DescriptorCatalogueBinding Binding,
            DescriptorCatalogueRegistryContext RegistryContext,
            GiaExportConfiguration Configuration,
            GiaBackendMappingPackage MappingPackage)
            : m_Binding(std::move(Binding))
            , m_RegistryContext(std::move(RegistryContext))
            , m_Configuration(std::move(Configuration))
            , m_MappingPackage(std::move(MappingPackage))
        {
        }

        DescriptorCatalogueBinding m_Binding;
        DescriptorCatalogueRegistryContext m_RegistryContext;
        GiaExportConfiguration m_Configuration;
        GiaBackendMappingPackage m_MappingPackage;
    };
}
