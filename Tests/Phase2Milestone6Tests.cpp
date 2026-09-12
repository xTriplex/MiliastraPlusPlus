#include <cstdlib>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraphIRJson.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

using namespace MiliastraPlusPlus;
using namespace MiliastraPlusPlus::GraphIRJson;

namespace
{
    void Check(bool Condition)
    {
        if (!Condition)
        {
            std::abort();
        }
    }

    void CheckType(const TypeDesc& Type)
    {
        GraphIR Graph;
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U), "Value", Type, std::nullopt
        });
        const auto Parsed = Deserialize(Serialize(Graph));
        Check(Parsed.has_value());
        Check(Parsed->GetVariables().front().Type == Type);
    }

    void CheckLiteral(const LiteralValue& Literal)
    {
        GraphIR Graph;
        Graph.AddVariable(GraphVariable{
            GraphVariableId(1U), "Value", TypeDesc::Generic(GenericParameterId(1U)), Literal
        });
        const auto Parsed = Deserialize(Serialize(Graph));
        Check(Parsed.has_value());
        Check(Parsed->GetVariables().front().DefaultValue == Literal);
    }
}

int main()
{
    GraphIR Empty;
    Check(Deserialize(Serialize(Empty)).has_value());

    CheckType(TypeDesc::Boolean());
    CheckType(TypeDesc::Integer());
    CheckType(TypeDesc::Float());
    CheckType(TypeDesc::String());
    CheckType(TypeDesc::Flow());
    CheckType(TypeDesc::Entity());
    CheckType(TypeDesc::GUID());
    CheckType(TypeDesc::Vector3());
    CheckType(TypeDesc::PrefabId());
    CheckType(TypeDesc::ConfigId());
    CheckType(TypeDesc::Faction());
    CheckType(TypeDesc::Generic(GenericParameterId(4U)));
    CheckType(TypeDesc::List(TypeDesc::Dictionary(
        TypeDesc::String(), TypeDesc::List(TypeDesc::Integer()))));
    CheckType(TypeDesc::StructObject(StructTypeId(8U)));

    CheckLiteral(LiteralValue{});
    CheckLiteral(LiteralValue(LiteralValue::Data{true}));
    CheckLiteral(LiteralValue(LiteralValue::Data{std::int64_t{-42}}));
    CheckLiteral(LiteralValue(LiteralValue::Data{3.5}));
    CheckLiteral(LiteralValue(LiteralValue::Data{std::string("text")}));
    CheckLiteral(LiteralValue(LiteralValue::Data{GuidValue{11U}}));
    CheckLiteral(LiteralValue(LiteralValue::Data{Vector3Value{1.0F, 2.0F, 3.0F}}));
    CheckLiteral(LiteralValue(LiteralValue::Data{PrefabIdValue{12U}}));
    CheckLiteral(LiteralValue(LiteralValue::Data{ConfigIdValue{13U}}));
    CheckLiteral(LiteralValue(LiteralValue::Data{FactionValue{14U}}));

    GraphIR Original;
    Original.AddNode(NodeInstance{NodeInstanceId(1U), NodeDescriptorId(7U)});
    Original.AddNode(NodeInstance{NodeInstanceId(2U), NodeDescriptorId(7U)});
    Original.AddVariable(GraphVariable{
        GraphVariableId(1U), "Score", TypeDesc::Integer(),
        LiteralValue(LiteralValue::Data{std::int64_t{9}})
    });
    Original.AddVariable(GraphVariable{
        GraphVariableId(2U), "NoDefault", TypeDesc::Float(), std::nullopt
    });
    Original.BindInput(
        NodeInstanceId(1U), PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{1}})
    );
    Original.BindInput(
        NodeInstanceId(1U), PinIndex(1U),
        OutputReference{NodeInstanceId(2U), PinIndex(3U)},
        TypeDesc::List(TypeDesc::Integer())
    );
    Original.BindInput(
        NodeInstanceId(2U), PinIndex(2U),
        GraphVariableReference{GraphVariableId(1U)}
    );
    Original.AddControlEdge(ControlEdge{
        NodeInstanceId(1U), PinIndex(4U), NodeInstanceId(2U), PinIndex(5U)
    });
    const nlohmann::json Serialized = Serialize(Original);
    Check(Serialized["irVersion"] == 2);
    Check(Serialized["inputBindings"][0U]["outputTypeConstraint"].is_null());
    Check(Serialized["inputBindings"][1U]["outputTypeConstraint"]["kind"] == "List");
    Check(Serialized.dump() == Serialize(Original).dump());
    const auto RoundTrip = Deserialize(Serialized);
    Check(RoundTrip.has_value());
    Check(Serialize(*RoundTrip).dump() == Serialized.dump());
    Check(RoundTrip->GetNodeCount() == 2U);
    Check(RoundTrip->GetVariableCount() == 2U);
    Check(RoundTrip->GetInputBindingCount() == 3U);
    Check(RoundTrip->GetControlEdgeCount() == 1U);
    Check(RoundTrip->GetInputBindings()[1].DestinationInputPin == PinIndex(1U));
    Check(std::get<OutputReference>(RoundTrip->GetInputBindings()[1].Binding).SourceNode == NodeInstanceId(2U));
    Check(RoundTrip->GetInputBindings()[0U].OutputTypeConstraint == std::nullopt);
    Check(RoundTrip->GetInputBindings()[1U].OutputTypeConstraint ==
        TypeDesc::List(TypeDesc::Integer()));
    Check(RoundTrip->GetInputBindings()[2U].OutputTypeConstraint == std::nullopt);

    nlohmann::json VersionOne = {
        {"irVersion", 1},
        {"nodes", nlohmann::json::array({
            {{"id", 1U}, {"descriptor", 7U}},
            {{"id", 2U}, {"descriptor", 7U}}
        })},
        {"variables", nlohmann::json::array()},
        {"inputBindings", nlohmann::json::array({
            {
                {"destinationNode", 1U},
                {"destinationPin", 0U},
                {"binding", {
                    {"kind", "OutputReference"},
                    {"sourceNode", 2U},
                    {"sourcePin", 3U}
                }}
            }
        })},
        {"controlEdges", nlohmann::json::array()}
    };
    const auto LegacyGraph = Deserialize(VersionOne);
    Check(LegacyGraph.has_value());
    Check(LegacyGraph->GetNodeCount() == 2U);
    Check(LegacyGraph->GetInputBindingCount() == 1U);
    Check(!LegacyGraph->GetInputBindings()[0U].OutputTypeConstraint.has_value());
    Check(std::get<OutputReference>(LegacyGraph->GetInputBindings()[0U].Binding) ==
        OutputReference{NodeInstanceId(2U), PinIndex(3U)});
    const nlohmann::json UpgradedLegacyGraph = Serialize(*LegacyGraph);
    Check(UpgradedLegacyGraph["irVersion"] == 2);
    Check(UpgradedLegacyGraph["inputBindings"][0U]["outputTypeConstraint"].is_null());
    Check(UpgradedLegacyGraph["nodes"] == VersionOne["nodes"]);
    Check(UpgradedLegacyGraph["inputBindings"][0U]["binding"] ==
        VersionOne["inputBindings"][0U]["binding"]);

    nlohmann::json VersionTwoMissingConstraint = Serialize(Original);
    VersionTwoMissingConstraint["inputBindings"][0U].erase("outputTypeConstraint");
    Check(!Deserialize(VersionTwoMissingConstraint).has_value());

    nlohmann::json VersionOneUnexpectedConstraint = VersionOne;
    VersionOneUnexpectedConstraint["inputBindings"][0U]["outputTypeConstraint"] = nullptr;
    Check(!Deserialize(VersionOneUnexpectedConstraint).has_value());

    for (const nlohmann::json UnsupportedVersion : {
        nlohmann::json(-1),
        nlohmann::json(0U),
        nlohmann::json(3U),
        nlohmann::json(2.0),
        nlohmann::json(std::numeric_limits<std::uint64_t>::max())
    })
    {
        nlohmann::json UnsupportedVersionGraph = Serialize(Empty);
        UnsupportedVersionGraph["irVersion"] = UnsupportedVersion;
        Check(!Deserialize(UnsupportedVersionGraph).has_value());
    }

    const auto DeserializeIntegerDefault = [](const nlohmann::json& Number)
    {
        nlohmann::json Document = Serialize(GraphIR{});
        Document["variables"].push_back({
            {"id", 1U},
            {"name", "Integer"},
            {"type", Detail::TypeToJson(TypeDesc::Integer())},
            {"default", {{"kind", "Integer"}, {"value", Number}}}
        });
        return Deserialize(Document);
    };
    Check(DeserializeIntegerDefault(
        nlohmann::json(std::numeric_limits<std::int64_t>::min())).has_value());
    Check(DeserializeIntegerDefault(
        nlohmann::json(std::numeric_limits<std::int64_t>::max())).has_value());
    Check(!DeserializeIntegerDefault(nlohmann::json(
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U
    )).has_value());
    Check(!DeserializeIntegerDefault(nlohmann::json(
        std::numeric_limits<std::uint64_t>::max()
    )).has_value());

    GraphIR InvalidIds;
    InvalidIds.AddNode(NodeInstance{NodeInstanceId{}, NodeDescriptorId{}});
    InvalidIds.AddVariable(GraphVariable{GraphVariableId{}, "", TypeDesc{}, std::nullopt});
    const auto InvalidRoundTrip = Deserialize(Serialize(InvalidIds));
    Check(InvalidRoundTrip.has_value());
    Check(!InvalidRoundTrip->GetNodes().front().Identifier.IsValid());
    Check(!InvalidRoundTrip->GetVariables().front().Identifier.IsValid());

    Check(!Deserialize(nlohmann::json::object()).has_value());
    nlohmann::json MissingField = {
        {"irVersion", 1}, {"nodes", nlohmann::json::array()},
        {"variables", nlohmann::json::array()},
        {"inputBindings", nlohmann::json::array()},
        {"controlEdges", nlohmann::json::array()}
    };
    MissingField.erase("nodes");
    Check(!Deserialize(MissingField).has_value());
    nlohmann::json UnknownField = Serialize(Empty);
    UnknownField["extra"] = 4;
    Check(Deserialize(UnknownField).has_value());
    nlohmann::json Unknown = Serialize(Empty);
    Unknown["nodes"].push_back({{"id", 1U}, {"descriptor", 2U}, {"kind", "unknown"}});
    Check(Deserialize(Unknown).has_value());

    nlohmann::json UnknownType = Serialize(Empty);
    UnknownType["variables"].push_back({
        {"id", 1U}, {"name", "Bad"}, {"type", {{"kind", "FutureType"}}}
    });
    Check(!Deserialize(UnknownType).has_value());

    nlohmann::json Overflow = Serialize(Empty);
    Overflow["nodes"].push_back({
        {"id", 1U}, {"descriptor", 0x100000000ULL}
    });
    Check(!Deserialize(Overflow).has_value());

    return 0;
}
