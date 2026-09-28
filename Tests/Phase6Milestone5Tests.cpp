#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "gia.pb.h"

#include "MiliastraPlusPlusGiaExporter.h"
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
    namespace GiaEnvelope
    {
        constexpr std::size_t HeaderSize = 20U;
        constexpr std::size_t TailSize = 4U;
        constexpr std::uint32_t SchemaVersion = 1U;
        constexpr std::uint32_t HeadTag = 0x0326U;
        constexpr std::uint32_t FileType = 3U;
        constexpr std::uint32_t TailTag = 0x0679U;
    }

    namespace TestGraphId
    {
        constexpr std::int32_t Node = 1;
        constexpr std::int32_t ClientGraph = 3;
        constexpr std::int32_t UserDefined = 10000;
        constexpr std::int32_t SystemDefined = 10001;
        constexpr std::int32_t SysCall = 22000;
        constexpr std::int32_t NodeGraph = 21001;
        constexpr std::int32_t BooleanFilter = 20001;
        constexpr std::int32_t Filter = 20001;
        constexpr std::int32_t BooleanFilterWhich = 10;
    }

    constexpr std::int32_t BooleanType = 5;
    constexpr std::int32_t EnumType = 13;

    constexpr char NodeMetadataJson[] = R"json([
{
    "subType": "bool_filter",
    "nodeType": "node_graph_end_boolean",
    "displayName": "\u8282\u70b9\u56fe\u7ed3\u675f(\u5e03\u5c14\u578b)",
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
            "name": "\u8f93\u51fa\u7ed3\u679c\uff08\u5e03\u5c14\u578b\uff09",
            "connectable": true,
            "connectionType": 5
        },
        {
            "index": 1,
            "kind": "input",
            "type": "enum",
            "clientVarType": 13,
            "defaultValue": 1000010,
            "name": "filter\u8fd4\u56de\u7c7b\u578b",
            "connectable": true,
            "connectionType": 210040
        }
    ],
    "outputs": [],
    "sampleFile": "\u5e03\u5c14\u8fc7\u6ee4\u5668\u8282\u70b9\\\u67e5\u8be2\u5b9e\u4f53\u662f\u5426\u5728\u573a_\u8fde\u7ebf.gia",
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

    struct ParsedGia final
    {
        std::uint32_t LeftSize = 0U;
        std::uint32_t SchemaVersion = 0U;
        std::uint32_t HeadTag = 0U;
        std::uint32_t FileType = 0U;
        std::uint32_t ProtoSize = 0U;
        std::uint32_t TailTag = 0U;
        std::span<const std::byte> Payload;
    };

    [[nodiscard]] bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return std::any_of(Diagnostics.begin(), Diagnostics.end(),
            [Code](const Diagnostic& DiagnosticValue)
            {
                return DiagnosticValue.Code == Code;
            }
        );
    }

    [[nodiscard]] std::uint32_t ReadBigEndianUint32(std::span<const std::byte> Bytes, std::size_t Offset)
    {
        return (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(Bytes[Offset])) << 24U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(Bytes[Offset + 1U])) << 16U) |
            (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(Bytes[Offset + 2U])) << 8U) |
            static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(Bytes[Offset + 3U]));
    }

    [[nodiscard]] ParsedGia ParseGia(const GiaEncodedGia& Encoded)
    {
        const std::span<const std::byte> Bytes = Encoded.GetBytes();
        MPP_CHECK(Bytes.size() >= GiaEnvelope::HeaderSize + GiaEnvelope::TailSize);

        ParsedGia Result;
        Result.LeftSize = ReadBigEndianUint32(Bytes, 0U);
        Result.SchemaVersion = ReadBigEndianUint32(Bytes, 4U);
        Result.HeadTag = ReadBigEndianUint32(Bytes, 8U);
        Result.FileType = ReadBigEndianUint32(Bytes, 12U);
        Result.ProtoSize = ReadBigEndianUint32(Bytes, 16U);
        Result.TailTag = ReadBigEndianUint32(Bytes, Bytes.size() - GiaEnvelope::TailSize);

        const std::size_t PayloadSize = Bytes.size() - GiaEnvelope::HeaderSize - GiaEnvelope::TailSize;
        Result.Payload = Bytes.subspan(GiaEnvelope::HeaderSize, PayloadSize);

        MPP_CHECK(Result.LeftSize == static_cast<std::uint32_t>(Bytes.size() - GiaEnvelope::TailSize));
        MPP_CHECK(Result.ProtoSize == static_cast<std::uint32_t>(PayloadSize));
        MPP_CHECK(Result.Payload.size() == Result.ProtoSize);
        MPP_CHECK(static_cast<std::uint64_t>(Result.ProtoSize) + GiaEnvelope::HeaderSize == Result.LeftSize);
        MPP_CHECK(static_cast<std::uint64_t>(Result.ProtoSize) + GiaEnvelope::HeaderSize + GiaEnvelope::TailSize == Bytes.size());
        return Result;
    }

    [[nodiscard]] Root DecodeRoot(const ParsedGia& Parsed)
    {
        MPP_CHECK(Parsed.Payload.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max()));
        Root RootValue;
        MPP_CHECK(RootValue.ParseFromArray(
            reinterpret_cast<const char*>(Parsed.Payload.data()),
            static_cast<int>(Parsed.Payload.size())
        ));
        return RootValue;
    }

    NormalizedNodeDescriptorRecord MakeRecord(std::string Identity, std::vector<NormalizedPinRecord> Pins, std::optional<ExecutionControlSchema> ControlSchema = std::nullopt)
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(Identity)),
            "P6.5 fixture descriptor",
            {NodeAvailability::Client},
            std::move(Pins),
            std::move(ControlSchema),
            SourceProvenance("p65.fixture", "record")
        );
    }

    NormalizedNodeDescriptorRecord MakeBooleanInputRecord(std::string Identity, std::optional<LiteralValue> DefaultValue = std::nullopt)
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "Input",
                    TypeDesc::Boolean(),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    std::move(DefaultValue)
                )
            }
        );
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeLegacyRecords()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.reserve(25U);
        for (std::uint32_t Index = 1U; Index <= 25U; ++Index)
        {
            Records.push_back(MakeBooleanInputRecord("legacy-" + std::to_string(Index), LiteralValue(LiteralValue::Data{false})));
        }
        return Records;
    }

    GiaExportConfiguration MakeConfiguration()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(65001),
            "P6.5 First Fixture Graph",
            GiaUniqueIdentifier(65002)
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetEvaluationInterval() == 0.3);
        return *Result;
    }

    GiaBackendPinMapping MakeInputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendTypeCode = BooleanType,
        GiaLiteralEncodingKind Encoding = GiaLiteralEncodingKind::Boolean)
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::InputParameter,
            GiaPinIndex(static_cast<std::int32_t>(SemanticPin)),
            std::nullopt,
            GiaBackendTypeCode(BackendTypeCode),
            Encoding,
            GiaPinEmissionPolicy::Emit,
            true
        );
    }

    template<typename MappingFactory>
    std::expected<GiaExportContext, DiagnosticCollection> MakeContext(
        std::vector<NormalizedNodeDescriptorRecord> Records,
        MappingFactory MappingFactoryFunction)
    {
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            "p65.authentic",
            "p65.fixture@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
        if (!Catalogue.has_value())
        {
            return std::unexpected(Catalogue.error());
        }

        const auto Snapshot = DescriptorCatalogueSnapshot::Create(*Catalogue);
        if (!Snapshot.has_value())
        {
            return std::unexpected(Snapshot.error());
        }

        const auto Registry = DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        if (!Registry.has_value())
        {
            return std::unexpected(Registry.error());
        }

        const auto MappingPackage = GiaBackendMappingPackage::Create(
            GiaBackendMappingIdentity(
                Catalogue->GetIdentity(),
                GiaBackendMappingSchemaVersion(1U),
                GiaExportTargetProfile::ClientBooleanFilter,
                GiaExportMode::Beyond
            ),
            MappingFactoryFunction(Catalogue->GetIdentity())
        );
        if (!MappingPackage.has_value())
        {
            return std::unexpected(MappingPackage.error());
        }

        return GiaExportContext::Create(
            DescriptorCatalogueBinding(Catalogue->GetIdentity()),
            *Registry,
            MakeConfiguration(),
            *MappingPackage
        );
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeAuthenticContext(
        bool IncludeMapping = true,
        bool IncludePinOne = true,
        bool IncludeConcrete = true,
        bool WrongTuple = false,
        bool WrongLiteralEncoding = false)
    {
        const auto SourceRecord = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(NodeMetadataJson, ModesJson, EnumEvidenceJson);
        MPP_CHECK(SourceRecord.has_value());

        auto Records = MakeLegacyRecords();
        Records.push_back(*SourceRecord);
        return MakeContext(
            std::move(Records),
            [=](const DescriptorCatalogueIdentity&)
            {
                if (!IncludeMapping)
                {
                    return std::vector<GiaBackendNodeMapping>{};
                }

                std::vector<GiaBackendPinMapping> Pins;
                Pins.push_back(MakeInputMapping(
                    0U,
                    WrongTuple ? EnumType : BooleanType,
                    WrongTuple || WrongLiteralEncoding
                        ? GiaLiteralEncodingKind::Enum
                        : GiaLiteralEncodingKind::Boolean
                ));
                if (IncludePinOne)
                {
                    Pins.push_back(MakeInputMapping(1U, EnumType, GiaLiteralEncodingKind::Enum));
                }

                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("200000"),
                        GiaNodeGenericId(200000),
                        IncludeConcrete
                            ? std::optional<GiaNodeConcreteId>(GiaNodeConcreteId(0))
                            : std::nullopt,
                        std::move(Pins),
                        SourceProvenance("p65.graph-encoding", "200000")
                    )
                };
            }
        );
    }

    NodeDescriptorId GetDescriptorId(const GiaExportContext& Context)
    {
        const auto* Entry = Context.GetRegistryContext().GetCatalogue().FindByExternalIdentity(ExternalNodeIdentity("200000"));
        MPP_CHECK(Entry != nullptr);
        return Entry->GetDescriptorIdentifier();
    }

    GraphIR MakeAuthenticGraph(const GiaExportContext& Context)
    {
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), GetDescriptorId(Context), std::nullopt});
        return Graph;
    }

    GraphIR MakeInvalidGraph()
    {
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), NodeDescriptorId(999999U), std::nullopt});
        return Graph;
    }

    GraphIR MakeGraphWithVariable(const GiaExportContext& Context)
    {
        GraphIR Graph = MakeAuthenticGraph(Context);
        Graph.AddVariable({
            GraphVariableId(1U),
            "ReturnType",
            TypeDesc::Boolean(),
            LiteralValue(LiteralValue::Data{false})
        });
        Graph.BindInput(NodeInstanceId(1U), PinIndex(0U), GraphVariableReference{GraphVariableId(1U)});
        return Graph;
    }

    [[nodiscard]] bool BytesEqual(std::span<const std::byte> Left, std::span<const std::byte> Right)
    {
        return Left.size() == Right.size() && std::equal(Left.begin(), Left.end(), Right.begin());
    }

    void CheckNodeProperty(const NodeProperty& Property, NodeGraph_Id_Class ExpectedClass, NodeProperty_Type ExpectedType, NodeGraph_Id_Kind ExpectedKind, std::int64_t ExpectedId)
    {
        MPP_CHECK(Property.class_() == ExpectedClass);
        MPP_CHECK(Property.type() == ExpectedType);
        MPP_CHECK(Property.kind() == ExpectedKind);
        MPP_CHECK(Property.nodeid() == ExpectedId);
    }

    void CheckPin(const NodePin& Pin, std::int32_t ExpectedType, std::int32_t ExpectedValue)
    {
        MPP_CHECK(Pin.has_i1());
        MPP_CHECK(Pin.has_i2());
        MPP_CHECK(Pin.i1().kind() == NodePin::Index::InParam);
        MPP_CHECK(Pin.i2().kind() == NodePin::Index::InParam);
        MPP_CHECK(Pin.i1().index() == Pin.i2().index());
        MPP_CHECK(!Pin.i1().has_nodeid());
        MPP_CHECK(!Pin.i2().has_nodeid());
        MPP_CHECK(Pin.type() == ExpectedType);
        MPP_CHECK(Pin.has_value());
        MPP_CHECK(Pin.connects_size() == 0);

        const VarBase& Value = Pin.value();
        MPP_CHECK(Value.class_() == VarBase::EnumBase);
        MPP_CHECK(Value.has_itemtype());
        MPP_CHECK(Value.itemtype().classbase() == VarBase::ItemType::Client);
        MPP_CHECK(Value.itemtype().has_type_client());
        MPP_CHECK(Value.itemtype().type_client().type() == static_cast<ClientVarType>(ExpectedType));
        MPP_CHECK(Value.alreadysetval());
        MPP_CHECK(Value.baseValues_case() == VarBase::kBEnum);
        MPP_CHECK(Value.benum().val() == ExpectedValue);
    }

    void CheckAuthenticRootGolden(const Root& RootValue)
    {
        MPP_CHECK(RootValue.has_graph());
        MPP_CHECK(RootValue.filepath().empty());
        MPP_CHECK(RootValue.gameversion().empty());
        MPP_CHECK(!RootValue.has_modeflag());
        MPP_CHECK(RootValue.accessories_size() == 0);

        const GraphUnit& GraphUnitValue = RootValue.graph();
        MPP_CHECK(GraphUnitValue.has_id());
        MPP_CHECK(GraphUnitValue.id().class_() == GraphUnit::Id::Node);
        MPP_CHECK(GraphUnitValue.id().type() == GraphUnit::Id::ClientGraph);
        MPP_CHECK(GraphUnitValue.id().id() == 65001);
        MPP_CHECK(GraphUnitValue.name() == "P6.5 First Fixture Graph");
        MPP_CHECK(GraphUnitValue.which() == GraphUnit::BooleanFilter);
        MPP_CHECK(GraphUnitValue.relatedids_size() == 0);
        MPP_CHECK(GraphUnitValue.has_graph());
        MPP_CHECK(GraphUnitValue.graph().has_inner());
        MPP_CHECK(GraphUnitValue.graph().inner().has_graph());

        const NodeGraph& NodeGraphValue = GraphUnitValue.graph().inner().graph();
        MPP_CHECK(NodeGraphValue.has_id());
        MPP_CHECK(NodeGraphValue.id().class_() == NodeGraph::Id::UserDefined);
        MPP_CHECK(NodeGraphValue.id().type() == NodeGraph::Id::BooleanFilter);
        MPP_CHECK(NodeGraphValue.id().kind() == NodeGraph::Id::NodeGraph);
        MPP_CHECK(NodeGraphValue.id().id() == 65001);
        MPP_CHECK(NodeGraphValue.name() == "P6.5 First Fixture Graph");
        MPP_CHECK(NodeGraphValue.has_entryslotindex());
        MPP_CHECK(NodeGraphValue.entryslotindex() == 1);
        MPP_CHECK(NodeGraphValue.has_evaluationinterval());
        MPP_CHECK(NodeGraphValue.evaluationinterval() == 0.3F);
        MPP_CHECK(NodeGraphValue.compositepins_size() == 0);
        MPP_CHECK(NodeGraphValue.comments_size() == 0);
        MPP_CHECK(NodeGraphValue.graphvalues_size() == 0);
        MPP_CHECK(NodeGraphValue.affiliations_size() == 0);
        MPP_CHECK(NodeGraphValue.nodes_size() == 1);

        const GraphNode& Node = NodeGraphValue.nodes(0);
        MPP_CHECK(Node.nodeindex() == 1);
        MPP_CHECK(Node.x() == 0.0F);
        MPP_CHECK(Node.y() == 0.0F);
        MPP_CHECK(Node.has_genericid());
        CheckNodeProperty(Node.genericid(), NodeGraph::Id::SystemDefined, NodeProperty::Filter, NodeGraph::Id::SysCall, 200000);
        MPP_CHECK(Node.has_concreteid());
        CheckNodeProperty(Node.concreteid(), NodeGraph::Id::SystemDefined, NodeProperty::Filter, NodeGraph::Id::SysCall, 0);
        MPP_CHECK(!Node.has_comments());
        MPP_CHECK(!Node.has_contextdeclaration());
        MPP_CHECK(!Node.has_signalversion());
        MPP_CHECK(Node.usingstruct_size() == 0);
        MPP_CHECK(!Node.has_statusnodeextension());
        MPP_CHECK(Node.pins_size() == 2);

        const NodePin& BooleanPin = Node.pins(0);
        MPP_CHECK(BooleanPin.i1().index() == 0);
        CheckPin(BooleanPin, BooleanType, 0);

        const NodePin& EnumPin = Node.pins(1);
        MPP_CHECK(EnumPin.i1().index() == 1);
        CheckPin(EnumPin, EnumType, 1000010);
    }

    void TestGiaExporterProducesAuthenticFramedBytes()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const GraphIR Graph = MakeAuthenticGraph(*Context);
        const auto Result = GiaExporter::Export(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetBytes().size() > GiaEnvelope::HeaderSize + GiaEnvelope::TailSize);

        const ParsedGia Parsed = ParseGia(*Result);
        MPP_CHECK(Parsed.SchemaVersion == GiaEnvelope::SchemaVersion);
        MPP_CHECK(Parsed.HeadTag == GiaEnvelope::HeadTag);
        MPP_CHECK(Parsed.FileType == GiaEnvelope::FileType);
        MPP_CHECK(Parsed.TailTag == GiaEnvelope::TailTag);
        MPP_CHECK(Parsed.LeftSize == Parsed.ProtoSize + GiaEnvelope::HeaderSize);
        MPP_CHECK(Parsed.ProtoSize == Result->GetBytes().size() - GiaEnvelope::HeaderSize - GiaEnvelope::TailSize);
    }

    void TestGiaExporterMatchesIndependentDecodedGolden()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const GraphIR Graph = MakeAuthenticGraph(*Context);
        const auto Result = GiaExporter::Export(Graph, *Context);
        MPP_CHECK(Result.has_value());

        const ParsedGia Parsed = ParseGia(*Result);
        const Root Decoded = DecodeRoot(Parsed);
        CheckAuthenticRootGolden(Decoded);
    }

    void TestGiaExporterProducesDeterministicEquivalentBytes()
    {
        const auto FirstContext = MakeAuthenticContext();
        const auto EquivalentContext = MakeAuthenticContext();
        MPP_CHECK(FirstContext.has_value() && EquivalentContext.has_value());
        const GraphIR FirstGraph = MakeAuthenticGraph(*FirstContext);
        const GraphIR EquivalentGraph = MakeAuthenticGraph(*EquivalentContext);

        const auto First = GiaExporter::Export(FirstGraph, *FirstContext);
        const auto Repeated = GiaExporter::Export(FirstGraph, *FirstContext);
        const auto Equivalent = GiaExporter::Export(EquivalentGraph, *EquivalentContext);
        MPP_CHECK(First.has_value() && Repeated.has_value() && Equivalent.has_value());
        MPP_CHECK(BytesEqual(First->GetBytes(), Repeated->GetBytes()));
        MPP_CHECK(BytesEqual(First->GetBytes(), Equivalent->GetBytes()));
    }

    void TestGiaExporterOwnsBytesAfterInputsExpire()
    {
        std::optional<GiaEncodedGia> Result;
        {
            const auto Context = MakeAuthenticContext();
            MPP_CHECK(Context.has_value());
            const GraphIR Graph = MakeAuthenticGraph(*Context);
            const auto Exported = GiaExporter::Export(Graph, *Context);
            MPP_CHECK(Exported.has_value());
            Result.emplace(*Exported);
        }

        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetBytes().size() > GiaEnvelope::HeaderSize + GiaEnvelope::TailSize);
        CheckAuthenticRootGolden(DecodeRoot(ParseGia(*Result)));
    }

    void TestGiaExporterDoesNotMutateInputs()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        const nlohmann::json GraphBefore = GraphIRJson::Serialize(Graph);
        const GiaExportConfiguration ConfigurationBefore = Context->GetConfiguration();
        const GiaBackendMappingPackage MappingBefore = Context->GetMappingPackage();
        const DescriptorCatalogueIdentity BindingBefore = Context->GetCatalogueBinding().GetIdentity();
        const DescriptorCatalogueSnapshot SnapshotBefore = Context->GetRegistryContext().GetSnapshot();

        const auto Result = GiaExporter::Export(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(GraphIRJson::Serialize(Graph) == GraphBefore);
        MPP_CHECK(Context->GetConfiguration() == ConfigurationBefore);
        MPP_CHECK(Context->GetMappingPackage() == MappingBefore);
        MPP_CHECK(Context->GetCatalogueBinding().GetIdentity() == BindingBefore);
        MPP_CHECK(Context->GetRegistryContext().GetSnapshot() == SnapshotBefore);
    }

    void TestGiaExporterPropagatesInvalidGraphDiagnostics()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaExporter::Export(MakeInvalidGraph(), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(!Result.error().empty());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingDescriptor));
    }

    void TestGiaExporterPropagatesMissingMappingDiagnostics()
    {
        const auto Context = MakeAuthenticContext(false);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaExporter::Export(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(Result.error().size() == 1U);
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingGiaBackendNodeMapping));
    }

    void TestGiaExporterPropagatesUnsupportedTypeDiagnostics()
    {
        const auto Context = MakeAuthenticContext(true, true, true, true);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaExporter::Export(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaBackendType));
    }

    void TestGiaExporterPropagatesGraphVariableDiagnostics()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaExporter::Export(MakeGraphWithVariable(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(!Result.error().empty());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaTargetGraphFeature));
    }

    void TestGiaExporterPreservesDiagnosticOrdering()
    {
        const auto Context = MakeAuthenticContext(true, true, true, false, true);
        MPP_CHECK(Context.has_value());
        const GraphIR Graph = MakeAuthenticGraph(*Context);
        const auto First = GiaExporter::Export(Graph, *Context);
        const auto Second = GiaExporter::Export(Graph, *Context);
        MPP_CHECK(!First.has_value() && !Second.has_value());
        MPP_CHECK(First.error().size() == 2U);
        MPP_CHECK(Second.error().size() == First.error().size());
        for (std::size_t Index = 0U; Index < First.error().size(); ++Index)
        {
            MPP_CHECK(First.error()[Index].Code == DiagnosticCode::UnsupportedGiaBackendValue);
            MPP_CHECK(First.error()[Index].Code == Second.error()[Index].Code);
            MPP_CHECK(First.error()[Index].Message == Second.error()[Index].Message);
            MPP_CHECK(First.error()[Index].ExternalIdentityKey == Second.error()[Index].ExternalIdentityKey);
        }
        MPP_CHECK(First.error()[0U].Message.find("required target literal encoding") != std::string::npos);
        MPP_CHECK(First.error()[1U].Message.find("descriptor default is unsupported") != std::string::npos);
    }

    void TestGiaExporterKeepsFactoryFailuresOutsideFacade()
    {
        const auto InvalidConfiguration = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(0),
            "",
            GiaUniqueIdentifier(0)
        );
        MPP_CHECK(!InvalidConfiguration.has_value());
        MPP_CHECK(HasCode(InvalidConfiguration.error(), DiagnosticCode::InvalidGiaExportConfiguration));

        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto InvalidContext = GiaExportContext::Create(
            DescriptorCatalogueBinding{},
            Context->GetRegistryContext(),
            Context->GetConfiguration(),
            Context->GetMappingPackage()
        );
        MPP_CHECK(!InvalidContext.has_value());
        MPP_CHECK(!InvalidContext.error().empty());
    }
}

int main()
{
    TestGiaExporterProducesAuthenticFramedBytes();
    TestGiaExporterMatchesIndependentDecodedGolden();
    TestGiaExporterProducesDeterministicEquivalentBytes();
    TestGiaExporterOwnsBytesAfterInputsExpire();
    TestGiaExporterDoesNotMutateInputs();
    TestGiaExporterPropagatesInvalidGraphDiagnostics();
    TestGiaExporterPropagatesMissingMappingDiagnostics();
    TestGiaExporterPropagatesUnsupportedTypeDiagnostics();
    TestGiaExporterPropagatesGraphVariableDiagnostics();
    TestGiaExporterPreservesDiagnosticOrdering();
    TestGiaExporterKeepsFactoryFailuresOutsideFacade();
    return EXIT_SUCCESS;
}
