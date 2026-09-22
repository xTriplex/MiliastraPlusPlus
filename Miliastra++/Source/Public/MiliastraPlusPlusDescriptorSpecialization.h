#pragma once

#include <algorithm>
#include <cstddef>
#include <expected>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptorCatalogue.h"

namespace MiliastraPlusPlus
{
    class DescriptorSpecializationPin final
    {
    public:
        DescriptorSpecializationPin(
            std::string Name,
            std::optional<TypeDesc> FixedType,
            bool Reflected,
            PinDirection Direction,
            PinCategory Category,
            PinCardinality Cardinality,
            bool AllowsLiteral,
            std::optional<LiteralValue> DefaultValue = std::nullopt
        )
            : m_Name(std::move(Name))
            , m_FixedType(std::move(FixedType))
            , m_Reflected(Reflected)
            , m_Direction(Direction)
            , m_Category(Category)
            , m_Cardinality(Cardinality)
            , m_AllowsLiteral(AllowsLiteral)
            , m_DefaultValue(std::move(DefaultValue))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            if (m_Name.empty() || m_FixedType.has_value() == m_Reflected)
            {
                return false;
            }
            if (m_FixedType.has_value() && !m_FixedType->IsValid())
            {
                return false;
            }
            if (m_Direction != PinDirection::Input &&
                m_Direction != PinDirection::Output)
            {
                return false;
            }
            if (m_Category != PinCategory::Data &&
                m_Category != PinCategory::Execution)
            {
                return false;
            }
            if (m_Cardinality != PinCardinality::Single &&
                m_Cardinality != PinCardinality::Optional &&
                m_Cardinality != PinCardinality::Multiple)
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool IsReflected() const
        {
            return m_Reflected;
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] const std::optional<TypeDesc>& GetFixedType() const
        {
            return m_FixedType;
        }

        [[nodiscard]] PinDirection GetDirection() const
        {
            return m_Direction;
        }

        [[nodiscard]] PinCategory GetCategory() const
        {
            return m_Category;
        }

        [[nodiscard]] PinCardinality GetCardinality() const
        {
            return m_Cardinality;
        }

        [[nodiscard]] bool AllowsLiteral() const
        {
            return m_AllowsLiteral;
        }

        [[nodiscard]] const std::optional<LiteralValue>& GetDefaultValue() const
        {
            return m_DefaultValue;
        }

        bool operator==(const DescriptorSpecializationPin& Other) const
        {
            if (m_Name != Other.m_Name || m_FixedType != Other.m_FixedType ||
                m_Reflected != Other.m_Reflected ||
                m_Direction != Other.m_Direction ||
                m_Category != Other.m_Category ||
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

    private:
        std::string m_Name;
        std::optional<TypeDesc> m_FixedType;
        bool m_Reflected;
        PinDirection m_Direction;
        PinCategory m_Category;
        PinCardinality m_Cardinality;
        bool m_AllowsLiteral;
        std::optional<LiteralValue> m_DefaultValue;
    };

    class DescriptorSpecializationPinBinding final
    {
    public:
        DescriptorSpecializationPinBinding(PinIndex FamilyPinIndex, TypeDesc ConcreteType)
            : m_FamilyPinIndex(FamilyPinIndex)
            , m_ConcreteType(std::move(ConcreteType))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_FamilyPinIndex.IsValid() && m_ConcreteType.IsValid();
        }

        [[nodiscard]] PinIndex GetFamilyPinIndex() const
        {
            return m_FamilyPinIndex;
        }

        [[nodiscard]] const TypeDesc& GetConcreteType() const
        {
            return m_ConcreteType;
        }

        bool operator==(const DescriptorSpecializationPinBinding&) const = default;

    private:
        PinIndex m_FamilyPinIndex;
        TypeDesc m_ConcreteType;
    };

