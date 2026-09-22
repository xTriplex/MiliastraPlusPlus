#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDescriptorCatalogue.h"

namespace MiliastraPlusPlus
{
    namespace GenshinClientBooleanFilterResultNodeSourceAdapterDetail
    {
        using Json = nlohmann::json;

        enum class ValidationStage : std::uint8_t
        {
            SourceDocumentParsing,
            MetadataRecordSelection,
            MetadataRecordValidation,
            PinValidation,
            ModeValidation,
            EnumEvidenceValidation,
            NormalizedRecordValidation
        };

        struct PendingDiagnostic final
        {
            ValidationStage Stage;
            DiagnosticCode Code;
            std::string ExternalIdentityKey;
            std::optional<std::uint64_t> PinIndex;
            std::string Message;
            std::optional<SourceProvenance> Provenance;
        };

        inline void Add(
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            DiagnosticCode Code,
            std::string Message,
            std::string ExternalIdentityKey = {},
            std::optional<std::uint64_t> PinIndex = std::nullopt,
            std::optional<SourceProvenance> Provenance = std::nullopt
        )
        {
            Diagnostics.push_back({
                .Stage = Stage,
                .Code = Code,
                .ExternalIdentityKey = std::move(ExternalIdentityKey),
                .PinIndex = PinIndex,
                .Message = std::move(Message),
                .Provenance = std::move(Provenance)
            });
        }

        [[nodiscard]] inline DiagnosticCollection Materialize(std::vector<PendingDiagnostic> Diagnostics)
        {
            std::sort(
                Diagnostics.begin(),
                Diagnostics.end(),
                [](const PendingDiagnostic& Left, const PendingDiagnostic& Right)
                {
                    if (Left.Stage != Right.Stage)
                    {
                        return static_cast<std::uint8_t>(Left.Stage) <
                            static_cast<std::uint8_t>(Right.Stage);
                    }
                    if (Left.Code != Right.Code)
                    {
                        return static_cast<std::uint32_t>(Left.Code) <
                            static_cast<std::uint32_t>(Right.Code);
                    }
                    if (Left.ExternalIdentityKey != Right.ExternalIdentityKey)
                    {
                        return Left.ExternalIdentityKey < Right.ExternalIdentityKey;
                    }
                    if (Left.PinIndex != Right.PinIndex)
                    {
                        return Left.PinIndex < Right.PinIndex;
                    }
                    return Left.Message < Right.Message;
                }
            );

            DiagnosticCollection Result;
            Result.reserve(Diagnostics.size());
            for (PendingDiagnostic& Pending : Diagnostics)
            {
                Result.push_back({
                    .Severity = DiagnosticSeverity::Error,
                    .Code = Pending.Code,
                    .Message = std::move(Pending.Message),
                    .PrimarySourceProvenance = std::move(Pending.Provenance),
                    .ExternalIdentityKey = Pending.ExternalIdentityKey.empty()
                        ? std::nullopt
                        : std::optional<std::string>(
                            std::move(Pending.ExternalIdentityKey))
                });
            }
            return Result;
        }

        [[nodiscard]] inline const Json* Find(const Json& Object, std::string_view Name)
        {
            if (!Object.is_object())
            {
                return nullptr;
            }
            const auto Iterator = Object.find(std::string(Name));
            return Iterator == Object.end() ? nullptr : &(*Iterator);
        }

