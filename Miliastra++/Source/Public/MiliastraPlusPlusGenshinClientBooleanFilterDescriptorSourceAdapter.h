#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <expected>
#include <iterator>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDescriptorCatalogue.h"

namespace MiliastraPlusPlus
{
    namespace GenshinClientBooleanFilterDescriptorSourceAdapterDetail
    {
        using JsonValue = nlohmann::json;
        using PendingDiagnostic = DescriptorCatalogueDetail::PendingDiagnostic;

        struct SourceRecordContext final
        {
            std::string ExternalKey;
            std::optional<SourceProvenance> Provenance;
        };

        struct ModeMembership final
        {
            bool IsValid = true;
            std::set<std::uint64_t> BeyondGenericIds;
            std::set<std::uint64_t> ClassicGenericIds;
        };

        struct SourceTypeMapping final
        {
            std::uint64_t ClientVariableType;
            TypeDesc NormalizedType;
        };

        struct ParsedPin final
        {
            std::uint64_t Index;
            NormalizedPinRecord NormalizedPin;
        };

        inline constexpr std::array<std::string_view, 10U> RecordFields = {
            "subType",
            "nodeType",
            "displayName",
            "graphType",
            "genericId",
            "concreteId",
            "inputs",
            "outputs",
            "sampleFile",
            "flows"
        };

        inline constexpr std::array<std::string_view, 12U> PinFields = {
            "index",
            "kind",
            "type",
            "name",
            "clientVarType",
            "defaultValue",
            "connectable",
            "connectionType",
            "reflective",
            "i2Index",
            "indexOfConcrete",
            "variants"
        };

        inline void AddPendingDiagnostic(
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            DiagnosticCode Code,
            std::string Message,
            std::string ExternalKey = {},
            std::optional<SourceProvenance> PrimarySourceProvenance = std::nullopt,
            std::optional<SourceProvenance> RelatedSourceProvenance = std::nullopt
        )
        {
            PendingDiagnostics.push_back({
                .ExternalKey = std::move(ExternalKey),
                .PrimarySourceProvenance = std::move(PrimarySourceProvenance),
                .RelatedSourceProvenance = std::move(RelatedSourceProvenance),
                .Code = Code,
                .Message = std::move(Message)
            });
        }

        inline void AddMissingFieldDiagnostic(std::vector<PendingDiagnostic>& PendingDiagnostics, std::string_view Field, const SourceRecordContext& Context)
        {
            AddPendingDiagnostic(
                PendingDiagnostics,
                DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                "Required source field is missing: " + std::string(Field) + ".",
                Context.ExternalKey,
                Context.Provenance
            );
        }

        inline void AddUnsupportedFormDiagnostic(std::vector<PendingDiagnostic>& PendingDiagnostics, std::string Message, const SourceRecordContext& Context)
        {
            AddPendingDiagnostic(
                PendingDiagnostics,
                DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                std::move(Message),
                Context.ExternalKey,
                Context.Provenance
            );
        }

        inline const JsonValue* FindMember(const JsonValue& Object, std::string_view Name)
        {
            const auto Iterator = Object.find(std::string(Name));
            return Iterator == Object.end() ? nullptr : &(*Iterator);
        }

        template<std::size_t Count>
        [[nodiscard]] inline bool ValidateKnownFields(
            const JsonValue& Object,
            const std::array<std::string_view, Count>& AllowedFields,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context
        )
        {
            bool Valid = true;
            for (auto Iterator = Object.begin(); Iterator != Object.end(); ++Iterator)
            {
                const std::string_view FieldName = Iterator.key();
                if (std::find(
                        AllowedFields.begin(),
                        AllowedFields.end(),
                        FieldName
                    ) == AllowedFields.end())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Unsupported source field: " + std::string(FieldName) + ".",
                        Context
                    );
                    Valid = false;
                }
            }

