#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptors.h"

namespace MiliastraPlusPlus
{
    /// Semantic schema version for normalized descriptor catalogue content.
    /// This is intentionally separate from any future snapshot encoding version.
    class DescriptorCatalogueSemanticSchemaVersion final
    {
    public:
        constexpr DescriptorCatalogueSemanticSchemaVersion() = default;

        explicit constexpr DescriptorCatalogueSemanticSchemaVersion(std::uint32_t Value)
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

        auto operator<=>(const DescriptorCatalogueSemanticSchemaVersion&) const = default;

    private:
        std::uint32_t m_Value = 0U;
    };

    /// Opaque canonical content identity supplied by the catalogue layer.
    /// P5.1 does not derive, hash, or normalize this value.
    class DescriptorCatalogueContentIdentifier final
    {
    public:
        DescriptorCatalogueContentIdentifier() = default;

        explicit DescriptorCatalogueContentIdentifier(std::string Value)
            : m_Value(std::move(Value))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return !m_Value.empty();
        }

        [[nodiscard]] const std::string& GetValue() const
        {
            return m_Value;
        }

        auto operator<=>(const DescriptorCatalogueContentIdentifier&) const = default;

    private:
        std::string m_Value;
    };

    /// Identifies one semantic catalogue context, not its serialized snapshot encoding.
    class DescriptorCatalogueIdentity final
    {
    public:
        DescriptorCatalogueIdentity() = default;

        DescriptorCatalogueIdentity(
            std::string SourceNamespace,
            std::string SourceRevision,
            DescriptorCatalogueSemanticSchemaVersion SemanticSchemaVersion,
            DescriptorCatalogueContentIdentifier CatalogueContentIdentifier
        )
            : m_SourceNamespace(std::move(SourceNamespace))
            , m_SourceRevision(std::move(SourceRevision))
            , m_SemanticSchemaVersion(SemanticSchemaVersion)
            , m_CatalogueContentIdentifier(std::move(CatalogueContentIdentifier))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return !m_SourceNamespace.empty() &&
                !m_SourceRevision.empty() &&
                m_SemanticSchemaVersion.IsValid() &&
                m_CatalogueContentIdentifier.IsValid();
        }

        [[nodiscard]] const std::string& GetSourceNamespace() const
        {
            return m_SourceNamespace;
        }

        [[nodiscard]] const std::string& GetSourceRevision() const
        {
            return m_SourceRevision;
        }

        [[nodiscard]] const DescriptorCatalogueSemanticSchemaVersion&
            GetSemanticSchemaVersion() const
        {
            return m_SemanticSchemaVersion;
        }

        [[nodiscard]] const DescriptorCatalogueContentIdentifier&
            GetCatalogueContentIdentifier() const
        {
            return m_CatalogueContentIdentifier;
        }

        auto operator<=>(const DescriptorCatalogueIdentity&) const = default;

    private:
        std::string m_SourceNamespace;
        std::string m_SourceRevision;
        DescriptorCatalogueSemanticSchemaVersion m_SemanticSchemaVersion;
        DescriptorCatalogueContentIdentifier m_CatalogueContentIdentifier;
    };

    /// Source identity for a node definition within one catalogue context.
    /// The key is already canonical at this boundary; P5.1 applies no source policy.
    class ExternalNodeIdentity final
    {
    public:
        ExternalNodeIdentity() = default;

        explicit ExternalNodeIdentity(std::string Key)
            : m_Key(std::move(Key))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return !m_Key.empty();
        }

        [[nodiscard]] const std::string& GetKey() const
        {
            return m_Key;
        }

        auto operator<=>(const ExternalNodeIdentity&) const = default;

    private:
        std::string m_Key;
    };

    /// Carries catalogue identity for a future catalogue-aware graph envelope.
    /// Raw GraphIR v3 remains unbound and carries no such binding.
    class DescriptorCatalogueBinding final
    {
    public:
        DescriptorCatalogueBinding() = default;

        explicit DescriptorCatalogueBinding(DescriptorCatalogueIdentity Identity)
            : m_Identity(std::move(Identity))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_Identity.IsValid();
        }

        [[nodiscard]] const DescriptorCatalogueIdentity& GetIdentity() const
        {
            return m_Identity;
        }

        auto operator<=>(const DescriptorCatalogueBinding&) const = default;

    private:
        DescriptorCatalogueIdentity m_Identity;
    };

    /// Allocation input carries provenance for diagnostics without making it part of identity.
    class DescriptorIdentifierAllocationCandidate final
    {
    public:
        DescriptorIdentifierAllocationCandidate() = default;

        explicit DescriptorIdentifierAllocationCandidate(
            ExternalNodeIdentity ExternalIdentity,
            std::optional<SourceProvenance> Provenance = std::nullopt
        )
            : m_ExternalIdentity(std::move(ExternalIdentity))
            , m_SourceProvenance(std::move(Provenance))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_ExternalIdentity.IsValid() &&
                (!m_SourceProvenance.has_value() || m_SourceProvenance->IsValid());
        }

        [[nodiscard]] const ExternalNodeIdentity& GetExternalIdentity() const
        {
            return m_ExternalIdentity;
        }

        [[nodiscard]] const std::optional<SourceProvenance>& GetSourceProvenance() const
        {
            return m_SourceProvenance;
        }

        auto operator<=>(const DescriptorIdentifierAllocationCandidate&) const = default;

    private:
        ExternalNodeIdentity m_ExternalIdentity;
        std::optional<SourceProvenance> m_SourceProvenance;
    };

    class DescriptorIdentifierAssignment final
    {
    public:
        DescriptorIdentifierAssignment() = default;

        DescriptorIdentifierAssignment(
            ExternalNodeIdentity ExternalIdentity,
            NodeDescriptorId DescriptorIdentifier
        )
            : m_ExternalIdentity(std::move(ExternalIdentity))
            , m_DescriptorIdentifier(DescriptorIdentifier)
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_ExternalIdentity.IsValid() && m_DescriptorIdentifier.IsValid();
        }

        [[nodiscard]] const ExternalNodeIdentity& GetExternalIdentity() const
        {
            return m_ExternalIdentity;
        }

        [[nodiscard]] NodeDescriptorId GetDescriptorIdentifier() const
        {
            return m_DescriptorIdentifier;
        }

        auto operator<=>(const DescriptorIdentifierAssignment&) const = default;

    private:
        ExternalNodeIdentity m_ExternalIdentity;
        NodeDescriptorId m_DescriptorIdentifier;
    };

    namespace DescriptorCatalogueDetail
    {
        struct PendingDiagnostic final
        {
            std::string ExternalKey;
            std::optional<SourceProvenance> PrimarySourceProvenance;
            std::optional<SourceProvenance> RelatedSourceProvenance;
            DiagnosticCode Code;
            std::string Message;
        };

        [[nodiscard]] inline bool IsSourceProvenanceLess(
            const std::optional<SourceProvenance>& Left,
            const std::optional<SourceProvenance>& Right
        )
        {
            if (!Left.has_value() || !Right.has_value())
            {
                return !Left.has_value() && Right.has_value();
            }

            return *Left < *Right;
        }

        [[nodiscard]] inline bool IsPendingDiagnosticLess(
            const PendingDiagnostic& Left,
            const PendingDiagnostic& Right
        )
        {
            if (Left.ExternalKey != Right.ExternalKey)
            {
                return Left.ExternalKey < Right.ExternalKey;
            }

            if (Left.PrimarySourceProvenance != Right.PrimarySourceProvenance)
            {
                return IsSourceProvenanceLess(
                    Left.PrimarySourceProvenance,
                    Right.PrimarySourceProvenance
                );
            }

            if (Left.RelatedSourceProvenance != Right.RelatedSourceProvenance)
            {
                return IsSourceProvenanceLess(
                    Left.RelatedSourceProvenance,
                    Right.RelatedSourceProvenance
                );
            }

            if (Left.Code != Right.Code)
            {
                return static_cast<int>(Left.Code) < static_cast<int>(Right.Code);
            }

            return Left.Message < Right.Message;
        }

        [[nodiscard]] inline std::optional<SourceProvenance>
            GetValidSourceProvenance(const DescriptorIdentifierAllocationCandidate& Candidate)
        {
            if (Candidate.GetSourceProvenance().has_value() &&
                Candidate.GetSourceProvenance()->IsValid())
            {
                return Candidate.GetSourceProvenance();
            }

            return std::nullopt;
        }

        [[nodiscard]] inline Diagnostic CreateDiagnostic(
            DiagnosticCode Code,
            std::string Message,
            std::optional<SourceProvenance> PrimarySourceProvenance = std::nullopt,
            std::optional<SourceProvenance> RelatedSourceProvenance = std::nullopt
        )
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = std::move(Message),
                .SourceNodeIdentifier = std::nullopt,
                .DestinationNodeIdentifier = std::nullopt,
                .SourcePinReference = std::nullopt,
                .DestinationPinReference = std::nullopt,
                .PrimarySourceProvenance = std::move(PrimarySourceProvenance),
                .RelatedSourceProvenance = std::move(RelatedSourceProvenance)
            };
        }

        [[nodiscard]] inline DiagnosticCollection MaterializePendingDiagnostics(
            std::vector<PendingDiagnostic> PendingDiagnostics
        )
        {
            std::sort(
                PendingDiagnostics.begin(),
                PendingDiagnostics.end(),
                IsPendingDiagnosticLess
            );

            DiagnosticCollection Diagnostics;
            Diagnostics.reserve(PendingDiagnostics.size());
            for (PendingDiagnostic& PendingDiagnostic : PendingDiagnostics)
            {
                Diagnostics.push_back(CreateDiagnostic(
                    PendingDiagnostic.Code,
                    std::move(PendingDiagnostic.Message),
                    std::move(PendingDiagnostic.PrimarySourceProvenance),
                    std::move(PendingDiagnostic.RelatedSourceProvenance)
                ));
            }

            return Diagnostics;
        }

        [[nodiscard]] constexpr bool IsDescriptorIdentifierCountWithinDomain(
            std::size_t CandidateCount
        ) noexcept
        {
            return CandidateCount <= static_cast<std::size_t>(
                std::numeric_limits<std::uint32_t>::max());
        }
    }

    /// Allocates catalogue-local numeric identities from exact external-key order.
    /// Provenance is used only to make duplicate diagnostics deterministic.
    /// Zero is reserved and IDs start at one.
    /// Assignments are stable for a fixed canonical key set but may change when that set changes.
    class DescriptorIdentifierAllocator final
    {
    public:
        [[nodiscard]] static std::expected<
            std::vector<DescriptorIdentifierAssignment>,
            DiagnosticCollection
        > Allocate(
            const std::vector<DescriptorIdentifierAllocationCandidate>& AllocationCandidates
        )
        {
            std::vector<DescriptorIdentifierAllocationCandidate> SortedCandidates(
                AllocationCandidates.begin(),
                AllocationCandidates.end()
            );

            std::sort(
                SortedCandidates.begin(),
                SortedCandidates.end(),
                [](
                    const DescriptorIdentifierAllocationCandidate& Left,
                    const DescriptorIdentifierAllocationCandidate& Right
                )
                {
                    if (Left.GetExternalIdentity() != Right.GetExternalIdentity())
                    {
                        return Left.GetExternalIdentity() < Right.GetExternalIdentity();
                    }

                    return DescriptorCatalogueDetail::IsSourceProvenanceLess(
                        Left.GetSourceProvenance(),
                        Right.GetSourceProvenance()
                    );
                }
            );

            std::vector<DescriptorCatalogueDetail::PendingDiagnostic> PendingDiagnostics;

            for (const DescriptorIdentifierAllocationCandidate& Candidate : SortedCandidates)
            {
                if (!Candidate.GetExternalIdentity().IsValid())
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Candidate.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance =
                            DescriptorCatalogueDetail::GetValidSourceProvenance(Candidate),
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidExternalNodeIdentity,
                        .Message = "External node identity key must not be empty."
                    });
                }

                if (Candidate.GetSourceProvenance().has_value() &&
                    !Candidate.GetSourceProvenance()->IsValid())
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Candidate.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance = std::nullopt,
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidSourceProvenance,
                        .Message =
                            "Source provenance requires non-empty document and record identifiers."
                    });
                }
            }

            for (std::size_t GroupStart = 0U; GroupStart < SortedCandidates.size();)
            {
                std::size_t GroupEnd = GroupStart + 1U;
                while (GroupEnd < SortedCandidates.size() &&
                    SortedCandidates[GroupStart].GetExternalIdentity() ==
                    SortedCandidates[GroupEnd].GetExternalIdentity())
                {
                    ++GroupEnd;
                }

                for (std::size_t DuplicateIndex = GroupStart + 1U;
                    DuplicateIndex < GroupEnd;
                    ++DuplicateIndex)
                {
                    const DescriptorIdentifierAllocationCandidate& PrimaryCandidate =
                        SortedCandidates[GroupStart];
                    const DescriptorIdentifierAllocationCandidate& DuplicateCandidate =
                        SortedCandidates[DuplicateIndex];
                    PendingDiagnostics.push_back({
                        .ExternalKey = DuplicateCandidate.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance =
                            DescriptorCatalogueDetail::GetValidSourceProvenance(PrimaryCandidate),
                        .RelatedSourceProvenance =
                            DescriptorCatalogueDetail::GetValidSourceProvenance(DuplicateCandidate),
                        .Code = DiagnosticCode::DuplicateExternalNodeIdentity,
                        .Message = "External node identity key is duplicated: " +
                            DuplicateCandidate.GetExternalIdentity().GetKey()
                    });
                }

                GroupStart = GroupEnd;
            }

            if (PendingDiagnostics.empty() &&
                !DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
                    SortedCandidates.size()))
            {
                PendingDiagnostics.push_back({
                    .ExternalKey = std::string(),
                    .PrimarySourceProvenance = std::nullopt,
                    .RelatedSourceProvenance = std::nullopt,
                    .Code = DiagnosticCode::DescriptorIdentifierExhausted,
                    .Message = "Descriptor identifier domain is exhausted."
                });
            }

            if (!PendingDiagnostics.empty())
            {
                return std::unexpected(
                    DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                        std::move(PendingDiagnostics)
                    )
                );
            }

            std::vector<DescriptorIdentifierAssignment> Assignments;
            Assignments.reserve(SortedCandidates.size());

            std::uint32_t NextDescriptorIdentifier = 1U;
            for (std::size_t Index = 0U; Index < SortedCandidates.size(); ++Index)
            {
                Assignments.emplace_back(
                    SortedCandidates[Index].GetExternalIdentity(),
                    NodeDescriptorId(NextDescriptorIdentifier)
                );

                if (Index + 1U < SortedCandidates.size())
                {
                    ++NextDescriptorIdentifier;
                }
            }

            return Assignments;
        }
    };

    /// Compares a graph binding with the trusted identity selected by a later catalogue loader.
    /// Numeric descriptor IDs are deliberately outside this operation.
    [[nodiscard]] inline std::expected<void, DiagnosticCollection>
        ValidateDescriptorCatalogueCompatibility(
            const DescriptorCatalogueBinding& GraphBinding,
            const DescriptorCatalogueIdentity& AvailableCatalogueIdentity
        )
    {
        std::vector<DescriptorCatalogueDetail::PendingDiagnostic> PendingDiagnostics;

        if (!GraphBinding.IsValid())
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::InvalidDescriptorCatalogueIdentity,
                .Message = "Graph binding contains an invalid descriptor catalogue identity."
            });
        }

        if (!AvailableCatalogueIdentity.IsValid())
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::InvalidDescriptorCatalogueIdentity,
                .Message = "Available catalogue identity is invalid."
            });
        }

        if (!PendingDiagnostics.empty())
        {
            return std::unexpected(
                DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                    std::move(PendingDiagnostics)
                )
            );
        }

        if (GraphBinding.GetIdentity() != AvailableCatalogueIdentity)
        {
            DiagnosticCollection Diagnostics;
            Diagnostics.push_back(DescriptorCatalogueDetail::CreateDiagnostic(
                DiagnosticCode::DescriptorCatalogueMismatch,
                "Graph binding does not match the available descriptor catalogue identity."
            ));
            return std::unexpected(std::move(Diagnostics));
        }

        return {};
    }
}
