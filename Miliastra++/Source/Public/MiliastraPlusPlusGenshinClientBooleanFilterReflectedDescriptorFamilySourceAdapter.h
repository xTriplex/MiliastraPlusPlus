#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <expected>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptorSpecialization.h"
#include "nlohmann/json.hpp"

namespace MiliastraPlusPlus
{
    namespace GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapterDetail
    {
        using JsonValue = nlohmann::json;
        using PendingDiagnostic = DescriptorCatalogueDetail::PendingDiagnostic;

        struct SourceContext final
        {
            std::string ExternalKey;
            std::optional<SourceProvenance> Provenance;
        };

        struct ModeMembership final
        {
            bool Valid = true;
            std::set<std::uint64_t> BeyondGenericIds;
            std::set<std::uint64_t> ClassicGenericIds;
        };

        inline constexpr std::array<std::string_view, 12U> RecordFields = {
            "subType",
            "nodeType",
            "displayName",
            "graphType",
            "genericId",
            "concreteId",
            "inputs",
            "outputs",
            "sampleFile",
            "reflectMap",
            "specialKind",
            "flows"
        };

        inline constexpr std::array<std::string_view, 6U> InputPinFields = {
            "index",
            "kind",
            "type",
            "reflective",
            "name",
            "connectable"
        };

        inline constexpr std::array<std::string_view, 5U> OutputPinFields = {
            "index",
            "kind",
            "type",
            "reflective",
            "name"
        };

        inline constexpr std::array<std::string_view, 3U> VariantFields = {
            "concreteId",
            "variantKey",
            "pins"
        };

        inline constexpr std::array<std::string_view, 7U> InputBindingFields = {
            "index",
            "kind",
            "type",
            "indexOfConcrete",
            "clientVarType",
            "connectable",
            "connectionType"
        };

        inline constexpr std::array<std::string_view, 6U> OutputBindingFields = {
            "index",
            "kind",
            "type",
            "indexOfConcrete",
            "clientVarType",
            "connectionType"
        };

        inline void AddDiagnostic(std::vector<PendingDiagnostic>& Diagnostics, DiagnosticCode Code, std::string Message, const SourceContext& Context = {})
        {
            Diagnostics.push_back({
                .ExternalKey = Context.ExternalKey,
                .PrimarySourceProvenance = Context.Provenance,
                .RelatedSourceProvenance = std::nullopt,
                .Code = Code,
                .Message = std::move(Message)
            });
        }

        inline void AddMissingField(std::vector<PendingDiagnostic>& Diagnostics, std::string_view Field, const SourceContext& Context = {})
        {
            AddDiagnostic(
                Diagnostics,
                DiagnosticCode::MissingGenshinClientBooleanFilterReflectedDescriptorFamilySourceField,
                "Required reflected-family source field is missing: " +
                    std::string(Field) + ".",
                Context
            );
        }

        inline void AddUnsupported(std::vector<PendingDiagnostic>& Diagnostics, std::string Message, const SourceContext& Context = {})
        {
            AddDiagnostic(
                Diagnostics,
                DiagnosticCode::UnsupportedGenshinClientBooleanFilterReflectedDescriptorFamilySourceForm,
                std::move(Message),
                Context
            );
        }

        [[nodiscard]] inline const JsonValue* FindMember(const JsonValue& Object, std::string_view Name)
        {
            const auto Iterator = Object.find(std::string(Name));
            return Iterator == Object.end() ? nullptr : &(*Iterator);
        }

