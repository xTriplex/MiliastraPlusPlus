#pragma once

#include <cstdint>
#include <exception>
#include <expected>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusGraphIR.h"

namespace MiliastraPlusPlus::GraphIRJson
{
    using Json = nlohmann::json;
    using ParseResult = std::expected<GraphIR, DiagnosticCollection>;

    namespace Detail
    {
        inline DiagnosticCollection Error(const std::string& Message)
        {
            return DiagnosticCollection{{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::MalformedGraphIRJson,
                .Message = Message
            }};
        }

        template<typename Type>
        std::expected<Type, DiagnosticCollection> Fail(const std::string& Message)
        {
            return std::unexpected(Error(Message));
        }

        inline bool HasObjectFields(
            const Json& Value,
            std::initializer_list<const char*> Fields)
        {
            if (!Value.is_object())
            {
                return false;
            }

            for (const char* Field : Fields)
            {
                if (!Value.contains(Field))
                {
                    return false;
                }
            }

            return true;
        }

        inline std::expected<std::uint32_t, DiagnosticCollection> UInt32(
            const Json& Value,
            const char* Name);

        inline Json TypeToJson(const TypeDesc& Type)
        {
            using Kind = TypeDesc::Kind;

            Json Result;

            switch (Type.GetKind())
            {
            case Kind::Invalid:
                Result["kind"] = "Invalid";
                break;

            case Kind::Boolean:
                Result["kind"] = "Boolean";
                break;

            case Kind::Integer:
                Result["kind"] = "Integer";
                break;

            case Kind::Float:
                Result["kind"] = "Float";
                break;

            case Kind::String:
                Result["kind"] = "String";
                break;

            case Kind::Flow:
                Result["kind"] = "Flow";
                break;

            case Kind::Entity:
                Result["kind"] = "Entity";
                break;

            case Kind::GUID:
                Result["kind"] = "GUID";
                break;

            case Kind::Vector3:
                Result["kind"] = "Vector3";
                break;

            case Kind::PrefabId:
                Result["kind"] = "PrefabId";
                break;

            case Kind::ConfigId:
                Result["kind"] = "ConfigId";
                break;

            case Kind::Faction:
                Result["kind"] = "Faction";
                break;

            case Kind::Generic:
                Result["kind"] = "Generic";
                Result["parameter"] = Type.GetGenericParameter().GetValue();
                break;

            case Kind::List:
                Result["kind"] = "List";
                Result["element"] = TypeToJson(*Type.GetElementType());
                break;

            case Kind::Dictionary:
                Result["kind"] = "Dictionary";
                Result["key"] = TypeToJson(*Type.GetKeyType());
                Result["value"] = TypeToJson(*Type.GetValueType());
                break;

            case Kind::StructObject:
                Result["kind"] = "StructObject";
                Result["structType"] = Type.GetStructType().GetValue();
                break;
            }

            return Result;
        }

        inline std::expected<TypeDesc, DiagnosticCollection> TypeFromJson(
            const Json& Value)
        {
            if (!Value.is_object()
                || !Value.contains("kind")
                || !Value["kind"].is_string())
            {
                return Fail<TypeDesc>("Malformed TypeDesc.");
            }

            const std::string Kind = Value["kind"].get<std::string>();

            if (Kind == "Invalid")
            {
                return TypeDesc{};
            }

            if (Kind == "Boolean")
            {
                return TypeDesc::Boolean();
            }

            if (Kind == "Integer")
            {
                return TypeDesc::Integer();
            }

            if (Kind == "Float")
            {
                return TypeDesc::Float();
            }

            if (Kind == "String")
            {
                return TypeDesc::String();
            }

            if (Kind == "Flow")
            {
                return TypeDesc::Flow();
            }

            if (Kind == "Entity")
            {
                return TypeDesc::Entity();
            }

            if (Kind == "GUID")
            {
                return TypeDesc::GUID();
            }

            if (Kind == "Vector3")
            {
                return TypeDesc::Vector3();
            }

            if (Kind == "PrefabId")
            {
                return TypeDesc::PrefabId();
            }

            if (Kind == "ConfigId")
            {
                return TypeDesc::ConfigId();
            }

            if (Kind == "Faction")
            {
                return TypeDesc::Faction();
            }

            if (Kind == "Generic")
            {
                const auto Parameter = UInt32(Value, "parameter");

                if (!Parameter)
                {
                    return std::unexpected(Parameter.error());
                }

                return TypeDesc::Generic(GenericParameterId(*Parameter));
            }

            if (Kind == "StructObject")
            {
                const auto StructType = UInt32(Value, "structType");

                if (!StructType)
                {
                    return std::unexpected(StructType.error());
                }

                return TypeDesc::StructObject(StructTypeId(*StructType));
            }

            if (Kind == "List")
            {
                if (!Value.contains("element"))
                {
                    return Fail<TypeDesc>("Malformed List TypeDesc.");
                }

                const auto Element = TypeFromJson(Value["element"]);

                if (!Element)
                {
                    return std::unexpected(Element.error());
                }

                return TypeDesc::List(*Element);
            }

            if (Kind == "Dictionary")
            {
                if (!Value.contains("key") || !Value.contains("value"))
                {
                    return Fail<TypeDesc>("Malformed Dictionary TypeDesc.");
                }

                const auto Key = TypeFromJson(Value["key"]);
                const auto Element = TypeFromJson(Value["value"]);

                if (!Key)
                {
                    return std::unexpected(Key.error());
                }

                if (!Element)
                {
                    return std::unexpected(Element.error());
                }

                return TypeDesc::Dictionary(*Key, *Element);
            }

            return Fail<TypeDesc>("Unknown TypeDesc kind: " + Kind);
        }

