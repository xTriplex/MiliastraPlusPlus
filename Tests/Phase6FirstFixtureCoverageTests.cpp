#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDescriptorCatalogueSnapshot.h"
#include "MiliastraPlusPlusGenshinClientBooleanFilterResultNodeSourceAdapter.h"
#include "MiliastraPlusPlusGraphIRJson.h"

using namespace MiliastraPlusPlus;

#define MPP_CHECK(Expression)                                                   \
    do                                                                          \
    {                                                                           \
        if (!(Expression))                                                      \
        {                                                                       \
            std::fprintf(stderr, "Check failed at line %d.\n", __LINE__);      \
            std::abort();                                                       \
        }                                                                       \
    } while (false)

namespace
{
    using Json = nlohmann::json;

    constexpr char NodeMetadataJson[] = R"json([
{
    "subType": "bool_filter",
    "nodeType": "node_graph_end_boolean",
    "displayName": "节点图结束(布尔型)",
    "graphType": 20001,
    "genericId": 200000,
    "concreteId": 0,
    "inputs": [
        {
            "index": 0,
            "kind": "input",
            "type": "bool",
            "clientVarType": 5,
            "defaultValue": 0,
            "name": "输出结果（布尔型）",
            "connectable": true,
            "connectionType": 5
        },
        {
            "index": 1,
            "kind": "input",
            "type": "enum",
            "clientVarType": 13,
            "defaultValue": 1000010,
            "name": "filter返回类型",
            "connectable": true,
            "connectionType": 210040
        }
    ],
    "outputs": [],
    "sampleFile": "布尔过滤器节点\\查询实体是否在场_连线.gia",
    "flows": []
}
])json";

    constexpr char ModesJson[] = R"json({
    "format": 1,
    "graphs": {
        "bool_filter": {
            "entryGenericId": 200000,
            "beyond": {
                "status": "available"
            }
        }
    }
})json";

    constexpr char EnumEvidenceJson[] = R"json({
    "family": "filter_return_type",
    "ioc": 38,
    "members": [
        {
            "identity": "filter_return_type_return_boolean",
            "value": 1000010
        },
        {
            "identity": "filter_return_type_return_integer",
            "value": 10000011
        }
    ]
})json";

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return std::any_of(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& DiagnosticValue)
            {
                return DiagnosticValue.Code == Code;
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeRecord(
        std::string Identity,
        TypeDesc Type = TypeDesc::Boolean(),
        std::optional<LiteralValue> Default = std::nullopt
    )
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(Identity)),
            "Fixture descriptor",
            {NodeAvailability::Client},
            {
                NormalizedPinRecord(
                    "Value",
                    std::move(Type),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    Default.has_value(),
                    std::move(Default)
                )
            },
            std::nullopt,
            SourceProvenance("fixture.document", "fixture.record")
        );
    }

    NormalizedNodeDescriptorRecord MakeEnumRecord(
        std::string Identity = "enum-record",
        std::string EnumIdentity = "filter_return_type",
        std::int64_t EnumValue = 1000010
    )
    {
        const EnumTypeIdentity TypeIdentity(std::move(EnumIdentity));
        return MakeRecord(
            std::move(Identity),
            TypeDesc::Enum(TypeIdentity),
            LiteralValue(LiteralValue::Data{
                EnumLiteralValue(TypeIdentity, EnumValue)
            })
        );
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeLegacyRecords()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.reserve(25U);
        for (std::uint32_t Index = 1U; Index <= 25U; ++Index)
        {
            Records.push_back(MakeRecord("legacy-" + std::to_string(Index)));
        }
        return Records;
    }

    std::expected<DescriptorCatalogue, DiagnosticCollection> BuildCombinedCatalogue()
    {
        const auto Result =
            GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
                NodeMetadataJson,
                ModesJson,
                EnumEvidenceJson
            );
        if (!Result.has_value())
        {
            return std::unexpected(Result.error());
        }

        std::vector<NormalizedNodeDescriptorRecord> Records = MakeLegacyRecords();
        Records.push_back(*Result);
        return DescriptorCatalogueBuilder::Build(
            "genshin.client-bool-filter-descriptor-source",
            "genshin-ts@26bdf2a9a3fadba934423940489236f0b53eb3ea;resources/client_node_metadata.json@93237c724f6453650ae9394077620c6e0fddb3d3;resources/client_node_modes.json@b7e14a0dd7102ccd682235cf958a2d3d36378033;src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/node_data/client_enum_values.ts@17b7d80dcd414739bd42093c6142f93b2af2cf6c;src/definitions/client_enums.ts@b23535b9054b60d2069e88c13316958124aaa8a8",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
    }

    Json MakeEnumTypeJson()
    {
        return Json{
            {"kind", "Enum"},
            {"identity", "filter_return_type"}
        };
    }

    Json MakeEnumLiteralJson()
    {
        return Json{
            {"kind", "Enum"},
            {"enumIdentity", "filter_return_type"},
            {"value", 1000010}
        };
    }

    Json MakeGraphJson(std::uint32_t Version)
    {
        Json Root{
            {"irVersion", Version},
            {"nodes", Json::array()},
            {"variables", Json::array()},
            {"inputBindings", Json::array()},
            {"controlEdges", Json::array()}
        };
        if (Version == 3U)
        {
            Root["executionModel"] = "Unstructured";
            Root["executionEntries"] = Json::array();
            Root["executionRegions"] = Json::array();
        }
        return Root;
    }

    void TestEnumTypeIdentityAndLiteralValue()
    {
        const EnumTypeIdentity Invalid;
        const EnumTypeIdentity BooleanFamily("filter_return_type");
        const EnumTypeIdentity IntegerFamily("other_family");
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(BooleanFamily.IsValid());
        MPP_CHECK(EnumTypeIdentity("filter_return_type") == BooleanFamily);
        MPP_CHECK(EnumTypeIdentity("Filter_Return_Type") != BooleanFamily);
        MPP_CHECK(EnumTypeIdentity(" filter_return_type") != BooleanFamily);
        MPP_CHECK(EnumTypeIdentity("a") < EnumTypeIdentity("b"));

        const EnumLiteralValue Default(BooleanFamily, 1000010);
        const EnumLiteralValue Zero(BooleanFamily, 0);
        const EnumLiteralValue Negative(BooleanFamily, -1);
        MPP_CHECK(Default.IsValid());
        MPP_CHECK(Zero.IsValid());
        MPP_CHECK(Negative.IsValid());
        MPP_CHECK(Default.GetValue() == 1000010);
        MPP_CHECK(Default.GetEnumTypeIdentity() == BooleanFamily);
        MPP_CHECK(Default != EnumLiteralValue(IntegerFamily, 1000010));
        MPP_CHECK(Default < EnumLiteralValue(BooleanFamily, 1000011));
    }

    void TestTypeDescEnumCompatibilityAndUnification()
    {
        const EnumTypeIdentity BooleanFamily("filter_return_type");
        const EnumTypeIdentity OtherFamily("other_family");
        const TypeDesc EnumType = TypeDesc::Enum(BooleanFamily);
        const TypeDesc SameEnumType = TypeDesc::Enum(BooleanFamily);
        const TypeDesc OtherEnumType = TypeDesc::Enum(OtherFamily);
        MPP_CHECK(EnumType.IsValid());
        MPP_CHECK(EnumType == SameEnumType);
        MPP_CHECK(EnumType != OtherEnumType);
        MPP_CHECK(EnumType.IsCompatibleWith(SameEnumType));
        MPP_CHECK(!EnumType.IsCompatibleWith(OtherEnumType));
        MPP_CHECK(!EnumType.IsCompatibleWith(TypeDesc::Integer()));
        MPP_CHECK(!TypeDesc::Integer().IsCompatibleWith(EnumType));
        MPP_CHECK(TypeDesc::List(EnumType).IsCompatibleWith(TypeDesc::List(SameEnumType)));
        MPP_CHECK(!TypeDesc::List(EnumType).IsCompatibleWith(TypeDesc::List(OtherEnumType)));

        const auto Unified = EnumType.Unify(SameEnumType);
        MPP_CHECK(Unified.has_value() && *Unified == EnumType);
        MPP_CHECK(!EnumType.Unify(OtherEnumType).has_value());

        const LiteralValue EnumLiteral(LiteralValue::Data{
            EnumLiteralValue(BooleanFamily, 1000010)
        });
        const LiteralValue OtherLiteral(LiteralValue::Data{
            EnumLiteralValue(OtherFamily, 1000010)
        });
        MPP_CHECK(EnumLiteral.Is<EnumLiteralValue>());
        MPP_CHECK(EnumLiteral.TryGet<EnumLiteralValue>() != nullptr);
        MPP_CHECK(EnumType.IsCompatibleWith(TypeDesc::Enum(
            EnumLiteral.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity())));
        MPP_CHECK(EnumLiteral != LiteralValue(LiteralValue::Data{std::int64_t(1000010)}));
        MPP_CHECK(OtherLiteral != EnumLiteral);
    }

    void TestDescriptorEnumDefaultValidation()
    {
        const EnumTypeIdentity Family("filter_return_type");
        const LiteralValue Matching(LiteralValue::Data{
            EnumLiteralValue(Family, 1000010)
        });
        const LiteralValue Mismatched(LiteralValue::Data{
            EnumLiteralValue(EnumTypeIdentity("other_family"), 1000010)
        });
        const NodeDescriptor Valid(
            NodeDescriptorId(1U),
            "Enum descriptor",
            {NodeAvailability::Client},
            {
                PinSchema(
                    "Value",
                    TypeDesc::Enum(Family),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    Matching
                )
            }
        );
        const NodeDescriptor Invalid(
            NodeDescriptorId(1U),
            "Enum descriptor",
            {NodeAvailability::Client},
            {
                PinSchema(
                    "Value",
                    TypeDesc::Enum(Family),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    Mismatched
                )
            }
        );
        MPP_CHECK(Valid.IsValid());
        MPP_CHECK(!Invalid.IsValid());
    }

    void TestCatalogueSchemaAndContentIdentity()
    {
        MPP_CHECK(CurrentDescriptorCatalogueSemanticSchemaVersion.GetValue() == 2U);
        const auto SchemaOne = DescriptorCatalogueBuilder::Build(
            "schema.tests",
            "schema.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(1U),
            {MakeRecord("legacy")}
        );
        MPP_CHECK(SchemaOne.has_value());

        const auto SchemaOneEnum = DescriptorCatalogueBuilder::Build(
            "schema.tests",
            "schema.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(1U),
            {MakeEnumRecord()}
        );
        MPP_CHECK(!SchemaOneEnum.has_value());
        MPP_CHECK(HasCode(
            SchemaOneEnum.error(),
            DiagnosticCode::InvalidNormalizedDescriptorRecord
        ));

        const auto SchemaTwoEnum = DescriptorCatalogueBuilder::Build(
            "schema.tests",
            "schema.tests@2",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord()}
        );
        MPP_CHECK(SchemaTwoEnum.has_value());

        const auto Unsupported = DescriptorCatalogueBuilder::Build(
            "schema.tests",
            "schema.tests@3",
            DescriptorCatalogueSemanticSchemaVersion(3U),
            {}
        );
        MPP_CHECK(!Unsupported.has_value());
        MPP_CHECK(HasCode(
            Unsupported.error(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion
        ));
    }

    void TestCatalogueEnumContentIdentityAndPermutation()
    {
        const auto First = DescriptorCatalogueBuilder::Build(
            "content.tests",
            "content.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord("enum", "filter_return_type", 1000010)}
        );
        const auto OtherFamily = DescriptorCatalogueBuilder::Build(
            "content.tests",
            "content.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord("enum", "other_family", 1000010)}
        );
        const auto OtherValue = DescriptorCatalogueBuilder::Build(
            "content.tests",
            "content.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord("enum", "filter_return_type", 1000011)}
        );
        MPP_CHECK(First.has_value());
        MPP_CHECK(OtherFamily.has_value());
        MPP_CHECK(OtherValue.has_value());
        MPP_CHECK(First->GetIdentity().GetCatalogueContentIdentifier() !=
            OtherFamily->GetIdentity().GetCatalogueContentIdentifier());
        MPP_CHECK(First->GetIdentity().GetCatalogueContentIdentifier() !=
            OtherValue->GetIdentity().GetCatalogueContentIdentifier());

        auto FirstRecords = MakeLegacyRecords();
        auto SecondRecords = MakeLegacyRecords();
        FirstRecords.push_back(MakeEnumRecord("enum-a"));
        SecondRecords.push_back(MakeEnumRecord("enum-a"));
        std::reverse(SecondRecords.begin(), SecondRecords.end());
        const auto FirstCatalogue = DescriptorCatalogueBuilder::Build(
            "content.tests", "content.tests@2", DescriptorCatalogueSemanticSchemaVersion(2U),
            std::move(FirstRecords)
        );
        const auto SecondCatalogue = DescriptorCatalogueBuilder::Build(
            "content.tests", "content.tests@2", DescriptorCatalogueSemanticSchemaVersion(2U),
            std::move(SecondRecords)
        );
        MPP_CHECK(FirstCatalogue.has_value());
        MPP_CHECK(SecondCatalogue.has_value());
        MPP_CHECK(*FirstCatalogue == *SecondCatalogue);
    }

    void TestGraphIRV3EnumRoundTrip()
    {
        Json Root = MakeGraphJson(3U);
        Root["variables"].push_back({
            {"id", 1U},
            {"name", "ReturnType"},
            {"type", MakeEnumTypeJson()},
            {"default", MakeEnumLiteralJson()}
        });
        Root["variables"].push_back({
            {"id", 2U},
            {"name", "NestedReturnType"},
            {"type", {
                {"kind", "List"},
                {"element", MakeEnumTypeJson()}
            }}
        });
        Root["inputBindings"].push_back({
            {"destinationNode", 1U},
            {"destinationPin", 0U},
            {"outputTypeConstraint", MakeEnumTypeJson()},
            {"binding", {
                {"kind", "Literal"},
                {"value", MakeEnumLiteralJson()}
            }}
        });

        const auto Parsed = GraphIRJson::Deserialize(Root);
        MPP_CHECK(Parsed.has_value());
        const Json RoundTrip = GraphIRJson::Serialize(*Parsed);
        MPP_CHECK(RoundTrip["irVersion"] == 3U);
        MPP_CHECK(RoundTrip["variables"][0U]["type"] == MakeEnumTypeJson());
        MPP_CHECK(RoundTrip["variables"][0U]["default"] == MakeEnumLiteralJson());
        MPP_CHECK(RoundTrip["variables"][1U]["type"]["element"] == MakeEnumTypeJson());
        MPP_CHECK(
            RoundTrip["inputBindings"][0U]["outputTypeConstraint"] ==
            MakeEnumTypeJson()
        );
        MPP_CHECK(
            RoundTrip["inputBindings"][0U]["binding"]["value"] ==
            MakeEnumLiteralJson()
        );
    }

    void TestGraphIRLegacyEnumRejection()
    {
        for (const std::uint32_t Version : {1U, 2U})
        {
            Json TypeDocument = MakeGraphJson(Version);
            TypeDocument["variables"].push_back({
                {"id", 1U},
                {"name", "EnumVariable"},
                {"type", MakeEnumTypeJson()}
            });
            MPP_CHECK(!GraphIRJson::Deserialize(TypeDocument).has_value());

            Json LiteralDocument = MakeGraphJson(Version);
            LiteralDocument["variables"].push_back({
                {"id", 1U},
                {"name", "IntegerVariable"},
                {"type", Json{{"kind", "Integer"}}},
                {"default", MakeEnumLiteralJson()}
            });
            MPP_CHECK(!GraphIRJson::Deserialize(LiteralDocument).has_value());
        }
    }

    void TestGraphIRLegacyVersionsRejectNestedEnumForms()
    {
        for (const std::uint32_t Version : {1U, 2U})
        {
            Json ListDocument = MakeGraphJson(Version);
            ListDocument["variables"].push_back({
                {"id", 1U},
                {"name", "NestedEnumList"},
                {"type", {
                    {"kind", "List"},
                    {"element", MakeEnumTypeJson()}
                }}
            });
            MPP_CHECK(!GraphIRJson::Deserialize(ListDocument).has_value());

            Json DictionaryDocument = MakeGraphJson(Version);
            DictionaryDocument["variables"].push_back({
                {"id", 1U},
                {"name", "NestedEnumDictionary"},
                {"type", {
                    {"kind", "Dictionary"},
                    {"key", Json{{"kind", "Integer"}}},
                    {"value", MakeEnumTypeJson()}
                }}
            });
            MPP_CHECK(!GraphIRJson::Deserialize(DictionaryDocument).has_value());
        }

        Json OutputTypeConstraintDocument = MakeGraphJson(2U);
        OutputTypeConstraintDocument["inputBindings"].push_back({
            {"destinationNode", 1U},
            {"destinationPin", 0U},
            {"outputTypeConstraint", {
                {"kind", "List"},
                {"element", MakeEnumTypeJson()}
            }},
            {"binding", {
                {"kind", "OutputReference"},
                {"sourceNode", 1U},
                {"sourcePin", 0U}
            }}
        });
        MPP_CHECK(!GraphIRJson::Deserialize(OutputTypeConstraintDocument).has_value());
    }

    void TestSnapshotV2EnumRoundTrip()
    {
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            "snapshot.enum.tests",
            "snapshot.enum.tests@2",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord()}
        );
        MPP_CHECK(Catalogue.has_value());
        const auto Snapshot = DescriptorCatalogueSnapshot::Create(*Catalogue);
        MPP_CHECK(Snapshot.has_value());
        const auto Written = DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Written.has_value());
        const Json Document = Json::parse(*Written);
        MPP_CHECK(Document["snapshotFormatVersion"] == 2U);
        MPP_CHECK(Document["entries"][0U]["record"]["pins"][0U]["type"] ==
            MakeEnumTypeJson());
        MPP_CHECK(Document["entries"][0U]["record"]["pins"][0U]["default"] ==
            MakeEnumLiteralJson());
        const auto Read = DescriptorCatalogueSnapshotPersistence::Read(*Written);
        MPP_CHECK(Read.has_value());
        MPP_CHECK(*Read == *Snapshot);
    }

    void TestSnapshotVersionSchemaMatrix()
    {
        const auto LegacyCatalogue = DescriptorCatalogueBuilder::Build(
            "snapshot.matrix.tests",
            "snapshot.matrix.tests@1",
            DescriptorCatalogueSemanticSchemaVersion(1U),
            {MakeRecord("legacy")}
        );
        MPP_CHECK(LegacyCatalogue.has_value());
        const auto LegacySnapshot = DescriptorCatalogueSnapshot::Create(*LegacyCatalogue);
        MPP_CHECK(LegacySnapshot.has_value());
        const auto LegacyJsonResult = DescriptorCatalogueSnapshotPersistence::Write(*LegacySnapshot);
        MPP_CHECK(LegacyJsonResult.has_value());
        Json LegacyJson = Json::parse(*LegacyJsonResult);
        LegacyJson["snapshotFormatVersion"] = 1U;
        MPP_CHECK(DescriptorCatalogueSnapshotPersistence::Read(LegacyJson.dump()).has_value());

        const auto EnumCatalogue = DescriptorCatalogueBuilder::Build(
            "snapshot.matrix.tests",
            "snapshot.matrix.tests@2",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            {MakeEnumRecord()}
        );
        MPP_CHECK(EnumCatalogue.has_value());
        const auto EnumSnapshot = DescriptorCatalogueSnapshot::Create(*EnumCatalogue);
        MPP_CHECK(EnumSnapshot.has_value());
        const auto EnumJsonResult = DescriptorCatalogueSnapshotPersistence::Write(*EnumSnapshot);
        MPP_CHECK(EnumJsonResult.has_value());
        Json EnumJson = Json::parse(*EnumJsonResult);

        EnumJson["snapshotFormatVersion"] = 1U;
        MPP_CHECK(!DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump()).has_value());
        MPP_CHECK(HasCode(
            DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump()).error(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion
        ));

        EnumJson = Json::parse(*EnumJsonResult);
        EnumJson["catalogue"]["semanticSchemaVersion"] = 1U;
        MPP_CHECK(!DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump()).has_value());
        MPP_CHECK(HasCode(
            DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump()).error(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        ));

        EnumJson = Json::parse(*EnumJsonResult);
        EnumJson["snapshotFormatVersion"] = 1U;
        EnumJson["catalogue"]["semanticSchemaVersion"] = 2U;
        const auto V1Schema2 = DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump());
        MPP_CHECK(!V1Schema2.has_value());
        MPP_CHECK(HasCode(
            V1Schema2.error(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion
        ));

        EnumJson = Json::parse(*EnumJsonResult);
        EnumJson["snapshotFormatVersion"] = 3U;
        const auto FutureVersion = DescriptorCatalogueSnapshotPersistence::Read(EnumJson.dump());
        MPP_CHECK(!FutureVersion.has_value());
        MPP_CHECK(HasCode(
            FutureVersion.error(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSnapshotVersion
        ));
    }

    void TestResultNodeAdapterAcceptsBoundedFixture()
    {
        const auto Result = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            NodeMetadataJson,
            ModesJson,
            EnumEvidenceJson
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetExternalIdentity().GetKey() == "200000");
        MPP_CHECK(Result->GetDisplayName() == "节点图结束(布尔型)");
        MPP_CHECK(Result->GetAvailability() == std::vector<NodeAvailability>{NodeAvailability::Client});
        MPP_CHECK(!Result->GetExecutionControlSchema().has_value());
        MPP_CHECK(Result->GetPins().size() == 2U);
        MPP_CHECK(Result->GetPins()[0U].GetName() == "输出结果（布尔型）");
        MPP_CHECK(Result->GetPins()[0U].GetType() == TypeDesc::Boolean());
        MPP_CHECK(Result->GetPins()[0U].GetDefaultValue()->Is<bool>());
        MPP_CHECK(*Result->GetPins()[0U].GetDefaultValue()->TryGet<bool>() == false);
        MPP_CHECK(Result->GetPins()[1U].GetName() == "filter返回类型");
        MPP_CHECK(Result->GetPins()[1U].GetType() ==
            TypeDesc::Enum(EnumTypeIdentity("filter_return_type")));
        MPP_CHECK(Result->GetPins()[1U].GetDefaultValue()->Is<EnumLiteralValue>());
        MPP_CHECK(Result->GetPins()[1U].GetDefaultValue()->TryGet<EnumLiteralValue>()->GetValue() == 1000010);
        MPP_CHECK(Result->GetSourceProvenance()->GetSourceRecordIdentifier() == "200000");
    }

    void TestResultNodeAdapterRejectsMalformedSource()
    {
        const auto Malformed = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            "{", ModesJson, EnumEvidenceJson);
        MPP_CHECK(!Malformed.has_value());
        MPP_CHECK(HasCode(
            Malformed.error(),
            DiagnosticCode::MalformedGenshinClientBooleanFilterResultNodeSource
        ));

        const auto Missing = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            "[]", ModesJson, EnumEvidenceJson);
        MPP_CHECK(!Missing.has_value());
        MPP_CHECK(HasCode(
            Missing.error(),
            DiagnosticCode::MissingGenshinClientBooleanFilterResultNodeSourceField
        ));
    }

    void TestResultNodeAdapterRejectsBoundedShapeViolations()
    {
        Json Metadata = Json::parse(NodeMetadataJson);
        Metadata[0U]["inputs"][0U]["defaultValue"] = false;
        const auto WrongBooleanDefault =
            GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
                Metadata.dump(), ModesJson, EnumEvidenceJson);
        MPP_CHECK(!WrongBooleanDefault.has_value());
        MPP_CHECK(HasCode(
            WrongBooleanDefault.error(),
            DiagnosticCode::UnsupportedGenshinClientBooleanFilterResultNodeSourceForm
        ));

        Metadata = Json::parse(NodeMetadataJson);
        Metadata[0U]["concreteId"] = 1U;
        const auto WrongConcreteId =
            GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
                Metadata.dump(), ModesJson, EnumEvidenceJson);
        MPP_CHECK(!WrongConcreteId.has_value());

        Metadata = Json::parse(NodeMetadataJson);
        Metadata[0U]["inputs"][1U]["connectionType"] = 13U;
        const auto WrongEnumConnection =
            GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
                Metadata.dump(), ModesJson, EnumEvidenceJson);
        MPP_CHECK(!WrongEnumConnection.has_value());
    }

    void TestResultNodeAdapterRejectsEvidenceAndModeViolations()
    {
        Json Evidence = Json::parse(EnumEvidenceJson);
        Evidence["ioc"] = 39U;
        const auto WrongIoc = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            NodeMetadataJson, ModesJson, Evidence.dump());
        MPP_CHECK(!WrongIoc.has_value());

        Json Modes = Json::parse(ModesJson);
        Modes["graphs"]["bool_filter"]["beyond"]["status"] = "unavailable";
        const auto WrongMode = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            NodeMetadataJson, Modes.dump(), EnumEvidenceJson);
        MPP_CHECK(!WrongMode.has_value());
    }

    void TestResultNodeAdapterDeterminismAndFailureAtomicity()
    {
        const auto First = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            NodeMetadataJson, ModesJson, EnumEvidenceJson);
        const auto Second = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            NodeMetadataJson, ModesJson, EnumEvidenceJson);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);

        Json Metadata = Json::parse(NodeMetadataJson);
        Metadata[0U]["unsupported"] = true;
        const auto Invalid = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            Metadata.dump(), ModesJson, EnumEvidenceJson);
        MPP_CHECK(!Invalid.has_value());
        MPP_CHECK(!Invalid.error().empty());
        MPP_CHECK(Invalid.error()[0U].Severity == DiagnosticSeverity::Error);
    }

    void TestResultNodeAdapterOrdersDiagnosticsByValidationStage()
    {
        Json Metadata = Json::parse(NodeMetadataJson);
        Metadata[0U]["subType"] = "not_bool_filter";
        Metadata[0U]["inputs"][0U]["defaultValue"] = false;

        Json Modes = Json::parse(ModesJson);
        Modes["graphs"]["bool_filter"]["beyond"]["status"] = "unavailable";

        Json Evidence = Json::parse(EnumEvidenceJson);
        Evidence["ioc"] = 39U;

        const auto Result = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
            Metadata.dump(),
            Modes.dump(),
            Evidence.dump()
        );
        MPP_CHECK(!Result.has_value());
        const DiagnosticCollection& Diagnostics = Result.error();
        MPP_CHECK(Diagnostics.size() == 4U);
        MPP_CHECK(
            Diagnostics[0U].Message.find("node_graph_end_boolean.subType") !=
            std::string::npos
        );
        MPP_CHECK(
            Diagnostics[1U].Message.find("Input defaultValue") !=
            std::string::npos
        );
        MPP_CHECK(
            Diagnostics[2U].Message.find(
                "modes.graphs.bool_filter.beyond.status") !=
            std::string::npos
        );
        MPP_CHECK(
            Diagnostics[3U].Message.find("enumEvidence.ioc") !=
            std::string::npos
        );
    }

    void TestCombinedCatalogueHasTwentySixRecords()
    {
        const auto Catalogue = BuildCombinedCatalogue();
        MPP_CHECK(Catalogue.has_value());
        MPP_CHECK(Catalogue->GetEntryCount() == 26U);
        const auto* Result = Catalogue->FindByExternalIdentity(
            ExternalNodeIdentity("200000"));
        MPP_CHECK(Result != nullptr);
        MPP_CHECK(Result->GetRecord().GetPins().size() == 2U);
        MPP_CHECK(Catalogue->GetIdentity().GetSemanticSchemaVersion().GetValue() == 2U);
        MPP_CHECK(Catalogue->GetIdentity().GetSourceNamespace() ==
            "genshin.client-bool-filter-descriptor-source");
    }

    void TestLegacyP53ScopeRemainsTwentyFiveRecords()
    {
        const auto Records = MakeLegacyRecords();
        MPP_CHECK(Records.size() == 25U);
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            "historical.p53",
            "historical.p53@1",
            DescriptorCatalogueSemanticSchemaVersion(1U),
            Records
        );
        MPP_CHECK(Catalogue.has_value());
        MPP_CHECK(Catalogue->GetEntryCount() == 25U);
    }

    void TestRegistryMaterializationPreservesEnum()
    {
        const auto Catalogue = BuildCombinedCatalogue();
        MPP_CHECK(Catalogue.has_value());
        const auto Snapshot = DescriptorCatalogueSnapshot::Create(*Catalogue);
        MPP_CHECK(Snapshot.has_value());
        const auto Context = DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        MPP_CHECK(Context.has_value());
        const auto* Entry = Context->GetCatalogue().FindByExternalIdentity(
            ExternalNodeIdentity("200000"));
        MPP_CHECK(Entry != nullptr);
        const NodeDescriptor* Descriptor = Context->GetRegistry().Find(
            Entry->GetDescriptorIdentifier());
        MPP_CHECK(Descriptor != nullptr);
        MPP_CHECK(Descriptor->GetPins()[1U].GetType() ==
            TypeDesc::Enum(EnumTypeIdentity("filter_return_type")));
        MPP_CHECK(Descriptor->GetPins()[1U].GetDefaultValue()->Is<EnumLiteralValue>());
        MPP_CHECK(Descriptor->GetPins()[1U].GetDefaultValue()->TryGet<EnumLiteralValue>()->GetValue() == 1000010);
    }
}

int main()
{
    TestEnumTypeIdentityAndLiteralValue();
    TestTypeDescEnumCompatibilityAndUnification();
    TestDescriptorEnumDefaultValidation();
    TestCatalogueSchemaAndContentIdentity();
    TestCatalogueEnumContentIdentityAndPermutation();
    TestGraphIRV3EnumRoundTrip();
    TestGraphIRLegacyEnumRejection();
    TestGraphIRLegacyVersionsRejectNestedEnumForms();
    TestSnapshotV2EnumRoundTrip();
    TestSnapshotVersionSchemaMatrix();
    TestResultNodeAdapterAcceptsBoundedFixture();
    TestResultNodeAdapterRejectsMalformedSource();
    TestResultNodeAdapterRejectsBoundedShapeViolations();
    TestResultNodeAdapterRejectsEvidenceAndModeViolations();
    TestResultNodeAdapterDeterminismAndFailureAtomicity();
    TestResultNodeAdapterOrdersDiagnosticsByValidationStage();
    TestCombinedCatalogueHasTwentySixRecords();
    TestLegacyP53ScopeRemainsTwentyFiveRecords();
    TestRegistryMaterializationPreservesEnum();
    return EXIT_SUCCESS;
}