    class DescriptorSpecializationVariant final
    {
    public:
        DescriptorSpecializationVariant(
            ExternalNodeIdentity ConcreteExternalIdentity,
            std::string SpecializationKey,
            std::vector<DescriptorSpecializationPinBinding> PinBindings
        )
            : m_ConcreteExternalIdentity(std::move(ConcreteExternalIdentity))
            , m_SpecializationKey(std::move(SpecializationKey))
            , m_PinBindings(std::move(PinBindings))
        {
            std::sort(
                m_PinBindings.begin(),
                m_PinBindings.end(),
                [](const DescriptorSpecializationPinBinding& Left, const DescriptorSpecializationPinBinding& Right)
                {
                    return Left.GetFamilyPinIndex() < Right.GetFamilyPinIndex();
                }
            );
        }

        [[nodiscard]] bool IsValid() const
        {
            if (!m_ConcreteExternalIdentity.IsValid() || m_SpecializationKey.empty())
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] const ExternalNodeIdentity& GetConcreteExternalIdentity() const
        {
            return m_ConcreteExternalIdentity;
        }

        [[nodiscard]] const std::string& GetSpecializationKey() const
        {
            return m_SpecializationKey;
        }

        [[nodiscard]] const std::vector<DescriptorSpecializationPinBinding>& GetPinBindings() const
        {
            return m_PinBindings;
        }

        bool operator==(const DescriptorSpecializationVariant& Other) const
        {
            return m_ConcreteExternalIdentity == Other.m_ConcreteExternalIdentity &&
                m_SpecializationKey == Other.m_SpecializationKey &&
                m_PinBindings == Other.m_PinBindings;
        }

    private:
        ExternalNodeIdentity m_ConcreteExternalIdentity;
        std::string m_SpecializationKey;
        std::vector<DescriptorSpecializationPinBinding> m_PinBindings;
    };

    class DescriptorSpecializationFamily final
    {
    public:
        DescriptorSpecializationFamily(
            ExternalNodeIdentity FamilyExternalIdentity,
            std::string DisplayName,
            std::vector<NodeAvailability> Availability,
            std::vector<DescriptorSpecializationPin> Pins,
            std::optional<ExecutionControlSchema> ControlSchema,
            std::optional<SourceProvenance> Provenance,
            std::vector<DescriptorSpecializationVariant> Variants
        )
            : m_FamilyExternalIdentity(std::move(FamilyExternalIdentity))
            , m_DisplayName(std::move(DisplayName))
            , m_Availability(
                DescriptorCatalogueDetail::CanonicalizeAvailability(Availability))
            , m_Pins(std::move(Pins))
            , m_ControlSchema(std::move(ControlSchema))
            , m_SourceProvenance(std::move(Provenance))
            , m_Variants(std::move(Variants))
        {
            std::sort(
                m_Variants.begin(),
                m_Variants.end(),
                [](const DescriptorSpecializationVariant& Left, const DescriptorSpecializationVariant& Right)
                {
                    return Left.GetConcreteExternalIdentity() <
                        Right.GetConcreteExternalIdentity();
                }
            );
        }

        [[nodiscard]] bool IsValid() const
        {
            if (!m_FamilyExternalIdentity.IsValid() || m_DisplayName.empty() ||
                m_Variants.empty() ||
                (m_SourceProvenance.has_value() && !m_SourceProvenance->IsValid()))
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < m_Availability.size(); ++Index)
            {
                if ((m_Availability[Index] != NodeAvailability::Server &&
                        m_Availability[Index] != NodeAvailability::Client) ||
                    (Index > 0U && m_Availability[Index - 1U] == m_Availability[Index]))
                {
                    return false;
                }
            }

            bool HasReflectedPin = false;
            for (std::size_t Index = 0U; Index < m_Pins.size(); ++Index)
            {
                if (!m_Pins[Index].IsValid())
                {
                    return false;
                }
                HasReflectedPin = HasReflectedPin || m_Pins[Index].IsReflected();
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (m_Pins[Prior].GetName() == m_Pins[Index].GetName())
                    {
                        return false;
                    }
                }
            }
            if (!HasReflectedPin)
            {
                return false;
            }

            for (const DescriptorSpecializationVariant& Variant : m_Variants)
            {
                if (!Variant.IsValid())
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] const ExternalNodeIdentity& GetFamilyExternalIdentity() const
        {
            return m_FamilyExternalIdentity;
        }

