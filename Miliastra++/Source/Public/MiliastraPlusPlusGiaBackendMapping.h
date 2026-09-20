#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptorCatalogue.h"
#include "MiliastraPlusPlusGiaExportConfiguration.h"

namespace MiliastraPlusPlus
{
    class GiaBackendMappingSchemaVersion final
    {
    public:
        constexpr GiaBackendMappingSchemaVersion() = default;

        explicit constexpr GiaBackendMappingSchemaVersion(std::uint32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value != 0U;
        }

        [[nodiscard]] constexpr std::uint32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaBackendMappingSchemaVersion&) const = default;

    private:
        std::uint32_t m_Value = 0U;
    };

    class GiaBackendMappingIdentity final
    {
    public:
        explicit GiaBackendMappingIdentity(
            DescriptorCatalogueIdentity CatalogueIdentity,
            GiaBackendMappingSchemaVersion SchemaVersion,
            GiaExportTargetProfile TargetProfile,
            GiaExportMode Mode
        )
            : m_CatalogueIdentity(std::move(CatalogueIdentity))
            , m_SchemaVersion(SchemaVersion)
            , m_TargetProfile(TargetProfile)
            , m_Mode(Mode)
        {
        }

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] const DescriptorCatalogueIdentity&
            GetCatalogueIdentity() const noexcept
        {
            return m_CatalogueIdentity;
        }

        [[nodiscard]] GiaBackendMappingSchemaVersion
            GetSchemaVersion() const noexcept
        {
            return m_SchemaVersion;
        }

        [[nodiscard]] GiaExportTargetProfile
            GetTargetProfile() const noexcept
        {
            return m_TargetProfile;
        }

        [[nodiscard]] GiaExportMode GetMode() const noexcept
        {
            return m_Mode;
        }

        auto operator<=>(const GiaBackendMappingIdentity&) const = default;

    private:
        DescriptorCatalogueIdentity m_CatalogueIdentity;
        GiaBackendMappingSchemaVersion m_SchemaVersion;
        GiaExportTargetProfile m_TargetProfile;
        GiaExportMode m_Mode;
    };

    class GiaNodeGenericId final
    {
    public:
        constexpr GiaNodeGenericId() = default;

        explicit constexpr GiaNodeGenericId(std::int32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value > 0;
        }

        [[nodiscard]] constexpr std::int32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaNodeGenericId&) const = default;

    private:
        std::int32_t m_Value = 0;
    };

    /// Zero is a valid present backend identifier; absence is represented by std::optional.
    class GiaNodeConcreteId final
    {
    public:
        explicit constexpr GiaNodeConcreteId(std::int32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value >= 0;
        }

        [[nodiscard]] constexpr std::int32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaNodeConcreteId&) const = default;

    private:
        std::int32_t m_Value;
    };

    /// This backend index is distinct from the canonical semantic PinIndex.
    class GiaPinIndex final
    {
    public:
        constexpr GiaPinIndex() = default;

        explicit constexpr GiaPinIndex(std::int32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value >= 0;
        }

        [[nodiscard]] constexpr std::int32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaPinIndex&) const = default;

    private:
        std::int32_t m_Value = -1;
    };

    enum class GiaPinKind
    {
        Unknown = 0,
        InputFlow = 1,
        OutputFlow = 2,
        InputParameter = 3,
        OutputParameter = 4,
        ClientExecution = 5,
        ClientSignal = 6
    };

    enum class GiaPinEmissionPolicy
    {
        Emit,
        Omit
    };

    enum class GiaLiteralEncodingKind
    {
        None,
        Boolean,
        Enum
    };

    class GiaBackendTypeCode final
    {
    public:
        constexpr GiaBackendTypeCode() = default;

        explicit constexpr GiaBackendTypeCode(std::int32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value > 0;
        }

        [[nodiscard]] constexpr std::int32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaBackendTypeCode&) const = default;

    private:
        std::int32_t m_Value = 0;
    };

    class GiaBackendPinMapping final
    {
    public:
        explicit GiaBackendPinMapping(
            PinIndex SemanticPinIndex,
            GiaPinKind PinKind,
            GiaPinIndex BackendIndex,
            std::optional<GiaPinIndex> SecondaryIndex,
            GiaBackendTypeCode BackendTypeCode,
            GiaLiteralEncodingKind LiteralEncoding,
            GiaPinEmissionPolicy EmissionPolicy,
            bool IsConnectable
        )
            : m_SemanticPinIndex(SemanticPinIndex)
            , m_PinKind(PinKind)
            , m_BackendIndex(BackendIndex)
            , m_SecondaryIndex(std::move(SecondaryIndex))
            , m_BackendTypeCode(BackendTypeCode)
            , m_LiteralEncoding(LiteralEncoding)
            , m_EmissionPolicy(EmissionPolicy)
            , m_IsConnectable(IsConnectable)
        {
        }

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] PinIndex GetSemanticPinIndex() const noexcept
        {
            return m_SemanticPinIndex;
        }

        [[nodiscard]] GiaPinKind GetPinKind() const noexcept
        {
            return m_PinKind;
        }

        [[nodiscard]] GiaPinIndex GetBackendIndex() const noexcept
        {
            return m_BackendIndex;
        }

        [[nodiscard]] const std::optional<GiaPinIndex>&
            GetSecondaryIndex() const noexcept
        {
            return m_SecondaryIndex;
        }

        [[nodiscard]] GiaBackendTypeCode GetBackendTypeCode() const noexcept
        {
            return m_BackendTypeCode;
        }

        [[nodiscard]] GiaLiteralEncodingKind
            GetLiteralEncoding() const noexcept
        {
            return m_LiteralEncoding;
        }

        [[nodiscard]] GiaPinEmissionPolicy GetEmissionPolicy() const noexcept
        {
            return m_EmissionPolicy;
        }

        [[nodiscard]] bool IsConnectable() const noexcept
        {
            return m_IsConnectable;
        }

        bool operator==(const GiaBackendPinMapping&) const = default;

    private:
        PinIndex m_SemanticPinIndex;
        GiaPinKind m_PinKind;
        GiaPinIndex m_BackendIndex;
        std::optional<GiaPinIndex> m_SecondaryIndex;
        GiaBackendTypeCode m_BackendTypeCode;
        GiaLiteralEncodingKind m_LiteralEncoding;
        GiaPinEmissionPolicy m_EmissionPolicy;
        bool m_IsConnectable;
    };

    /// ExternalNodeIdentity is an opaque exact lookup key and is never parsed here.
    class GiaBackendNodeMapping final
    {
    public:
        explicit GiaBackendNodeMapping(
            ExternalNodeIdentity ExternalIdentity,
            GiaNodeGenericId GenericNodeIdentifier,
            std::optional<GiaNodeConcreteId> ConcreteNodeIdentifier,
            std::vector<GiaBackendPinMapping> PinMappings,
            std::optional<SourceProvenance> Provenance = std::nullopt
        )
            : m_ExternalIdentity(std::move(ExternalIdentity))
            , m_GenericNodeIdentifier(GenericNodeIdentifier)
            , m_ConcreteNodeIdentifier(std::move(ConcreteNodeIdentifier))
            , m_PinMappings(std::move(PinMappings))
            , m_SourceProvenance(std::move(Provenance))
        {
        }

        [[nodiscard]] bool IsValid() const noexcept;

        [[nodiscard]] const ExternalNodeIdentity&
            GetExternalIdentity() const noexcept
        {
            return m_ExternalIdentity;
        }

        [[nodiscard]] GiaNodeGenericId
            GetGenericNodeIdentifier() const noexcept
        {
            return m_GenericNodeIdentifier;
        }

        [[nodiscard]] const std::optional<GiaNodeConcreteId>&
            GetConcreteNodeIdentifier() const noexcept
        {
            return m_ConcreteNodeIdentifier;
        }

        [[nodiscard]] const std::vector<GiaBackendPinMapping>&
            GetPinMappings() const noexcept
        {
            return m_PinMappings;
        }

        [[nodiscard]] const std::optional<SourceProvenance>&
            GetSourceProvenance() const noexcept
        {
            return m_SourceProvenance;
        }

        bool operator==(const GiaBackendNodeMapping& Other) const
        {
            return m_ExternalIdentity == Other.m_ExternalIdentity &&
                m_GenericNodeIdentifier == Other.m_GenericNodeIdentifier &&
                m_ConcreteNodeIdentifier == Other.m_ConcreteNodeIdentifier &&
                m_PinMappings == Other.m_PinMappings;
        }

    private:
        ExternalNodeIdentity m_ExternalIdentity;
        GiaNodeGenericId m_GenericNodeIdentifier;
        std::optional<GiaNodeConcreteId> m_ConcreteNodeIdentifier;
        std::vector<GiaBackendPinMapping> m_PinMappings;
        std::optional<SourceProvenance> m_SourceProvenance;
    };

    class GiaBackendMappingPackage final
    {
    public:
        GiaBackendMappingPackage(const GiaBackendMappingPackage&) = default;
        GiaBackendMappingPackage(GiaBackendMappingPackage&&) = default;
        GiaBackendMappingPackage& operator=(const GiaBackendMappingPackage&) = default;
        GiaBackendMappingPackage& operator=(GiaBackendMappingPackage&&) = default;

        [[nodiscard]] static std::expected<
            GiaBackendMappingPackage,
            DiagnosticCollection
        > Create(
            GiaBackendMappingIdentity Identity,
            std::vector<GiaBackendNodeMapping> NodeMappings
        );

        [[nodiscard]] const GiaBackendMappingIdentity&
            GetIdentity() const noexcept
        {
            return m_Identity;
        }

        [[nodiscard]] std::span<const GiaBackendNodeMapping>
            GetNodeMappings() const noexcept
        {
            return m_NodeMappings;
        }

        [[nodiscard]] std::size_t GetNodeMappingCount() const noexcept
        {
            return m_NodeMappings.size();
        }

        [[nodiscard]] const GiaBackendNodeMapping* FindByExternalIdentity(
            const ExternalNodeIdentity& Identity
        ) const noexcept;

        [[nodiscard]] bool IsValid() const noexcept;

        bool operator==(const GiaBackendMappingPackage& Other) const
        {
            return m_Identity == Other.m_Identity &&
                m_NodeMappings == Other.m_NodeMappings;
        }

    private:
        GiaBackendMappingPackage(
            GiaBackendMappingIdentity Identity,
            std::vector<GiaBackendNodeMapping> NodeMappings
        )
            : m_Identity(std::move(Identity))
            , m_NodeMappings(std::move(NodeMappings))
        {
        }

        GiaBackendMappingIdentity m_Identity;
        std::vector<GiaBackendNodeMapping> m_NodeMappings;
    };

    namespace GiaBackendMappingDetail
    {
        [[nodiscard]] inline Diagnostic MakeDiagnostic(
            DiagnosticCode Code,
            std::string Message,
            std::optional<std::string> ExternalIdentityKey = std::nullopt,
            std::optional<SourceProvenance> Provenance = std::nullopt
        )
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = std::move(Message),
                .PrimarySourceProvenance = std::move(Provenance),
                .ExternalIdentityKey = std::move(ExternalIdentityKey)
            };
        }

        [[nodiscard]] constexpr bool IsSupportedTargetProfile(
            GiaExportTargetProfile Profile
        )
        {
            return Profile == GiaExportTargetProfile::ClientBooleanFilter;
        }

        [[nodiscard]] constexpr bool IsSupportedMode(GiaExportMode Mode)
        {
            return Mode == GiaExportMode::Beyond;
        }

        [[nodiscard]] constexpr bool IsValidPinKind(GiaPinKind Kind)
        {
            switch (Kind)
            {
            case GiaPinKind::InputFlow:
            case GiaPinKind::OutputFlow:
            case GiaPinKind::InputParameter:
            case GiaPinKind::OutputParameter:
            case GiaPinKind::ClientExecution:
            case GiaPinKind::ClientSignal:
                return true;
            case GiaPinKind::Unknown:
                return false;
            }
            return false;
        }

        [[nodiscard]] constexpr bool IsValidEmissionPolicy(
            GiaPinEmissionPolicy Policy
        )
        {
            switch (Policy)
            {
            case GiaPinEmissionPolicy::Emit:
            case GiaPinEmissionPolicy::Omit:
                return true;
            }
            return false;
        }

        [[nodiscard]] constexpr bool IsValidLiteralEncoding(
            GiaLiteralEncodingKind Encoding
        )
        {
            switch (Encoding)
            {
            case GiaLiteralEncodingKind::None:
            case GiaLiteralEncodingKind::Boolean:
            case GiaLiteralEncodingKind::Enum:
                return true;
            }
            return false;
        }

        [[nodiscard]] inline std::optional<std::string> GetIdentityKey(
            const ExternalNodeIdentity& Identity
        )
        {
            if (!Identity.IsValid())
            {
                return std::nullopt;
            }
            return Identity.GetKey();
        }

        [[nodiscard]] inline bool ComparePinMappings(
            const GiaBackendPinMapping& Left,
            const GiaBackendPinMapping& Right
        )
        {
            return std::tuple{
                Left.GetSemanticPinIndex().GetValue(),
                static_cast<int>(Left.GetPinKind()),
                Left.GetBackendIndex().GetValue(),
                Left.GetSecondaryIndex(),
                Left.GetBackendTypeCode().GetValue(),
                static_cast<int>(Left.GetLiteralEncoding()),
                static_cast<int>(Left.GetEmissionPolicy()),
                Left.IsConnectable()
            } < std::tuple{
                Right.GetSemanticPinIndex().GetValue(),
                static_cast<int>(Right.GetPinKind()),
                Right.GetBackendIndex().GetValue(),
                Right.GetSecondaryIndex(),
                Right.GetBackendTypeCode().GetValue(),
                static_cast<int>(Right.GetLiteralEncoding()),
                static_cast<int>(Right.GetEmissionPolicy()),
                Right.IsConnectable()
            };
        }

        [[nodiscard]] inline bool CompareNodeMappings(
            const GiaBackendNodeMapping& Left,
            const GiaBackendNodeMapping& Right
        )
        {
            if (Left.GetExternalIdentity().GetKey() !=
                Right.GetExternalIdentity().GetKey())
            {
                return Left.GetExternalIdentity().GetKey() <
                    Right.GetExternalIdentity().GetKey();
            }

            if (Left.GetGenericNodeIdentifier() !=
                Right.GetGenericNodeIdentifier())
            {
                return Left.GetGenericNodeIdentifier() <
                    Right.GetGenericNodeIdentifier();
            }

            if (Left.GetConcreteNodeIdentifier() !=
                Right.GetConcreteNodeIdentifier())
            {
                return Left.GetConcreteNodeIdentifier() <
                    Right.GetConcreteNodeIdentifier();
            }

            if (Left.GetPinMappings() != Right.GetPinMappings())
            {
                return std::lexicographical_compare(
                    Left.GetPinMappings().begin(),
                    Left.GetPinMappings().end(),
                    Right.GetPinMappings().begin(),
                    Right.GetPinMappings().end(),
                    ComparePinMappings
                );
            }

            return Left.GetSourceProvenance() < Right.GetSourceProvenance();
        }

        [[nodiscard]] inline std::string DescribePinMapping(
            const GiaBackendPinMapping& PinMapping
        )
        {
            return "semantic pin " +
                std::to_string(PinMapping.GetSemanticPinIndex().GetValue()) +
                ", backend kind " +
                std::to_string(static_cast<int>(PinMapping.GetPinKind())) +
                ", backend index " +
                std::to_string(PinMapping.GetBackendIndex().GetValue());
        }

        [[nodiscard]] inline bool HasDuplicateSemanticPin(
            const std::vector<GiaBackendPinMapping>& PinMappings,
            std::size_t Index
        )
        {
            const PinIndex Current = PinMappings[Index].GetSemanticPinIndex();
            for (std::size_t PriorIndex = 0U; PriorIndex < Index; ++PriorIndex)
            {
                if (PinMappings[PriorIndex].GetSemanticPinIndex() == Current)
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] inline bool HasDuplicateBackendCoordinate(
            const std::vector<GiaBackendPinMapping>& PinMappings,
            std::size_t Index
        )
        {
            const GiaBackendPinMapping& Current = PinMappings[Index];
            if (Current.GetEmissionPolicy() != GiaPinEmissionPolicy::Emit)
            {
                return false;
            }

            for (std::size_t PriorIndex = 0U; PriorIndex < Index; ++PriorIndex)
            {
                const GiaBackendPinMapping& Prior = PinMappings[PriorIndex];
                if (Prior.GetEmissionPolicy() == GiaPinEmissionPolicy::Emit &&
                    Prior.GetPinKind() == Current.GetPinKind() &&
                    Prior.GetBackendIndex() == Current.GetBackendIndex())
                {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]] constexpr int GetDiagnosticStage(DiagnosticCode Code)
        {
            switch (Code)
            {
            case DiagnosticCode::InvalidGiaBackendMappingPackage:
            case DiagnosticCode::UnsupportedGiaBackendMappingSchemaVersion:
            case DiagnosticCode::UnsupportedGiaExportTarget:
                return 0;
            case DiagnosticCode::InvalidExternalNodeIdentity:
            case DiagnosticCode::DuplicateExternalNodeIdentity:
            case DiagnosticCode::InvalidGiaBackendNodeMapping:
            case DiagnosticCode::InvalidSourceProvenance:
                return 1;
            case DiagnosticCode::InvalidGiaBackendPinMapping:
            case DiagnosticCode::DuplicateGiaBackendPinMapping:
                return 2;
            default:
                return 3;
            }
        }

        [[nodiscard]] inline std::string GetDiagnosticExternalKey(
            const Diagnostic& DiagnosticValue
        )
        {
            return DiagnosticValue.ExternalIdentityKey.value_or(std::string{});
        }

        [[nodiscard]] inline bool CompareDiagnostics(
            const Diagnostic& Left,
            const Diagnostic& Right
        )
        {
            const auto LeftKey = std::tuple{
                GetDiagnosticStage(Left.Code),
                static_cast<int>(Left.Code),
                GetDiagnosticExternalKey(Left)
            };
            const auto RightKey = std::tuple{
                GetDiagnosticStage(Right.Code),
                static_cast<int>(Right.Code),
                GetDiagnosticExternalKey(Right)
            };
            return LeftKey < RightKey;
        }
    }

    inline bool GiaBackendMappingIdentity::IsValid() const noexcept
    {
        return m_CatalogueIdentity.IsValid() &&
            m_SchemaVersion.IsValid() &&
            GiaBackendMappingDetail::IsSupportedTargetProfile(m_TargetProfile) &&
            GiaBackendMappingDetail::IsSupportedMode(m_Mode);
    }

    inline bool GiaBackendPinMapping::IsValid() const noexcept
    {
        if (!m_SemanticPinIndex.IsValid() ||
            !m_BackendIndex.IsValid() ||
            !m_BackendTypeCode.IsValid() ||
            !GiaBackendMappingDetail::IsValidPinKind(m_PinKind) ||
            !GiaBackendMappingDetail::IsValidEmissionPolicy(m_EmissionPolicy) ||
            !GiaBackendMappingDetail::IsValidLiteralEncoding(m_LiteralEncoding))
        {
            return false;
        }

        if (m_SecondaryIndex.has_value() && !m_SecondaryIndex->IsValid())
        {
            return false;
        }

        const bool IsFlowKind =
            m_PinKind == GiaPinKind::InputFlow ||
            m_PinKind == GiaPinKind::OutputFlow ||
            m_PinKind == GiaPinKind::ClientExecution ||
            m_PinKind == GiaPinKind::ClientSignal;
        if (IsFlowKind && m_LiteralEncoding != GiaLiteralEncodingKind::None)
        {
            return false;
        }

        if (m_EmissionPolicy == GiaPinEmissionPolicy::Omit &&
            (m_IsConnectable || m_LiteralEncoding != GiaLiteralEncodingKind::None))
        {
            return false;
        }

        return true;
    }

    inline bool GiaBackendNodeMapping::IsValid() const noexcept
    {
        if (!m_ExternalIdentity.IsValid() ||
            !m_GenericNodeIdentifier.IsValid() ||
            (m_ConcreteNodeIdentifier.has_value() &&
                !m_ConcreteNodeIdentifier->IsValid()) ||
            (m_SourceProvenance.has_value() &&
                !m_SourceProvenance->IsValid()))
        {
            return false;
        }

        return std::all_of(
            m_PinMappings.begin(),
            m_PinMappings.end(),
            [](const GiaBackendPinMapping& Mapping)
            {
                return Mapping.IsValid();
            }
        );
    }

    inline std::expected<GiaBackendMappingPackage, DiagnosticCollection>
        GiaBackendMappingPackage::Create(
            GiaBackendMappingIdentity Identity,
            std::vector<GiaBackendNodeMapping> NodeMappings
        )
    {
        DiagnosticCollection Diagnostics;

        if (!Identity.GetCatalogueIdentity().IsValid())
        {
            Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                DiagnosticCode::InvalidGiaBackendMappingPackage,
                "The GIA backend mapping package has an invalid catalogue identity."
            ));
        }

        if (Identity.GetSchemaVersion().GetValue() != 1U)
        {
            Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                DiagnosticCode::UnsupportedGiaBackendMappingSchemaVersion,
                "The GIA backend mapping schema version is unsupported."
            ));
        }

        if (!GiaBackendMappingDetail::IsSupportedTargetProfile(
                Identity.GetTargetProfile()
            ) ||
            !GiaBackendMappingDetail::IsSupportedMode(Identity.GetMode()))
        {
            Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                DiagnosticCode::UnsupportedGiaExportTarget,
                "The GIA backend mapping package target or mode is unsupported."
            ));
        }

        std::vector<GiaBackendNodeMapping> CanonicalNodeMappings;
        CanonicalNodeMappings.reserve(NodeMappings.size());
        for (const GiaBackendNodeMapping& NodeMapping : NodeMappings)
        {
            std::vector<GiaBackendPinMapping> CanonicalPinMappings =
                NodeMapping.GetPinMappings();
            std::sort(
                CanonicalPinMappings.begin(),
                CanonicalPinMappings.end(),
                GiaBackendMappingDetail::ComparePinMappings
            );
            CanonicalNodeMappings.emplace_back(
                NodeMapping.GetExternalIdentity(),
                NodeMapping.GetGenericNodeIdentifier(),
                NodeMapping.GetConcreteNodeIdentifier(),
                std::move(CanonicalPinMappings),
                NodeMapping.GetSourceProvenance()
            );
        }

        std::sort(
            CanonicalNodeMappings.begin(),
            CanonicalNodeMappings.end(),
            GiaBackendMappingDetail::CompareNodeMappings
        );

        for (std::size_t Index = 0U;
            Index < CanonicalNodeMappings.size();
            ++Index)
        {
            const GiaBackendNodeMapping& NodeMapping =
                CanonicalNodeMappings[Index];
            const std::optional<std::string> ExternalIdentityKey =
                GiaBackendMappingDetail::GetIdentityKey(
                    NodeMapping.GetExternalIdentity()
                );

            if (!NodeMapping.GetExternalIdentity().IsValid())
            {
                Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidExternalNodeIdentity,
                    "A GIA backend mapping has an invalid external node identity.",
                    std::nullopt,
                    NodeMapping.GetSourceProvenance()
                ));
            }
            else if (Index > 0U &&
                CanonicalNodeMappings[Index - 1U].GetExternalIdentity() ==
                    NodeMapping.GetExternalIdentity())
            {
                Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                    DiagnosticCode::DuplicateExternalNodeIdentity,
                    "A GIA backend mapping package contains a duplicate external node identity.",
                    ExternalIdentityKey,
                    NodeMapping.GetSourceProvenance()
                ));
            }

            if (!NodeMapping.GetGenericNodeIdentifier().IsValid() ||
                (NodeMapping.GetConcreteNodeIdentifier().has_value() &&
                    !NodeMapping.GetConcreteNodeIdentifier()->IsValid()))
            {
                Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidGiaBackendNodeMapping,
                    "A GIA backend node mapping has an invalid backend node identifier.",
                    ExternalIdentityKey,
                    NodeMapping.GetSourceProvenance()
                ));
            }

            if (NodeMapping.GetSourceProvenance().has_value() &&
                !NodeMapping.GetSourceProvenance()->IsValid())
            {
                Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                    DiagnosticCode::InvalidSourceProvenance,
                    "A GIA backend mapping has invalid source provenance.",
                    ExternalIdentityKey,
                    NodeMapping.GetSourceProvenance()
                ));
            }

            const auto& PinMappings = NodeMapping.GetPinMappings();
            for (std::size_t PinIndex = 0U;
                PinIndex < PinMappings.size();
                ++PinIndex)
            {
                const GiaBackendPinMapping& PinMapping = PinMappings[PinIndex];
                if (!PinMapping.IsValid())
                {
                    Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                        DiagnosticCode::InvalidGiaBackendPinMapping,
                        "A GIA backend node mapping has an invalid " +
                            GiaBackendMappingDetail::DescribePinMapping(
                                PinMapping
                            ) +
                            ".",
                        ExternalIdentityKey,
                        NodeMapping.GetSourceProvenance()
                    ));
                }

                if (GiaBackendMappingDetail::HasDuplicateSemanticPin(
                        PinMappings,
                        PinIndex
                    ) ||
                    GiaBackendMappingDetail::HasDuplicateBackendCoordinate(
                        PinMappings,
                        PinIndex
                    ))
                {
                    Diagnostics.push_back(GiaBackendMappingDetail::MakeDiagnostic(
                        DiagnosticCode::DuplicateGiaBackendPinMapping,
                        "A GIA backend node mapping contains a duplicate " +
                            GiaBackendMappingDetail::DescribePinMapping(
                                PinMapping
                            ) +
                            ".",
                        ExternalIdentityKey,
                        NodeMapping.GetSourceProvenance()
                    ));
                }
            }
        }

        std::stable_sort(
            Diagnostics.begin(),
            Diagnostics.end(),
            GiaBackendMappingDetail::CompareDiagnostics
        );

        if (!Diagnostics.empty())
        {
            return std::unexpected(std::move(Diagnostics));
        }

        return GiaBackendMappingPackage(
            std::move(Identity),
            std::move(CanonicalNodeMappings)
        );
    }

    inline const GiaBackendNodeMapping*
        GiaBackendMappingPackage::FindByExternalIdentity(
            const ExternalNodeIdentity& Identity
        ) const noexcept
    {
        const auto Iterator = std::lower_bound(
            m_NodeMappings.begin(),
            m_NodeMappings.end(),
            Identity.GetKey(),
            [](const GiaBackendNodeMapping& Mapping, const std::string& Key)
            {
                return Mapping.GetExternalIdentity().GetKey() < Key;
            }
        );
        if (Iterator == m_NodeMappings.end() ||
            Iterator->GetExternalIdentity() != Identity)
        {
            return nullptr;
        }
        return &*Iterator;
    }

    inline bool GiaBackendMappingPackage::IsValid() const noexcept
    {
        if (!m_Identity.IsValid() ||
            m_Identity.GetSchemaVersion().GetValue() != 1U)
        {
            return false;
        }

        for (std::size_t Index = 0U;
            Index < m_NodeMappings.size();
            ++Index)
        {
            const GiaBackendNodeMapping& NodeMapping = m_NodeMappings[Index];
            if (!NodeMapping.IsValid())
            {
                return false;
            }
            if (Index > 0U &&
                m_NodeMappings[Index - 1U].GetExternalIdentity() >=
                    NodeMapping.GetExternalIdentity())
            {
                return false;
            }

            const auto& PinMappings = NodeMapping.GetPinMappings();
            for (std::size_t PinIndex = 0U;
                PinIndex < PinMappings.size();
                ++PinIndex)
            {
                if (PinIndex > 0U &&
                    !GiaBackendMappingDetail::ComparePinMappings(
                        PinMappings[PinIndex - 1U],
                        PinMappings[PinIndex]
                    ))
                {
                    return false;
                }
                if (GiaBackendMappingDetail::HasDuplicateSemanticPin(
                        PinMappings,
                        PinIndex
                    ) ||
                    GiaBackendMappingDetail::HasDuplicateBackendCoordinate(
                        PinMappings,
                        PinIndex
                    ))
                {
                    return false;
                }
            }
        }

        return true;
    }
}