        inline Json LiteralToJson(const LiteralValue& Literal)
        {
            Json Result;

            if (!Literal.IsValid())
            {
                Result["kind"] = "Invalid";
                return Result;
            }

            std::visit(
                [&Result](const auto& Value)
                {
                    using Type = std::decay_t<decltype(Value)>;

                    if constexpr (std::is_same_v<Type, bool>)
                    {
                        Result["kind"] = "Boolean";
                        Result["value"] = Value;
                    }
                    else if constexpr (std::is_same_v<Type, std::int64_t>)
                    {
                        Result["kind"] = "Integer";
                        Result["value"] = Value;
                    }
                    else if constexpr (std::is_same_v<Type, double>)
                    {
                        Result["kind"] = "Float";
                        Result["value"] = Value;
                    }
                    else if constexpr (std::is_same_v<Type, std::string>)
                    {
                        Result["kind"] = "String";
                        Result["value"] = Value;
                    }
                    else if constexpr (
                        std::is_same_v<Type, GuidValue>
                        || std::is_same_v<Type, PrefabIdValue>
                        || std::is_same_v<Type, ConfigIdValue>
                        || std::is_same_v<Type, FactionValue>)
                    {
                        if constexpr (std::is_same_v<Type, GuidValue>)
                        {
                            Result["kind"] = "GUID";
                        }
                        else if constexpr (std::is_same_v<Type, PrefabIdValue>)
                        {
                            Result["kind"] = "PrefabId";
                        }
                        else if constexpr (std::is_same_v<Type, ConfigIdValue>)
                        {
                            Result["kind"] = "ConfigId";
                        }
                        else
                        {
                            Result["kind"] = "Faction";
                        }

                        Result["value"] = Value.Value;
                    }
                    else if constexpr (std::is_same_v<Type, Vector3Value>)
                    {
                        Result["kind"] = "Vector3";
                        Result["x"] = Value.X;
                        Result["y"] = Value.Y;
                        Result["z"] = Value.Z;
                    }
                },
                Literal.GetData());

            return Result;
        }

        inline std::expected<LiteralValue, DiagnosticCollection> LiteralFromJson(
            const Json& Value)
        {
            if (!Value.is_object()
                || !Value.contains("kind")
                || !Value["kind"].is_string())
            {
                return Fail<LiteralValue>("Malformed LiteralValue.");
            }

            const std::string Kind = Value["kind"].get<std::string>();

            if (Kind == "Invalid")
            {
                return LiteralValue{};
            }

            if (!Value.contains("value") && Kind != "Vector3")
            {
                return Fail<LiteralValue>("LiteralValue is missing value.");
            }

            if (Kind == "Boolean" && Value["value"].is_boolean())
            {
                return LiteralValue(
                    LiteralValue::Data{
                        Value["value"].get<bool>()
                    });
            }

            if (Kind == "Integer" && Value["value"].is_number_unsigned())
            {
                const std::uint64_t Number = Value["value"].get<std::uint64_t>();
                if (Number > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max()))
                {
                    return Fail<LiteralValue>(
                        "Integer literal is outside the signed 64-bit graph range.");
                }
                return LiteralValue(
                    LiteralValue::Data{static_cast<std::int64_t>(Number)});
            }