        [[nodiscard]] const std::string& GetDisplayName() const
        {
            return m_DisplayName;
        }

        [[nodiscard]] const std::vector<NodeAvailability>& GetAvailability() const
        {
            return m_Availability;
        }

        [[nodiscard]] const std::vector<DescriptorSpecializationPin>& GetPins() const
        {
            return m_Pins;
        }

        [[nodiscard]] const std::optional<ExecutionControlSchema>& GetExecutionControlSchema() const
        {
            return m_ControlSchema;
        }

        [[nodiscard]] const std::optional<SourceProvenance>& GetSourceProvenance() const
        {
            return m_SourceProvenance;
        }

        [[nodiscard]] const std::vector<DescriptorSpecializationVariant>& GetVariants() const
        {
            return m_Variants;
        }

        bool operator==(const DescriptorSpecializationFamily& Other) const
        {
            if (m_FamilyExternalIdentity != Other.m_FamilyExternalIdentity ||
                m_DisplayName != Other.m_DisplayName ||
                m_Availability != Other.m_Availability ||
                m_Pins != Other.m_Pins ||
                m_ControlSchema.has_value() != Other.m_ControlSchema.has_value() ||
                m_SourceProvenance != Other.m_SourceProvenance ||
                m_Variants != Other.m_Variants)
            {
                return false;
            }

            return !m_ControlSchema.has_value() ||
                DescriptorCatalogueDetail::AreExecutionControlSchemasEqual(
                    *m_ControlSchema,
                    *Other.m_ControlSchema
                );
        }

    private:
        ExternalNodeIdentity m_FamilyExternalIdentity;
        std::string m_DisplayName;
        std::vector<NodeAvailability> m_Availability;
        std::vector<DescriptorSpecializationPin> m_Pins;
        std::optional<ExecutionControlSchema> m_ControlSchema;
        std::optional<SourceProvenance> m_SourceProvenance;
        std::vector<DescriptorSpecializationVariant> m_Variants;
    };

    class DescriptorFamilySpecializer;

    class DescriptorSpecializationResult final
    {
    public:
        DescriptorSpecializationResult(const DescriptorSpecializationResult&) = default;
        DescriptorSpecializationResult(DescriptorSpecializationResult&&) = default;
        DescriptorSpecializationResult& operator=(const DescriptorSpecializationResult&) = default;
        DescriptorSpecializationResult& operator=(DescriptorSpecializationResult&&) = default;

        [[nodiscard]] bool IsValid() const;

        [[nodiscard]] const std::vector<DescriptorSpecializationFamily>& GetFamilies() const
        {
            return m_Families;
        }

        [[nodiscard]] const std::vector<NormalizedNodeDescriptorRecord>& GetConcreteRecords() const
        {
            return m_ConcreteRecords;
        }

        bool operator==(const DescriptorSpecializationResult& Other) const
        {
            return m_Families == Other.m_Families &&
                m_ConcreteRecords == Other.m_ConcreteRecords;
        }

    private:
        DescriptorSpecializationResult(std::vector<DescriptorSpecializationFamily> Families, std::vector<NormalizedNodeDescriptorRecord> ConcreteRecords)
            : m_Families(std::move(Families))
            , m_ConcreteRecords(std::move(ConcreteRecords))
        {
        }

        friend class DescriptorFamilySpecializer;

        std::vector<DescriptorSpecializationFamily> m_Families;
        std::vector<NormalizedNodeDescriptorRecord> m_ConcreteRecords;
    };

    namespace DescriptorSpecializationDetail
    {
        using PendingDiagnostic = DescriptorCatalogueDetail::PendingDiagnostic;

        inline void AddDiagnostic(
            std::vector<PendingDiagnostic>& Diagnostics,
            DiagnosticCode Code,
            std::string Message,
            std::string ExternalKey = {},
            std::optional<SourceProvenance> PrimaryProvenance = std::nullopt,
            std::optional<SourceProvenance> RelatedProvenance = std::nullopt
        )
        {
            Diagnostics.push_back({
                .ExternalKey = std::move(ExternalKey),
                .PrimarySourceProvenance = std::move(PrimaryProvenance),
                .RelatedSourceProvenance = std::move(RelatedProvenance),
                .Code = Code,
                .Message = std::move(Message)
            });
        }

