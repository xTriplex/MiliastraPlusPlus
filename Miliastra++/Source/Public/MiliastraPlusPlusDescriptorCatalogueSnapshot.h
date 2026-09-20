#pragma once

#include <algorithm>
#include <bit>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDescriptorCatalogue.h"
#include "MiliastraPlusPlusDescriptorSpecialization.h"

namespace MiliastraPlusPlus
{
    namespace DescriptorCatalogueSnapshotDetail
    {
        using Json = nlohmann::json;
        using OrderedJson = nlohmann::ordered_json;

        inline DiagnosticCollection Failure(
            DiagnosticCode Code,
            std::string Message
        )
        {
            return DiagnosticCollection{
                Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = Code,
                    .Message = std::move(Message)
                }
            };
        }

        template<typename Type>
        [[nodiscard]] inline std::expected<Type, DiagnosticCollection> FailureExpected(
            DiagnosticCode Code,
            std::string Message
        )
        {
            return std::unexpected(Failure(Code, std::move(Message)));
        }

        class DuplicateMemberSax final : public nlohmann::json_sax<Json>
        {
        public:
            bool null() override { return true; }
            bool boolean(bool) override { return true; }
            bool number_integer(number_integer_t) override { return true; }
            bool number_unsigned(number_unsigned_t) override { return true; }
            bool number_float(number_float_t, const string_t&) override { return true; }
            bool string(string_t&) override { return true; }
            bool binary(binary_t&) override { return true; }

            bool start_object(std::size_t) override
            {
                m_ObjectMembers.emplace_back();
                return true;
            }

            bool key(string_t& Value) override
            {
                if (m_ObjectMembers.empty() ||
                    !m_ObjectMembers.back().insert(Value).second)
                {
                    m_HasDuplicate = true;
                    return false;
                }
                return true;
            }

            bool end_object() override
            {
                if (!m_ObjectMembers.empty())
                {
                    m_ObjectMembers.pop_back();
                }
                return true;
            }

            bool start_array(std::size_t) override { return true; }
            bool end_array() override { return true; }

            bool parse_error(
                std::size_t,
                const std::string&,
                const nlohmann::detail::exception&
            ) override
            {
                m_HasParseError = true;
                return false;
            }

            [[nodiscard]] bool HasDuplicate() const { return m_HasDuplicate; }
            [[nodiscard]] bool HasParseError() const { return m_HasParseError; }

        private:
            std::vector<std::set<std::string>> m_ObjectMembers;
            bool m_HasDuplicate = false;
            bool m_HasParseError = false;
        };