            if (Kind == "Integer" && Value["value"].is_number_integer())
            {
                return LiteralValue(
                    LiteralValue::Data{
                        Value["value"].get<std::int64_t>()
                    });
            }

            if (Kind == "Float" && Value["value"].is_number_float())
            {
                return LiteralValue(
                    LiteralValue::Data{
                        Value["value"].get<double>()
                    });
            }

            if (Kind == "String" && Value["value"].is_string())
            {
                return LiteralValue(
                    LiteralValue::Data{
                        Value["value"].get<std::string>()
                    });
            }

            if ((Kind == "GUID"
                 || Kind == "PrefabId"
                 || Kind == "ConfigId"
                 || Kind == "Faction")
                && Value["value"].is_number_unsigned())
            {
                const auto Number = Value["value"].get<std::uint64_t>();

                if (Kind == "GUID")
                {
                    return LiteralValue(
                        LiteralValue::Data{
                            GuidValue{Number}
                        });
                }

                if (Kind == "PrefabId")
                {
                    return LiteralValue(
                        LiteralValue::Data{
                            PrefabIdValue{Number}
                        });
                }

                if (Kind == "ConfigId")
                {
                    return LiteralValue(
                        LiteralValue::Data{
                            ConfigIdValue{Number}
                        });
                }

                return LiteralValue(
                    LiteralValue::Data{
                        FactionValue{Number}
                    });
            }

            if (Kind == "Vector3"
                && Value.contains("x")
                && Value.contains("y")
                && Value.contains("z")
                && Value["x"].is_number()
                && Value["y"].is_number()
                && Value["z"].is_number())
            {
                return LiteralValue(
                    LiteralValue::Data{
                        Vector3Value{
                            Value["x"].get<float>(),
                            Value["y"].get<float>(),
                            Value["z"].get<float>()
                        }
                    });
            }

            return Fail<LiteralValue>(
                "Malformed or unknown LiteralValue kind: " + Kind);
        }

        inline std::uint64_t Id(
            const Json& Value,
            const char* Name)
        {
            return Value.at(Name).get<std::uint64_t>();
        }

