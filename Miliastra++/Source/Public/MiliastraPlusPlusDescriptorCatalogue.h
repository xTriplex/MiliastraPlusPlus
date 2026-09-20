#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
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

    /// Owns one source-independent pin definition before catalogue allocation.
    /// Pin order remains semantic in its containing normalized record.
    class NormalizedPinRecord final
    {
    public:
        NormalizedPinRecord();

        NormalizedPinRecord(
            std::string Name,
            TypeDesc Type,
            PinDirection Direction,
            PinCategory Category,
            PinCardinality Cardinality = PinCardinality::Single,
            bool AllowsLiteral = false,
            std::optional<LiteralValue> DefaultValue = std::nullopt
        );

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const std::string& GetName() const;
        [[nodiscard]] const TypeDesc& GetType() const;
        [[nodiscard]] PinDirection GetDirection() const;
        [[nodiscard]] PinCategory GetCategory() const;
        [[nodiscard]] PinCardinality GetCardinality() const;
        [[nodiscard]] bool AllowsLiteral() const;
        [[nodiscard]] const std::optional<LiteralValue>& GetDefaultValue() const;

        bool operator==(const NormalizedPinRecord& Other) const;

    private:
        std::string m_Name;
        TypeDesc m_Type;
        PinDirection m_Direction;
        PinCategory m_Category;
        PinCardinality m_Cardinality;
        bool m_AllowsLiteral = false;
        std::optional<LiteralValue> m_DefaultValue;
    };

    /// Owns one source-independent descriptor definition without a numeric ID.
    /// Its external identity is scoped by the catalogue built around it.
    class NormalizedNodeDescriptorRecord final
    {
    public:
        NormalizedNodeDescriptorRecord();

        NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity ExternalIdentity,
            std::string DisplayName,
            std::vector<NodeAvailability> Availability,
            std::vector<NormalizedPinRecord> Pins,
            std::optional<ExecutionControlSchema> ControlSchema = std::nullopt,
            std::optional<SourceProvenance> Provenance = std::nullopt
        );

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const ExternalNodeIdentity& GetExternalIdentity() const;
        [[nodiscard]] const std::string& GetDisplayName() const;
        [[nodiscard]] const std::vector<NodeAvailability>& GetAvailability() const;
        [[nodiscard]] const std::vector<NormalizedPinRecord>& GetPins() const;
        [[nodiscard]] const std::optional<ExecutionControlSchema>&
            GetExecutionControlSchema() const;
        [[nodiscard]] const std::optional<SourceProvenance>&
            GetSourceProvenance() const;

        [[nodiscard]] bool operator==(
            const NormalizedNodeDescriptorRecord& Other
        ) const;

    private:
        ExternalNodeIdentity m_ExternalIdentity;
        std::string m_DisplayName;
        std::vector<NodeAvailability> m_Availability;
        std::vector<NormalizedPinRecord> m_Pins;
        std::optional<ExecutionControlSchema> m_ControlSchema;
        std::optional<SourceProvenance> m_SourceProvenance;
    };

    /// Couples one validated normalized record to its catalogue-local ID.
    class DescriptorCatalogueEntry final
    {
    public:
        DescriptorCatalogueEntry();

        DescriptorCatalogueEntry(
            NormalizedNodeDescriptorRecord Record,
            NodeDescriptorId DescriptorIdentifier
        );

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const NormalizedNodeDescriptorRecord& GetRecord() const;
        [[nodiscard]] const ExternalNodeIdentity& GetExternalIdentity() const;
        [[nodiscard]] const std::optional<SourceProvenance>&
            GetSourceProvenance() const;
        [[nodiscard]] NodeDescriptorId GetDescriptorIdentifier() const;

        [[nodiscard]] bool operator==(
            const DescriptorCatalogueEntry& Other
        ) const;

    private:
        NormalizedNodeDescriptorRecord m_Record;
        NodeDescriptorId m_DescriptorIdentifier;
    };

    class DescriptorCatalogueBuilder;

    /// Immutable validated semantic catalogue ordered by exact external identity.
    class DescriptorCatalogue final
    {
    public:
        DescriptorCatalogue();

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const DescriptorCatalogueIdentity& GetIdentity() const;
        [[nodiscard]] const std::vector<DescriptorCatalogueEntry>& GetEntries() const;
        [[nodiscard]] const DescriptorCatalogueEntry* FindByExternalIdentity(
            const ExternalNodeIdentity& ExternalIdentity
        ) const;
        [[nodiscard]] const DescriptorCatalogueEntry* FindByDescriptorIdentifier(
            NodeDescriptorId DescriptorIdentifier
        ) const;
        [[nodiscard]] std::size_t GetEntryCount() const;

        [[nodiscard]] bool operator==(const DescriptorCatalogue& Other) const;

    private:
        DescriptorCatalogue(
            DescriptorCatalogueIdentity Identity,
            std::vector<DescriptorCatalogueEntry> Entries
        );

        friend class DescriptorCatalogueBuilder;

        DescriptorCatalogueIdentity m_Identity;
        std::vector<DescriptorCatalogueEntry> m_Entries;
    };

    /// Stateless factory for complete immutable semantic catalogues.
    class DescriptorCatalogueBuilder final
    {
    public:
        DescriptorCatalogueBuilder() = delete;

        [[nodiscard]] static std::expected<
            DescriptorCatalogue,
            DiagnosticCollection
        > Build(
            std::string SourceNamespace,
            std::string SourceRevision,
            DescriptorCatalogueSemanticSchemaVersion SemanticSchemaVersion,
            std::vector<NormalizedNodeDescriptorRecord> Records
        );
    };

    inline constexpr DescriptorCatalogueSemanticSchemaVersion
        CurrentDescriptorCatalogueSemanticSchemaVersion{2U};

    namespace DescriptorCatalogueDetail
    {
        [[nodiscard]] inline std::vector<NodeAvailability> CanonicalizeAvailability(
            const std::vector<NodeAvailability>& Availability
        )
        {
            std::vector<NodeAvailability> CanonicalAvailability;
            CanonicalAvailability.reserve(Availability.size());

            for (const NodeAvailability Value : Availability)
            {
                if (Value == NodeAvailability::Server)
                {
                    CanonicalAvailability.push_back(Value);
                }
            }
            for (const NodeAvailability Value : Availability)
            {
                if (Value == NodeAvailability::Client)
                {
                    CanonicalAvailability.push_back(Value);
                }
            }
            for (const NodeAvailability Value : Availability)
            {
                if (Value != NodeAvailability::Server &&
                    Value != NodeAvailability::Client)
                {
                    CanonicalAvailability.push_back(Value);
                }
            }

            return CanonicalAvailability;
        }

        [[nodiscard]] inline bool AreNormalizedLiteralValuesEqual(
            const LiteralValue& Left,
            const LiteralValue& Right
        )
        {
            if (Left.GetData().index() != Right.GetData().index())
            {
                return false;
            }

            if (Left.Is<std::monostate>())
            {
                return true;
            }
            if (Left.Is<bool>())
            {
                return *Left.TryGet<bool>() == *Right.TryGet<bool>();
            }
            if (Left.Is<std::int64_t>())
            {
                return *Left.TryGet<std::int64_t>() == *Right.TryGet<std::int64_t>();
            }
            if (Left.Is<double>())
            {
                return std::bit_cast<std::uint64_t>(*Left.TryGet<double>()) ==
                    std::bit_cast<std::uint64_t>(*Right.TryGet<double>());
            }
            if (Left.Is<std::string>())
            {
                return *Left.TryGet<std::string>() == *Right.TryGet<std::string>();
            }
            if (Left.Is<GuidValue>())
            {
                return *Left.TryGet<GuidValue>() == *Right.TryGet<GuidValue>();
            }
            if (Left.Is<Vector3Value>())
            {
                const Vector3Value& LeftValue = *Left.TryGet<Vector3Value>();
                const Vector3Value& RightValue = *Right.TryGet<Vector3Value>();
                return std::bit_cast<std::uint32_t>(LeftValue.X) ==
                        std::bit_cast<std::uint32_t>(RightValue.X) &&
                    std::bit_cast<std::uint32_t>(LeftValue.Y) ==
                        std::bit_cast<std::uint32_t>(RightValue.Y) &&
                    std::bit_cast<std::uint32_t>(LeftValue.Z) ==
                        std::bit_cast<std::uint32_t>(RightValue.Z);
            }
            if (Left.Is<PrefabIdValue>())
            {
                return *Left.TryGet<PrefabIdValue>() == *Right.TryGet<PrefabIdValue>();
            }
            if (Left.Is<ConfigIdValue>())
            {
                return *Left.TryGet<ConfigIdValue>() == *Right.TryGet<ConfigIdValue>();
            }
            if (Left.Is<FactionValue>())
            {
                return *Left.TryGet<FactionValue>() == *Right.TryGet<FactionValue>();
            }
            if (Left.Is<EnumLiteralValue>())
            {
                return *Left.TryGet<EnumLiteralValue>() ==
                    *Right.TryGet<EnumLiteralValue>();
            }

            return false;
        }

        [[nodiscard]] inline bool AreExecutionControlSchemasEqual(
            const ExecutionControlSchema& Left,
            const ExecutionControlSchema& Right
        )
        {
            return std::visit(
                [](
                    const auto& LeftControl,
                    const auto& RightControl
                ) -> bool
                {
                    using LeftType = std::decay_t<decltype(LeftControl)>;
                    using RightType = std::decay_t<decltype(RightControl)>;
                    if constexpr (!std::is_same_v<LeftType, RightType>)
                    {
                        return false;
                    }
                    else if constexpr (std::is_same_v<LeftType, EntryControlSchema>)
                    {
                        return LeftControl.ExecutionOutput == RightControl.ExecutionOutput;
                    }
                    else if constexpr (std::is_same_v<LeftType, SequenceControlSchema>)
                    {
                        return LeftControl.ExecutionInput == RightControl.ExecutionInput &&
                            LeftControl.ExecutionOutput == RightControl.ExecutionOutput;
                    }
                    else if constexpr (std::is_same_v<LeftType, BranchControlSchema>)
                    {
                        return LeftControl.ExecutionInput == RightControl.ExecutionInput &&
                            LeftControl.ConditionInput == RightControl.ConditionInput &&
                            LeftControl.TrueOutput == RightControl.TrueOutput &&
                            LeftControl.FalseOutput == RightControl.FalseOutput;
                    }
                    else if constexpr (std::is_same_v<LeftType, JoinControlSchema>)
                    {
                        return LeftControl.ExecutionInput == RightControl.ExecutionInput &&
                            LeftControl.ExecutionOutput == RightControl.ExecutionOutput;
                    }
                    else if constexpr (std::is_same_v<LeftType, LoopControlSchema>)
                    {
                        return LeftControl.ExecutionInput == RightControl.ExecutionInput &&
                            LeftControl.BodyOutput == RightControl.BodyOutput &&
                            LeftControl.ExitOutput == RightControl.ExitOutput &&
                            LeftControl.RepeatInput == RightControl.RepeatInput &&
                            LeftControl.BreakInput == RightControl.BreakInput &&
                            LeftControl.ExitPolicy == RightControl.ExitPolicy &&
                            LeftControl.ConditionInput == RightControl.ConditionInput;
                    }
                    else
                    {
                        return LeftControl.ExecutionInput == RightControl.ExecutionInput;
                    }
                },
                Left,
                Right
            );
        }

        [[nodiscard]] inline bool IsNormalizedPinSemanticallyValid(
            const NormalizedPinRecord& Pin
        )
        {
            std::vector<PinSchema> Pins;
            Pins.emplace_back(
                Pin.GetName(),
                Pin.GetType(),
                Pin.GetDirection(),
                Pin.GetCategory(),
                Pin.GetCardinality(),
                Pin.AllowsLiteral(),
                Pin.GetDefaultValue()
            );

            const NodeDescriptor Descriptor(
                NodeDescriptorId(1U),
                "NormalizedPin",
                {},
                std::move(Pins)
            );
            return Descriptor.IsValid();
        }

        [[nodiscard]] inline bool IsNormalizedRecordSemanticPayloadValid(
            const NormalizedNodeDescriptorRecord& Record
        )
        {
            std::vector<PinSchema> Pins;
            Pins.reserve(Record.GetPins().size());
            for (const NormalizedPinRecord& Pin : Record.GetPins())
            {
                Pins.emplace_back(
                    Pin.GetName(),
                    Pin.GetType(),
                    Pin.GetDirection(),
                    Pin.GetCategory(),
                    Pin.GetCardinality(),
                    Pin.AllowsLiteral(),
                    Pin.GetDefaultValue()
                );
            }

            const NodeDescriptor Descriptor(
                NodeDescriptorId(1U),
                Record.GetDisplayName(),
                Record.GetAvailability(),
                std::move(Pins),
                Record.GetExecutionControlSchema()
            );
            return Descriptor.IsValid();
        }

        [[nodiscard]] inline bool ContainsEnumType(const TypeDesc& Type)
        {
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Enum:
                return true;
            case TypeDesc::Kind::List:
                return Type.GetElementType() != nullptr &&
                    ContainsEnumType(*Type.GetElementType());
            case TypeDesc::Kind::Dictionary:
                return Type.GetKeyType() != nullptr &&
                    ContainsEnumType(*Type.GetKeyType()) ||
                    Type.GetValueType() != nullptr &&
                    ContainsEnumType(*Type.GetValueType());
            default:
                return false;
            }
        }

        [[nodiscard]] inline bool RecordContainsEnum(
            const NormalizedNodeDescriptorRecord& Record
        )
        {
            for (const NormalizedPinRecord& Pin : Record.GetPins())
            {
                if (ContainsEnumType(Pin.GetType()) ||
                    (Pin.GetDefaultValue().has_value() &&
                        Pin.GetDefaultValue()->Is<EnumLiteralValue>()))
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] inline std::optional<SourceProvenance>
            GetValidSourceProvenance(const NormalizedNodeDescriptorRecord& Record)
        {
            if (Record.GetSourceProvenance().has_value() &&
                Record.GetSourceProvenance()->IsValid())
            {
                return Record.GetSourceProvenance();
            }

            return std::nullopt;
        }

        [[nodiscard]] inline std::vector<PendingDiagnostic>
            CollectNormalizedRecordDiagnostics(
                const std::vector<NormalizedNodeDescriptorRecord>& Records
            )
        {
            std::vector<PendingDiagnostic> PendingDiagnostics;

            for (const NormalizedNodeDescriptorRecord& Record : Records)
            {
                const std::optional<SourceProvenance> ValidProvenance =
                    GetValidSourceProvenance(Record);

                if (!Record.GetExternalIdentity().IsValid())
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Record.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance = ValidProvenance,
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidExternalNodeIdentity,
                        .Message = "External node identity key must not be empty."
                    });
                }

                if (Record.GetSourceProvenance().has_value() &&
                    !Record.GetSourceProvenance()->IsValid())
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Record.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance = std::nullopt,
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidSourceProvenance,
                        .Message =
                            "Source provenance requires non-empty document and record identifiers."
                    });
                }

                if (!IsNormalizedRecordSemanticPayloadValid(Record))
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Record.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance = ValidProvenance,
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidNormalizedDescriptorRecord,
                        .Message = "Normalized descriptor record is invalid."
                    });
                }
            }

            std::vector<std::size_t> SortedRecordIndexes;
            SortedRecordIndexes.reserve(Records.size());
            for (std::size_t Index = 0U; Index < Records.size(); ++Index)
            {
                SortedRecordIndexes.push_back(Index);
            }

            std::sort(
                SortedRecordIndexes.begin(),
                SortedRecordIndexes.end(),
                [&Records](std::size_t LeftIndex, std::size_t RightIndex)
                {
                    const NormalizedNodeDescriptorRecord& Left = Records[LeftIndex];
                    const NormalizedNodeDescriptorRecord& Right = Records[RightIndex];
                    if (Left.GetExternalIdentity() != Right.GetExternalIdentity())
                    {
                        return Left.GetExternalIdentity() < Right.GetExternalIdentity();
                    }

                    return IsSourceProvenanceLess(
                        Left.GetSourceProvenance(),
                        Right.GetSourceProvenance()
                    );
                }
            );

            for (std::size_t GroupStart = 0U; GroupStart < SortedRecordIndexes.size();)
            {
                std::size_t GroupEnd = GroupStart + 1U;
                while (GroupEnd < SortedRecordIndexes.size() &&
                    Records[SortedRecordIndexes[GroupStart]].GetExternalIdentity() ==
                    Records[SortedRecordIndexes[GroupEnd]].GetExternalIdentity())
                {
                    ++GroupEnd;
                }

                for (std::size_t DuplicateIndex = GroupStart + 1U;
                    DuplicateIndex < GroupEnd;
                    ++DuplicateIndex)
                {
                    const NormalizedNodeDescriptorRecord& PrimaryRecord =
                        Records[SortedRecordIndexes[GroupStart]];
                    const NormalizedNodeDescriptorRecord& DuplicateRecord =
                        Records[SortedRecordIndexes[DuplicateIndex]];
                    PendingDiagnostics.push_back({
                        .ExternalKey = DuplicateRecord.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance = GetValidSourceProvenance(PrimaryRecord),
                        .RelatedSourceProvenance = GetValidSourceProvenance(DuplicateRecord),
                        .Code = DiagnosticCode::DuplicateExternalNodeIdentity,
                        .Message = "External node identity key is duplicated: " +
                            DuplicateRecord.GetExternalIdentity().GetKey()
                    });
                }

                GroupStart = GroupEnd;
            }

            return PendingDiagnostics;
        }

        [[nodiscard]] inline std::vector<NormalizedNodeDescriptorRecord>
            GetCanonicalRecords(
                const std::vector<NormalizedNodeDescriptorRecord>& Records
            )
        {
            std::vector<NormalizedNodeDescriptorRecord> CanonicalRecords(
                Records.begin(),
                Records.end()
            );
            std::sort(
                CanonicalRecords.begin(),
                CanonicalRecords.end(),
                [](const NormalizedNodeDescriptorRecord& Left,
                    const NormalizedNodeDescriptorRecord& Right)
                {
                    return Left.GetExternalIdentity() < Right.GetExternalIdentity();
                }
            );
            return CanonicalRecords;
        }

        class CanonicalContentEncoder final
        {
        public:
            void AppendByte(std::uint8_t Value)
            {
                m_Bytes.push_back(Value);
            }

            void AppendUnsigned32(std::uint32_t Value)
            {
                m_Bytes.push_back(static_cast<std::uint8_t>((Value >> 24U) & 0xFFU));
                m_Bytes.push_back(static_cast<std::uint8_t>((Value >> 16U) & 0xFFU));
                m_Bytes.push_back(static_cast<std::uint8_t>((Value >> 8U) & 0xFFU));
                m_Bytes.push_back(static_cast<std::uint8_t>(Value & 0xFFU));
            }

            void AppendUnsigned64(std::uint64_t Value)
            {
                for (std::uint32_t Shift = 56U;; Shift -= 8U)
                {
                    m_Bytes.push_back(static_cast<std::uint8_t>((Value >> Shift) & 0xFFU));
                    if (Shift == 0U)
                    {
                        break;
                    }
                }
            }

            void AppendString(const std::string& Value)
            {
                AppendUnsigned64(static_cast<std::uint64_t>(Value.size()));
                for (const char Character : Value)
                {
                    m_Bytes.push_back(static_cast<std::uint8_t>(
                        static_cast<unsigned char>(Character)));
                }
            }

            void AppendTypeDescription(const TypeDesc& Type)
            {
                switch (Type.GetKind())
                {
                case TypeDesc::Kind::Boolean:
                    AppendByte(0x01U);
                    break;
                case TypeDesc::Kind::Integer:
                    AppendByte(0x02U);
                    break;
                case TypeDesc::Kind::Float:
                    AppendByte(0x03U);
                    break;
                case TypeDesc::Kind::String:
                    AppendByte(0x04U);
                    break;
                case TypeDesc::Kind::Flow:
                    AppendByte(0x05U);
                    break;
                case TypeDesc::Kind::Entity:
                    AppendByte(0x06U);
                    break;
                case TypeDesc::Kind::GUID:
                    AppendByte(0x07U);
                    break;
                case TypeDesc::Kind::Vector3:
                    AppendByte(0x08U);
                    break;
                case TypeDesc::Kind::PrefabId:
                    AppendByte(0x09U);
                    break;
                case TypeDesc::Kind::ConfigId:
                    AppendByte(0x0AU);
                    break;
                case TypeDesc::Kind::Faction:
                    AppendByte(0x0BU);
                    break;
                case TypeDesc::Kind::Generic:
                    AppendByte(0x0CU);
                    AppendUnsigned32(Type.GetGenericParameter().GetValue());
                    break;
                case TypeDesc::Kind::List:
                    AppendByte(0x0DU);
                    AppendTypeDescription(*Type.GetElementType());
                    break;
                case TypeDesc::Kind::Dictionary:
                    AppendByte(0x0EU);
                    AppendTypeDescription(*Type.GetKeyType());
                    AppendTypeDescription(*Type.GetValueType());
                    break;
                case TypeDesc::Kind::StructObject:
                    AppendByte(0x0FU);
                    AppendUnsigned32(Type.GetStructType().GetValue());
                    break;
                case TypeDesc::Kind::Enum:
                    AppendByte(0x10U);
                    AppendString(Type.GetEnumTypeIdentity().GetValue());
                    break;
                case TypeDesc::Kind::Invalid:
                    AppendByte(0x00U);
                    break;
                }
            }

            void AppendLiteralValue(const LiteralValue& Literal)
            {
                if (Literal.Is<std::monostate>())
                {
                    AppendByte(0x00U);
                }
                else if (Literal.Is<bool>())
                {
                    AppendByte(0x01U);
                    AppendByte(*Literal.TryGet<bool>() ? 0x01U : 0x00U);
                }
                else if (Literal.Is<std::int64_t>())
                {
                    AppendByte(0x02U);
                    const std::int64_t Value = *Literal.TryGet<std::int64_t>();
                    if (Value < 0)
                    {
                        AppendByte(0x01U);
                        const std::uint64_t Magnitude =
                            static_cast<std::uint64_t>(-(Value + 1)) + 1U;
                        AppendUnsigned64(Magnitude);
                    }
                    else
                    {
                        AppendByte(0x00U);
                        AppendUnsigned64(static_cast<std::uint64_t>(Value));
                    }
                }
                else if (Literal.Is<double>())
                {
                    AppendByte(0x03U);
                    AppendUnsigned64(std::bit_cast<std::uint64_t>(
                        *Literal.TryGet<double>()));
                }
                else if (Literal.Is<std::string>())
                {
                    AppendByte(0x04U);
                    AppendString(*Literal.TryGet<std::string>());
                }
                else if (Literal.Is<GuidValue>())
                {
                    AppendByte(0x05U);
                    AppendUnsigned64(Literal.TryGet<GuidValue>()->Value);
                }
                else if (Literal.Is<Vector3Value>())
                {
                    AppendByte(0x06U);
                    const Vector3Value& Value = *Literal.TryGet<Vector3Value>();
                    AppendUnsigned32(std::bit_cast<std::uint32_t>(Value.X));
                    AppendUnsigned32(std::bit_cast<std::uint32_t>(Value.Y));
                    AppendUnsigned32(std::bit_cast<std::uint32_t>(Value.Z));
                }
                else if (Literal.Is<PrefabIdValue>())
                {
                    AppendByte(0x07U);
                    AppendUnsigned64(Literal.TryGet<PrefabIdValue>()->Value);
                }
                else if (Literal.Is<ConfigIdValue>())
                {
                    AppendByte(0x08U);
                    AppendUnsigned64(Literal.TryGet<ConfigIdValue>()->Value);
                }
                else if (Literal.Is<FactionValue>())
                {
                    AppendByte(0x09U);
                    AppendUnsigned64(Literal.TryGet<FactionValue>()->Value);
                }
                else if (Literal.Is<EnumLiteralValue>())
                {
                    AppendByte(0x0AU);
                    const EnumLiteralValue& Value =
                        *Literal.TryGet<EnumLiteralValue>();
                    AppendString(Value.GetEnumTypeIdentity().GetValue());
                    const std::int64_t Number = Value.GetValue();
                    if (Number < 0)
                    {
                        AppendByte(0x01U);
                        const std::uint64_t Magnitude =
                            static_cast<std::uint64_t>(-(Number + 1)) + 1U;
                        AppendUnsigned64(Magnitude);
                    }
                    else
                    {
                        AppendByte(0x00U);
                        AppendUnsigned64(static_cast<std::uint64_t>(Number));
                    }
                }
            }

            void AppendControlSchema(
                const std::optional<ExecutionControlSchema>& ControlSchema
            )
            {
                if (!ControlSchema.has_value())
                {
                    AppendByte(0x00U);
                    return;
                }

                AppendByte(0x01U);
                std::visit(
                    [this](const auto& Control)
                    {
                        using ControlType = std::decay_t<decltype(Control)>;
                        if constexpr (std::is_same_v<ControlType, EntryControlSchema>)
                        {
                            AppendByte(0x01U);
                            AppendUnsigned32(Control.ExecutionOutput.GetValue());
                        }
                        else if constexpr (std::is_same_v<ControlType, SequenceControlSchema>)
                        {
                            AppendByte(0x02U);
                            AppendUnsigned32(Control.ExecutionInput.GetValue());
                            AppendUnsigned32(Control.ExecutionOutput.GetValue());
                        }
                        else if constexpr (std::is_same_v<ControlType, BranchControlSchema>)
                        {
                            AppendByte(0x03U);
                            AppendUnsigned32(Control.ExecutionInput.GetValue());
                            AppendUnsigned32(Control.ConditionInput.GetValue());
                            AppendUnsigned32(Control.TrueOutput.GetValue());
                            AppendUnsigned32(Control.FalseOutput.GetValue());
                        }
                        else if constexpr (std::is_same_v<ControlType, JoinControlSchema>)
                        {
                            AppendByte(0x04U);
                            AppendUnsigned32(Control.ExecutionInput.GetValue());
                            AppendUnsigned32(Control.ExecutionOutput.GetValue());
                        }
                        else if constexpr (std::is_same_v<ControlType, LoopControlSchema>)
                        {
                            AppendByte(0x05U);
                            AppendUnsigned32(Control.ExecutionInput.GetValue());
                            AppendUnsigned32(Control.BodyOutput.GetValue());
                            AppendUnsigned32(Control.ExitOutput.GetValue());
                            AppendUnsigned32(Control.RepeatInput.GetValue());
                            AppendUnsigned32(Control.BreakInput.GetValue());
                            if (Control.ExitPolicy == LoopExitPolicy::Conditional)
                            {
                                AppendByte(0x01U);
                            }
                            else
                            {
                                AppendByte(0x02U);
                            }
                            if (Control.ConditionInput.has_value())
                            {
                                AppendByte(0x01U);
                                AppendUnsigned32(Control.ConditionInput->GetValue());
                            }
                            else
                            {
                                AppendByte(0x00U);
                            }
                        }
                        else
                        {
                            AppendByte(0x06U);
                            AppendUnsigned32(Control.ExecutionInput.GetValue());
                        }
                    },
                    *ControlSchema
                );
            }

            [[nodiscard]] std::vector<std::uint8_t> TakeBytes() &&
            {
                return std::move(m_Bytes);
            }

        private:
            std::vector<std::uint8_t> m_Bytes;
        };

        [[nodiscard]] inline std::vector<std::uint8_t> EncodeCanonicalContent(
            const std::vector<NormalizedNodeDescriptorRecord>& Records
        )
        {
            const std::string Domain = "MiliastraPlusPlus.DescriptorCatalogue.Content.v1";
            CanonicalContentEncoder Encoder;
            for (const char Character : Domain)
            {
                Encoder.AppendByte(static_cast<std::uint8_t>(
                    static_cast<unsigned char>(Character)));
            }

            const std::vector<NormalizedNodeDescriptorRecord> CanonicalRecords =
                GetCanonicalRecords(Records);
            Encoder.AppendUnsigned64(static_cast<std::uint64_t>(CanonicalRecords.size()));
            for (const NormalizedNodeDescriptorRecord& Record : CanonicalRecords)
            {
                Encoder.AppendString(Record.GetExternalIdentity().GetKey());
                Encoder.AppendUnsigned64(static_cast<std::uint64_t>(
                    Record.GetAvailability().size()));
                for (const NodeAvailability Availability : Record.GetAvailability())
                {
                    Encoder.AppendByte(
                        Availability == NodeAvailability::Server ? 0x01U : 0x02U);
                }

                Encoder.AppendUnsigned64(static_cast<std::uint64_t>(Record.GetPins().size()));
                for (const NormalizedPinRecord& Pin : Record.GetPins())
                {
                    Encoder.AppendString(Pin.GetName());
                    Encoder.AppendTypeDescription(Pin.GetType());
                    Encoder.AppendByte(
                        Pin.GetDirection() == PinDirection::Input ? 0x01U : 0x02U);
                    Encoder.AppendByte(
                        Pin.GetCategory() == PinCategory::Data ? 0x01U : 0x02U);
                    switch (Pin.GetCardinality())
                    {
                    case PinCardinality::Single:
                        Encoder.AppendByte(0x01U);
                        break;
                    case PinCardinality::Optional:
                        Encoder.AppendByte(0x02U);
                        break;
                    case PinCardinality::Multiple:
                        Encoder.AppendByte(0x03U);
                        break;
                    }
                    Encoder.AppendByte(Pin.AllowsLiteral() ? 0x01U : 0x00U);
                    if (Pin.GetDefaultValue().has_value())
                    {
                        Encoder.AppendByte(0x01U);
                        Encoder.AppendLiteralValue(*Pin.GetDefaultValue());
                    }
                    else
                    {
                        Encoder.AppendByte(0x00U);
                    }
                }

                Encoder.AppendControlSchema(Record.GetExecutionControlSchema());
            }

            return std::move(Encoder).TakeBytes();
        }

        [[nodiscard]] inline constexpr std::uint32_t RotateRight(
            std::uint32_t Value,
            std::uint32_t Shift
        ) noexcept
        {
            return (Value >> Shift) | (Value << (32U - Shift));
        }

        [[nodiscard]] inline std::array<std::uint8_t, 32U> ComputeSha256(
            const std::vector<std::uint8_t>& Bytes
        )
        {
            static constexpr std::array<std::uint32_t, 64U> RoundConstants = {
                0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
                0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
                0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
                0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
                0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
                0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
                0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
                0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
                0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
                0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
                0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
                0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
                0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
                0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
                0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
                0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U
            };

            std::vector<std::uint8_t> PaddedBytes(Bytes.begin(), Bytes.end());
            PaddedBytes.push_back(0x80U);
            while ((PaddedBytes.size() + 8U) % 64U != 0U)
            {
                PaddedBytes.push_back(0x00U);
            }

            const std::uint64_t BitLength = static_cast<std::uint64_t>(Bytes.size()) * 8U;
            for (std::uint32_t Shift = 56U;; Shift -= 8U)
            {
                PaddedBytes.push_back(static_cast<std::uint8_t>(
                    (BitLength >> Shift) & 0xFFU));
                if (Shift == 0U)
                {
                    break;
                }
            }

            std::array<std::uint32_t, 8U> Hash = {
                0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
                0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U
            };

            for (std::size_t BlockOffset = 0U;
                BlockOffset < PaddedBytes.size();
                BlockOffset += 64U)
            {
                std::array<std::uint32_t, 64U> Schedule{};
                for (std::size_t Word = 0U; Word < 16U; ++Word)
                {
                    const std::size_t Offset = BlockOffset + Word * 4U;
                    Schedule[Word] =
                        (static_cast<std::uint32_t>(PaddedBytes[Offset]) << 24U) |
                        (static_cast<std::uint32_t>(PaddedBytes[Offset + 1U]) << 16U) |
                        (static_cast<std::uint32_t>(PaddedBytes[Offset + 2U]) << 8U) |
                        static_cast<std::uint32_t>(PaddedBytes[Offset + 3U]);
                }
                for (std::size_t Word = 16U; Word < 64U; ++Word)
                {
                    const std::uint32_t First = Schedule[Word - 15U];
                    const std::uint32_t Second = Schedule[Word - 2U];
                    const std::uint32_t SmallSigmaZero =
                        RotateRight(First, 7U) ^ RotateRight(First, 18U) ^
                        (First >> 3U);
                    const std::uint32_t SmallSigmaOne =
                        RotateRight(Second, 17U) ^ RotateRight(Second, 19U) ^
                        (Second >> 10U);
                    Schedule[Word] = Schedule[Word - 16U] + SmallSigmaZero +
                        Schedule[Word - 7U] + SmallSigmaOne;
                }

                std::uint32_t A = Hash[0U];
                std::uint32_t B = Hash[1U];
                std::uint32_t C = Hash[2U];
                std::uint32_t D = Hash[3U];
                std::uint32_t E = Hash[4U];
                std::uint32_t F = Hash[5U];
                std::uint32_t G = Hash[6U];
                std::uint32_t H = Hash[7U];

                for (std::size_t Word = 0U; Word < 64U; ++Word)
                {
                    const std::uint32_t BigSigmaOne =
                        RotateRight(E, 6U) ^ RotateRight(E, 11U) ^ RotateRight(E, 25U);
                    const std::uint32_t Choose = (E & F) ^ ((~E) & G);
                    const std::uint32_t First = H + BigSigmaOne + Choose +
                        RoundConstants[Word] + Schedule[Word];
                    const std::uint32_t BigSigmaZero =
                        RotateRight(A, 2U) ^ RotateRight(A, 13U) ^ RotateRight(A, 22U);
                    const std::uint32_t Majority = (A & B) ^ (A & C) ^ (B & C);
                    const std::uint32_t Second = BigSigmaZero + Majority;

                    H = G;
                    G = F;
                    F = E;
                    E = D + First;
                    D = C;
                    C = B;
                    B = A;
                    A = First + Second;
                }

                Hash[0U] += A;
                Hash[1U] += B;
                Hash[2U] += C;
                Hash[3U] += D;
                Hash[4U] += E;
                Hash[5U] += F;
                Hash[6U] += G;
                Hash[7U] += H;
            }

            std::array<std::uint8_t, 32U> Digest{};
            for (std::size_t Index = 0U; Index < Hash.size(); ++Index)
            {
                Digest[Index * 4U] = static_cast<std::uint8_t>(Hash[Index] >> 24U);
                Digest[Index * 4U + 1U] = static_cast<std::uint8_t>(Hash[Index] >> 16U);
                Digest[Index * 4U + 2U] = static_cast<std::uint8_t>(Hash[Index] >> 8U);
                Digest[Index * 4U + 3U] = static_cast<std::uint8_t>(Hash[Index]);
            }
            return Digest;
        }

        [[nodiscard]] inline DescriptorCatalogueContentIdentifier
            MakeContentIdentifier(const std::vector<std::uint8_t>& CanonicalBytes)
        {
            const std::array<std::uint8_t, 32U> Digest = ComputeSha256(CanonicalBytes);
            constexpr char HexDigits[] = "0123456789abcdef";
            std::string HexValue;
            HexValue.reserve(64U);
            for (const std::uint8_t Byte : Digest)
            {
                HexValue.push_back(HexDigits[(Byte >> 4U) & 0x0FU]);
                HexValue.push_back(HexDigits[Byte & 0x0FU]);
            }
            return DescriptorCatalogueContentIdentifier(std::move(HexValue));
        }

        [[nodiscard]] inline DescriptorCatalogueContentIdentifier
            DeriveValidatedContentIdentifier(
                const std::vector<NormalizedNodeDescriptorRecord>& Records
            )
        {
            return MakeContentIdentifier(EncodeCanonicalContent(Records));
        }

        [[nodiscard]] inline bool IsCatalogueContentIdentifierConsistent(
            const std::vector<DescriptorCatalogueEntry>& Entries,
            const DescriptorCatalogueContentIdentifier& ContentIdentifier
        )
        {
            std::vector<NormalizedNodeDescriptorRecord> Records;
            Records.reserve(Entries.size());
            for (const DescriptorCatalogueEntry& Entry : Entries)
            {
                Records.push_back(Entry.GetRecord());
            }

            return DeriveValidatedContentIdentifier(Records) == ContentIdentifier;
        }
    }

    inline NormalizedPinRecord::NormalizedPinRecord()
        : m_Direction(PinDirection::Input)
        , m_Category(PinCategory::Data)
        , m_Cardinality(PinCardinality::Single)
    {
    }

    inline NormalizedPinRecord::NormalizedPinRecord(
        std::string Name,
        TypeDesc Type,
        PinDirection Direction,
        PinCategory Category,
        PinCardinality Cardinality,
        bool AllowsLiteral,
        std::optional<LiteralValue> DefaultValue
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

    inline bool NormalizedPinRecord::IsValid() const
    {
        return DescriptorCatalogueDetail::IsNormalizedPinSemanticallyValid(*this);
    }

    inline const std::string& NormalizedPinRecord::GetName() const
    {
        return m_Name;
    }

    inline const TypeDesc& NormalizedPinRecord::GetType() const
    {
        return m_Type;
    }

    inline PinDirection NormalizedPinRecord::GetDirection() const
    {
        return m_Direction;
    }

    inline PinCategory NormalizedPinRecord::GetCategory() const
    {
        return m_Category;
    }

    inline PinCardinality NormalizedPinRecord::GetCardinality() const
    {
        return m_Cardinality;
    }

    inline bool NormalizedPinRecord::AllowsLiteral() const
    {
        return m_AllowsLiteral;
    }

    inline const std::optional<LiteralValue>& NormalizedPinRecord::GetDefaultValue() const
    {
        return m_DefaultValue;
    }

    inline bool NormalizedPinRecord::operator==(
        const NormalizedPinRecord& Other
    ) const
    {
        if (m_Name != Other.m_Name || m_Type != Other.m_Type ||
            m_Direction != Other.m_Direction || m_Category != Other.m_Category ||
            m_Cardinality != Other.m_Cardinality ||
            m_AllowsLiteral != Other.m_AllowsLiteral ||
            m_DefaultValue.has_value() != Other.m_DefaultValue.has_value())
        {
            return false;
        }

        return !m_DefaultValue.has_value() ||
            DescriptorCatalogueDetail::AreNormalizedLiteralValuesEqual(
                *m_DefaultValue,
                *Other.m_DefaultValue
            );
    }

    inline NormalizedNodeDescriptorRecord::NormalizedNodeDescriptorRecord() = default;

    inline NormalizedNodeDescriptorRecord::NormalizedNodeDescriptorRecord(
        ExternalNodeIdentity ExternalIdentity,
        std::string DisplayName,
        std::vector<NodeAvailability> Availability,
        std::vector<NormalizedPinRecord> Pins,
        std::optional<ExecutionControlSchema> ControlSchema,
        std::optional<SourceProvenance> Provenance
    )
        : m_ExternalIdentity(std::move(ExternalIdentity))
        , m_DisplayName(std::move(DisplayName))
        , m_Availability(
            DescriptorCatalogueDetail::CanonicalizeAvailability(Availability))
        , m_Pins(std::move(Pins))
        , m_ControlSchema(std::move(ControlSchema))
        , m_SourceProvenance(std::move(Provenance))
    {
    }

    inline bool NormalizedNodeDescriptorRecord::IsValid() const
    {
        return m_ExternalIdentity.IsValid() &&
            (!m_SourceProvenance.has_value() || m_SourceProvenance->IsValid()) &&
            DescriptorCatalogueDetail::IsNormalizedRecordSemanticPayloadValid(*this);
    }

    inline const ExternalNodeIdentity&
        NormalizedNodeDescriptorRecord::GetExternalIdentity() const
    {
        return m_ExternalIdentity;
    }

    inline const std::string& NormalizedNodeDescriptorRecord::GetDisplayName() const
    {
        return m_DisplayName;
    }

    inline const std::vector<NodeAvailability>&
        NormalizedNodeDescriptorRecord::GetAvailability() const
    {
        return m_Availability;
    }

    inline const std::vector<NormalizedPinRecord>&
        NormalizedNodeDescriptorRecord::GetPins() const
    {
        return m_Pins;
    }

    inline const std::optional<ExecutionControlSchema>&
        NormalizedNodeDescriptorRecord::GetExecutionControlSchema() const
    {
        return m_ControlSchema;
    }

    inline const std::optional<SourceProvenance>&
        NormalizedNodeDescriptorRecord::GetSourceProvenance() const
    {
        return m_SourceProvenance;
    }

    inline bool NormalizedNodeDescriptorRecord::operator==(
        const NormalizedNodeDescriptorRecord& Other
    ) const
    {
        if (m_ExternalIdentity != Other.m_ExternalIdentity ||
            m_DisplayName != Other.m_DisplayName ||
            m_Availability != Other.m_Availability ||
            m_Pins.size() != Other.m_Pins.size() ||
            m_ControlSchema.has_value() != Other.m_ControlSchema.has_value() ||
            m_SourceProvenance != Other.m_SourceProvenance)
        {
            return false;
        }

        for (std::size_t Index = 0U; Index < m_Pins.size(); ++Index)
        {
            if (!(m_Pins[Index] == Other.m_Pins[Index]))
            {
                return false;
            }
        }

        return !m_ControlSchema.has_value() ||
            DescriptorCatalogueDetail::AreExecutionControlSchemasEqual(
                *m_ControlSchema,
                *Other.m_ControlSchema
            );
    }

    inline DescriptorCatalogueEntry::DescriptorCatalogueEntry() = default;

    inline DescriptorCatalogueEntry::DescriptorCatalogueEntry(
        NormalizedNodeDescriptorRecord Record,
        NodeDescriptorId DescriptorIdentifier
    )
        : m_Record(std::move(Record))
        , m_DescriptorIdentifier(DescriptorIdentifier)
    {
    }

    inline bool DescriptorCatalogueEntry::IsValid() const
    {
        return m_Record.IsValid() && m_DescriptorIdentifier.IsValid();
    }

    inline const NormalizedNodeDescriptorRecord&
        DescriptorCatalogueEntry::GetRecord() const
    {
        return m_Record;
    }

    inline const ExternalNodeIdentity&
        DescriptorCatalogueEntry::GetExternalIdentity() const
    {
        return m_Record.GetExternalIdentity();
    }

    inline const std::optional<SourceProvenance>&
        DescriptorCatalogueEntry::GetSourceProvenance() const
    {
        return m_Record.GetSourceProvenance();
    }

    inline NodeDescriptorId DescriptorCatalogueEntry::GetDescriptorIdentifier() const
    {
        return m_DescriptorIdentifier;
    }

    inline bool DescriptorCatalogueEntry::operator==(
        const DescriptorCatalogueEntry& Other
    ) const
    {
        return m_Record == Other.m_Record &&
            m_DescriptorIdentifier == Other.m_DescriptorIdentifier;
    }

    inline DescriptorCatalogue::DescriptorCatalogue() = default;

    inline DescriptorCatalogue::DescriptorCatalogue(
        DescriptorCatalogueIdentity Identity,
        std::vector<DescriptorCatalogueEntry> Entries
    )
        : m_Identity(std::move(Identity))
        , m_Entries(std::move(Entries))
    {
    }

    inline bool DescriptorCatalogue::IsValid() const
    {
        if (!m_Identity.IsValid() ||
            !DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
                m_Entries.size()))
        {
            return false;
        }

        std::uint32_t ExpectedDescriptorIdentifier = 1U;
        for (std::size_t Index = 0U; Index < m_Entries.size(); ++Index)
        {
            const DescriptorCatalogueEntry& Entry = m_Entries[Index];
            if (!Entry.IsValid() ||
                Entry.GetDescriptorIdentifier() !=
                    NodeDescriptorId(ExpectedDescriptorIdentifier))
            {
                return false;
            }
            if (Index > 0U &&
                !(m_Entries[Index - 1U].GetExternalIdentity() <
                    Entry.GetExternalIdentity()))
            {
                return false;
            }

            if (Index + 1U < m_Entries.size())
            {
                ++ExpectedDescriptorIdentifier;
            }
        }

        return DescriptorCatalogueDetail::IsCatalogueContentIdentifierConsistent(
            m_Entries,
            m_Identity.GetCatalogueContentIdentifier()
        );
    }

    inline const DescriptorCatalogueIdentity& DescriptorCatalogue::GetIdentity() const
    {
        return m_Identity;
    }

    inline const std::vector<DescriptorCatalogueEntry>&
        DescriptorCatalogue::GetEntries() const
    {
        return m_Entries;
    }

    inline const DescriptorCatalogueEntry* DescriptorCatalogue::FindByExternalIdentity(
        const ExternalNodeIdentity& ExternalIdentity
    ) const
    {
        for (const DescriptorCatalogueEntry& Entry : m_Entries)
        {
            if (Entry.GetExternalIdentity() == ExternalIdentity)
            {
                return &Entry;
            }
        }

        return nullptr;
    }

    inline const DescriptorCatalogueEntry* DescriptorCatalogue::FindByDescriptorIdentifier(
        NodeDescriptorId DescriptorIdentifier
    ) const
    {
        for (const DescriptorCatalogueEntry& Entry : m_Entries)
        {
            if (Entry.GetDescriptorIdentifier() == DescriptorIdentifier)
            {
                return &Entry;
            }
        }

        return nullptr;
    }

    inline std::size_t DescriptorCatalogue::GetEntryCount() const
    {
        return m_Entries.size();
    }

    inline bool DescriptorCatalogue::operator==(const DescriptorCatalogue& Other) const
    {
        return m_Identity == Other.m_Identity && m_Entries == Other.m_Entries;
    }

    [[nodiscard]] inline std::expected<void, DiagnosticCollection>
        ValidateNormalizedDescriptorRecord(
            const NormalizedNodeDescriptorRecord& Record
        )
    {
        const std::vector<NormalizedNodeDescriptorRecord> Records = {Record};
        std::vector<DescriptorCatalogueDetail::PendingDiagnostic> PendingDiagnostics =
            DescriptorCatalogueDetail::CollectNormalizedRecordDiagnostics(Records);
        if (!PendingDiagnostics.empty())
        {
            return std::unexpected(
                DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                    std::move(PendingDiagnostics)
                )
            );
        }

        return {};
    }

    [[nodiscard]] inline std::expected<
        DescriptorCatalogueContentIdentifier,
        DiagnosticCollection
    > DeriveDescriptorCatalogueContentIdentifier(
        const std::vector<NormalizedNodeDescriptorRecord>& Records
    )
    {
        std::vector<DescriptorCatalogueDetail::PendingDiagnostic> PendingDiagnostics =
            DescriptorCatalogueDetail::CollectNormalizedRecordDiagnostics(Records);
        if (!PendingDiagnostics.empty())
        {
            return std::unexpected(
                DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                    std::move(PendingDiagnostics)
                )
            );
        }

        return DescriptorCatalogueDetail::DeriveValidatedContentIdentifier(Records);
    }

    [[nodiscard]] inline std::expected<
        DescriptorCatalogue,
        DiagnosticCollection
    > DescriptorCatalogueBuilder::Build(
        std::string SourceNamespace,
        std::string SourceRevision,
        DescriptorCatalogueSemanticSchemaVersion SemanticSchemaVersion,
        std::vector<NormalizedNodeDescriptorRecord> Records
    )
    {
        std::vector<DescriptorCatalogueDetail::PendingDiagnostic> PendingDiagnostics =
            DescriptorCatalogueDetail::CollectNormalizedRecordDiagnostics(Records);

        if (SourceNamespace.empty())
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::InvalidDescriptorCatalogueIdentity,
                .Message = "Descriptor catalogue source namespace must not be empty."
            });
        }
        if (SourceRevision.empty())
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::InvalidDescriptorCatalogueIdentity,
                .Message = "Descriptor catalogue source revision must not be empty."
            });
        }
        if (!SemanticSchemaVersion.IsValid())
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::InvalidDescriptorCatalogueIdentity,
                .Message = "Descriptor catalogue semantic schema version must not be zero."
            });
        }
        else if (SemanticSchemaVersion.GetValue() != 1U &&
            SemanticSchemaVersion.GetValue() != 2U)
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::string(),
                .PrimarySourceProvenance = std::nullopt,
                .RelatedSourceProvenance = std::nullopt,
                .Code = DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion,
                .Message = "Descriptor catalogue semantic schema version is unsupported."
            });
        }

        if (SemanticSchemaVersion.GetValue() == 1U)
        {
            for (const NormalizedNodeDescriptorRecord& Record : Records)
            {
                if (DescriptorCatalogueDetail::RecordContainsEnum(Record))
                {
                    PendingDiagnostics.push_back({
                        .ExternalKey = Record.GetExternalIdentity().GetKey(),
                        .PrimarySourceProvenance =
                            DescriptorCatalogueDetail::GetValidSourceProvenance(Record),
                        .RelatedSourceProvenance = std::nullopt,
                        .Code = DiagnosticCode::InvalidNormalizedDescriptorRecord,
                        .Message =
                            "Enum semantic values require descriptor catalogue schema version 2."
                    });
                }
            }
        }

        if (!PendingDiagnostics.empty())
        {
            return std::unexpected(
                DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                    std::move(PendingDiagnostics)
                )
            );
        }

        const std::vector<NormalizedNodeDescriptorRecord> CanonicalRecords =
            DescriptorCatalogueDetail::GetCanonicalRecords(Records);
        const DescriptorCatalogueContentIdentifier ContentIdentifier =
            DescriptorCatalogueDetail::DeriveValidatedContentIdentifier(CanonicalRecords);
        const DescriptorCatalogueIdentity Identity(
            std::move(SourceNamespace),
            std::move(SourceRevision),
            SemanticSchemaVersion,
            ContentIdentifier
        );

        std::vector<DescriptorIdentifierAllocationCandidate> AllocationCandidates;
        AllocationCandidates.reserve(CanonicalRecords.size());
        for (const NormalizedNodeDescriptorRecord& Record : CanonicalRecords)
        {
            AllocationCandidates.emplace_back(
                Record.GetExternalIdentity(),
                Record.GetSourceProvenance()
            );
        }

        const auto AllocationResult = DescriptorIdentifierAllocator::Allocate(
            AllocationCandidates
        );
        if (!AllocationResult.has_value())
        {
            return std::unexpected(AllocationResult.error());
        }

        std::vector<DescriptorCatalogueEntry> Entries;
        Entries.reserve(CanonicalRecords.size());
        for (std::size_t Index = 0U; Index < CanonicalRecords.size(); ++Index)
        {
            Entries.emplace_back(
                CanonicalRecords[Index],
                (*AllocationResult)[Index].GetDescriptorIdentifier()
            );
        }

        return DescriptorCatalogue(std::move(Identity), std::move(Entries));
    }

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