        [[nodiscard]] inline const DescriptorSpecializationPinBinding* FindBinding(const DescriptorSpecializationVariant& Variant, PinIndex FamilyPinIndex)
        {
            for (const DescriptorSpecializationPinBinding& Binding :
                Variant.GetPinBindings())
            {
                if (Binding.GetFamilyPinIndex() == FamilyPinIndex)
                {
                    return &Binding;
                }
            }

            return nullptr;
        }

        [[nodiscard]] inline bool IsVariantBindingRelationValid(
            const DescriptorSpecializationFamily& Family,
            const DescriptorSpecializationVariant& Variant,
            std::vector<PendingDiagnostic>* Diagnostics
        )
        {
            bool Valid = true;
            const std::vector<DescriptorSpecializationPin>& Pins = Family.GetPins();

            for (const DescriptorSpecializationPinBinding& Binding :
                Variant.GetPinBindings())
            {
                const std::uint32_t Index = Binding.GetFamilyPinIndex().GetValue();
                if (!Binding.IsValid() || Index >= Pins.size())
                {
                    if (Diagnostics != nullptr)
                    {
                        AddDiagnostic(
                            *Diagnostics,
                            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding,
                            "Specialization binding references an invalid family pin.",
                            Variant.GetConcreteExternalIdentity().GetKey(),
                            Family.GetSourceProvenance()
                        );
                    }
                    Valid = false;
                    continue;
                }

                if (!Pins[Index].IsReflected())
                {
                    if (Diagnostics != nullptr)
                    {
                        AddDiagnostic(
                            *Diagnostics,
                            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding,
                            "Specialization binding references a fixed family pin.",
                            Variant.GetConcreteExternalIdentity().GetKey(),
                            Family.GetSourceProvenance()
                        );
                    }
                    Valid = false;
                }
            }

            for (std::size_t Index = 0U; Index < Pins.size(); ++Index)
            {
                std::size_t BindingCount = 0U;
                for (const DescriptorSpecializationPinBinding& Binding :
                    Variant.GetPinBindings())
                {
                    if (Binding.GetFamilyPinIndex() ==
                        PinIndex(static_cast<std::uint32_t>(Index)))
                    {
                        ++BindingCount;
                    }
                }

                const std::size_t ExpectedCount = Pins[Index].IsReflected() ? 1U : 0U;
                if (BindingCount != ExpectedCount)
                {
                    if (Diagnostics != nullptr)
                    {
                        AddDiagnostic(
                            *Diagnostics,
                            DiagnosticCode::InvalidDescriptorSpecializationVariantBinding,
                            Pins[Index].IsReflected()
                                ? "Reflected family pin must have exactly one binding."
                                : "Fixed family pin must not have a binding.",
                            Variant.GetConcreteExternalIdentity().GetKey(),
                            Family.GetSourceProvenance()
                        );
                    }
                    Valid = false;
                }
            }

            return Valid;
        }

        [[nodiscard]] inline std::optional<NormalizedNodeDescriptorRecord> MakeConcreteRecord(
            const DescriptorSpecializationFamily& Family,
            const DescriptorSpecializationVariant& Variant
        )
        {
            if (!IsVariantBindingRelationValid(Family, Variant, nullptr))
            {
                return std::nullopt;
            }

            std::vector<NormalizedPinRecord> Pins;
            Pins.reserve(Family.GetPins().size());
            for (std::size_t Index = 0U; Index < Family.GetPins().size(); ++Index)
            {
                const DescriptorSpecializationPin& FamilyPin = Family.GetPins()[Index];
                TypeDesc ConcreteType;
                if (FamilyPin.IsReflected())
                {
                    const DescriptorSpecializationPinBinding* Binding = FindBinding(
                        Variant,
                        PinIndex(static_cast<std::uint32_t>(Index))
                    );
                    if (Binding == nullptr)
                    {
                        return std::nullopt;
                    }
                    ConcreteType = Binding->GetConcreteType();
                }
                else
                {
                    ConcreteType = *FamilyPin.GetFixedType();
                }

                Pins.emplace_back(
                    FamilyPin.GetName(),
                    std::move(ConcreteType),
                    FamilyPin.GetDirection(),
                    FamilyPin.GetCategory(),
                    FamilyPin.GetCardinality(),
                    FamilyPin.AllowsLiteral(),
                    FamilyPin.GetDefaultValue()
                );
            }

            return NormalizedNodeDescriptorRecord(
                Variant.GetConcreteExternalIdentity(),
                Family.GetDisplayName(),
                Family.GetAvailability(),
                std::move(Pins),
                Family.GetExecutionControlSchema(),
                Family.GetSourceProvenance()
            );
        }
    }