        inline std::expected<std::uint32_t, DiagnosticCollection> UInt32(
            const Json& Value,
            const char* Name)
        {
            if (!Value.contains(Name)
                || !Value[Name].is_number_unsigned()
                || Value[Name].get<std::uint64_t>()
                    > std::numeric_limits<std::uint32_t>::max())
            {
                return Fail<std::uint32_t>(
                    std::string("Invalid 32-bit integer field: ") + Name);
            }

            return Value[Name].get<std::uint32_t>();
        }
    }

    [[nodiscard]] inline Json Serialize(const GraphIR& Graph)
    {
        Json Result{
            {"irVersion", 2},
            {"nodes", Json::array()},
            {"variables", Json::array()},
            {"inputBindings", Json::array()},
            {"controlEdges", Json::array()}
        };

        for (const NodeInstance& Node : Graph.GetNodes())
        {
            Result["nodes"].push_back({
                {"id", Node.Identifier.GetValue()},
                {"descriptor", Node.Descriptor.GetValue()}
                                      });
        }

        for (const GraphVariable& Variable : Graph.GetVariables())
        {
            Json Value{
                {"id", Variable.Identifier.GetValue()},
                {"name", Variable.Name},
                {"type", Detail::TypeToJson(Variable.Type)}
            };

            if (Variable.DefaultValue)
            {
                Value["default"] = Detail::LiteralToJson(
                    *Variable.DefaultValue);
            }

            Result["variables"].push_back(Value);
        }

        for (const InputBindingRecord& Record : Graph.GetInputBindings())
        {
            Json Value{
                {"destinationNode", Record.DestinationNode.GetValue()},
                {"destinationPin", Record.DestinationInputPin.GetValue()},
                {"outputTypeConstraint", Record.OutputTypeConstraint.has_value()
                    ? Detail::TypeToJson(*Record.OutputTypeConstraint)
                    : Json(nullptr)}
            };

            std::visit(
                [&Value](const auto& Binding)
                {
                    using Type = std::decay_t<decltype(Binding)>;

                    if constexpr (std::is_same_v<Type, LiteralValue>)
                    {
                        Value["binding"] = {
                            {"kind", "Literal"},
                            {"value", Detail::LiteralToJson(Binding)}
                        };
                    }
                    else if constexpr (std::is_same_v<Type, OutputReference>)
                    {
                        Value["binding"] = {
                            {"kind", "OutputReference"},
                            {"sourceNode", Binding.SourceNode.GetValue()},
                            {"sourcePin", Binding.SourceOutputPin.GetValue()}
                        };
                    }
                    else
                    {
                        Value["binding"] = {
                            {"kind", "GraphVariableReference"},
                            {"variable", Binding.Variable.GetValue()}
                        };
                    }
                },
                Record.Binding);

            Result["inputBindings"].push_back(Value);
        }

        for (const ControlEdge& Edge : Graph.GetControlEdges())
        {
            Result["controlEdges"].push_back({
                {"sourceNode", Edge.SourceNode.GetValue()},
                {"sourcePin", Edge.SourceOutputPin.GetValue()},
                {"destinationNode", Edge.DestinationNode.GetValue()},
                {"destinationPin", Edge.DestinationInputPin.GetValue()}
                                             });
        }

        return Result;
    }

    [[nodiscard]] inline ParseResult Deserialize(const Json& Root)
    {
        try
        {
            if (!Root.is_object()
                || !Root.contains("irVersion")
                || !Root.contains("nodes")
                || !Root.contains("variables")
                || !Root.contains("inputBindings")
                || !Root.contains("controlEdges")
                || !Root["nodes"].is_array()
                || !Root["variables"].is_array()
                || !Root["inputBindings"].is_array()
                || !Root["controlEdges"].is_array())
            {
                return Detail::Fail<GraphIR>(
                    "Malformed GraphIR JSON root.");
            }

            const Json& VersionValue = Root["irVersion"];
            std::uint32_t Version = 0U;
            if (VersionValue.is_number_unsigned())
            {
                const std::uint64_t ParsedVersion = VersionValue.get<std::uint64_t>();
                if (ParsedVersion < 1U || ParsedVersion > 2U)
                {
                    return Detail::Fail<GraphIR>(
                        "Unsupported GraphIR JSON version.");
                }
                Version = static_cast<std::uint32_t>(ParsedVersion);
            }
            else if (VersionValue.is_number_integer())
            {
                const std::int64_t ParsedVersion = VersionValue.get<std::int64_t>();
                if (ParsedVersion < 1 || ParsedVersion > 2)
                {
                    return Detail::Fail<GraphIR>(
                        "Unsupported GraphIR JSON version.");
                }
                Version = static_cast<std::uint32_t>(ParsedVersion);
            }
            else
            {
                return Detail::Fail<GraphIR>(
                    "Unsupported GraphIR JSON version.");
            }

            GraphIR Graph;

            for (const Json& Value : Root["nodes"])
            {
                if (!Detail::HasObjectFields(
                    Value,
                    {"id", "descriptor"})
                    || !Value["id"].is_number_unsigned())
                {
                    return Detail::Fail<GraphIR>(
                        "Malformed node entry.");
                }

                const auto Descriptor = Detail::UInt32(
                    Value,
                    "descriptor");

                if (!Descriptor)
                {
                    return std::unexpected(Descriptor.error());
                }

                Graph.AddNode({
                    NodeInstanceId(
                        Detail::Id(Value, "id")),
                    NodeDescriptorId(*Descriptor)
                              });
            }

            for (const Json& Value : Root["variables"])
            {
                if (!Detail::HasObjectFields(
                    Value,
                    {"id", "name", "type"})
                    || !Value["id"].is_number_unsigned()
                    || !Value["name"].is_string())
                {
                    return Detail::Fail<GraphIR>(
                        "Malformed graph variable entry.");
                }

                const auto Type = Detail::TypeFromJson(
                    Value["type"]);

                if (!Type)
                {
                    return std::unexpected(Type.error());
                }

                std::optional<LiteralValue> Default;

                if (Value.contains("default"))
                {
                    const auto Parsed = Detail::LiteralFromJson(
                        Value["default"]);

                    if (!Parsed)
                    {
                        return std::unexpected(Parsed.error());
                    }

                    Default = *Parsed;
                }

                Graph.AddVariable({
                    GraphVariableId(
                        Detail::Id(Value, "id")),
                    Value["name"].get<std::string>(),
                    *Type,
                    Default
                                  });
            }

            for (const Json& Value : Root["inputBindings"])
            {
                const bool HasRequiredBindingFields = Version == 1U
                    ? Detail::HasObjectFields(
                        Value,
                        {"destinationNode", "destinationPin", "binding"})
                    : Detail::HasObjectFields(
                        Value,
                        {
                            "destinationNode",
                            "destinationPin",
                            "binding",
                            "outputTypeConstraint"
                        });
                if (!HasRequiredBindingFields
                    || (Version == 1U && Value.contains("outputTypeConstraint"))
                    || !Value["binding"].is_object())
                {
                    return Detail::Fail<GraphIR>(
                        "Malformed input binding entry.");
                }

                const auto DestinationPin = Detail::UInt32(
                    Value,
                    "destinationPin");

                if (!DestinationPin)
                {
                    return std::unexpected(DestinationPin.error());
                }

                std::optional<TypeDesc> OutputTypeConstraint;
                if (Version == 2U && !Value["outputTypeConstraint"].is_null())
                {
                    const auto ParsedConstraint = Detail::TypeFromJson(
                        Value["outputTypeConstraint"]);
                    if (!ParsedConstraint)
                    {
                        return std::unexpected(ParsedConstraint.error());
                    }
                    OutputTypeConstraint = *ParsedConstraint;
                }

                const Json& Binding = Value["binding"];

                if (!Binding.contains("kind")
                    || !Binding["kind"].is_string())
                {
                    return Detail::Fail<GraphIR>(
                        "Input binding kind is missing.");
                }

                const std::string Kind =
                    Binding["kind"].get<std::string>();

                InputBinding Parsed;

                if (Kind == "Literal"
                    && Binding.contains("value"))
                {
                    const auto Literal = Detail::LiteralFromJson(
                        Binding["value"]);

                    if (!Literal)
                    {
                        return std::unexpected(Literal.error());
                    }

                    Parsed = *Literal;
                }
                else if (
                    Kind == "OutputReference"
                    && Detail::HasObjectFields(
                        Binding,
                        {"sourceNode", "sourcePin"}))
                {
                    const auto SourcePin = Detail::UInt32(
                        Binding,
                        "sourcePin");

                    if (!SourcePin)
                    {
                        return std::unexpected(SourcePin.error());
                    }

                    Parsed = OutputReference{
                        NodeInstanceId(
                            Detail::Id(Binding, "sourceNode")),
                        PinIndex(*SourcePin)
                    };
                }
                else if (
                    Kind == "GraphVariableReference"
                    && Binding.contains("variable")
                    && Binding["variable"].is_number_unsigned())
                {
                    Parsed = GraphVariableReference{
                        GraphVariableId(
                            Detail::Id(Binding, "variable"))
                    };
                }
                else
                {
                    return Detail::Fail<GraphIR>(
                        "Malformed or unknown input binding kind.");
                }

                Graph.BindInput(
                    NodeInstanceId(
                        Detail::Id(Value, "destinationNode")),
                    PinIndex(*DestinationPin),
                    std::move(Parsed),
                    std::move(OutputTypeConstraint));
            }

            for (const Json& Value : Root["controlEdges"])
            {
                if (!Detail::HasObjectFields(
                    Value,
                    {
                        "sourceNode",
                        "sourcePin",
                        "destinationNode",
                        "destinationPin"
                    }))
                {
                    return Detail::Fail<GraphIR>(
                        "Malformed control edge entry.");
                }

                const auto SourcePin = Detail::UInt32(
                    Value,
                    "sourcePin");

                const auto DestinationPin = Detail::UInt32(
                    Value,
                    "destinationPin");

                if (!SourcePin)
                {
                    return std::unexpected(SourcePin.error());
                }

                if (!DestinationPin)
                {
                    return std::unexpected(DestinationPin.error());
                }

                Graph.AddControlEdge({
                    NodeInstanceId(
                        Detail::Id(Value, "sourceNode")),
                    PinIndex(*SourcePin),
                    NodeInstanceId(
                        Detail::Id(Value, "destinationNode")),
                    PinIndex(*DestinationPin)
                                     });
            }

            return Graph;
        }
        catch (const std::exception& Exception)
        {
            return Detail::Fail<GraphIR>(
                std::string("Malformed GraphIR JSON: ")
                + Exception.what());
        }
    }
}