        [[nodiscard]] inline bool HasExactMembers(
            const Json& Object,
            std::initializer_list<std::string_view> Names
        )
        {
            if (!Object.is_object() || Object.size() != Names.size())
            {
                return false;
            }

            for (const std::string_view Name : Names)
            {
                if (!Object.contains(Name))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] inline bool IsUint32(const Json& Value)
        {
            return Value.is_number_unsigned() &&
                Value.get<std::uint64_t>() <= std::numeric_limits<std::uint32_t>::max();
        }

        [[nodiscard]] inline std::expected<
            std::uint32_t,
            DiagnosticCollection
        > ParseUint32(const Json& Value)
        {
            if (!IsUint32(Value))
            {
                return FailureExpected<std::uint32_t>(
                    DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                    "Snapshot integer is not an exact uint32 value."
                );
            }
            return static_cast<std::uint32_t>(Value.get<std::uint64_t>());
        }

        [[nodiscard]] inline std::expected<std::int64_t, DiagnosticCollection>
            ParseInt64(const Json& Value)
        {
            if (Value.is_number_integer())
            {
                return Value.get<std::int64_t>();
            }
            if (Value.is_number_unsigned() &&
                Value.get<std::uint64_t>() <=
                    static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
            {
                return static_cast<std::int64_t>(Value.get<std::uint64_t>());
            }
            return FailureExpected<std::int64_t>(
                DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                "Snapshot integer is not an exact int64 value."
            );
        }

        [[nodiscard]] inline std::expected<std::uint64_t, DiagnosticCollection>
            ParseUint64(const Json& Value)
        {
            if (!Value.is_number_unsigned())
            {
                return FailureExpected<std::uint64_t>(
                    DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                    "Snapshot integer is not an exact uint64 value."
                );
            }
            return Value.get<std::uint64_t>();
        }

        [[nodiscard]] inline std::expected<std::string, DiagnosticCollection>
            ParseString(const Json& Value)
        {
            if (!Value.is_string())
            {
                return FailureExpected<std::string>(
                    DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                    "Snapshot value is not a string."
                );
            }
            return Value.get<std::string>();
        }

        [[nodiscard]] inline OrderedJson EncodeType(const TypeDesc& Type);
        [[nodiscard]] inline std::expected<TypeDesc, DiagnosticCollection>
            DecodeType(const Json& Value, bool AllowEnum);
        [[nodiscard]] inline OrderedJson EncodeLiteral(const LiteralValue& Value);
        [[nodiscard]] inline std::expected<LiteralValue, DiagnosticCollection>
            DecodeLiteral(const Json& Value, bool AllowEnum);
        [[nodiscard]] inline OrderedJson EncodeControl(
            const std::optional<ExecutionControlSchema>& Control
        );
        [[nodiscard]] inline std::expected<
            std::optional<ExecutionControlSchema>,
            DiagnosticCollection
        > DecodeControl(const Json& Value);

        [[nodiscard]] inline std::string HexEncode(
            std::uint64_t Value,
            std::size_t Digits
        )
        {
            constexpr char DigitsTable[] = "0123456789abcdef";
            std::string Result(Digits, '0');
            for (std::size_t Index = 0U; Index < Digits; ++Index)
            {
                const std::size_t Shift = (Digits - Index - 1U) * 4U;
                Result[Index] = DigitsTable[(Value >> Shift) & 0xFU];
            }
            return Result;
        }

        [[nodiscard]] inline std::expected<std::uint64_t, DiagnosticCollection>
            DecodeHex(const Json& Value, std::size_t Digits)
        {
            const auto StringResult = ParseString(Value);
            if (!StringResult.has_value() || StringResult->size() != Digits)
            {
                return FailureExpected<std::uint64_t>(
                    DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                    "Snapshot hexadecimal value has the wrong width."
                );
            }

            std::uint64_t Result = 0U;
            for (const char Character : *StringResult)
            {
                std::uint64_t Nibble = 0U;
                if (Character >= '0' && Character <= '9')
                {
                    Nibble = static_cast<std::uint64_t>(Character - '0');
                }
                else if (Character >= 'a' && Character <= 'f')
                {
                    Nibble = static_cast<std::uint64_t>(Character - 'a' + 10);
                }
                else
                {
                    return FailureExpected<std::uint64_t>(
                        DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                        "Snapshot hexadecimal value is not lowercase canonical hex."
                    );
                }
                Result = (Result << 4U) | Nibble;
            }
            return Result;
        }

        [[nodiscard]] inline OrderedJson EncodeLiteral(const LiteralValue& Value)
        {
            OrderedJson Result = OrderedJson::object();
            std::visit([&Result](const auto& Item)
            {
                using ItemType = std::decay_t<decltype(Item)>;
                if constexpr (std::is_same_v<ItemType, bool>)
                {
                    Result["kind"] = "Boolean";
                    Result["value"] = Item;
                }
                else if constexpr (std::is_same_v<ItemType, std::int64_t>)
                {
                    Result["kind"] = "Integer";
                    Result["value"] = Item;
                }
                else if constexpr (std::is_same_v<ItemType, double>)
                {
                    Result["kind"] = "Float";
                    Result["bits"] = HexEncode(std::bit_cast<std::uint64_t>(Item), 16U);
                }
                else if constexpr (std::is_same_v<ItemType, std::string>)
                {
                    Result["kind"] = "String";
                    Result["value"] = Item;
                }
                else if constexpr (std::is_same_v<ItemType, GuidValue>)
                {
                    Result["kind"] = "GUID";
                    Result["value"] = Item.Value;
                }
                else if constexpr (std::is_same_v<ItemType, Vector3Value>)
                {
                    Result["kind"] = "Vector3";
                    Result["xBits"] = HexEncode(std::bit_cast<std::uint32_t>(Item.X), 8U);
                    Result["yBits"] = HexEncode(std::bit_cast<std::uint32_t>(Item.Y), 8U);
                    Result["zBits"] = HexEncode(std::bit_cast<std::uint32_t>(Item.Z), 8U);
                }
                else if constexpr (std::is_same_v<ItemType, PrefabIdValue>)
                {
                    Result["kind"] = "PrefabId";
                    Result["value"] = Item.Value;
                }
                else if constexpr (std::is_same_v<ItemType, ConfigIdValue>)
                {
                    Result["kind"] = "ConfigId";
                    Result["value"] = Item.Value;
                }
                else if constexpr (std::is_same_v<ItemType, FactionValue>)
                {
                    Result["kind"] = "Faction";
                    Result["value"] = Item.Value;
                }
                else if constexpr (std::is_same_v<ItemType, EnumLiteralValue>)
                {
                    Result["kind"] = "Enum";
                    Result["enumIdentity"] = Item.GetEnumTypeIdentity().GetValue();
                    Result["value"] = Item.GetValue();
                }
            }, Value.GetData());
            return Result;
        }

        [[nodiscard]] inline std::expected<LiteralValue, DiagnosticCollection>
            DecodeLiteral(const Json& Value, bool AllowEnum)
        {
            if (!Value.is_object() || !Value.contains("kind") || !Value["kind"].is_string())
            {
                return FailureExpected<LiteralValue>(
                    DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                    "Snapshot literal is missing its kind."
                );
            }

            const std::string Kind = Value["kind"].get<std::string>();
            if (Kind == "Boolean")
            {
                if (!HasExactMembers(Value, {"kind", "value"}) || !Value["value"].is_boolean())
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Boolean literal shape is invalid.");
                }
                return LiteralValue(LiteralValue::Data{Value["value"].get<bool>()});
            }
            if (Kind == "Integer")
            {
                if (!HasExactMembers(Value, {"kind", "value"}))
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Integer literal shape is invalid.");
                }
                const auto Parsed = ParseInt64(Value["value"]);
                if (!Parsed.has_value()) return std::unexpected(Parsed.error());
                return LiteralValue(LiteralValue::Data{*Parsed});
            }
            if (Kind == "Float")
            {
                if (!HasExactMembers(Value, {"kind", "bits"}))
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Float literal shape is invalid.");
                }
                const auto Bits = DecodeHex(Value["bits"], 16U);
                if (!Bits.has_value()) return std::unexpected(Bits.error());
                return LiteralValue(LiteralValue::Data{std::bit_cast<double>(*Bits)});
            }
            if (Kind == "String")
            {
                if (!HasExactMembers(Value, {"kind", "value"}) || !Value["value"].is_string())
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "String literal shape is invalid.");
                }
                return LiteralValue(LiteralValue::Data{Value["value"].get<std::string>()});
            }
            if (Kind == "Enum")
            {
                if (!AllowEnum ||
                    !HasExactMembers(Value, {"kind", "enumIdentity", "value"}) ||
                    !Value["enumIdentity"].is_string() ||
                    Value["enumIdentity"].get<std::string>().empty())
                {
                    return FailureExpected<LiteralValue>(
                        DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                        "Enum literal shape is invalid for this snapshot version or schema."
                    );
                }
                const auto Parsed = ParseInt64(Value["value"]);
                if (!Parsed.has_value())
                {
                    return std::unexpected(Parsed.error());
                }
                return LiteralValue(LiteralValue::Data{
                    EnumLiteralValue(
                        EnumTypeIdentity(Value["enumIdentity"].get<std::string>()),
                        *Parsed)
                });
            }

            const auto DecodeUnsignedLiteral = [&Value](
                const char* ExpectedKind,
                auto Constructor
            ) -> std::expected<LiteralValue, DiagnosticCollection>
            {
                if (!HasExactMembers(Value, {"kind", "value"}))
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Unsigned literal shape is invalid.");
                }
                const auto Parsed = ParseUint64(Value["value"]);
                if (!Parsed.has_value()) return std::unexpected(Parsed.error());
                return LiteralValue(LiteralValue::Data{Constructor(*Parsed)});
            };

            if (Kind == "GUID") return DecodeUnsignedLiteral("GUID", [](std::uint64_t Value) { return GuidValue{Value}; });
            if (Kind == "PrefabId") return DecodeUnsignedLiteral("PrefabId", [](std::uint64_t Value) { return PrefabIdValue{Value}; });
            if (Kind == "ConfigId") return DecodeUnsignedLiteral("ConfigId", [](std::uint64_t Value) { return ConfigIdValue{Value}; });
            if (Kind == "Faction") return DecodeUnsignedLiteral("Faction", [](std::uint64_t Value) { return FactionValue{Value}; });
            if (Kind == "Vector3")
            {
                if (!HasExactMembers(Value, {"kind", "xBits", "yBits", "zBits"}))
                {
                    return FailureExpected<LiteralValue>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Vector3 literal shape is invalid.");
                }
                const auto X = DecodeHex(Value["xBits"], 8U);
                const auto Y = DecodeHex(Value["yBits"], 8U);
                const auto Z = DecodeHex(Value["zBits"], 8U);
                if (!X.has_value()) return std::unexpected(X.error());
                if (!Y.has_value()) return std::unexpected(Y.error());
                if (!Z.has_value()) return std::unexpected(Z.error());
                return LiteralValue(LiteralValue::Data{Vector3Value{
                    std::bit_cast<float>(static_cast<std::uint32_t>(*X)),
                    std::bit_cast<float>(static_cast<std::uint32_t>(*Y)),
                    std::bit_cast<float>(static_cast<std::uint32_t>(*Z))
                }});
            }

            return FailureExpected<LiteralValue>(
                DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                "Snapshot literal kind is unsupported."
            );
        }

        [[nodiscard]] inline OrderedJson EncodeType(const TypeDesc& Type)
        {
            OrderedJson Result = OrderedJson::object();
            const auto AddKind = [&Result](const char* Kind) { Result["kind"] = Kind; };
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Boolean: AddKind("Boolean"); break;
            case TypeDesc::Kind::Integer: AddKind("Integer"); break;
            case TypeDesc::Kind::Float: AddKind("Float"); break;
            case TypeDesc::Kind::String: AddKind("String"); break;
            case TypeDesc::Kind::Flow: AddKind("Flow"); break;
            case TypeDesc::Kind::Entity: AddKind("Entity"); break;
            case TypeDesc::Kind::GUID: AddKind("GUID"); break;
            case TypeDesc::Kind::Vector3: AddKind("Vector3"); break;
            case TypeDesc::Kind::PrefabId: AddKind("PrefabId"); break;
            case TypeDesc::Kind::ConfigId: AddKind("ConfigId"); break;
            case TypeDesc::Kind::Faction: AddKind("Faction"); break;
            case TypeDesc::Kind::Generic:
                AddKind("Generic");
                Result["parameter"] = Type.GetGenericParameter().GetValue();
                break;
            case TypeDesc::Kind::List:
                AddKind("List");
                Result["element"] = EncodeType(*Type.GetElementType());
                break;
            case TypeDesc::Kind::Dictionary:
                AddKind("Dictionary");
                Result["key"] = EncodeType(*Type.GetKeyType());
                Result["value"] = EncodeType(*Type.GetValueType());
                break;
            case TypeDesc::Kind::StructObject:
                AddKind("StructObject");
                Result["structType"] = Type.GetStructType().GetValue();
                break;
            case TypeDesc::Kind::Enum:
                AddKind("Enum");
                Result["identity"] = Type.GetEnumTypeIdentity().GetValue();
                break;
            case TypeDesc::Kind::Invalid:
                break;
            }
            return Result;
        }

        [[nodiscard]] inline std::expected<TypeDesc, DiagnosticCollection>
            DecodeType(const Json& Value, bool AllowEnum)
        {
            if (!Value.is_object() || !Value.contains("kind") || !Value["kind"].is_string())
            {
                return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot type is missing its kind.");
            }
            const std::string Kind = Value["kind"].get<std::string>();
            if (Kind == "Boolean" || Kind == "Integer" || Kind == "Float" || Kind == "String" ||
                Kind == "Flow" || Kind == "Entity" || Kind == "GUID" || Kind == "Vector3" ||
                Kind == "PrefabId" || Kind == "ConfigId" || Kind == "Faction")
            {
                if (!HasExactMembers(Value, {"kind"}))
                {
                    return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Scalar type has extra or missing members.");
                }
                if (Kind == "Boolean") return TypeDesc::Boolean();
                if (Kind == "Integer") return TypeDesc::Integer();
                if (Kind == "Float") return TypeDesc::Float();
                if (Kind == "String") return TypeDesc::String();
                if (Kind == "Flow") return TypeDesc::Flow();
                if (Kind == "Entity") return TypeDesc::Entity();
                if (Kind == "GUID") return TypeDesc::GUID();
                if (Kind == "Vector3") return TypeDesc::Vector3();
                if (Kind == "PrefabId") return TypeDesc::PrefabId();
                if (Kind == "ConfigId") return TypeDesc::ConfigId();
                return TypeDesc::Faction();
            }
            if (Kind == "Generic")
            {
                if (!HasExactMembers(Value, {"kind", "parameter"}) || !IsUint32(Value["parameter"]) || Value["parameter"] == 0U)
                {
                    return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Generic type shape is invalid.");
                }
                return TypeDesc::Generic(GenericParameterId(Value["parameter"].get<std::uint32_t>()));
            }
            if (Kind == "StructObject")
            {
                if (!HasExactMembers(Value, {"kind", "structType"}) || !IsUint32(Value["structType"]) || Value["structType"] == 0U)
                {
                    return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "StructObject type shape is invalid.");
                }
                return TypeDesc::StructObject(StructTypeId(Value["structType"].get<std::uint32_t>()));
            }
            if (Kind == "Enum")
            {
                if (!AllowEnum ||
                    !HasExactMembers(Value, {"kind", "identity"}) ||
                    !Value["identity"].is_string() ||
                    Value["identity"].get<std::string>().empty())
                {
                    return FailureExpected<TypeDesc>(
                        DiagnosticCode::MalformedDescriptorCatalogueSnapshot,
                        "Enum type shape is invalid for this snapshot version or schema."
                    );
                }
                return TypeDesc::Enum(
                    EnumTypeIdentity(Value["identity"].get<std::string>()));
            }
            if (Kind == "List")
            {
                if (!HasExactMembers(Value, {"kind", "element"}))
                {
                    return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "List type shape is invalid.");
                }
                const auto Element = DecodeType(Value["element"], AllowEnum);
                if (!Element.has_value()) return std::unexpected(Element.error());
                return TypeDesc::List(*Element);
            }
            if (Kind == "Dictionary")
            {
                if (!HasExactMembers(Value, {"kind", "key", "value"}))
                {
                    return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Dictionary type shape is invalid.");
                }
                const auto Key = DecodeType(Value["key"], AllowEnum);
                const auto Item = DecodeType(Value["value"], AllowEnum);
                if (!Key.has_value()) return std::unexpected(Key.error());
                if (!Item.has_value()) return std::unexpected(Item.error());
                return TypeDesc::Dictionary(*Key, *Item);
            }
            return FailureExpected<TypeDesc>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot type kind is unsupported.");
        }

        [[nodiscard]] inline OrderedJson EncodeControl(
            const std::optional<ExecutionControlSchema>& Control
        )
        {
            if (!Control.has_value()) return nullptr;
            OrderedJson Result = OrderedJson::object();
            std::visit([&Result](const auto& Value)
            {
                using ControlType = std::decay_t<decltype(Value)>;
                if constexpr (std::is_same_v<ControlType, EntryControlSchema>)
                {
                    Result["kind"] = "Entry";
                    Result["executionOutput"] = Value.ExecutionOutput.GetValue();
                }
                else if constexpr (std::is_same_v<ControlType, SequenceControlSchema>)
                {
                    Result["kind"] = "Sequence";
                    Result["executionInput"] = Value.ExecutionInput.GetValue();
                    Result["executionOutput"] = Value.ExecutionOutput.GetValue();
                }
                else if constexpr (std::is_same_v<ControlType, BranchControlSchema>)
                {
                    Result["kind"] = "Branch";
                    Result["executionInput"] = Value.ExecutionInput.GetValue();
                    Result["conditionInput"] = Value.ConditionInput.GetValue();
                    Result["trueOutput"] = Value.TrueOutput.GetValue();
                    Result["falseOutput"] = Value.FalseOutput.GetValue();
                }
                else if constexpr (std::is_same_v<ControlType, JoinControlSchema>)
                {
                    Result["kind"] = "Join";
                    Result["executionInput"] = Value.ExecutionInput.GetValue();
                    Result["executionOutput"] = Value.ExecutionOutput.GetValue();
                }
                else if constexpr (std::is_same_v<ControlType, LoopControlSchema>)
                {
                    Result["kind"] = "Loop";
                    Result["executionInput"] = Value.ExecutionInput.GetValue();
                    Result["bodyOutput"] = Value.BodyOutput.GetValue();
                    Result["exitOutput"] = Value.ExitOutput.GetValue();
                    Result["repeatInput"] = Value.RepeatInput.GetValue();
                    Result["breakInput"] = Value.BreakInput.GetValue();
                    Result["exitPolicy"] = Value.ExitPolicy == LoopExitPolicy::Conditional ? "Conditional" : "Unconditional";
                    Result["conditionInput"] = Value.ConditionInput.has_value()
                        ? OrderedJson(Value.ConditionInput->GetValue())
                        : OrderedJson(nullptr);
                }
                else if constexpr (std::is_same_v<ControlType, ReturnControlSchema>)
                {
                    Result["kind"] = "Return";
                    Result["executionInput"] = Value.ExecutionInput.GetValue();
                }
            }, *Control);
            return Result;
        }

        [[nodiscard]] inline std::expected<std::optional<ExecutionControlSchema>, DiagnosticCollection>
            DecodeControl(const Json& Value)
        {
            if (Value.is_null()) return std::optional<ExecutionControlSchema>{};
            if (!Value.is_object() || !Value.contains("kind") || !Value["kind"].is_string())
            {
                return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Execution control requires a kind discriminator.");
            }
            const std::string Kind = Value["kind"].get<std::string>();
            const auto Index = [&Value](const char* Name) -> std::expected<PinIndex, DiagnosticCollection>
            {
                if (!Value.contains(Name)) return FailureExpected<PinIndex>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Execution control member is missing.");
                const auto Parsed = ParseUint32(Value[Name]);
                if (!Parsed.has_value()) return std::unexpected(Parsed.error());
                const PinIndex Result(*Parsed);
                if (!Result.IsValid()) return FailureExpected<PinIndex>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Execution control pin index is invalid.");
                return Result;
            };
            if (Kind == "Entry")
            {
                if (!HasExactMembers(Value, {"kind", "executionOutput"})) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Entry control shape is invalid.");
                const auto Output = Index("executionOutput"); if (!Output.has_value()) return std::unexpected(Output.error());
                return std::optional<ExecutionControlSchema>(EntryControlSchema{*Output});
            }
            if (Kind == "Sequence" || Kind == "Join")
            {
                if (!HasExactMembers(Value, {"kind", "executionInput", "executionOutput"})) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Sequence or Join control shape is invalid.");
                const auto Input = Index("executionInput"); const auto Output = Index("executionOutput");
                if (!Input.has_value()) return std::unexpected(Input.error()); if (!Output.has_value()) return std::unexpected(Output.error());
                if (Kind == "Sequence") return std::optional<ExecutionControlSchema>(SequenceControlSchema{*Input, *Output});
                return std::optional<ExecutionControlSchema>(JoinControlSchema{*Input, *Output});
            }
            if (Kind == "Branch")
            {
                if (!HasExactMembers(Value, {"kind", "executionInput", "conditionInput", "trueOutput", "falseOutput"})) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Branch control shape is invalid.");
                const auto Input = Index("executionInput"); const auto Condition = Index("conditionInput"); const auto TrueOutput = Index("trueOutput"); const auto FalseOutput = Index("falseOutput");
                if (!Input.has_value()) return std::unexpected(Input.error()); if (!Condition.has_value()) return std::unexpected(Condition.error()); if (!TrueOutput.has_value()) return std::unexpected(TrueOutput.error()); if (!FalseOutput.has_value()) return std::unexpected(FalseOutput.error());
                return std::optional<ExecutionControlSchema>(BranchControlSchema{*Input, *Condition, *TrueOutput, *FalseOutput});
            }
            if (Kind == "Loop")
            {
                if (!HasExactMembers(Value, {"kind", "executionInput", "bodyOutput", "exitOutput", "repeatInput", "breakInput", "exitPolicy", "conditionInput"}) || !Value["exitPolicy"].is_string()) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Loop control shape is invalid.");
                const auto Input = Index("executionInput"); const auto Body = Index("bodyOutput"); const auto Exit = Index("exitOutput"); const auto Repeat = Index("repeatInput"); const auto Break = Index("breakInput");
                if (!Input.has_value()) return std::unexpected(Input.error()); if (!Body.has_value()) return std::unexpected(Body.error()); if (!Exit.has_value()) return std::unexpected(Exit.error()); if (!Repeat.has_value()) return std::unexpected(Repeat.error()); if (!Break.has_value()) return std::unexpected(Break.error());
                const std::string Policy = Value["exitPolicy"].get<std::string>();
                if (Policy != "Conditional" && Policy != "Unconditional") return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Loop exit policy is invalid.");
                std::optional<PinIndex> Condition;
                if (!Value["conditionInput"].is_null())
                {
                    const auto Parsed = Index("conditionInput"); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); Condition = *Parsed;
                }
                if ((Policy == "Conditional") != Condition.has_value()) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Loop condition input does not match its exit policy.");
                return std::optional<ExecutionControlSchema>(LoopControlSchema{*Input, *Body, *Exit, *Repeat, *Break, Policy == "Conditional" ? LoopExitPolicy::Conditional : LoopExitPolicy::Unconditional, Condition});
            }
            if (Kind == "Return")
            {
                if (!HasExactMembers(Value, {"kind", "executionInput"})) return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Return control shape is invalid.");
                const auto Input = Index("executionInput"); if (!Input.has_value()) return std::unexpected(Input.error());
                return std::optional<ExecutionControlSchema>(ReturnControlSchema{*Input});
            }
            return FailureExpected<std::optional<ExecutionControlSchema>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Execution control kind is unsupported.");
        }

        [[nodiscard]] inline bool IsCanonicalAvailability(const std::vector<NodeAvailability>& Values)
        {
            for (std::size_t Index = 0U; Index < Values.size(); ++Index)
            {
                if (Values[Index] != NodeAvailability::Server && Values[Index] != NodeAvailability::Client)
                {
                    return false;
                }
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (Values[Prior] == Values[Index]) return false;
                }
                if (Index > 0U && Values[Index - 1U] == NodeAvailability::Client && Values[Index] == NodeAvailability::Server)
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] inline OrderedJson EncodeAvailability(const std::vector<NodeAvailability>& Values)
        {
            OrderedJson Result = OrderedJson::array();
            for (const NodeAvailability Value : Values) Result.push_back(Value == NodeAvailability::Server ? "Server" : "Client");
            return Result;
        }

        [[nodiscard]] inline std::expected<std::vector<NodeAvailability>, DiagnosticCollection>
            DecodeAvailability(const Json& Value)
        {
            if (!Value.is_array()) return FailureExpected<std::vector<NodeAvailability>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Availability must be an array.");
            std::vector<NodeAvailability> Result;
            for (const Json& Item : Value)
            {
                if (!Item.is_string()) return FailureExpected<std::vector<NodeAvailability>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Availability value is not a string.");
                const std::string Name = Item.get<std::string>();
                if (Name == "Server") Result.push_back(NodeAvailability::Server);
                else if (Name == "Client") Result.push_back(NodeAvailability::Client);
                else return FailureExpected<std::vector<NodeAvailability>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Availability value is unsupported.");
            }
            if (!IsCanonicalAvailability(Result)) return FailureExpected<std::vector<NodeAvailability>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Availability is not canonical.");
            return Result;
        }

        [[nodiscard]] inline const char* DirectionName(PinDirection Value)
        {
            return Value == PinDirection::Input ? "Input" : "Output";
        }

        [[nodiscard]] inline const char* CategoryName(PinCategory Value)
        {
            return Value == PinCategory::Data ? "Data" : "Execution";
        }

        [[nodiscard]] inline const char* CardinalityName(PinCardinality Value)
        {
            switch (Value)
            {
            case PinCardinality::Single: return "Single";
            case PinCardinality::Optional: return "Optional";
            case PinCardinality::Multiple: return "Multiple";
            }
            return "";
        }

        [[nodiscard]] inline std::expected<PinDirection, DiagnosticCollection> DecodeDirection(const Json& Value)
        {
            if (!Value.is_string()) return FailureExpected<PinDirection>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin direction is not a string.");
            if (Value.get<std::string>() == "Input") return PinDirection::Input;
            if (Value.get<std::string>() == "Output") return PinDirection::Output;
            return FailureExpected<PinDirection>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin direction is unsupported.");
        }

        [[nodiscard]] inline std::expected<PinCategory, DiagnosticCollection> DecodeCategory(const Json& Value)
        {
            if (!Value.is_string()) return FailureExpected<PinCategory>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin category is not a string.");
            if (Value.get<std::string>() == "Data") return PinCategory::Data;
            if (Value.get<std::string>() == "Execution") return PinCategory::Execution;
            return FailureExpected<PinCategory>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin category is unsupported.");
        }

        [[nodiscard]] inline std::expected<PinCardinality, DiagnosticCollection> DecodeCardinality(const Json& Value)
        {
            if (!Value.is_string()) return FailureExpected<PinCardinality>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin cardinality is not a string.");
            const std::string Name = Value.get<std::string>();
            if (Name == "Single") return PinCardinality::Single;
            if (Name == "Optional") return PinCardinality::Optional;
            if (Name == "Multiple") return PinCardinality::Multiple;
            return FailureExpected<PinCardinality>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Pin cardinality is unsupported.");
        }

        [[nodiscard]] inline OrderedJson EncodeProvenance(const std::optional<SourceProvenance>& Provenance)
        {
            if (!Provenance.has_value()) return nullptr;
            OrderedJson Result = OrderedJson::object();
            Result["sourceDocumentIdentifier"] = Provenance->GetSourceDocumentIdentifier();
            Result["sourceRecordIdentifier"] = Provenance->GetSourceRecordIdentifier();
            return Result;
        }

        [[nodiscard]] inline std::expected<std::optional<SourceProvenance>, DiagnosticCollection>
            DecodeProvenance(const Json& Value)
        {
            if (Value.is_null()) return std::optional<SourceProvenance>{};
            if (!HasExactMembers(Value, {"sourceDocumentIdentifier", "sourceRecordIdentifier"})) return FailureExpected<std::optional<SourceProvenance>>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Provenance shape is invalid.");
            const auto Document = ParseString(Value["sourceDocumentIdentifier"]); const auto Record = ParseString(Value["sourceRecordIdentifier"]);
            if (!Document.has_value()) return std::unexpected(Document.error()); if (!Record.has_value()) return std::unexpected(Record.error());
            SourceProvenance Result(*Document, *Record);
            if (!Result.IsValid()) return FailureExpected<std::optional<SourceProvenance>>(DiagnosticCode::InvalidSourceProvenance, "Snapshot provenance is invalid.");
            return std::optional<SourceProvenance>(std::move(Result));
        }

        [[nodiscard]] inline OrderedJson EncodeNormalizedPin(const NormalizedPinRecord& Pin)
        {
            OrderedJson Result = OrderedJson::object();
            Result["name"] = Pin.GetName();
            Result["type"] = EncodeType(Pin.GetType());
            Result["direction"] = DirectionName(Pin.GetDirection());
            Result["category"] = CategoryName(Pin.GetCategory());
            Result["cardinality"] = CardinalityName(Pin.GetCardinality());
            Result["allowsLiteral"] = Pin.AllowsLiteral();
            Result["default"] = Pin.GetDefaultValue().has_value() ? EncodeLiteral(*Pin.GetDefaultValue()) : OrderedJson(nullptr);
            return Result;
        }

        [[nodiscard]] inline std::expected<NormalizedPinRecord, DiagnosticCollection>
            DecodeNormalizedPin(const Json& Value, bool AllowEnum)
        {
            if (!HasExactMembers(Value, {"name", "type", "direction", "category", "cardinality", "allowsLiteral", "default"})) return FailureExpected<NormalizedPinRecord>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Normalized pin shape is invalid.");
            const auto Name = ParseString(Value["name"]); const auto Type = DecodeType(Value["type"], AllowEnum); const auto Direction = DecodeDirection(Value["direction"]); const auto Category = DecodeCategory(Value["category"]); const auto Cardinality = DecodeCardinality(Value["cardinality"]);
            if (!Name.has_value()) return std::unexpected(Name.error()); if (!Type.has_value()) return std::unexpected(Type.error()); if (!Direction.has_value()) return std::unexpected(Direction.error()); if (!Category.has_value()) return std::unexpected(Category.error()); if (!Cardinality.has_value()) return std::unexpected(Cardinality.error());
            if (!Value["allowsLiteral"].is_boolean()) return FailureExpected<NormalizedPinRecord>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Normalized pin literal policy is invalid.");
            std::optional<LiteralValue> Default;
            if (!Value["default"].is_null()) { const auto Parsed = DecodeLiteral(Value["default"], AllowEnum); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); Default = *Parsed; }
            NormalizedPinRecord Result(*Name, *Type, *Direction, *Category, *Cardinality, Value["allowsLiteral"].get<bool>(), std::move(Default));
            if (!Result.IsValid()) return FailureExpected<NormalizedPinRecord>(DiagnosticCode::InvalidNormalizedDescriptorRecord, "Normalized pin is semantically invalid.");
            return Result;
        }

        [[nodiscard]] inline OrderedJson EncodeRecord(const NormalizedNodeDescriptorRecord& Record)
        {
            OrderedJson Result = OrderedJson::object();
            Result["availability"] = EncodeAvailability(Record.GetAvailability());
            Result["displayName"] = Record.GetDisplayName();
            Result["executionControl"] = EncodeControl(Record.GetExecutionControlSchema());
            Result["externalIdentity"] = Record.GetExternalIdentity().GetKey();
            Result["pins"] = OrderedJson::array();
            for (const NormalizedPinRecord& Pin : Record.GetPins()) Result["pins"].push_back(EncodeNormalizedPin(Pin));
            Result["provenance"] = EncodeProvenance(Record.GetSourceProvenance());
            return Result;
        }

        [[nodiscard]] inline std::expected<NormalizedNodeDescriptorRecord, DiagnosticCollection>
            DecodeRecord(const Json& Value, bool AllowEnum)
        {
            if (!HasExactMembers(Value, {"availability", "displayName", "executionControl", "externalIdentity", "pins", "provenance"})) return FailureExpected<NormalizedNodeDescriptorRecord>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Normalized record shape is invalid.");
            const auto Availability = DecodeAvailability(Value["availability"]); const auto DisplayName = ParseString(Value["displayName"]); const auto Identity = ParseString(Value["externalIdentity"]); const auto Control = DecodeControl(Value["executionControl"]); const auto Provenance = DecodeProvenance(Value["provenance"]);
            if (!Availability.has_value()) return std::unexpected(Availability.error()); if (!DisplayName.has_value()) return std::unexpected(DisplayName.error()); if (!Identity.has_value()) return std::unexpected(Identity.error()); if (!Control.has_value()) return std::unexpected(Control.error()); if (!Provenance.has_value()) return std::unexpected(Provenance.error());
            if (!Value["pins"].is_array()) return FailureExpected<NormalizedNodeDescriptorRecord>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Normalized record pins are not an array.");
            std::vector<NormalizedPinRecord> Pins; for (const Json& Pin : Value["pins"]) { const auto Parsed = DecodeNormalizedPin(Pin, AllowEnum); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); Pins.push_back(*Parsed); }
            NormalizedNodeDescriptorRecord Result(ExternalNodeIdentity(*Identity), *DisplayName, *Availability, std::move(Pins), *Control, *Provenance);
            const auto Validation = ValidateNormalizedDescriptorRecord(Result); if (!Validation.has_value()) return std::unexpected(Validation.error());
            return Result;
        }

        [[nodiscard]] inline OrderedJson EncodeFamilyPin(const DescriptorSpecializationPin& Pin)
        {
            OrderedJson Result = OrderedJson::object();
            Result["name"] = Pin.GetName();
            Result["fixedType"] = Pin.GetFixedType().has_value() ? EncodeType(*Pin.GetFixedType()) : OrderedJson(nullptr);
            Result["reflected"] = Pin.IsReflected();
            Result["direction"] = DirectionName(Pin.GetDirection());
            Result["category"] = CategoryName(Pin.GetCategory());
            Result["cardinality"] = CardinalityName(Pin.GetCardinality());
            Result["allowsLiteral"] = Pin.AllowsLiteral();
            Result["default"] = Pin.GetDefaultValue().has_value() ? EncodeLiteral(*Pin.GetDefaultValue()) : OrderedJson(nullptr);
            return Result;
        }

        [[nodiscard]] inline std::expected<DescriptorSpecializationPin, DiagnosticCollection>
            DecodeFamilyPin(const Json& Value, bool AllowEnum)
        {
            if (!HasExactMembers(Value, {"name", "fixedType", "reflected", "direction", "category", "cardinality", "allowsLiteral", "default"})) return FailureExpected<DescriptorSpecializationPin>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization family pin shape is invalid.");
            const auto Name = ParseString(Value["name"]); const auto Direction = DecodeDirection(Value["direction"]); const auto Category = DecodeCategory(Value["category"]); const auto Cardinality = DecodeCardinality(Value["cardinality"]);
            if (!Name.has_value()) return std::unexpected(Name.error()); if (!Direction.has_value()) return std::unexpected(Direction.error()); if (!Category.has_value()) return std::unexpected(Category.error()); if (!Cardinality.has_value()) return std::unexpected(Cardinality.error());
            if (!Value["reflected"].is_boolean() || !Value["allowsLiteral"].is_boolean()) return FailureExpected<DescriptorSpecializationPin>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization family pin boolean field is invalid.");
            std::optional<TypeDesc> FixedType; if (!Value["fixedType"].is_null()) { const auto Parsed = DecodeType(Value["fixedType"], AllowEnum); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); FixedType = *Parsed; }
            std::optional<LiteralValue> Default; if (!Value["default"].is_null()) { const auto Parsed = DecodeLiteral(Value["default"], AllowEnum); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); Default = *Parsed; }
            DescriptorSpecializationPin Result(*Name, std::move(FixedType), Value["reflected"].get<bool>(), *Direction, *Category, *Cardinality, Value["allowsLiteral"].get<bool>(), std::move(Default));
            if (!Result.IsValid()) return FailureExpected<DescriptorSpecializationPin>(DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch, "Specialization family pin is invalid.");
            return Result;
        }

        [[nodiscard]] inline OrderedJson EncodeFamily(const DescriptorSpecializationFamily& Family);
        [[nodiscard]] inline std::expected<DescriptorSpecializationFamily, DiagnosticCollection> DecodeFamily(const Json& Value, bool AllowEnum);

        [[nodiscard]] inline OrderedJson EncodeFamily(const DescriptorSpecializationFamily& Family)
        {
            OrderedJson Result = OrderedJson::object();
            Result["familyExternalIdentity"] = Family.GetFamilyExternalIdentity().GetKey();
            Result["displayName"] = Family.GetDisplayName();
            Result["availability"] = EncodeAvailability(Family.GetAvailability());
            Result["pins"] = OrderedJson::array(); for (const auto& Pin : Family.GetPins()) Result["pins"].push_back(EncodeFamilyPin(Pin));
            Result["executionControl"] = EncodeControl(Family.GetExecutionControlSchema());
            Result["provenance"] = EncodeProvenance(Family.GetSourceProvenance());
            Result["variants"] = OrderedJson::array();
            for (const auto& Variant : Family.GetVariants())
            {
                OrderedJson VariantJson = OrderedJson::object();
                VariantJson["concreteExternalIdentity"] = Variant.GetConcreteExternalIdentity().GetKey();
                VariantJson["specializationKey"] = Variant.GetSpecializationKey();
                VariantJson["pinBindings"] = OrderedJson::array();
                for (const auto& Binding : Variant.GetPinBindings())
                {
                    OrderedJson BindingJson = OrderedJson::object();
                    BindingJson["familyPinIndex"] = Binding.GetFamilyPinIndex().GetValue();
                    BindingJson["concreteType"] = EncodeType(Binding.GetConcreteType());
                    VariantJson["pinBindings"].push_back(std::move(BindingJson));
                }
                Result["variants"].push_back(std::move(VariantJson));
            }
            return Result;
        }

        [[nodiscard]] inline std::expected<DescriptorSpecializationFamily, DiagnosticCollection> DecodeFamily(const Json& Value, bool AllowEnum)
        {
            if (!HasExactMembers(Value, {"familyExternalIdentity", "displayName", "availability", "pins", "executionControl", "provenance", "variants"})) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization family shape is invalid.");
            const auto Identity = ParseString(Value["familyExternalIdentity"]); const auto DisplayName = ParseString(Value["displayName"]); const auto Availability = DecodeAvailability(Value["availability"]); const auto Control = DecodeControl(Value["executionControl"]); const auto Provenance = DecodeProvenance(Value["provenance"]);
            if (!Identity.has_value()) return std::unexpected(Identity.error()); if (!DisplayName.has_value()) return std::unexpected(DisplayName.error()); if (!Availability.has_value()) return std::unexpected(Availability.error()); if (!Control.has_value()) return std::unexpected(Control.error()); if (!Provenance.has_value()) return std::unexpected(Provenance.error());
            if (!Value["pins"].is_array() || !Value["variants"].is_array()) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization family arrays are invalid.");
            std::vector<DescriptorSpecializationPin> Pins; for (const Json& Pin : Value["pins"]) { const auto Parsed = DecodeFamilyPin(Pin, AllowEnum); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); Pins.push_back(*Parsed); }
            std::vector<DescriptorSpecializationVariant> Variants;
            std::string PreviousIdentity;
            for (const Json& VariantValue : Value["variants"])
            {
                if (!HasExactMembers(VariantValue, {"concreteExternalIdentity", "specializationKey", "pinBindings"})) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization variant shape is invalid.");
                const auto ConcreteIdentity = ParseString(VariantValue["concreteExternalIdentity"]); const auto Key = ParseString(VariantValue["specializationKey"]);
                if (!ConcreteIdentity.has_value()) return std::unexpected(ConcreteIdentity.error()); if (!Key.has_value()) return std::unexpected(Key.error());
                if (!PreviousIdentity.empty() && *ConcreteIdentity <= PreviousIdentity) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization variants are not canonical."); PreviousIdentity = *ConcreteIdentity;
                if (!VariantValue["pinBindings"].is_array()) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization bindings are not an array.");
                std::vector<DescriptorSpecializationPinBinding> Bindings; std::uint32_t PreviousIndex = 0U; bool HasPrevious = false;
                for (const Json& BindingValue : VariantValue["pinBindings"])
                {
                    if (!HasExactMembers(BindingValue, {"familyPinIndex", "concreteType"}) || !IsUint32(BindingValue["familyPinIndex"])) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization binding shape is invalid.");
                    const std::uint32_t Index = BindingValue["familyPinIndex"].get<std::uint32_t>(); if (Index == std::numeric_limits<std::uint32_t>::max() || (HasPrevious && Index <= PreviousIndex)) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization bindings are not canonical."); HasPrevious = true; PreviousIndex = Index;
                    const auto Type = DecodeType(BindingValue["concreteType"], AllowEnum); if (!Type.has_value()) return std::unexpected(Type.error()); Bindings.emplace_back(PinIndex(Index), *Type);
                }
                Variants.emplace_back(ExternalNodeIdentity(*ConcreteIdentity), *Key, std::move(Bindings));
            }
            DescriptorSpecializationFamily Result(ExternalNodeIdentity(*Identity), *DisplayName, *Availability, std::move(Pins), *Control, *Provenance, std::move(Variants));
            if (!Result.IsValid()) return FailureExpected<DescriptorSpecializationFamily>(DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch, "Specialization family is invalid.");
            return Result;
        }

        [[nodiscard]] inline std::expected<Json, DiagnosticCollection> ParseStrictJson(const std::string& Input)
        {
            DuplicateMemberSax Sax;
            if (!Json::sax_parse(Input, &Sax) || Sax.HasDuplicate() || Sax.HasParseError())
            {
                return FailureExpected<Json>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot JSON is malformed or contains duplicate object members.");
            }
            try
            {
                return Json::parse(Input);
            }
            catch (...)
            {
                return FailureExpected<Json>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot JSON is malformed.");
            }
        }

        [[nodiscard]] inline bool SameRecordByIdentity(
            const DescriptorCatalogueEntry& Entry,
            const NormalizedNodeDescriptorRecord& Record
        )
        {
            return Entry.GetExternalIdentity() == Record.GetExternalIdentity() &&
                Entry.GetRecord() == Record;
        }

        [[nodiscard]] inline bool SameNodeDescriptor(
            const NodeDescriptor& Left,
            const NodeDescriptor& Right
        )
        {
            if (Left.GetIdentifier() != Right.GetIdentifier() || Left.GetName() != Right.GetName() || Left.GetAvailability() != Right.GetAvailability() || Left.GetPins().size() != Right.GetPins().size() || Left.GetExecutionControlSchema().has_value() != Right.GetExecutionControlSchema().has_value()) return false;
            for (std::size_t Index = 0U; Index < Left.GetPins().size(); ++Index)
            {
                const PinSchema& LeftPin = Left.GetPins()[Index]; const PinSchema& RightPin = Right.GetPins()[Index];
                if (LeftPin.GetName() != RightPin.GetName() || LeftPin.GetType() != RightPin.GetType() || LeftPin.GetDirection() != RightPin.GetDirection() || LeftPin.GetCategory() != RightPin.GetCategory() || LeftPin.GetCardinality() != RightPin.GetCardinality() || LeftPin.AllowsLiteral() != RightPin.AllowsLiteral()) return false;
                if (LeftPin.GetDefaultValue().has_value() != RightPin.GetDefaultValue().has_value()) return false;
                if (LeftPin.GetDefaultValue().has_value() && !DescriptorCatalogueDetail::AreNormalizedLiteralValuesEqual(*LeftPin.GetDefaultValue(), *RightPin.GetDefaultValue())) return false;
            }
            if (!Left.GetExecutionControlSchema().has_value()) return true;
            return DescriptorCatalogueDetail::AreExecutionControlSchemasEqual(*Left.GetExecutionControlSchema(), *Right.GetExecutionControlSchema());
        }

        [[nodiscard]] inline NodeDescriptor MakeNodeDescriptor(const DescriptorCatalogueEntry& Entry)
        {
            const NormalizedNodeDescriptorRecord& Record = Entry.GetRecord();
            std::vector<PinSchema> Pins;
            for (const NormalizedPinRecord& Pin : Record.GetPins()) Pins.emplace_back(Pin.GetName(), Pin.GetType(), Pin.GetDirection(), Pin.GetCategory(), Pin.GetCardinality(), Pin.AllowsLiteral(), Pin.GetDefaultValue());
            return NodeDescriptor(Entry.GetDescriptorIdentifier(), Record.GetDisplayName(), Record.GetAvailability(), std::move(Pins), Record.GetExecutionControlSchema());
        }
    }

    class DescriptorCatalogueSnapshot final
    {
    public:
        DescriptorCatalogueSnapshot(const DescriptorCatalogueSnapshot&) = default;
        DescriptorCatalogueSnapshot(DescriptorCatalogueSnapshot&&) = default;
        DescriptorCatalogueSnapshot& operator=(const DescriptorCatalogueSnapshot&) = default;
        DescriptorCatalogueSnapshot& operator=(DescriptorCatalogueSnapshot&&) = default;

        [[nodiscard]] static std::expected<DescriptorCatalogueSnapshot, DiagnosticCollection> Create(
            DescriptorCatalogue Catalogue,
            std::optional<DescriptorSpecializationResult> SpecializationResult = std::nullopt
        );

        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] const DescriptorCatalogue& GetCatalogue() const { return m_Catalogue; }
        [[nodiscard]] const std::optional<DescriptorSpecializationResult>& GetSpecializationResult() const { return m_SpecializationResult; }
        bool operator==(const DescriptorCatalogueSnapshot& Other) const { return m_Catalogue == Other.m_Catalogue && m_SpecializationResult == Other.m_SpecializationResult; }

    private:
        DescriptorCatalogueSnapshot(DescriptorCatalogue Catalogue, std::optional<DescriptorSpecializationResult> SpecializationResult)
            : m_Catalogue(std::move(Catalogue)), m_SpecializationResult(std::move(SpecializationResult)) {}
        DescriptorCatalogue m_Catalogue;
        std::optional<DescriptorSpecializationResult> m_SpecializationResult;
    };

    class DescriptorCatalogueSnapshotPersistence final
    {
    public:
        DescriptorCatalogueSnapshotPersistence() = delete;
        inline static constexpr std::uint32_t CurrentSnapshotFormatVersion = 2U;
        [[nodiscard]] static std::expected<std::string, DiagnosticCollection> Write(const DescriptorCatalogueSnapshot& Snapshot);
        [[nodiscard]] static std::expected<DescriptorCatalogueSnapshot, DiagnosticCollection> Read(std::string SnapshotJson);
    };

    class DescriptorCatalogueRegistryContext final
    {
    public:
        DescriptorCatalogueRegistryContext(const DescriptorCatalogueRegistryContext&) = default;
        DescriptorCatalogueRegistryContext(DescriptorCatalogueRegistryContext&&) = default;
        DescriptorCatalogueRegistryContext& operator=(const DescriptorCatalogueRegistryContext&) = default;
        DescriptorCatalogueRegistryContext& operator=(DescriptorCatalogueRegistryContext&&) = default;
        [[nodiscard]] static std::expected<DescriptorCatalogueRegistryContext, DiagnosticCollection> Materialize(DescriptorCatalogueSnapshot Snapshot);
        [[nodiscard]] bool IsValid() const;
        [[nodiscard]] const DescriptorCatalogueSnapshot& GetSnapshot() const { return m_Snapshot; }
        [[nodiscard]] const DescriptorCatalogue& GetCatalogue() const { return m_Snapshot.GetCatalogue(); }
        [[nodiscard]] const DescriptorCatalogueIdentity& GetCatalogueIdentity() const { return m_Snapshot.GetCatalogue().GetIdentity(); }
        [[nodiscard]] const NodeDescriptorRegistry& GetRegistry() const { return m_Registry; }
    private:
        DescriptorCatalogueRegistryContext(DescriptorCatalogueSnapshot Snapshot, NodeDescriptorRegistry Registry)
            : m_Snapshot(std::move(Snapshot)), m_Registry(std::move(Registry)) {}
        DescriptorCatalogueSnapshot m_Snapshot;
        NodeDescriptorRegistry m_Registry;
    };

    namespace DescriptorCatalogueSnapshotDetail
    {
        inline bool IsSpecializationConsistent(const DescriptorCatalogue& Catalogue, const DescriptorSpecializationResult& Result)
        {
            if (!Result.IsValid() || Result.GetConcreteRecords().empty()) return false;
            std::set<std::string> Claimed;
            for (const auto& Record : Result.GetConcreteRecords())
            {
                if (!Claimed.insert(Record.GetExternalIdentity().GetKey()).second) return false;
                const DescriptorCatalogueEntry* Entry = Catalogue.FindByExternalIdentity(Record.GetExternalIdentity());
                if (Entry == nullptr || !SameRecordByIdentity(*Entry, Record)) return false;
            }
            return true;
        }

        inline OrderedJson EncodeSnapshot(const DescriptorCatalogueSnapshot& Snapshot)
        {
            OrderedJson Root = OrderedJson::object();
            OrderedJson Catalogue = OrderedJson::object();
            const auto& Identity = Snapshot.GetCatalogue().GetIdentity();
            Catalogue["contentIdentifier"] = Identity.GetCatalogueContentIdentifier().GetValue();
            Catalogue["semanticSchemaVersion"] = Identity.GetSemanticSchemaVersion().GetValue();
            Catalogue["sourceNamespace"] = Identity.GetSourceNamespace();
            Catalogue["sourceRevision"] = Identity.GetSourceRevision();
            Root["catalogue"] = std::move(Catalogue);
            Root["entries"] = OrderedJson::array();
            for (const auto& Entry : Snapshot.GetCatalogue().GetEntries())
            {
                OrderedJson EntryJson = OrderedJson::object();
                EntryJson["nodeDescriptorId"] = Entry.GetDescriptorIdentifier().GetValue();
                EntryJson["record"] = EncodeRecord(Entry.GetRecord());
                Root["entries"].push_back(std::move(EntryJson));
            }
            Root["snapshotFormatVersion"] = DescriptorCatalogueSnapshotPersistence::CurrentSnapshotFormatVersion;
            if (!Snapshot.GetSpecializationResult().has_value())
            {
                Root["specialization"] = nullptr;
            }
            else
            {
                OrderedJson Specialization = OrderedJson::object();
                Specialization["families"] = OrderedJson::array();
                for (const auto& Family : Snapshot.GetSpecializationResult()->GetFamilies()) Specialization["families"].push_back(EncodeFamily(Family));
                OrderedJson Wrapper = OrderedJson::object(); Wrapper["families"] = std::move(Specialization["families"]); Root["specialization"] = std::move(Wrapper);
            }
            return Root;
        }
    }

    inline bool DescriptorCatalogueSnapshot::IsValid() const
    {
        return m_Catalogue.IsValid() && (!m_SpecializationResult.has_value() ||
            DescriptorCatalogueSnapshotDetail::IsSpecializationConsistent(m_Catalogue, *m_SpecializationResult));
    }

    inline std::expected<DescriptorCatalogueSnapshot, DiagnosticCollection> DescriptorCatalogueSnapshot::Create(
        DescriptorCatalogue Catalogue,
        std::optional<DescriptorSpecializationResult> SpecializationResult
    )
    {
        if (!Catalogue.IsValid()) return std::unexpected(DescriptorCatalogueSnapshotDetail::Failure(DiagnosticCode::DescriptorCatalogueMismatch, "Cannot create a snapshot from an invalid catalogue."));
        if (SpecializationResult.has_value() && !DescriptorCatalogueSnapshotDetail::IsSpecializationConsistent(Catalogue, *SpecializationResult)) return std::unexpected(DescriptorCatalogueSnapshotDetail::Failure(DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch, "Specialization metadata does not match the catalogue subset."));
        return DescriptorCatalogueSnapshot(std::move(Catalogue), std::move(SpecializationResult));
    }

    inline std::expected<std::string, DiagnosticCollection> DescriptorCatalogueSnapshotPersistence::Write(const DescriptorCatalogueSnapshot& Snapshot)
    {
        if (!Snapshot.IsValid()) return std::unexpected(DescriptorCatalogueSnapshotDetail::Failure(DiagnosticCode::DescriptorCatalogueMismatch, "Cannot write an invalid descriptor catalogue snapshot."));
        return DescriptorCatalogueSnapshotDetail::EncodeSnapshot(Snapshot).dump();
    }

    inline std::expected<DescriptorCatalogueSnapshot, DiagnosticCollection> DescriptorCatalogueSnapshotPersistence::Read(std::string SnapshotJson)
    {
        using namespace DescriptorCatalogueSnapshotDetail;
        const auto Parsed = ParseStrictJson(SnapshotJson); if (!Parsed.has_value()) return std::unexpected(Parsed.error()); const Json& Root = *Parsed;
        if (!HasExactMembers(Root, {"catalogue", "entries", "snapshotFormatVersion", "specialization"})) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot root shape is invalid.");
        if (!IsUint32(Root["snapshotFormatVersion"])) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot format version is invalid.");
        const std::uint32_t Version = Root["snapshotFormatVersion"].get<std::uint32_t>(); if (Version != 1U && Version != 2U) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::UnsupportedDescriptorCatalogueSnapshotVersion, "Snapshot format version is unsupported.");
        const Json& CatalogueJson = Root["catalogue"];
        if (!HasExactMembers(CatalogueJson, {"contentIdentifier", "semanticSchemaVersion", "sourceNamespace", "sourceRevision"})) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Catalogue identity shape is invalid.");
        const auto Content = ParseString(CatalogueJson["contentIdentifier"]); const auto Namespace = ParseString(CatalogueJson["sourceNamespace"]); const auto Revision = ParseString(CatalogueJson["sourceRevision"]); const auto Schema = ParseUint32(CatalogueJson["semanticSchemaVersion"]);
        if (!Content.has_value()) return std::unexpected(Content.error()); if (!Namespace.has_value()) return std::unexpected(Namespace.error()); if (!Revision.has_value()) return std::unexpected(Revision.error()); if (!Schema.has_value()) return std::unexpected(Schema.error());
        if (*Schema == 2U && Version == 1U) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion, "Snapshot format version 1 cannot carry catalogue semantic schema version 2.");
        const bool AllowEnum = Version == 2U && *Schema == 2U;
        const DescriptorCatalogueContentIdentifier PersistedContentIdentifier(*Content); if (!PersistedContentIdentifier.IsValid()) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::InvalidDescriptorCatalogueIdentity, "Persisted catalogue content identifier is invalid.");
        if (!Root["entries"].is_array()) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot entries are not an array.");
        std::vector<NormalizedNodeDescriptorRecord> Records; std::vector<std::uint32_t> PersistedIds; std::string PreviousIdentity;
        for (const Json& EntryJson : Root["entries"])
        {
            if (!HasExactMembers(EntryJson, {"nodeDescriptorId", "record"}) || !IsUint32(EntryJson["nodeDescriptorId"]) || EntryJson["nodeDescriptorId"] == 0U) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot entry shape or ID is invalid.");
            const auto Record = DecodeRecord(EntryJson["record"], AllowEnum); if (!Record.has_value()) return std::unexpected(Record.error());
            if (!PreviousIdentity.empty() && Record->GetExternalIdentity().GetKey() <= PreviousIdentity) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Snapshot entries are not canonical."); PreviousIdentity = Record->GetExternalIdentity().GetKey();
            Records.push_back(*Record); PersistedIds.push_back(EntryJson["nodeDescriptorId"].get<std::uint32_t>());
        }
        const auto Catalogue = DescriptorCatalogueBuilder::Build(*Namespace, *Revision, DescriptorCatalogueSemanticSchemaVersion(*Schema), std::move(Records)); if (!Catalogue.has_value()) return std::unexpected(Catalogue.error());
        if (Catalogue->GetIdentity().GetCatalogueContentIdentifier().GetValue() != *Content) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::DescriptorCatalogueSnapshotContentMismatch, "Persisted catalogue content identifier does not match rebuilt content.");
        if (Catalogue->GetEntries().size() != PersistedIds.size()) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::DescriptorCatalogueSnapshotIdentifierMismatch, "Persisted descriptor ID count does not match rebuilt catalogue.");
        for (std::size_t Index = 0U; Index < PersistedIds.size(); ++Index) if (Catalogue->GetEntries()[Index].GetDescriptorIdentifier().GetValue() != PersistedIds[Index]) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::DescriptorCatalogueSnapshotIdentifierMismatch, "Persisted descriptor ID does not match deterministic allocation.");

        std::optional<DescriptorSpecializationResult> Sidecar;
        if (!Root["specialization"].is_null())
        {
            const Json& Wrapper = Root["specialization"]; if (!HasExactMembers(Wrapper, {"families"}) || !Wrapper["families"].is_array() || Wrapper["families"].empty()) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization wrapper is invalid.");
            std::vector<DescriptorSpecializationFamily> Families; std::string PreviousFamily;
            for (const Json& FamilyJson : Wrapper["families"])
            {
                const auto Family = DecodeFamily(FamilyJson, AllowEnum); if (!Family.has_value()) return std::unexpected(Family.error()); if (!PreviousFamily.empty() && Family->GetFamilyExternalIdentity().GetKey() <= PreviousFamily) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::MalformedDescriptorCatalogueSnapshot, "Specialization families are not canonical."); PreviousFamily = Family->GetFamilyExternalIdentity().GetKey(); Families.push_back(*Family);
            }
            const auto Specialized = DescriptorFamilySpecializer::Specialize(std::move(Families)); if (!Specialized.has_value()) return FailureExpected<DescriptorCatalogueSnapshot>(DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch, "Specialization sidecar could not be reconstructed."); Sidecar = *Specialized;
        }
        auto Result = DescriptorCatalogueSnapshot::Create(*Catalogue, std::move(Sidecar)); if (!Result.has_value()) return std::unexpected(Result.error()); return Result;
    }

    inline bool DescriptorCatalogueRegistryContext::IsValid() const
    {
        if (!m_Snapshot.IsValid() || m_Registry.Size() != m_Snapshot.GetCatalogue().GetEntryCount()) return false;
        for (const auto& Entry : m_Snapshot.GetCatalogue().GetEntries())
        {
            const NodeDescriptor* Descriptor = m_Registry.Find(Entry.GetDescriptorIdentifier()); if (Descriptor == nullptr || !DescriptorCatalogueSnapshotDetail::SameNodeDescriptor(*Descriptor, DescriptorCatalogueSnapshotDetail::MakeNodeDescriptor(Entry))) return false;
        }
        return true;
    }

    inline std::expected<DescriptorCatalogueRegistryContext, DiagnosticCollection> DescriptorCatalogueRegistryContext::Materialize(DescriptorCatalogueSnapshot Snapshot)
    {
        if (!Snapshot.IsValid()) return std::unexpected(DescriptorCatalogueSnapshotDetail::Failure(DiagnosticCode::DescriptorCatalogueMismatch, "Cannot materialize an invalid descriptor catalogue snapshot."));
        NodeDescriptorRegistry Registry;
        for (const auto& Entry : Snapshot.GetCatalogue().GetEntries())
        {
            const auto Registered = Registry.Register(DescriptorCatalogueSnapshotDetail::MakeNodeDescriptor(Entry));
            if (!Registered.has_value()) return std::unexpected(DiagnosticCollection{Registered.error()});
        }
        DescriptorCatalogueRegistryContext Result(std::move(Snapshot), std::move(Registry));
        if (!Result.IsValid()) return std::unexpected(DescriptorCatalogueSnapshotDetail::Failure(DiagnosticCode::DescriptorCatalogueMismatch, "Materialized registry does not match the catalogue."));
        return Result;
    }
}