    inline bool DescriptorSpecializationResult::IsValid() const
    {
        std::size_t VariantCount = 0U;
        for (std::size_t FamilyIndex = 0U;
            FamilyIndex < m_Families.size();
            ++FamilyIndex)
        {
            const DescriptorSpecializationFamily& Family = m_Families[FamilyIndex];
            if (!Family.IsValid() ||
                (FamilyIndex > 0U &&
                    !(m_Families[FamilyIndex - 1U].GetFamilyExternalIdentity() <
                        Family.GetFamilyExternalIdentity())))
            {
                return false;
            }

            VariantCount += Family.GetVariants().size();
            for (std::size_t VariantIndex = 0U;
                VariantIndex < Family.GetVariants().size();
                ++VariantIndex)
            {
                const DescriptorSpecializationVariant& Variant =
                    Family.GetVariants()[VariantIndex];
                for (std::size_t Prior = 0U; Prior < VariantIndex; ++Prior)
                {
                    if (Family.GetVariants()[Prior].GetSpecializationKey() ==
                            Variant.GetSpecializationKey() ||
                        Family.GetVariants()[Prior].GetConcreteExternalIdentity() ==
                            Variant.GetConcreteExternalIdentity())
                    {
                        return false;
                    }
                }
                if (!DescriptorSpecializationDetail::IsVariantBindingRelationValid(
                        Family,
                        Variant,
                        nullptr
                    ))
                {
                    return false;
                }

                const auto Expected = DescriptorSpecializationDetail::MakeConcreteRecord(
                    Family,
                    Variant
                );
                if (!Expected.has_value())
                {
                    return false;
                }

                std::size_t MatchCount = 0U;
                for (const NormalizedNodeDescriptorRecord& Record : m_ConcreteRecords)
                {
                    if (Record.GetExternalIdentity() ==
                        Variant.GetConcreteExternalIdentity())
                    {
                        ++MatchCount;
                        if (!(Record == *Expected) ||
                            !ValidateNormalizedDescriptorRecord(Record).has_value())
                        {
                            return false;
                        }
                    }
                }
                if (MatchCount != 1U)
                {
                    return false;
                }
            }
        }

        if (VariantCount != m_ConcreteRecords.size())
        {
            return false;
        }

        for (std::size_t Index = 0U; Index < m_ConcreteRecords.size(); ++Index)
        {
            if (!ValidateNormalizedDescriptorRecord(m_ConcreteRecords[Index]).has_value() ||
                (Index > 0U &&
                    !(m_ConcreteRecords[Index - 1U].GetExternalIdentity() <
                        m_ConcreteRecords[Index].GetExternalIdentity())))
            {
                return false;
            }

            std::size_t VariantMatches = 0U;
            for (const DescriptorSpecializationFamily& Family : m_Families)
            {
                for (const DescriptorSpecializationVariant& Variant :
                    Family.GetVariants())
                {
                    if (Variant.GetConcreteExternalIdentity() ==
                        m_ConcreteRecords[Index].GetExternalIdentity())
                    {
                        ++VariantMatches;
                    }
                }
            }
            if (VariantMatches != 1U)
            {
                return false;
            }
        }

        return true;
    }

    class DescriptorFamilySpecializer final
    {
    public:
        DescriptorFamilySpecializer() = delete;