        template<std::size_t Count>
        [[nodiscard]] inline bool ValidateKnownFields(
            const JsonValue& Object,
            const std::array<std::string_view, Count>& AllowedFields,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            bool Valid = true;
            for (auto Iterator = Object.begin(); Iterator != Object.end(); ++Iterator)
            {
                const std::string_view Field = Iterator.key();
                if (std::find(
                        AllowedFields.begin(),
                        AllowedFields.end(),
                        Field
                    ) == AllowedFields.end())
                {
                    AddUnsupported(
                        Diagnostics,
                        "Unsupported reflected-family source field: " +
                            std::string(Field) + ".",
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
                    const std::int64_t Signed = Value.get<std::int64_t>();
                    if (Signed >= 0)
                    {
                        return static_cast<std::uint64_t>(Signed);
                    }
                }
            }
            catch (const nlohmann::json::exception&)
            {
            }

            return std::nullopt;
        }

        [[nodiscard]] inline std::string ToCanonicalDecimal(std::uint64_t Value)
        {
            std::array<char, 32U> Buffer{};
            const auto Result = std::to_chars(
                Buffer.data(),
                Buffer.data() + Buffer.size(),
                Value
            );
            return std::string(Buffer.data(), Result.ptr);
        }

        [[nodiscard]] inline std::optional<std::uint64_t> ReadUnsignedInteger(
            const JsonValue& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            const JsonValue* Value = FindMember(Object, Field);
            if (Value == nullptr)
            {
                AddMissingField(Diagnostics, Field, Context);
                return std::nullopt;
            }

            const std::optional<std::uint64_t> Converted =
                TryConvertUnsignedInteger(*Value);
            if (!Converted.has_value())
            {
                AddUnsupported(
                    Diagnostics,
                    "Reflected-family source field must be a non-negative integer: " +
                        std::string(Field) + ".",
                    Context
                );
            }
            return Converted;
        }

        [[nodiscard]] inline std::optional<std::string> ReadString(
            const JsonValue& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            const JsonValue* Value = FindMember(Object, Field);
            if (Value == nullptr)
            {
                AddMissingField(Diagnostics, Field, Context);
                return std::nullopt;
            }
            if (!Value->is_string())
            {
                AddUnsupported(
                    Diagnostics,
                    "Reflected-family source field must be a string: " +
                        std::string(Field) + ".",
                    Context
                );
                return std::nullopt;
            }

            std::string Result = Value->get<std::string>();
            if (Result.empty())
            {
                AddUnsupported(
                    Diagnostics,
                    "Reflected-family source field must not be empty: " +
                        std::string(Field) + ".",
                    Context
                );
                return std::nullopt;
            }
            return Result;
        }

        [[nodiscard]] inline std::optional<bool> ReadBoolean(
            const JsonValue& Object,
            std::string_view Field,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            const JsonValue* Value = FindMember(Object, Field);
            if (Value == nullptr)
            {
                AddMissingField(Diagnostics, Field, Context);
                return std::nullopt;
            }
            if (!Value->is_boolean())
            {
                AddUnsupported(
                    Diagnostics,
                    "Reflected-family source field must be boolean: " +
                        std::string(Field) + ".",
                    Context
                );
                return std::nullopt;
            }
            return Value->get<bool>();
        }

        inline bool ParseModeGroup(
            const JsonValue& Group,
            std::string_view Name,
            std::set<std::uint64_t>& GenericIds,
            ModeMembership& Mode,
            std::vector<PendingDiagnostic>& Diagnostics
        )
        {
            if (!Group.is_object())
            {
                AddUnsupported(
                    Diagnostics,
                    "Required bool-filter mode group must be an object."
                );
                Mode.Valid = false;
                return false;
            }

            bool Valid = true;
            const JsonValue* Status = FindMember(Group, "status");
            if (Status == nullptr)
            {
                AddMissingField(Diagnostics, std::string(Name) + ".status");
                Valid = false;
            }
            else if (!Status->is_string() || Status->get<std::string>() != "available")
            {
                AddUnsupported(Diagnostics, "Bool-filter mode status must be available.");
                Valid = false;
            }

            const JsonValue* Values = FindMember(Group, "genericIds");
            if (Values == nullptr)
            {
                AddMissingField(Diagnostics, std::string(Name) + ".genericIds");
                Valid = false;
            }
            else if (!Values->is_array())
            {
                AddUnsupported(Diagnostics, "Bool-filter genericIds must be an array.");
                Valid = false;
            }
            else
            {
                for (const JsonValue& Value : *Values)
                {
                    const std::optional<std::uint64_t> GenericId =
                        TryConvertUnsignedInteger(Value);
                    if (!GenericId.has_value())
                    {
                        AddUnsupported(
                            Diagnostics,
                            "Bool-filter genericIds entries must be non-negative integers."
                        );
                        Valid = false;
                    }
                    else
                    {
                        GenericIds.insert(*GenericId);
                    }
                }
            }

            Mode.Valid = Mode.Valid && Valid;
            return Valid;
        }

        [[nodiscard]] inline ModeMembership ParseModeDocument(const JsonValue& Document, std::vector<PendingDiagnostic>& Diagnostics)
        {
            ModeMembership Mode;
            if (!Document.is_object())
            {
                AddDiagnostic(
                    Diagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
                    "Mode source document must be a JSON object."
                );
                Mode.Valid = false;
                return Mode;
            }

            const JsonValue* Format = FindMember(Document, "format");
            if (Format == nullptr)
            {
                AddMissingField(Diagnostics, "format");
                Mode.Valid = false;
            }
            else
            {
                const auto FormatValue = TryConvertUnsignedInteger(*Format);
                if (!FormatValue.has_value() || *FormatValue != 1U)
                {
                    AddUnsupported(Diagnostics, "Mode document format must be integer 1.");
                    Mode.Valid = false;
                }
            }

            const JsonValue* Graphs = FindMember(Document, "graphs");
            if (Graphs == nullptr)
            {
                AddMissingField(Diagnostics, "graphs");
                Mode.Valid = false;
                return Mode;
            }
            if (!Graphs->is_object())
            {
                AddUnsupported(Diagnostics, "Mode document graphs must be an object.");
                Mode.Valid = false;
                return Mode;
            }

            const JsonValue* BooleanFilter = FindMember(*Graphs, "bool_filter");
            if (BooleanFilter == nullptr)
            {
                AddMissingField(Diagnostics, "graphs.bool_filter");
                Mode.Valid = false;
                return Mode;
            }
            if (!BooleanFilter->is_object())
            {
                AddUnsupported(
                    Diagnostics,
                    "Mode document graphs.bool_filter must be an object."
                );
                Mode.Valid = false;
                return Mode;
            }

            const JsonValue* Beyond = FindMember(*BooleanFilter, "beyond");
            if (Beyond == nullptr)
            {
                AddMissingField(Diagnostics, "graphs.bool_filter.beyond");
                Mode.Valid = false;
            }
            else
            {
                ParseModeGroup(
                    *Beyond,
                    "graphs.bool_filter.beyond",
                    Mode.BeyondGenericIds,
                    Mode,
                    Diagnostics
                );
            }

            const JsonValue* Classic = FindMember(*BooleanFilter, "classic");
            if (Classic == nullptr)
            {
                AddMissingField(Diagnostics, "graphs.bool_filter.classic");
                Mode.Valid = false;
            }
            else
            {
                ParseModeGroup(
                    *Classic,
                    "graphs.bool_filter.classic",
                    Mode.ClassicGenericIds,
                    Mode,
                    Diagnostics
                );
            }

            return Mode;
        }

        [[nodiscard]] inline std::optional<DescriptorSpecializationPin> ParseBasePin(
            const JsonValue& Pin,
            bool Input,
            std::uint64_t ExpectedIndex,
            std::string_view ExpectedName,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            if (!Pin.is_object())
            {
                AddUnsupported(Diagnostics, "Family pin must be an object.", Context);
                return std::nullopt;
            }

            if (Input)
            {
                static_cast<void>(ValidateKnownFields(
                    Pin,
                    InputPinFields,
                    Diagnostics,
                    Context
                ));
            }
            else
            {
                static_cast<void>(ValidateKnownFields(
                    Pin,
                    OutputPinFields,
                    Diagnostics,
                    Context
                ));
            }
            const auto Index = ReadUnsignedInteger(Pin, "index", Diagnostics, Context);
            const auto Kind = ReadString(Pin, "kind", Diagnostics, Context);
            const auto Type = ReadString(Pin, "type", Diagnostics, Context);
            const auto Reflected = ReadBoolean(Pin, "reflective", Diagnostics, Context);
            const auto Name = ReadString(Pin, "name", Diagnostics, Context);
            std::optional<bool> Connectable;
            if (Input)
            {
                Connectable = ReadBoolean(Pin, "connectable", Diagnostics, Context);
            }

            bool Valid = Index.has_value() && *Index == ExpectedIndex &&
                Kind.has_value() && *Kind == (Input ? "input" : "output") &&
                Type.has_value() && *Type == "generic" &&
                Reflected.has_value() && *Reflected && Name.has_value() &&
                *Name == ExpectedName &&
                (!Input || (Connectable.has_value() && *Connectable));
            if (!Valid)
            {
                AddUnsupported(
                    Diagnostics,
                    "Family pin does not match the selected reflected source form.",
                    Context
                );
                return std::nullopt;
            }

            return DescriptorSpecializationPin(
                *Name,
                std::nullopt,
                true,
                Input ? PinDirection::Input : PinDirection::Output,
                PinCategory::Data,
                PinCardinality::Single,
                false
            );
        }

        [[nodiscard]] inline std::optional<DescriptorSpecializationPinBinding> ParseBinding(
            const JsonValue& Pin,
            std::uint64_t ConcreteId,
            std::vector<bool>& SeenPins,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            if (!Pin.is_object())
            {
                AddUnsupported(Diagnostics, "Variant binding must be an object.", Context);
                return std::nullopt;
            }

            const auto Kind = ReadString(Pin, "kind", Diagnostics, Context);
            const bool Input = Kind.has_value() && *Kind == "input";
            const bool Output = Kind.has_value() && *Kind == "output";
            if (!Input && !Output)
            {
                AddUnsupported(
                    Diagnostics,
                    "Variant binding kind must be input or output.",
                    Context
                );
                return std::nullopt;
            }

            if (Input)
            {
                static_cast<void>(ValidateKnownFields(
                    Pin,
                    InputBindingFields,
                    Diagnostics,
                    Context
                ));
            }
            else
            {
                static_cast<void>(ValidateKnownFields(
                    Pin,
                    OutputBindingFields,
                    Diagnostics,
                    Context
                ));
            }
            const auto SourceIndex = ReadUnsignedInteger(
                Pin, "index", Diagnostics, Context);
            const auto Type = ReadString(Pin, "type", Diagnostics, Context);
            const auto IndexOfConcrete = ReadUnsignedInteger(
                Pin, "indexOfConcrete", Diagnostics, Context);
            const auto ClientVariableType = ReadUnsignedInteger(
                Pin, "clientVarType", Diagnostics, Context);
            const auto ConnectionType = ReadUnsignedInteger(
                Pin, "connectionType", Diagnostics, Context);
            std::optional<bool> Connectable;
            if (Input)
            {
                Connectable = ReadBoolean(Pin, "connectable", Diagnostics, Context);
            }

            std::optional<std::uint32_t> FamilyPinIndex;
            if (SourceIndex.has_value())
            {
                if (Input && *SourceIndex <= 1U)
                {
                    FamilyPinIndex = static_cast<std::uint32_t>(*SourceIndex);
                }
                else if (Output && *SourceIndex == 0U)
                {
                    FamilyPinIndex = 2U;
                }
            }

            const bool IntegerVariant = ConcreteId == 1011U;
            const std::string_view ExpectedType = IntegerVariant ? "int" : "float";
            const std::uint64_t ExpectedCode = IntegerVariant ? 3U : 7U;
            const std::uint64_t ExpectedConcreteIndex = IntegerVariant ? 0U : 1U;
            const bool Valid = FamilyPinIndex.has_value() && Type.has_value() &&
                *Type == ExpectedType && IndexOfConcrete.has_value() &&
                *IndexOfConcrete == ExpectedConcreteIndex &&
                ClientVariableType.has_value() && *ClientVariableType == ExpectedCode &&
                ConnectionType.has_value() && *ConnectionType == ExpectedCode &&
                (!Input || (Connectable.has_value() && *Connectable));
            if (!Valid)
            {
                AddUnsupported(
                    Diagnostics,
                    "Variant binding does not match its selected concrete source form.",
                    Context
                );
                return std::nullopt;
            }

            if (SeenPins[*FamilyPinIndex])
            {
                AddUnsupported(
                    Diagnostics,
                    "Variant contains a duplicate reflected pin binding.",
                    Context
                );
                return std::nullopt;
            }
            SeenPins[*FamilyPinIndex] = true;

            return DescriptorSpecializationPinBinding(
                PinIndex(*FamilyPinIndex),
                IntegerVariant ? TypeDesc::Integer() : TypeDesc::Float()
            );
        }

        [[nodiscard]] inline std::optional<DescriptorSpecializationVariant> ParseVariant(
            const JsonValue& Variant,
            const std::string& FamilyKey,
            std::vector<PendingDiagnostic>& Diagnostics,
            const SourceContext& Context
        )
        {
            if (!Variant.is_object())
            {
                AddUnsupported(Diagnostics, "Reflect-map variant must be an object.", Context);
                return std::nullopt;
            }

            static_cast<void>(ValidateKnownFields(
                Variant,
                VariantFields,
                Diagnostics,
                Context
            ));
            const auto ConcreteId = ReadUnsignedInteger(
                Variant, "concreteId", Diagnostics, Context);
            const auto VariantKey = ReadString(
                Variant, "variantKey", Diagnostics, Context);
            const JsonValue* Pins = FindMember(Variant, "pins");
            if (Pins == nullptr)
            {
                AddMissingField(Diagnostics, "pins", Context);
            }
            else if (!Pins->is_array())
            {
                AddUnsupported(Diagnostics, "Variant pins must be an array.", Context);
            }

            if (!ConcreteId.has_value() || !VariantKey.has_value() ||
                Pins == nullptr || !Pins->is_array())
            {
                return std::nullopt;
            }

            const bool IntegerVariant = *ConcreteId == 1011U;
            const bool FloatVariant = *ConcreteId == 1012U;
            const std::string ExpectedKey = IntegerVariant ? "3,3" : "7,7";
            if ((!IntegerVariant && !FloatVariant) || *VariantKey != ExpectedKey ||
                Pins->size() != 3U)
            {
                AddUnsupported(
                    Diagnostics,
                    "Reflect-map variant does not match a selected concrete ID/key pair.",
                    Context
                );
                return std::nullopt;
            }

            std::vector<bool> SeenPins(3U, false);
            std::vector<DescriptorSpecializationPinBinding> Bindings;
            Bindings.reserve(3U);
            for (const JsonValue& Pin : *Pins)
            {
                const auto Binding = ParseBinding(
                    Pin,
                    *ConcreteId,
                    SeenPins,
                    Diagnostics,
                    Context
                );
                if (Binding.has_value())
                {
                    Bindings.push_back(*Binding);
                }
            }

            if (!std::all_of(SeenPins.begin(), SeenPins.end(), [](bool Seen)
                {
                    return Seen;
                }))
            {
                AddUnsupported(
                    Diagnostics,
                    "Variant must bind every selected reflected family pin exactly once.",
                    Context
                );
                return std::nullopt;
            }

            const std::string ConcreteKey = ToCanonicalDecimal(*ConcreteId);
            const std::string ConcreteExternalKey =
                "family=" + FamilyKey + ";concrete=" + ConcreteKey +
                ";variant-bytes=" + ToCanonicalDecimal(VariantKey->size()) +
                ":" + *VariantKey;
            return DescriptorSpecializationVariant(
                ExternalNodeIdentity(ConcreteExternalKey),
                *VariantKey,
                std::move(Bindings)
            );
        }

        [[nodiscard]] inline std::optional<DescriptorSpecializationFamily> ParseRecord(
            const JsonValue& Record,
            const ModeMembership& Mode,
            std::vector<PendingDiagnostic>& Diagnostics
        )
        {
            SourceContext Context;
            if (!Record.is_object())
            {
                AddDiagnostic(
                    Diagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
                    "Node source record must be a JSON object."
                );
                return std::nullopt;
            }

            const auto GenericId = ReadUnsignedInteger(
                Record, "genericId", Diagnostics, Context);
            if (GenericId.has_value())
            {
                Context.ExternalKey = ToCanonicalDecimal(*GenericId);
            }
            const auto SampleFile = ReadString(
                Record, "sampleFile", Diagnostics, Context);
            if (GenericId.has_value() && SampleFile.has_value())
            {
                Context.Provenance = SourceProvenance(
                    *SampleFile,
                    Context.ExternalKey
                );
            }

            static_cast<void>(ValidateKnownFields(
                Record,
                RecordFields,
                Diagnostics,
                Context
            ));
            const auto SubType = ReadString(Record, "subType", Diagnostics, Context);
            const auto NodeType = ReadString(Record, "nodeType", Diagnostics, Context);
            const auto DisplayName = ReadString(
                Record, "displayName", Diagnostics, Context);
            const auto GraphType = ReadUnsignedInteger(
                Record, "graphType", Diagnostics, Context);
            const auto SpecialKind = ReadString(
                Record, "specialKind", Diagnostics, Context);

            bool RecordShapeValid = SubType.has_value() && *SubType == "bool_filter" &&
                NodeType.has_value() && *NodeType == "get_random_number" &&
                GraphType.has_value() && *GraphType == 20001U &&
                GenericId.has_value() && *GenericId == 200032U &&
                SpecialKind.has_value() && *SpecialKind == "reflect";
            if (!RecordShapeValid)
            {
                AddUnsupported(
                    Diagnostics,
                    "Node record is outside the selected reflected bool-filter family.",
                    Context
                );
            }

            const JsonValue* ConcreteId = FindMember(Record, "concreteId");
            if (ConcreteId == nullptr)
            {
                AddMissingField(Diagnostics, "concreteId", Context);
                RecordShapeValid = false;
            }
            else if (!ConcreteId->is_null())
            {
                AddUnsupported(
                    Diagnostics,
                    "Selected reflected family concreteId must be null.",
                    Context
                );
                RecordShapeValid = false;
            }

            const JsonValue* Flows = FindMember(Record, "flows");
            if (Flows == nullptr)
            {
                AddMissingField(Diagnostics, "flows", Context);
                RecordShapeValid = false;
            }
            else if (!Flows->is_array() || !Flows->empty())
            {
                AddUnsupported(
                    Diagnostics,
                    "Selected reflected family flows must be an empty array.",
                    Context
                );
                RecordShapeValid = false;
            }

            if (!Mode.Valid || !GenericId.has_value() ||
                !Mode.BeyondGenericIds.contains(*GenericId) ||
                !Mode.ClassicGenericIds.contains(*GenericId))
            {
                AddUnsupported(
                    Diagnostics,
                    "Selected reflected family must be available in both bool-filter modes.",
                    Context
                );
                RecordShapeValid = false;
            }

            const JsonValue* Inputs = FindMember(Record, "inputs");
            const JsonValue* Outputs = FindMember(Record, "outputs");
            const JsonValue* ReflectMap = FindMember(Record, "reflectMap");
            if (Inputs == nullptr)
            {
                AddMissingField(Diagnostics, "inputs", Context);
            }
            if (Outputs == nullptr)
            {
                AddMissingField(Diagnostics, "outputs", Context);
            }
            if (ReflectMap == nullptr)
            {
                AddMissingField(Diagnostics, "reflectMap", Context);
            }
            if (Inputs == nullptr || Outputs == nullptr || ReflectMap == nullptr)
            {
                return std::nullopt;
            }
            if (!Inputs->is_array() || Inputs->size() != 2U ||
                !Outputs->is_array() || Outputs->size() != 1U ||
                !ReflectMap->is_array() || ReflectMap->size() != 2U)
            {
                AddUnsupported(
                    Diagnostics,
                    "Selected reflected family requires two inputs, one output, and two variants.",
                    Context
                );
                return std::nullopt;
            }

            std::vector<DescriptorSpecializationPin> Pins;
            const auto Input0 = ParseBasePin(
                (*Inputs)[0U], true, 0U,
                "\xE4\xB8\x8B\xE9\x99\x90", Diagnostics, Context);
            const auto Input1 = ParseBasePin(
                (*Inputs)[1U], true, 1U,
                "\xE4\xB8\x8A\xE9\x99\x90", Diagnostics, Context);
            const auto Output0 = ParseBasePin(
                (*Outputs)[0U], false, 0U,
                "\xE9\x9A\x8F\xE6\x9C\xBA\xE6\x95\xB0", Diagnostics, Context);
            if (Input0.has_value())
            {
                Pins.push_back(*Input0);
            }
            if (Input1.has_value())
            {
                Pins.push_back(*Input1);
            }
            if (Output0.has_value())
            {
                Pins.push_back(*Output0);
            }

            std::vector<DescriptorSpecializationVariant> Variants;
            std::set<std::uint64_t> ConcreteIds;
            std::set<std::string> VariantKeys;
            for (const JsonValue& VariantValue : *ReflectMap)
            {
                const auto ConcreteIdValue = VariantValue.is_object()
                    ? ReadUnsignedInteger(
                        VariantValue, "concreteId", Diagnostics, Context)
                    : std::nullopt;
                const auto VariantKeyValue = VariantValue.is_object()
                    ? ReadString(VariantValue, "variantKey", Diagnostics, Context)
                    : std::nullopt;
                if (ConcreteIdValue.has_value() &&
                    !ConcreteIds.insert(*ConcreteIdValue).second)
                {
                    AddUnsupported(
                        Diagnostics,
                        "Source concreteId is duplicated.",
                        Context
                    );
                }
                if (VariantKeyValue.has_value() &&
                    !VariantKeys.insert(*VariantKeyValue).second)
                {
                    AddUnsupported(
                        Diagnostics,
                        "Source variantKey is duplicated.",
                        Context
                    );
                }

                const auto Variant = ParseVariant(
                    VariantValue,
                    Context.ExternalKey,
                    Diagnostics,
                    Context
                );
                if (Variant.has_value())
                {
                    Variants.push_back(*Variant);
                }
            }

            if (!RecordShapeValid || !DisplayName.has_value() ||
                !Context.Provenance.has_value() || Pins.size() != 3U ||
                Variants.size() != 2U || ConcreteIds.size() != 2U ||
                VariantKeys.size() != 2U)
            {
                return std::nullopt;
            }

            DescriptorSpecializationFamily Family(
                ExternalNodeIdentity(Context.ExternalKey),
                *DisplayName,
                {NodeAvailability::Client},
                std::move(Pins),
                std::nullopt,
                Context.Provenance,
                std::move(Variants)
            );
            if (!Family.IsValid())
            {
                AddUnsupported(
                    Diagnostics,
                    "Selected reflected family does not produce a valid backend-neutral family.",
                    Context
                );
                return std::nullopt;
            }

            return Family;
        }
    }