            return Valid;
        }

        [[nodiscard]] inline std::optional<std::uint64_t> TryConvertUnsignedInteger(const JsonValue& Value)
        {
            try
            {
                if (Value.is_number_unsigned())
                {
                    return Value.get<std::uint64_t>();
                }

                if (Value.is_number_integer())
                {
                    const std::int64_t SignedValue = Value.get<std::int64_t>();
                    if (SignedValue < 0)
                    {
                        return std::nullopt;
                    }

                    return static_cast<std::uint64_t>(SignedValue);
                }
            }
            catch (const nlohmann::json::exception&)
            {
                return std::nullopt;
            }

            return std::nullopt;
        }

        [[nodiscard]] inline std::optional<std::uint64_t> ReadRequiredUnsignedInteger(
            const JsonValue& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context
        )
        {
            const JsonValue* Value = FindMember(Object, Field);
            if (Value == nullptr)
            {
                AddMissingFieldDiagnostic(PendingDiagnostics, Field, Context);
                return std::nullopt;
            }

            const std::optional<std::uint64_t> Converted =
                TryConvertUnsignedInteger(*Value);
            if (!Converted.has_value())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source field must be a non-negative integer: " +
                        std::string(Field) + ".",
                    Context
                );
            }

            return Converted;
        }

        [[nodiscard]] inline std::optional<std::string> ReadRequiredString(
            const JsonValue& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context
        )
        {
            const JsonValue* Value = FindMember(Object, Field);
            if (Value == nullptr)
            {
                AddMissingFieldDiagnostic(PendingDiagnostics, Field, Context);
                return std::nullopt;
            }

            if (!Value->is_string())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source field must be a string: " + std::string(Field) + ".",
                    Context
                );
                return std::nullopt;
            }

            std::string Result = Value->get<std::string>();
            if (Result.empty())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source field must not be empty: " + std::string(Field) + ".",
                    Context
                );
                return std::nullopt;
            }

            return Result;
        }

        [[nodiscard]] inline std::string ToCanonicalDecimal(std::uint64_t Value)
        {
            std::array<char, 20U> Buffer{};
            const auto Conversion = std::to_chars(
                Buffer.data(),
                Buffer.data() + Buffer.size(),
                Value
            );
            if (Conversion.ec != std::errc())
            {
                return {};
            }

            return std::string(Buffer.data(), Conversion.ptr);
        }

        [[nodiscard]] inline std::optional<SourceTypeMapping> GetSourceTypeMapping(const std::string& SourceType)
        {
            if (SourceType == "bool")
            {
                return SourceTypeMapping{5U, TypeDesc::Boolean()};
            }
            if (SourceType == "float")
            {
                return SourceTypeMapping{7U, TypeDesc::Float()};
            }
            if (SourceType == "vec3")
            {
                return SourceTypeMapping{11U, TypeDesc::Vector3()};
            }
            if (SourceType == "guid")
            {
                return SourceTypeMapping{14U, TypeDesc::GUID()};
            }
            if (SourceType == "config_id")
            {
                return SourceTypeMapping{18U, TypeDesc::ConfigId()};
            }
            if (SourceType == "faction")
            {
                return SourceTypeMapping{16U, TypeDesc::Faction()};
            }

            return std::nullopt;
        }

        [[nodiscard]] inline std::optional<float> TryConvertFiniteFloat32(const JsonValue& Value)
        {
            if (!Value.is_number())
            {
                return std::nullopt;
            }

            try
            {
                const double DoubleValue = Value.get<double>();
                if (!std::isfinite(DoubleValue) ||
                    DoubleValue > static_cast<double>(
                        std::numeric_limits<float>::max()) ||
                    DoubleValue < -static_cast<double>(
                        std::numeric_limits<float>::max()))
                {
                    return std::nullopt;
                }

                const float FloatValue = static_cast<float>(DoubleValue);
                if (!std::isfinite(FloatValue))
                {
                    return std::nullopt;
                }

                return FloatValue;
            }
            catch (const nlohmann::json::exception&)
            {
                return std::nullopt;
            }
        }

        template<typename ValueType>
        [[nodiscard]] inline LiteralValue MakeLiteralValue(ValueType Value)
        {
            using StoredValueType = std::decay_t<ValueType>;
            return LiteralValue(LiteralValue::Data(
                std::in_place_type<StoredValueType>,
                std::move(Value)
            ));
        }

        [[nodiscard]] inline std::optional<LiteralValue> ConvertDefaultValue(
            const JsonValue& Value,
            const std::string& SourceType,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context
        )
        {
            if (SourceType == "bool")
            {
                const std::optional<std::uint64_t> IntegerValue =
                    TryConvertUnsignedInteger(Value);
                if (IntegerValue.has_value() && *IntegerValue <= 1U)
                {
                    return MakeLiteralValue(*IntegerValue == 1U);
                }
            }
            else if (SourceType == "float")
            {
                const std::optional<float> FloatValue =
                    TryConvertFiniteFloat32(Value);
                if (FloatValue.has_value())
                {
                    return MakeLiteralValue(static_cast<double>(*FloatValue));
                }
            }
            else if (SourceType == "vec3")
            {
                if (Value.is_array() && Value.size() == 3U)
                {
                    const std::optional<float> X = TryConvertFiniteFloat32(Value[0U]);
                    const std::optional<float> Y = TryConvertFiniteFloat32(Value[1U]);
                    const std::optional<float> Z = TryConvertFiniteFloat32(Value[2U]);
                    if (X.has_value() && Y.has_value() && Z.has_value())
                    {
                        return MakeLiteralValue(Vector3Value{*X, *Y, *Z});
                    }
                }
            }
            else if (SourceType == "guid")
            {
                const std::optional<std::uint64_t> IntegerValue =
                    TryConvertUnsignedInteger(Value);
                if (IntegerValue.has_value())
                {
                    return MakeLiteralValue(GuidValue{*IntegerValue});
                }
            }
            else if (SourceType == "config_id")
            {
                const std::optional<std::uint64_t> IntegerValue =
                    TryConvertUnsignedInteger(Value);
                if (IntegerValue.has_value())
                {
                    return MakeLiteralValue(ConfigIdValue{*IntegerValue});
                }
            }
            else if (SourceType == "faction")
            {
                const std::optional<std::uint64_t> IntegerValue =
                    TryConvertUnsignedInteger(Value);
                if (IntegerValue.has_value())
                {
                    return MakeLiteralValue(FactionValue{*IntegerValue});
                }
            }

            AddUnsupportedFormDiagnostic(
                PendingDiagnostics,
                "Source defaultValue is not representable for source type: " +
                    SourceType + ".",
                Context
            );
            return std::nullopt;
        }

        [[nodiscard]] inline std::optional<ParsedPin> ParsePin(
            const JsonValue& Pin,
            bool IsInput,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context
        )
        {
            if (!Pin.is_object())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source pin must be an object.",
                    Context
                );
                return std::nullopt;
            }

            bool Valid = ValidateKnownFields(
                Pin,
                PinFields,
                PendingDiagnostics,
                Context
            );

            const auto ReadKind = [&]()
            {
                return ReadRequiredString(
                    Pin, "kind", PendingDiagnostics, Context);
            };
            const std::optional<std::string> Kind = ReadKind();
            if (!Kind.has_value() || *Kind != (IsInput ? "input" : "output"))
            {
                if (Kind.has_value())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Source pin kind does not match its input/output array.",
                        Context
                    );
                }
                Valid = false;
            }

            const std::optional<std::uint64_t> Index = ReadRequiredUnsignedInteger(
                Pin, "index", PendingDiagnostics, Context);
            if (!Index.has_value())
            {
                Valid = false;
            }

            const std::optional<std::string> SourceType = ReadRequiredString(
                Pin, "type", PendingDiagnostics, Context);
            std::optional<SourceTypeMapping> TypeMapping;
            if (!SourceType.has_value())
            {
                Valid = false;
            }
            else
            {
                TypeMapping = GetSourceTypeMapping(*SourceType);
                if (!TypeMapping.has_value())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Source pin type is outside the bounded P5.3 type set: " +
                            *SourceType + ".",
                        Context
                    );
                    Valid = false;
                }
            }

            const std::optional<std::string> Name = ReadRequiredString(
                Pin, "name", PendingDiagnostics, Context);
            if (!Name.has_value())
            {
                Valid = false;
            }

            const std::optional<std::uint64_t> ClientVariableType =
                ReadRequiredUnsignedInteger(
                    Pin, "clientVarType", PendingDiagnostics, Context);
            if (!ClientVariableType.has_value())
            {
                Valid = false;
            }
            else if (TypeMapping.has_value() &&
                *ClientVariableType != TypeMapping->ClientVariableType)
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source pin type and clientVarType do not agree.",
                    Context
                );
                Valid = false;
            }

            if (Pin.contains("reflective"))
            {
                if (!Pin.at("reflective").is_boolean() ||
                    Pin.at("reflective").get<bool>())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Reflective source pins are outside the bounded P5.3 form.",
                        Context
                    );
                    Valid = false;
                }
            }

            if (Pin.contains("indexOfConcrete"))
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Concrete-variant source pin fields are unsupported.",
                    Context
                );
                Valid = false;
            }

            if (Pin.contains("variants"))
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Concrete-variant source pin fields are unsupported.",
                    Context
                );
                Valid = false;
            }

            if (Pin.contains("i2Index"))
            {
                const std::optional<std::uint64_t> I2Index =
                    TryConvertUnsignedInteger(Pin.at("i2Index"));
                if (!I2Index.has_value())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "i2Index must be a non-negative integer matching index.",
                        Context
                    );
                    Valid = false;
                }
                else if (Index.has_value() && *I2Index != *Index)
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "i2Index must match index.",
                        Context
                    );
                    Valid = false;
                }
            }

            std::optional<LiteralValue> DefaultValue;
            if (IsInput)
            {
                const JsonValue* Default = FindMember(Pin, "defaultValue");
                if (Default == nullptr)
                {
                    AddMissingFieldDiagnostic(
                        PendingDiagnostics, "defaultValue", Context);
                    Valid = false;
                }
                else if (SourceType.has_value() && TypeMapping.has_value())
                {
                    DefaultValue = ConvertDefaultValue(
                        *Default,
                        *SourceType,
                        PendingDiagnostics,
                        Context
                    );
                    if (!DefaultValue.has_value())
                    {
                        Valid = false;
                    }
                }
                else
                {
                    Valid = false;
                }

                const JsonValue* Connectable = FindMember(Pin, "connectable");
                if (Connectable == nullptr)
                {
                    AddMissingFieldDiagnostic(
                        PendingDiagnostics, "connectable", Context);
                    Valid = false;
                }
                else if (!Connectable->is_boolean())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Input connectable must be a boolean.",
                        Context
                    );
                    Valid = false;
                }

                const JsonValue* ConnectionType =
                    FindMember(Pin, "connectionType");
                if (ConnectionType != nullptr)
                {
                    const std::optional<std::uint64_t> ConvertedConnectionType =
                        TryConvertUnsignedInteger(*ConnectionType);
                    if (!ConvertedConnectionType.has_value())
                    {
                        AddUnsupportedFormDiagnostic(
                            PendingDiagnostics,
                            "connectionType must match clientVarType.",
                            Context
                        );
                        Valid = false;
                    }
                    else if (ClientVariableType.has_value() &&
                        *ConvertedConnectionType != *ClientVariableType)
                    {
                        AddUnsupportedFormDiagnostic(
                            PendingDiagnostics,
                            "connectionType must match clientVarType.",
                            Context
                        );
                        Valid = false;
                    }
                }
            }
            else
            {
                if (Pin.contains("defaultValue"))
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Output pins must not contain defaultValue.",
                        Context
                    );
                    Valid = false;
                }

                const std::optional<std::uint64_t> ConnectionType =
                    ReadRequiredUnsignedInteger(
                        Pin, "connectionType", PendingDiagnostics, Context);
                if (!ConnectionType.has_value())
                {
                    Valid = false;
                }
                else if (ClientVariableType.has_value() &&
                    *ConnectionType != *ClientVariableType)
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "connectionType must match clientVarType.",
                        Context
                    );
                    Valid = false;
                }

                const JsonValue* Connectable = FindMember(Pin, "connectable");
                if (Connectable != nullptr && !Connectable->is_boolean())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "Output connectable must be a boolean when present.",
                        Context
                    );
                    Valid = false;
                }
            }

            if (!Valid || !Index.has_value() || !Name.has_value() ||
                !TypeMapping.has_value())
            {
                return std::nullopt;
            }

            return ParsedPin{
                .Index = *Index,
                .NormalizedPin = NormalizedPinRecord(
                    *Name,
                    TypeMapping->NormalizedType,
                    IsInput ? PinDirection::Input : PinDirection::Output,
                    PinCategory::Data,
                    PinCardinality::Single,
                    IsInput,
                    std::move(DefaultValue)
                )
            };
        }

        inline void ParsePinArray(
            const JsonValue& PinArray,
            bool IsInput,
            std::vector<NormalizedPinRecord>& NormalizedPins,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            const SourceRecordContext& Context,
            bool& Valid
        )
        {
            std::optional<std::uint64_t> PreviousIndex;
            for (const JsonValue& Pin : PinArray)
            {
                if (Pin.is_object())
                {
                    const JsonValue* IndexValue = FindMember(Pin, "index");
                    if (IndexValue != nullptr)
                    {
                        const std::optional<std::uint64_t> Index =
                            TryConvertUnsignedInteger(*IndexValue);
                        if (Index.has_value())
                        {
                            if (PreviousIndex.has_value() && *Index <= *PreviousIndex)
                            {
                                AddUnsupportedFormDiagnostic(
                                    PendingDiagnostics,
                                    "Source pin indexes must be strictly increasing.",
                                    Context
                                );
                                Valid = false;
                            }
                            PreviousIndex = *Index;
                        }
                    }
                }

                const std::optional<ParsedPin> Parsed = ParsePin(
                    Pin,
                    IsInput,
                    PendingDiagnostics,
                    Context
                );
                if (!Parsed.has_value())
                {
                    Valid = false;
                }
                else
                {
                    NormalizedPins.push_back(Parsed->NormalizedPin);
                }
            }
        }

        [[nodiscard]] inline std::optional<NormalizedNodeDescriptorRecord> ParseRecord(
            const JsonValue& Record,
            const ModeMembership& Mode,
            std::vector<PendingDiagnostic>& PendingDiagnostics
        )
        {
            if (!Record.is_object())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "Source record must be an object."
                );
                return std::nullopt;
            }

            SourceRecordContext Context;
            bool Valid = true;

            const std::optional<std::uint64_t> GenericId =
                ReadRequiredUnsignedInteger(
                    Record, "genericId", PendingDiagnostics, Context);
            if (!GenericId.has_value())
            {
                Valid = false;
            }
            else
            {
                Context.ExternalKey = ToCanonicalDecimal(*GenericId);
                if (Context.ExternalKey.empty())
                {
                    AddUnsupportedFormDiagnostic(
                        PendingDiagnostics,
                        "genericId could not be formatted as canonical decimal text.",
                        Context
                    );
                    Valid = false;
                }
            }

            const std::optional<std::string> SampleFile = ReadRequiredString(
                Record, "sampleFile", PendingDiagnostics, Context);
            if (!SampleFile.has_value())
            {
                Valid = false;
            }
            else if (!Context.ExternalKey.empty())
            {
                Context.Provenance = SourceProvenance(
                    *SampleFile,
                    Context.ExternalKey
                );
            }

            if (!ValidateKnownFields(
                    Record,
                    RecordFields,
                    PendingDiagnostics,
                    Context
                ))
            {
                Valid = false;
            }

            const std::optional<std::string> SubType = ReadRequiredString(
                Record, "subType", PendingDiagnostics, Context);
            if (!SubType.has_value())
            {
                Valid = false;
            }
            else if (*SubType != "bool_filter")
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source record subType is outside the bounded bool_filter form.",
                    Context
                );
                Valid = false;
            }

            if (!ReadRequiredString(
                    Record, "nodeType", PendingDiagnostics, Context
                ).has_value())
            {
                Valid = false;
            }

            const std::optional<std::string> DisplayName = ReadRequiredString(
                Record, "displayName", PendingDiagnostics, Context);
            if (!DisplayName.has_value())
            {
                Valid = false;
            }

            const std::optional<std::uint64_t> GraphType =
                ReadRequiredUnsignedInteger(
                    Record, "graphType", PendingDiagnostics, Context);
            if (!GraphType.has_value())
            {
                Valid = false;
            }
            else if (*GraphType != 20001U)
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source record graphType is outside the bounded bool_filter form.",
                    Context
                );
                Valid = false;
            }

            const JsonValue* ConcreteId = FindMember(Record, "concreteId");
            if (ConcreteId == nullptr)
            {
                AddMissingFieldDiagnostic(
                    PendingDiagnostics, "concreteId", Context);
                Valid = false;
            }
            else if (!ConcreteId->is_null() && !ConcreteId->is_string() &&
                !ConcreteId->is_number())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "concreteId must be a number, string, or null.",
                    Context
                );
                Valid = false;
            }

            const JsonValue* Inputs = FindMember(Record, "inputs");
            if (Inputs == nullptr)
            {
                AddMissingFieldDiagnostic(PendingDiagnostics, "inputs", Context);
                Valid = false;
            }
            else if (!Inputs->is_array())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "inputs must be an array.",
                    Context
                );
                Valid = false;
            }

            const JsonValue* Outputs = FindMember(Record, "outputs");
            if (Outputs == nullptr)
            {
                AddMissingFieldDiagnostic(PendingDiagnostics, "outputs", Context);
                Valid = false;
            }
            else if (!Outputs->is_array())
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "outputs must be an array.",
                    Context
                );
                Valid = false;
            }

            const JsonValue* Flows = FindMember(Record, "flows");
            if (Flows != nullptr && (!Flows->is_array() || !Flows->empty()))
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source flow metadata must be absent or an empty array.",
                    Context
                );
                Valid = false;
            }

            if (GenericId.has_value() && Mode.IsValid &&
                (Mode.BeyondGenericIds.find(*GenericId) == Mode.BeyondGenericIds.end() ||
                    Mode.ClassicGenericIds.find(*GenericId) == Mode.ClassicGenericIds.end()))
            {
                AddUnsupportedFormDiagnostic(
                    PendingDiagnostics,
                    "Source genericId is not available in both bool_filter modes.",
                    Context
                );
                Valid = false;
            }

            std::vector<NormalizedPinRecord> Pins;
            if (Inputs != nullptr && Inputs->is_array())
            {
                ParsePinArray(
                    *Inputs,
                    true,
                    Pins,
                    PendingDiagnostics,
                    Context,
                    Valid
                );
            }
            if (Outputs != nullptr && Outputs->is_array())
            {
                ParsePinArray(
                    *Outputs,
                    false,
                    Pins,
                    PendingDiagnostics,
                    Context,
                    Valid
                );
            }

            if (!Valid || !DisplayName.has_value() || Context.ExternalKey.empty())
            {
                return std::nullopt;
            }

            return NormalizedNodeDescriptorRecord(
                ExternalNodeIdentity(Context.ExternalKey),
                *DisplayName,
                {NodeAvailability::Client},
                std::move(Pins),
                std::nullopt,
                Context.Provenance
            );
        }

        inline bool ParseModeGroup(
            const JsonValue& ModeGroup,
            std::string_view ModeName,
            std::set<std::uint64_t>& GenericIds,
            std::vector<PendingDiagnostic>& PendingDiagnostics,
            ModeMembership& Mode
        )
        {
            if (!ModeGroup.is_object())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "bool_filter mode group must be an object."
                );
                Mode.IsValid = false;
                return false;
            }

            bool Valid = true;
            const JsonValue* Status = FindMember(ModeGroup, "status");
            if (Status == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: " + std::string(ModeName) + ".status."
                );
                Valid = false;
            }
            else if (!Status->is_string() || Status->get<std::string>() != "available")
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "bool_filter mode status must be available."
                );
                Valid = false;
            }

            const JsonValue* GenericIdArray = FindMember(ModeGroup, "genericIds");
            if (GenericIdArray == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: " + std::string(ModeName) +
                        ".genericIds."
                );
                Valid = false;
            }
            else if (!GenericIdArray->is_array())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "bool_filter genericIds must be an array."
                );
                Valid = false;
            }
            else
            {
                for (const JsonValue& Value : *GenericIdArray)
                {
                    const std::optional<std::uint64_t> GenericId =
                        TryConvertUnsignedInteger(Value);
                    if (!GenericId.has_value())
                    {
                        AddPendingDiagnostic(
                            PendingDiagnostics,
                            DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                            "bool_filter genericIds entries must be non-negative integers."
                        );
                        Valid = false;
                    }
                    else
                    {
                        GenericIds.insert(*GenericId);
                    }
                }
            }

            if (!Valid)
            {
                Mode.IsValid = false;
            }

            return Valid;
        }

        inline ModeMembership ParseModeDocument(const JsonValue& ModeDocument, std::vector<PendingDiagnostic>& PendingDiagnostics)
        {
            ModeMembership Mode;
            if (!ModeDocument.is_object())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource,
                    "Mode source document must be a JSON object."
                );
                Mode.IsValid = false;
                return Mode;
            }

            const JsonValue* Format = FindMember(ModeDocument, "format");
            if (Format == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: format."
                );
                Mode.IsValid = false;
            }
            else
            {
                const std::optional<std::uint64_t> FormatValue =
                    TryConvertUnsignedInteger(*Format);
                if (!FormatValue.has_value() || *FormatValue != 1U)
                {
                    AddPendingDiagnostic(
                        PendingDiagnostics,
                        DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                        "Mode document format must be integer 1."
                    );
                    Mode.IsValid = false;
                }
            }

            const JsonValue* Graphs = FindMember(ModeDocument, "graphs");
            if (Graphs == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: graphs."
                );
                Mode.IsValid = false;
                return Mode;
            }
            if (!Graphs->is_object())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "Mode document graphs must be an object."
                );
                Mode.IsValid = false;
                return Mode;
            }

            const JsonValue* BooleanFilter = FindMember(*Graphs, "bool_filter");
            if (BooleanFilter == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: graphs.bool_filter."
                );
                Mode.IsValid = false;
                return Mode;
            }
            if (!BooleanFilter->is_object())
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
                    "Mode document graphs.bool_filter must be an object."
                );
                Mode.IsValid = false;
                return Mode;
            }

            const JsonValue* Beyond = FindMember(*BooleanFilter, "beyond");
            if (Beyond == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: graphs.bool_filter.beyond."
                );
                Mode.IsValid = false;
            }
            else
            {
                ParseModeGroup(
                    *Beyond,
                    "graphs.bool_filter.beyond",
                    Mode.BeyondGenericIds,
                    PendingDiagnostics,
                    Mode
                );
            }

            const JsonValue* Classic = FindMember(*BooleanFilter, "classic");
            if (Classic == nullptr)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MissingGenshinClientBooleanFilterDescriptorSourceField,
                    "Required mode field is missing: graphs.bool_filter.classic."
                );
                Mode.IsValid = false;
            }
            else
            {
                ParseModeGroup(
                    *Classic,
                    "graphs.bool_filter.classic",
                    Mode.ClassicGenericIds,
                    PendingDiagnostics,
                    Mode
                );
            }

            return Mode;
        }

        [[nodiscard]] inline DiagnosticCollection MaterializeDiagnostics(std::vector<PendingDiagnostic> PendingDiagnostics)
        {
            return DescriptorCatalogueDetail::MaterializePendingDiagnostics(
                std::move(PendingDiagnostics)
            );
        }
    }

    /// Adapts the bounded checked-in client bool-filter source package into P5.2 records.
    class GenshinClientBooleanFilterDescriptorSourceAdapter final
    {
    public:
        GenshinClientBooleanFilterDescriptorSourceAdapter() = delete;

        [[nodiscard]] static std::expected<
            std::vector<NormalizedNodeDescriptorRecord>,
            DiagnosticCollection
        > Adapt(
            std::string NodeMetadataJson,
            std::string NodeModesJson
        )
        {
            using namespace GenshinClientBooleanFilterDescriptorSourceAdapterDetail;

            std::vector<PendingDiagnostic> PendingDiagnostics;
            std::optional<JsonValue> NodeDocument;
            std::optional<JsonValue> ModeDocument;

            try
            {
                NodeDocument = JsonValue::parse(NodeMetadataJson);
            }
            catch (const nlohmann::json::exception&)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource,
                    "Node source document contains malformed JSON."
                );
            }

            try
            {
                ModeDocument = JsonValue::parse(NodeModesJson);
            }
            catch (const nlohmann::json::exception&)
            {
                AddPendingDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource,
                    "Mode source document contains malformed JSON."
                );
            }

            ModeMembership Mode;
            if (ModeDocument.has_value())
            {
                Mode = ParseModeDocument(*ModeDocument, PendingDiagnostics);
            }
            else
            {
                Mode.IsValid = false;
            }

            std::vector<NormalizedNodeDescriptorRecord> Records;
            if (NodeDocument.has_value())
            {
                if (!NodeDocument->is_array())
                {
                    AddPendingDiagnostic(
                        PendingDiagnostics,
                        DiagnosticCode::MalformedGenshinClientBooleanFilterDescriptorSource,
                        "Node source document must be a JSON array."
                    );
                }
                else
                {
                    Records.reserve(NodeDocument->size());
                    for (const JsonValue& Record : *NodeDocument)
                    {
                        const std::optional<NormalizedNodeDescriptorRecord> Parsed =
                            ParseRecord(Record, Mode, PendingDiagnostics);
                        if (Parsed.has_value())
                        {
                            Records.push_back(*Parsed);
                        }
                    }
                }
            }

            std::vector<PendingDiagnostic> NormalizedDiagnostics =
                DescriptorCatalogueDetail::CollectNormalizedRecordDiagnostics(Records);
            PendingDiagnostics.insert(
                PendingDiagnostics.end(),
                std::make_move_iterator(NormalizedDiagnostics.begin()),
                std::make_move_iterator(NormalizedDiagnostics.end())
            );

            if (!PendingDiagnostics.empty())
            {
                return std::unexpected(MaterializeDiagnostics(std::move(PendingDiagnostics)));
            }

            std::sort(
                Records.begin(),
                Records.end(),
                [](const NormalizedNodeDescriptorRecord& Left, const NormalizedNodeDescriptorRecord& Right)
                {
                    return Left.GetExternalIdentity() < Right.GetExternalIdentity();
                }
            );

            return Records;
        }
    };
}