        [[nodiscard]] static std::expected<
            DescriptorSpecializationResult,
            DiagnosticCollection
        > Specialize(
            std::vector<DescriptorSpecializationFamily> Families
        )
        {
            using namespace DescriptorSpecializationDetail;

            std::sort(
                Families.begin(),
                Families.end(),
                [](const DescriptorSpecializationFamily& Left, const DescriptorSpecializationFamily& Right)
                {
                    return Left.GetFamilyExternalIdentity() <
                        Right.GetFamilyExternalIdentity();
                }
            );

            std::vector<PendingDiagnostic> PendingDiagnostics;
            for (std::size_t FamilyIndex = 0U;
                FamilyIndex < Families.size();
                ++FamilyIndex)
            {
                const DescriptorSpecializationFamily& Family = Families[FamilyIndex];
                if (!Family.IsValid())
                {
                    AddDiagnostic(
                        PendingDiagnostics,
                        DiagnosticCode::InvalidDescriptorSpecializationFamily,
                        "Descriptor specialization family is invalid.",
                        Family.GetFamilyExternalIdentity().GetKey(),
                        Family.GetSourceProvenance()
                    );
                }

                if (FamilyIndex > 0U &&
                    Families[FamilyIndex - 1U].GetFamilyExternalIdentity() ==
                        Family.GetFamilyExternalIdentity())
                {
                    AddDiagnostic(
                        PendingDiagnostics,
                        DiagnosticCode::DuplicateExternalNodeIdentity,
                        "Descriptor specialization family identity is duplicated.",
                        Family.GetFamilyExternalIdentity().GetKey(),
                        Families[FamilyIndex - 1U].GetSourceProvenance(),
                        Family.GetSourceProvenance()
                    );
                }

                const auto& Variants = Family.GetVariants();
                for (std::size_t VariantIndex = 0U;
                    VariantIndex < Variants.size();
                    ++VariantIndex)
                {
                    const DescriptorSpecializationVariant& Variant =
                        Variants[VariantIndex];
                    static_cast<void>(IsVariantBindingRelationValid(
                        Family,
                        Variant,
                        &PendingDiagnostics
                    ));

                    for (std::size_t Prior = 0U; Prior < VariantIndex; ++Prior)
                    {
                        if (Variants[Prior].GetSpecializationKey() ==
                                Variant.GetSpecializationKey() ||
                            Variants[Prior].GetConcreteExternalIdentity() ==
                                Variant.GetConcreteExternalIdentity())
                        {
                            AddDiagnostic(
                                PendingDiagnostics,
                                DiagnosticCode::DuplicateDescriptorSpecializationVariant,
                                "Descriptor specialization variant is duplicated within its family.",
                                Variant.GetConcreteExternalIdentity().GetKey(),
                                Family.GetSourceProvenance()
                            );
                        }
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

            std::vector<NormalizedNodeDescriptorRecord> ConcreteRecords;
            for (const DescriptorSpecializationFamily& Family : Families)
            {
                for (const DescriptorSpecializationVariant& Variant :
                    Family.GetVariants())
                {
                    ConcreteRecords.push_back(*MakeConcreteRecord(Family, Variant));
                }
            }

            std::sort(
                ConcreteRecords.begin(),
                ConcreteRecords.end(),
                [](const NormalizedNodeDescriptorRecord& Left, const NormalizedNodeDescriptorRecord& Right)
                {
                    return Left.GetExternalIdentity() < Right.GetExternalIdentity();
                }
            );

            std::vector<PendingDiagnostic> NormalizedDiagnostics =
                DescriptorCatalogueDetail::CollectNormalizedRecordDiagnostics(
                    ConcreteRecords
                );
            if (!NormalizedDiagnostics.empty())
            {
                return std::unexpected(
                    DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                        std::move(NormalizedDiagnostics)
                    )
                );
            }

            DescriptorSpecializationResult Result(
                std::move(Families),
                std::move(ConcreteRecords)
            );
            if (!Result.IsValid())
            {
                AddDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::InvalidDescriptorSpecializationFamily,
                    "Descriptor specialization result failed its integrity contract."
                );
                return std::unexpected(
                    DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                        std::move(PendingDiagnostics)
                    )
                );
            }

            return Result;
        }
    };
}