        template<std::size_t Count>
        [[nodiscard]] inline bool HasExactFields(
            const Json& Object,
            const std::array<std::string_view, Count>& Fields,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope
        )
        {
            bool Valid = Object.is_object();
            for (const std::string_view Field : Fields)
            {
                if (Find(Object, Field) == nullptr)
                {
                    Add(
                        Diagnostics,
                        Stage,
                        DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                        "Required source field is missing: " + std::string(Scope) +
                            "." + std::string(Field) + ".",
                        std::string(ExternalIdentityKey),
                        std::nullopt,
                        Provenance
                    );
                    Valid = false;
                }
            }
            if (Object.is_object())
            {
                for (auto Iterator = Object.begin(); Iterator != Object.end(); ++Iterator)
                {
                    const std::string_view Field = Iterator.key();
                    if (std::find(Fields.begin(), Fields.end(), Field) == Fields.end())
                    {
                        Add(
                            Diagnostics,
                            Stage,
                            DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                            "Unsupported source field: " + std::string(Scope) +
                                "." + std::string(Field) + ".",
                            std::string(ExternalIdentityKey),
                            std::nullopt,
                            Provenance
                        );
                        Valid = false;
                    }
                }
            }
            return Valid;
        }

        [[nodiscard]] inline std::optional<std::string> ReadString(
            const Json& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope
        )
        {
            const Json* Value = Find(Object, Field);
            if (Value == nullptr)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    std::nullopt,
                    Provenance
                );
                return std::nullopt;
            }
            if (!Value->is_string() || Value->get<std::string>().empty())
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Source field must be a non-empty string: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    std::nullopt,
                    Provenance
                );
                return std::nullopt;
            }
            return Value->get<std::string>();
        }

        [[nodiscard]] inline std::optional<std::uint64_t> ReadUnsigned(
            const Json& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope,
            std::optional<std::uint64_t> PinIndex = std::nullopt
        )
        {
            const Json* Value = Find(Object, Field);
            if (Value == nullptr)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    PinIndex,
                    Provenance
                );
                return std::nullopt;
            }
            if (Value->is_number_unsigned())
            {
                return Value->get<std::uint64_t>();
            }
            if (Value->is_number_integer())
            {
                const std::int64_t SignedValue = Value->get<std::int64_t>();
                if (SignedValue >= 0)
                {
                    return static_cast<std::uint64_t>(SignedValue);
                }
            }
            Add(
                Diagnostics,
                Stage,
                DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                "Source field must be a non-negative integer: " + std::string(Scope) +
                    "." + std::string(Field) + ".",
                std::string(ExternalIdentityKey),
                PinIndex,
                Provenance
            );
            return std::nullopt;
        }

        [[nodiscard]] inline bool ReadBoolean(
            const Json& Object,
            std::string_view Field,
            bool Expected,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope,
            std::optional<std::uint64_t> PinIndex = std::nullopt
        )
        {
            const Json* Value = Find(Object, Field);
            if (Value == nullptr)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    PinIndex,
                    Provenance
                );
                return false;
            }
            if (!Value->is_boolean() || Value->get<bool>() != Expected)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Source boolean field has an unsupported value: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    PinIndex,
                    Provenance
                );
                return false;
            }
            return true;
        }

        [[nodiscard]] inline bool ReadExactString(
            const Json& Object,
            std::string_view Field,
            std::string_view Expected,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope,
            std::optional<std::uint64_t> PinIndex = std::nullopt
        )
        {
            const std::optional<std::string> Value = ReadString(
                Object,
                Field,
                Diagnostics,
                Stage,
                ExternalIdentityKey,
                Provenance,
                Scope
            );
            if (!Value.has_value())
            {
                return false;
            }
            if (*Value != Expected)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Source field has an unsupported value: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    PinIndex,
                    Provenance
                );
                return false;
            }
            return true;
        }

        [[nodiscard]] inline bool ReadExactUnsigned(
            const Json& Object,
            std::string_view Field,
            std::uint64_t Expected,
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            std::string_view ExternalIdentityKey,
            const std::optional<SourceProvenance>& Provenance,
            std::string_view Scope,
            std::optional<std::uint64_t> PinIndex = std::nullopt
        )
        {
            const std::optional<std::uint64_t> Value = ReadUnsigned(
                Object,
                Field,
                Diagnostics,
                Stage,
                ExternalIdentityKey,
                Provenance,
                Scope,
                PinIndex
            );
            if (!Value.has_value())
            {
                return false;
            }
            if (*Value != Expected)
            {
                Add(
                    Diagnostics,
                    Stage,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Source field has an unsupported value: " + std::string(Scope) +
                        "." + std::string(Field) + ".",
                    std::string(ExternalIdentityKey),
                    PinIndex,
                    Provenance
                );
                return false;
            }
            return true;
        }

        [[nodiscard]] inline bool ValidateEnumEvidence(const Json& Evidence, std::vector<PendingDiagnostic>& Diagnostics)
        {
            bool Valid = HasExactFields(
                Evidence,
                std::array<std::string_view, 3U>{"family", "ioc", "members"},
                Diagnostics,
                ValidationStage::EnumEvidenceValidation,
                "",
                std::nullopt,
                "enumEvidence"
            );
            if (!Valid)
            {
                return false;
            }

            Valid = ReadExactString(
                Evidence,
                "family",
                "filter_return_type",
                Diagnostics,
                ValidationStage::EnumEvidenceValidation,
                "",
                std::nullopt,
                "enumEvidence"
            ) && Valid;
            Valid = ReadExactUnsigned(
                Evidence,
                "ioc",
                38U,
                Diagnostics,
                ValidationStage::EnumEvidenceValidation,
                "",
                std::nullopt,
                "enumEvidence"
            ) && Valid;

            const Json& Members = Evidence.at("members");
            if (!Members.is_array() || Members.size() != 2U)
            {
                Add(
                    Diagnostics,
                    ValidationStage::EnumEvidenceValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Enum evidence must contain exactly two members."
                );
                return false;
            }

            struct Member final
            {
                std::string Identity;
                std::uint64_t Value = 0U;
            };
            std::vector<Member> ParsedMembers;
            for (const Json& MemberValue : Members)
            {
                if (!HasExactFields(
                    MemberValue,
                    std::array<std::string_view, 2U>{"identity", "value"},
                    Diagnostics,
                    ValidationStage::EnumEvidenceValidation,
                    "",
                    std::nullopt,
                    "enumEvidence.members"
                ))
                {
                    Valid = false;
                    continue;
                }
                const std::optional<std::string> Identity = ReadString(
                    MemberValue,
                    "identity",
                    Diagnostics,
                    ValidationStage::EnumEvidenceValidation,
                    "",
                    std::nullopt,
                    "enumEvidence.members"
                );
                const std::optional<std::uint64_t> Value = ReadUnsigned(
                    MemberValue,
                    "value",
                    Diagnostics,
                    ValidationStage::EnumEvidenceValidation,
                    "",
                    std::nullopt,
                    "enumEvidence.members"
                );
                if (!Identity.has_value() || !Value.has_value())
                {
                    Valid = false;
                    continue;
                }
                ParsedMembers.push_back({*Identity, *Value});
            }

            std::sort(
                ParsedMembers.begin(),
                ParsedMembers.end(),
                [](const Member& Left, const Member& Right)
                {
                    return Left.Identity < Right.Identity;
                }
            );

            const bool HasBooleanMember = std::any_of(
                ParsedMembers.begin(),
                ParsedMembers.end(),
                [](const Member& Value)
                {
                    return Value.Identity == "filter_return_type_return_boolean" &&
                        Value.Value == 1000010U;
                }
            );
            const bool HasIntegerMember = std::any_of(
                ParsedMembers.begin(),
                ParsedMembers.end(),
                [](const Member& Value)
                {
                    return Value.Identity == "filter_return_type_return_integer" &&
                        Value.Value == 10000011U;
                }
            );
            if (!HasBooleanMember || !HasIntegerMember)
            {
                Add(
                    Diagnostics,
                    ValidationStage::EnumEvidenceValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Enum evidence does not contain the required filter return members."
                );
                Valid = false;
            }
            return Valid;
        }

        [[nodiscard]] inline bool ValidateModes(const Json& Modes, std::vector<PendingDiagnostic>& Diagnostics)
        {
            bool Valid = Modes.is_object();
            if (!Valid)
            {
                Add(
                    Diagnostics,
                    ValidationStage::SourceDocumentParsing,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterResultNodeSource,
                    "Mode source document must be a JSON object."
                );
                return false;
            }

            Valid = ReadExactUnsigned(
                Modes,
                "format",
                1U,
                Diagnostics,
                ValidationStage::ModeValidation,
                "",
                std::nullopt,
                "modes"
            ) && Valid;

            const Json* Graphs = Find(Modes, "graphs");
            if (Graphs == nullptr)
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModeValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: modes.graphs."
                );
                return false;
            }
            const Json* BooleanFilter = Find(*Graphs, "bool_filter");
            if (BooleanFilter == nullptr)
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModeValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: modes.graphs.bool_filter."
                );
                return false;
            }
            if (!BooleanFilter->is_object())
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModeValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "modes.graphs.bool_filter must be an object."
                );
                return false;
            }

            Valid = ReadExactUnsigned(
                *BooleanFilter,
                "entryGenericId",
                200000U,
                Diagnostics,
                ValidationStage::ModeValidation,
                "200000",
                std::nullopt,
                "modes.graphs.bool_filter"
            ) && Valid;

            const Json* Beyond = Find(*BooleanFilter, "beyond");
            if (Beyond == nullptr)
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModeValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: modes.graphs.bool_filter.beyond.",
                    "200000"
                );
                return false;
            }
            if (!Beyond->is_object())
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModeValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "modes.graphs.bool_filter.beyond must be an object.",
                    "200000"
                );
                return false;
            }
            Valid = ReadExactString(
                *Beyond,
                "status",
                "available",
                Diagnostics,
                ValidationStage::ModeValidation,
                "200000",
                std::nullopt,
                "modes.graphs.bool_filter.beyond"
            ) && Valid;
            return Valid;
        }

        [[nodiscard]] inline bool ValidatePin(const Json& Pin, std::uint64_t ExpectedIndex, std::vector<PendingDiagnostic>& Diagnostics, const SourceProvenance& Provenance)
        {
            constexpr std::array<std::string_view, 8U> PinFields = {
                "index", "kind", "type", "clientVarType", "defaultValue",
                "name", "connectable", "connectionType"
            };
            const std::string ExternalIdentityKey = "200000";
            bool Valid = HasExactFields(
                Pin,
                PinFields,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "inputs[" + std::to_string(ExpectedIndex) + "]"
            );
            if (!Valid)
            {
                return false;
            }

            Valid = ReadExactUnsigned(
                Pin,
                "index",
                ExpectedIndex,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            Valid = ReadExactString(
                Pin,
                "kind",
                "input",
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            const char* ExpectedType = ExpectedIndex == 0U ? "bool" : "enum";
            Valid = ReadExactString(
                Pin,
                "type",
                ExpectedType,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            Valid = ReadExactUnsigned(
                Pin,
                "clientVarType",
                ExpectedIndex == 0U ? 5U : 13U,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            Valid = ReadExactString(
                Pin,
                "name",
                ExpectedIndex == 0U ? "输出结果（布尔型）" : "filter返回类型",
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            Valid = ReadBoolean(
                Pin,
                "connectable",
                true,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;
            Valid = ReadExactUnsigned(
                Pin,
                "connectionType",
                ExpectedIndex == 0U ? 5U : 210040U,
                Diagnostics,
                ValidationStage::PinValidation,
                ExternalIdentityKey,
                Provenance,
                "input",
                ExpectedIndex
            ) && Valid;

            const Json* DefaultValue = Find(Pin, "defaultValue");
            if (DefaultValue == nullptr)
            {
                Add(
                    Diagnostics,
                    ValidationStage::PinValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: input.defaultValue.",
                    ExternalIdentityKey,
                    ExpectedIndex,
                    Provenance
                );
                return false;
            }
            const std::optional<std::uint64_t> ParsedDefault =
                (DefaultValue->is_number_unsigned() || DefaultValue->is_number_integer())
                ? std::optional<std::uint64_t>(
                    DefaultValue->is_number_unsigned()
                        ? DefaultValue->get<std::uint64_t>()
                        : (DefaultValue->get<std::int64_t>() >= 0
                            ? static_cast<std::uint64_t>(
                                DefaultValue->get<std::int64_t>())
                            : std::numeric_limits<std::uint64_t>::max()))
                : std::nullopt;
            const std::uint64_t ExpectedDefault = ExpectedIndex == 0U ? 0U : 1000010U;
            if (!ParsedDefault.has_value() || *ParsedDefault != ExpectedDefault)
            {
                Add(
                    Diagnostics,
                    ValidationStage::PinValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Input defaultValue is outside the bounded result-node form.",
                    ExternalIdentityKey,
                    ExpectedIndex,
                    Provenance
                );
                Valid = false;
            }
            return Valid;
        }
    }

    /// Admits only the one authentic result-node shape required by the first fixture.
    class GenshinClientBooleanFilterResultNodeSourceAdapter final
    {
    public:
        GenshinClientBooleanFilterResultNodeSourceAdapter() = delete;

        [[nodiscard]] static std::expected<
            NormalizedNodeDescriptorRecord,
            DiagnosticCollection
        > Adapt(
            std::string NodeMetadataJson,
            std::string NodeModesJson,
            std::string EnumEvidenceJson
        )
        {
            using namespace GenshinClientBooleanFilterResultNodeSourceAdapterDetail;

            std::vector<PendingDiagnostic> PendingDiagnostics;
            Json NodeMetadata;
            Json NodeModes;
            Json EnumEvidence;
            try
            {
                NodeMetadata = Json::parse(NodeMetadataJson);
                NodeModes = Json::parse(NodeModesJson);
                EnumEvidence = Json::parse(EnumEvidenceJson);
            }
            catch (const Json::exception&)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::SourceDocumentParsing,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterResultNodeSource,
                    "Result-node source JSON is malformed."
                );
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }

            if (!NodeMetadata.is_array())
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::SourceDocumentParsing,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterResultNodeSource,
                    "Node metadata source document must be a JSON array."
                );
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }

            const Json* SelectedRecord = nullptr;
            std::size_t MatchingRecordCount = 0U;
            for (const Json& Candidate : NodeMetadata)
            {
                const Json* NodeType = Find(Candidate, "nodeType");
                if (NodeType != nullptr && NodeType->is_string() &&
                    NodeType->get<std::string>() == "node_graph_end_boolean")
                {
                    SelectedRecord = &Candidate;
                    ++MatchingRecordCount;
                }
            }
            if (MatchingRecordCount == 0U)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordSelection,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required node_graph_end_boolean source record is missing."
                );
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }
            if (MatchingRecordCount != 1U)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordSelection,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "Node metadata contains multiple node_graph_end_boolean records."
                );
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }

            constexpr std::array<std::string_view, 10U> RecordFields = {
                "subType", "nodeType", "displayName", "graphType", "genericId",
                "concreteId", "inputs", "outputs", "sampleFile", "flows"
            };
            bool Valid = HasExactFields(
                *SelectedRecord,
                RecordFields,
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                std::nullopt,
                "node_graph_end_boolean"
            );

            const std::optional<std::string> SampleFile = ReadString(
                *SelectedRecord,
                "sampleFile",
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                std::nullopt,
                "node_graph_end_boolean"
            );
            if (!SampleFile.has_value())
            {
                Valid = false;
            }
            else if (*SampleFile != "布尔过滤器节点\\查询实体是否在场_连线.gia")
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "node_graph_end_boolean sampleFile is outside the bounded source form.",
                    "200000"
                );
                Valid = false;
            }

            const SourceProvenance Provenance(
                SampleFile.value_or("node_graph_end_boolean"),
                "200000"
            );
            Valid = ReadExactString(
                *SelectedRecord,
                "subType",
                "bool_filter",
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;
            Valid = ReadExactString(
                *SelectedRecord,
                "nodeType",
                "node_graph_end_boolean",
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;
            Valid = ReadExactString(
                *SelectedRecord,
                "displayName",
                "节点图结束(布尔型)",
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;
            Valid = ReadExactUnsigned(
                *SelectedRecord,
                "graphType",
                20001U,
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;
            Valid = ReadExactUnsigned(
                *SelectedRecord,
                "genericId",
                200000U,
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;
            Valid = ReadExactUnsigned(
                *SelectedRecord,
                "concreteId",
                0U,
                PendingDiagnostics,
                ValidationStage::MetadataRecordValidation,
                "200000",
                Provenance,
                "node_graph_end_boolean"
            ) && Valid;

            const Json* Inputs = Find(*SelectedRecord, "inputs");
            const Json* Outputs = Find(*SelectedRecord, "outputs");
            const Json* Flows = Find(*SelectedRecord, "flows");
            if (Inputs == nullptr || Outputs == nullptr || Flows == nullptr)
            {
                Valid = false;
            }
            if (Inputs == nullptr)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: node_graph_end_boolean.inputs.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
            }
            else if (!Inputs->is_array() || Inputs->size() != 2U)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "node_graph_end_boolean must contain exactly two inputs.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
                Valid = false;
            }
            if (Outputs == nullptr)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: node_graph_end_boolean.outputs.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
            }
            else if (!Outputs->is_array() || !Outputs->empty())
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "node_graph_end_boolean outputs must be empty.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
                Valid = false;
            }
            if (Flows == nullptr)
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField,
                    "Required source field is missing: node_graph_end_boolean.flows.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
            }
            else if (!Flows->is_array() || !Flows->empty())
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::MetadataRecordValidation,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm,
                    "node_graph_end_boolean flows must be empty.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
                Valid = false;
            }

            if (Inputs != nullptr && Inputs->is_array() && Inputs->size() == 2U)
            {
                Valid = ValidatePin(
                    (*Inputs)[0U],
                    0U,
                    PendingDiagnostics,
                    Provenance
                ) && Valid;
                Valid = ValidatePin(
                    (*Inputs)[1U],
                    1U,
                    PendingDiagnostics,
                    Provenance
                ) && Valid;
            }

            Valid = ValidateModes(NodeModes, PendingDiagnostics) && Valid;
            Valid = ValidateEnumEvidence(EnumEvidence, PendingDiagnostics) && Valid;
            if (!Valid || !Provenance.IsValid())
            {
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }

            const EnumTypeIdentity EnumIdentity("filter_return_type");
            std::vector<NormalizedPinRecord> Pins;
            Pins.emplace_back(
                "输出结果（布尔型）",
                TypeDesc::Boolean(),
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                true,
                LiteralValue(LiteralValue::Data{false})
            );
            Pins.emplace_back(
                "filter返回类型",
                TypeDesc::Enum(EnumIdentity),
                PinDirection::Input,
                PinCategory::Data,
                PinCardinality::Single,
                true,
                LiteralValue(LiteralValue::Data{
                    EnumLiteralValue(EnumIdentity, 1000010)
                })
            );

            NormalizedNodeDescriptorRecord Result(
                ExternalNodeIdentity("200000"),
                "节点图结束(布尔型)",
                {NodeAvailability::Client},
                std::move(Pins),
                std::nullopt,
                Provenance
            );
            if (!Result.IsValid())
            {
                Add(
                    PendingDiagnostics,
                    ValidationStage::NormalizedRecordValidation,
                    DiagnosticCode::InvalidNormalizedDescriptorRecord,
                    "Normalized result-node descriptor is invalid.",
                    "200000",
                    std::nullopt,
                    Provenance
                );
                return std::unexpected(Materialize(std::move(PendingDiagnostics)));
            }
            return Result;
        }
    };
}