    class GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter final
    {
    public:
        GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapter() = delete;

        [[nodiscard]] static std::expected<
            std::vector<DescriptorSpecializationFamily>,
            DiagnosticCollection
        > Adapt(
            std::string NodeMetadataJson,
            std::string NodeModesJson
        )
        {
            using namespace
                GenshinClientBooleanFilterReflectedDescriptorFamilySourceAdapterDetail;

            std::vector<PendingDiagnostic> PendingDiagnostics;
            std::optional<JsonValue> NodeDocument;
            std::optional<JsonValue> ModeDocument;
            try
            {
                NodeDocument = JsonValue::parse(NodeMetadataJson);
            }
            catch (const nlohmann::json::exception&)
            {
                AddDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
                    "Node source document contains malformed JSON."
                );
            }

            try
            {
                ModeDocument = JsonValue::parse(NodeModesJson);
            }
            catch (const nlohmann::json::exception&)
            {
                AddDiagnostic(
                    PendingDiagnostics,
                    DiagnosticCode::MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
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
                Mode.Valid = false;
            }

            std::vector<DescriptorSpecializationFamily> Families;
            if (NodeDocument.has_value())
            {
                if (!NodeDocument->is_array())
                {
                    AddDiagnostic(
                        PendingDiagnostics,
                        DiagnosticCode::MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
                        "Node source document must be a JSON array."
                    );
                }
                else if (NodeDocument->size() != 1U)
                {
                    AddUnsupported(
                        PendingDiagnostics,
                        "Node source document must contain exactly one selected family record."
                    );
                }
                else
                {
                    const auto Family = ParseRecord(
                        (*NodeDocument)[0U],
                        Mode,
                        PendingDiagnostics
                    );
                    if (Family.has_value())
                    {
                        Families.push_back(*Family);
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

            return Families;
        }
    };
}
