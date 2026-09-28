#include <algorithm>
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

#include "MiliastraPlusPlusGiaProtobufEncoder.h"
#include "MiliastraPlusPlusGiaGraphResolver.h"
#include "MiliastraPlusPlusGiaGraphLowerer.h"
#include "MiliastraPlusPlusGiaExportContext.h"
#include "MiliastraPlusPlusGenshinClientBooleanFilterResultNodeSourceAdapter.h"

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
    using TestSnapshot = GiaProtobufEncoderDetail::GiaProtobufTestSnapshot;
    using TestNode = GiaProtobufEncoderDetail::GiaProtobufTestNode;
    using TestPin = GiaProtobufEncoderDetail::GiaProtobufTestPin;

    namespace TestPinKind
    {
        constexpr std::int32_t InFlow = 1;
        constexpr std::int32_t OutFlow = 2;
        constexpr std::int32_t InParam = 3;
        constexpr std::int32_t OutParam = 4;
    }

    namespace TestGraphId
    {
        constexpr std::int32_t Node = 1;
        constexpr std::int32_t ClientGraph = 3;
        constexpr std::int32_t UserDefined = 10000;
        constexpr std::int32_t SystemDefined = 10001;
        constexpr std::int32_t SysCall = 22000;
        constexpr std::int32_t NodeGraph = 21001;
    }

    namespace TestGraphType
    {
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

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return std::any_of(Diagnostics.begin(), Diagnostics.end(),
            [Code](const Diagnostic& DiagnosticValue)
            {
                return DiagnosticValue.Code == Code;
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeRecord(std::string Identity, std::vector<NormalizedPinRecord> Pins, std::optional<ExecutionControlSchema> ControlSchema = std::nullopt)
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(Identity)),
            "P6.4 fixture descriptor",
            {NodeAvailability::Client},
            std::move(Pins),
            std::move(ControlSchema),
            SourceProvenance("p64.fixture", "record")
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

    NormalizedNodeDescriptorRecord MakeLegacyRecord(std::uint32_t Index)
    {
        return MakeBooleanInputRecord("legacy-" + std::to_string(Index), LiteralValue(LiteralValue::Data{false}));
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeLegacyRecords()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.reserve(25U);
        for (std::uint32_t Index = 1U; Index <= 25U; ++Index)
        {
            Records.push_back(MakeLegacyRecord(Index));
        }
        return Records;
    }

    NormalizedNodeDescriptorRecord MakeFlowRecord(std::string Identity)
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord("InputFlow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution),
                NormalizedPinRecord("OutputFlow", TypeDesc::Flow(), PinDirection::Output, PinCategory::Execution)
            },
            SequenceControlSchema{PinIndex(0U), PinIndex(1U)}
        );
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeThreeNodeRecords()
    {
        return {
            MakeRecord(
                "three-source",
                {
                    NormalizedPinRecord("OutputValue", TypeDesc::Boolean(), PinDirection::Output, PinCategory::Data),
                    NormalizedPinRecord("OutputFlow", TypeDesc::Flow(), PinDirection::Output, PinCategory::Execution)
                },
                EntryControlSchema{PinIndex(1U)}
            ),
            MakeRecord(
                "three-intermediate",
                {
                    NormalizedPinRecord("InputValue", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data, PinCardinality::Single, true),
                    NormalizedPinRecord("OutputValue", TypeDesc::Boolean(), PinDirection::Output, PinCategory::Data),
                    NormalizedPinRecord("InputFlow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution),
                    NormalizedPinRecord("OutputFlow", TypeDesc::Flow(), PinDirection::Output, PinCategory::Execution)
                },
                SequenceControlSchema{PinIndex(2U), PinIndex(3U)}
            ),
            MakeRecord(
                "three-destination",
                {
                    NormalizedPinRecord("InputValue", TypeDesc::Boolean(), PinDirection::Input, PinCategory::Data, PinCardinality::Single, true),
                    NormalizedPinRecord("InputFlow", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution)
                }
            )
        };
    }

    GiaExportConfiguration MakeConfiguration()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(64001),
            "P6.4 Fixture Graph",
            GiaUniqueIdentifier(64002),
            0.5
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    GiaBackendPinMapping MakeInputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendTypeCode = 5,
        GiaLiteralEncodingKind Encoding = GiaLiteralEncodingKind::Boolean,
        bool Connectable = true,
        std::int32_t BackendIndex = -1,
        std::optional<std::int32_t> SecondaryIndex = std::nullopt)
    {
        const std::int32_t EffectiveBackendIndex = BackendIndex < 0 ? static_cast<std::int32_t>(SemanticPin) : BackendIndex;
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::InputParameter,
            GiaPinIndex(EffectiveBackendIndex),
            SecondaryIndex.has_value() ? std::optional<GiaPinIndex>(GiaPinIndex(*SecondaryIndex)) : std::nullopt,
            GiaBackendTypeCode(BackendTypeCode),
            Encoding,
            GiaPinEmissionPolicy::Emit,
            Connectable
        );
    }

    GiaBackendPinMapping MakeOutputMapping(std::uint32_t SemanticPin, std::int32_t BackendIndex)
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::OutputParameter,
            GiaPinIndex(BackendIndex),
            std::nullopt,
            GiaBackendTypeCode(5),
            GiaLiteralEncodingKind::Boolean,
            GiaPinEmissionPolicy::Emit,
            true
        );
    }

    GiaBackendPinMapping MakeFlowMapping(std::uint32_t SemanticPin, GiaPinKind Kind, std::int32_t BackendIndex)
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            Kind,
            GiaPinIndex(BackendIndex),
            std::nullopt,
            GiaBackendTypeCode(1),
            GiaLiteralEncodingKind::None,
            GiaPinEmissionPolicy::Emit,
            false
        );
    }

    template<typename MappingFactory>
    std::expected<GiaExportContext, DiagnosticCollection> MakeContext(
        std::vector<NormalizedNodeDescriptorRecord> Records,
        MappingFactory MappingFactoryFunction,
        std::string SourceNamespace)
    {
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            std::move(SourceNamespace),
            "p64.fixture@1",
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
        return GiaExportContext::Create(DescriptorCatalogueBinding(Catalogue->GetIdentity()), *Registry, MakeConfiguration(), *MappingPackage);
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeAuthenticContext()
    {
        const auto SourceRecord = GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(NodeMetadataJson, ModesJson, EnumEvidenceJson);
        MPP_CHECK(SourceRecord.has_value());
        auto Records = MakeLegacyRecords();
        Records.push_back(*SourceRecord);
        return MakeContext(
            std::move(Records),
            [](const DescriptorCatalogueIdentity&)
            {
                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("200000"),
                        GiaNodeGenericId(200000),
                        GiaNodeConcreteId(0),
                        {
                            MakeInputMapping(0U),
                            MakeInputMapping(1U, 13, GiaLiteralEncodingKind::Enum)
                        },
                        SourceProvenance("p64.graph-encoding", "200000")
                    )
                };
            },
            "p64.authentic"
        );
    }

    NodeDescriptorId GetDescriptorId(const GiaExportContext& Context, std::string_view Identity)
    {
        const auto& Catalogue = Context.GetRegistryContext().GetCatalogue();
        const auto* Entry = Catalogue.FindByExternalIdentity(ExternalNodeIdentity(std::string(Identity)));
        MPP_CHECK(Entry != nullptr);
        return Entry->GetDescriptorIdentifier();
    }

    GraphIR MakeAuthenticGraph(const GiaExportContext& Context)
    {
        GraphIR Graph;
        Graph.AddNode({
            NodeInstanceId(1U),
            GetDescriptorId(Context, "200000"),
            std::nullopt
        });
        return Graph;
    }

    std::expected<GiaResolvedBackendGraph, DiagnosticCollection> MakeAuthenticResolved(bool ExplicitLiterals = false)
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        if (ExplicitLiterals)
        {
            Graph.BindInput(NodeInstanceId(1U), PinIndex(0U), LiteralValue(LiteralValue::Data{true}));
            Graph.BindInput(NodeInstanceId(1U), PinIndex(1U), LiteralValue(EnumLiteralValue(EnumTypeIdentity("filter_return_type"), 10000011)));
        }
        const auto Backend = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Backend.has_value());
        return GiaGraphResolver::Resolve(*Backend);
    }

    std::vector<GiaBackendNodeMapping> MakeThreeNodeMappings(bool Reverse, bool ExplicitSecondaryZero)
    {
        std::vector<GiaBackendNodeMapping> Mappings{
            GiaBackendNodeMapping(
                ExternalNodeIdentity("three-source"),
                GiaNodeGenericId(301),
                GiaNodeConcreteId(0),
                {
                    MakeOutputMapping(0U, 8),
                    MakeFlowMapping(1U, GiaPinKind::OutputFlow, 10)
                }
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("three-intermediate"),
                GiaNodeGenericId(302),
                GiaNodeConcreteId(0),
                {
                    MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean, true, 4, 7),
                    MakeOutputMapping(1U, 5),
                    MakeFlowMapping(2U, GiaPinKind::InputFlow, 12),
                    MakeFlowMapping(3U, GiaPinKind::OutputFlow, 13)
                }
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("three-destination"),
                GiaNodeGenericId(303),
                GiaNodeConcreteId(0),
                {
                    ExplicitSecondaryZero
                        ? MakeInputMapping(
                            0U,
                            5,
                            GiaLiteralEncodingKind::Boolean,
                            true,
                            6,
                            0
                        )
                        : MakeInputMapping(0U),
                    MakeFlowMapping(1U, GiaPinKind::InputFlow, 14)
                }
            )
        };
        if (Reverse)
        {
            std::reverse(Mappings.begin(), Mappings.end());
        }
        return Mappings;
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeThreeNodeContext(bool ReverseMappings = false, bool ExplicitSecondaryZero = false)
    {
        return MakeContext(
            MakeThreeNodeRecords(),
            [ReverseMappings, ExplicitSecondaryZero](const DescriptorCatalogueIdentity&)
            {
                return MakeThreeNodeMappings(ReverseMappings, ExplicitSecondaryZero);
            },
            "p64.three-node"
        );
    }

    GraphIR MakeThreeNodeGraph(const GiaExportContext& Context, bool ReverseNodes, bool ReverseBindings, bool ReverseControlEdges)
    {
        const std::vector<NodeInstance> Nodes{
            {NodeInstanceId(1U), GetDescriptorId(Context, "three-source"), std::nullopt},
            {NodeInstanceId(2U), GetDescriptorId(Context, "three-intermediate"), std::nullopt},
            {NodeInstanceId(3U), GetDescriptorId(Context, "three-destination"), std::nullopt}
        };
        GraphIR Graph;
        if (ReverseNodes)
        {
            for (auto Iterator = Nodes.rbegin(); Iterator != Nodes.rend(); ++Iterator)
            {
                Graph.AddNode(*Iterator);
            }
        }
        else
        {
            for (const NodeInstance& Node : Nodes)
            {
                Graph.AddNode(Node);
            }
        }

        if (ReverseBindings)
        {
            Graph.BindInput(NodeInstanceId(3U), PinIndex(0U), OutputReference{NodeInstanceId(2U), PinIndex(1U)});
            Graph.BindInput(NodeInstanceId(2U), PinIndex(0U), OutputReference{NodeInstanceId(1U), PinIndex(0U)});
        }
        else
        {
            Graph.BindInput(NodeInstanceId(2U), PinIndex(0U), OutputReference{NodeInstanceId(1U), PinIndex(0U)});
            Graph.BindInput(NodeInstanceId(3U), PinIndex(0U), OutputReference{NodeInstanceId(2U), PinIndex(1U)});
        }

        const ControlEdge First{NodeInstanceId(1U), PinIndex(1U), NodeInstanceId(2U), PinIndex(2U)};
        const ControlEdge Second{NodeInstanceId(2U), PinIndex(3U), NodeInstanceId(3U), PinIndex(1U)};
        if (ReverseControlEdges)
        {
            Graph.AddControlEdge(Second);
            Graph.AddControlEdge(First);
        }
        else
        {
            Graph.AddControlEdge(First);
            Graph.AddControlEdge(Second);
        }
        return Graph;
    }

    std::expected<GiaResolvedBackendGraph, DiagnosticCollection> MakeThreeResolved(bool Reverse, bool ExplicitSecondaryZero = false)
    {
        const auto Context = MakeThreeNodeContext(Reverse, ExplicitSecondaryZero);
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeThreeNodeGraph(*Context, Reverse, !Reverse, Reverse);
        const auto Backend = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Backend.has_value());
        return GiaGraphResolver::Resolve(*Backend);
    }

    bool ParseRoot(const GiaEncodedProtobuf& Encoded, TestSnapshot& Snapshot)
    {
        const std::span<const std::byte> Bytes = Encoded.GetBytes();
        const auto Inspected = GiaProtobufEncoderDetail::InspectForTesting(Bytes);
        if (!Inspected.has_value())
        {
            return false;
        }
        Snapshot = *Inspected;
        return true;
    }

    const TestNode& FindNode(const TestSnapshot& Snapshot, std::int32_t NodeIndex)
    {
        for (const TestNode& Node : Snapshot.Nodes)
        {
            if (Node.NodeIndex == NodeIndex)
            {
                return Node;
            }
        }
        MPP_CHECK(false);
        return Snapshot.Nodes[0U];
    }

    const TestPin& FindPin(const TestNode& Node, std::int32_t Kind, std::int32_t PrimaryIndex)
    {
        for (const TestPin& Pin : Node.Pins)
        {
            if (Pin.Kind == Kind && Pin.PrimaryIndex == PrimaryIndex)
            {
                return Pin;
            }
        }
        MPP_CHECK(false);
        return Node.Pins[0U];
    }

    void TestGiaProtobufEncoderAcceptsAuthenticFirstFixture()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(!Result->GetBytes().empty());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        MPP_CHECK(Decoded.HasGraph);
    }

    void TestGiaProtobufEncoderMapsRootAndGraphHeader()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        MPP_CHECK(Decoded.GraphClass == TestGraphId::Node);
        MPP_CHECK(Decoded.GraphType == TestGraphId::ClientGraph);
        MPP_CHECK(Decoded.GraphUnitIdentifier == 64001);
        MPP_CHECK(Decoded.GraphWhich == TestGraphType::BooleanFilterWhich);
        MPP_CHECK(Decoded.NodeGraphClass == TestGraphId::UserDefined);
        MPP_CHECK(Decoded.NodeGraphType == TestGraphType::BooleanFilter);
        MPP_CHECK(Decoded.NodeGraphKind == TestGraphId::NodeGraph);
        MPP_CHECK(Decoded.EntrySlot == 1);
        MPP_CHECK(Decoded.EvaluationInterval == 0.5F);
    }

    void TestGiaProtobufEncoderPreservesConcreteZeroPresence()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestNode& Node = FindNode(Decoded, 1);
        MPP_CHECK(Node.HasConcrete);
        MPP_CHECK(Node.ConcreteNodeId == 0);
    }

    void TestGiaProtobufEncoderMapsNodePropertiesAndPositions()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestNode& Node = FindNode(Decoded, 2);
        MPP_CHECK(Node.GenericClass == TestGraphId::SystemDefined);
        MPP_CHECK(Node.GenericType == TestGraphType::Filter);
        MPP_CHECK(Node.GenericKind == TestGraphId::SysCall);
        MPP_CHECK(Node.GenericNodeId == 302);
        MPP_CHECK(Node.X == 800.0F);
        MPP_CHECK(Node.Y == 0.0F);
    }

    void TestGiaProtobufEncoderMapsParameterPinsAndSecondaryIndexes()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Pin = FindPin(FindNode(Decoded, 2), TestPinKind::InParam, 4);
        MPP_CHECK(Pin.Kind == TestPinKind::InParam);
        MPP_CHECK(Pin.PrimaryIndex == 4);
        MPP_CHECK(Pin.SecondaryKind == TestPinKind::InParam);
        MPP_CHECK(Pin.SecondaryIndex == 7);
        MPP_CHECK(Pin.TypeCode == BooleanType);

        const auto ExplicitZeroGraph = MakeThreeResolved(false, true);
        MPP_CHECK(ExplicitZeroGraph.has_value());
        const auto ExplicitZeroResult = GiaProtobufEncoder::Encode(*ExplicitZeroGraph);
        MPP_CHECK(ExplicitZeroResult.has_value());
        TestSnapshot ExplicitZeroSnapshot;
        MPP_CHECK(ParseRoot(*ExplicitZeroResult, ExplicitZeroSnapshot));
        const TestPin& ExplicitZeroPin = FindPin(FindNode(ExplicitZeroSnapshot, 3), TestPinKind::InParam, 6);
        MPP_CHECK(ExplicitZeroPin.SecondaryKind == TestPinKind::InParam);
        MPP_CHECK(ExplicitZeroPin.SecondaryIndex == 0);
    }

    void TestGiaProtobufEncoderMapsFlowPins()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Input = FindPin(FindNode(Decoded, 2), TestPinKind::InFlow, 12);
        const TestPin& Output = FindPin(FindNode(Decoded, 2), TestPinKind::OutFlow, 13);
        MPP_CHECK(Input.SecondaryKind == TestPinKind::InFlow);
        MPP_CHECK(Input.SecondaryIndex == 12);
        MPP_CHECK(Output.SecondaryKind == TestPinKind::OutFlow);
        MPP_CHECK(Output.SecondaryIndex == 13);
        MPP_CHECK(!Input.HasValue);
        MPP_CHECK(!Output.HasValue);
    }

    void TestGiaProtobufEncoderEncodesBooleanDescriptorDefault()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Pin = FindPin(FindNode(Decoded, 1), TestPinKind::InParam, 0);
        MPP_CHECK(Pin.AlreadySetValue);
        MPP_CHECK(Pin.ValueType == BooleanType);
        MPP_CHECK(Pin.HasLiteral && Pin.LiteralValue == 0);
    }

    void TestGiaProtobufEncoderEncodesEnumDescriptorDefault()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Pin = FindPin(FindNode(Decoded, 1), TestPinKind::InParam, 1);
        MPP_CHECK(Pin.ValueType == EnumType);
        MPP_CHECK(Pin.HasLiteral && Pin.LiteralValue == 1000010);
    }

    void TestGiaProtobufEncoderEncodesExplicitLiteral()
    {
        const auto Graph = MakeAuthenticResolved(true);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestNode& Node = FindNode(Decoded, 1);
        MPP_CHECK(FindPin(Node, TestPinKind::InParam, 0).LiteralValue == 1);
        MPP_CHECK(FindPin(Node, TestPinKind::InParam, 1).LiteralValue == 10000011);
    }

    void TestGiaProtobufEncoderEncodesConnectedInputWithoutLiteral()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Pin = FindPin(FindNode(Decoded, 3), TestPinKind::InParam, 0);
        MPP_CHECK(Pin.HasValue);
        MPP_CHECK(!Pin.AlreadySetValue);
        MPP_CHECK(!Pin.HasLiteral);
        MPP_CHECK(Pin.ValueType == BooleanType);
    }

    void TestGiaProtobufEncoderEncodesDataConnections()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Destination = FindPin(FindNode(Decoded, 2), TestPinKind::InParam, 4);
        MPP_CHECK(Destination.Connections.size() == 1U);
        MPP_CHECK(Destination.Connections[0U].PeerNodeIndex == 1);
        MPP_CHECK(Destination.Connections[0U].ConnectKind == TestPinKind::OutParam);
        MPP_CHECK(Destination.Connections[0U].ConnectIndex == 8);
        MPP_CHECK(Destination.Connections[0U].Connect2Index == 8);
    }

    void TestGiaProtobufEncoderEncodesControlConnections()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        const TestPin& Source = FindPin(FindNode(Decoded, 1), TestPinKind::OutFlow, 10);
        MPP_CHECK(Source.Connections.size() == 1U);
        MPP_CHECK(Source.Connections[0U].PeerNodeIndex == 2);
        MPP_CHECK(Source.Connections[0U].ConnectKind == TestPinKind::InFlow);
        MPP_CHECK(Source.Connections[0U].ConnectIndex == 12);
    }

    void TestGiaProtobufEncoderOmitsUnsupportedOptionalSections()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        MPP_CHECK(Decoded.Accessories == 0U);
        MPP_CHECK(Decoded.FilePath.empty());
        MPP_CHECK(!Decoded.HasModeFlag);
        MPP_CHECK(Decoded.GameVersion.empty());
        MPP_CHECK(Decoded.CompositePins == 0U);
        MPP_CHECK(Decoded.Comments == 0U);
        MPP_CHECK(Decoded.GraphValues == 0U);
        MPP_CHECK(Decoded.Affiliations == 0U);
    }

    void TestGiaProtobufEncoderDecodesIntoFreshMessage()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot First;
        TestSnapshot Second;
        MPP_CHECK(ParseRoot(*Result, First));
        MPP_CHECK(ParseRoot(*Result, Second));
        MPP_CHECK(&First != &Second);
        MPP_CHECK(First == Second);
    }

    void TestGiaProtobufEncoderValidatesDecodedStructureIndependently()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
        MPP_CHECK(Decoded.HasGraph);
        MPP_CHECK(Decoded.HasInner);
        MPP_CHECK(Decoded.HasNodeGraph);
        MPP_CHECK(Decoded.HasEntrySlot);
        MPP_CHECK(Decoded.HasEvaluationInterval);
        MPP_CHECK(Decoded.Nodes.size() == 1U);
    }

    void TestGiaProtobufEncoderRejectsInvalidResolvedModel()
    {
        const GiaResolvedBackendGraph Invalid = GiaResolvedBackendGraphDetail::CreateForTesting({}, {}, {}, {});
        const auto Result = GiaProtobufEncoder::Encode(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(Result.error().size() == 1U);
        MPP_CHECK(Result.error()[0U].Code == DiagnosticCode::InvalidGiaEncodingInput);
    }

    void TestGiaProtobufEncoderRejectsGraphUnitIdRangeFailure()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        GiaResolvedGraphHeader Header = Graph->GetHeader();
        Header.GraphIdentifier = GiaGraphIdentifier(std::numeric_limits<std::int64_t>::max());
        const GiaResolvedBackendGraph Invalid = GiaResolvedBackendGraphDetail::CreateForTesting(
                std::move(Header),
                Graph->GetNodes(),
                Graph->GetDataConnections(),
                Graph->GetControlConnections()
            );
        MPP_CHECK(Invalid.IsValid());
        const auto Result = GiaProtobufEncoder::Encode(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaEncodingInput));
    }

    void TestGiaProtobufEncoderReturnsNoBytesOnSerializationOrDecodeFailure()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure::Serialization);
        const auto SerializationResult = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(!SerializationResult.has_value());
        MPP_CHECK(HasCode(SerializationResult.error(), DiagnosticCode::GiaProtobufSerializationFailure));
        GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure::Decode);
        const auto DecodeResult = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(!DecodeResult.has_value());
        MPP_CHECK(HasCode(DecodeResult.error(), DiagnosticCode::GiaProtobufDecodeFailure));
    }

    void TestGiaProtobufEncoderProducesDeterministicBytes()
    {
        const auto FirstGraph = MakeAuthenticResolved();
        const auto SecondGraph = MakeAuthenticResolved();
        MPP_CHECK(FirstGraph.has_value() && SecondGraph.has_value());
        const auto First = GiaProtobufEncoder::Encode(*FirstGraph);
        const auto Repeated = GiaProtobufEncoder::Encode(*FirstGraph);
        const auto Second = GiaProtobufEncoder::Encode(*SecondGraph);
        MPP_CHECK(First.has_value() && Repeated.has_value() && Second.has_value());
        MPP_CHECK(First->GetBytes().size() == Repeated->GetBytes().size());
        MPP_CHECK(std::equal(First->GetBytes().begin(), First->GetBytes().end(), Repeated->GetBytes().begin()));
        MPP_CHECK(First->GetBytes().size() == Second->GetBytes().size());
        MPP_CHECK(std::equal(First->GetBytes().begin(), First->GetBytes().end(), Second->GetBytes().begin()));
    }

    void TestGiaProtobufEncoderOwnsBytesAfterInputExpires()
    {
        std::optional<GiaEncodedProtobuf> Result;
        {
            const auto Graph = MakeAuthenticResolved();
            MPP_CHECK(Graph.has_value());
            const auto Encoded = GiaProtobufEncoder::Encode(*Graph);
            MPP_CHECK(Encoded.has_value());
            Result.emplace(*Encoded);
        }
        MPP_CHECK(Result.has_value());
        MPP_CHECK(!Result->GetBytes().empty());
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*Result, Decoded));
    }

    void TestGiaProtobufEncoderDoesNotMutateResolvedGraph()
    {
        const auto Graph = MakeThreeResolved(false);
        MPP_CHECK(Graph.has_value());
        const GiaResolvedBackendGraph Before = *Graph;
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(*Graph == Before);
    }

    void TestGiaProtobufEncoderReportsMalformedDecodedStructure()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure::MalformedDecodedStructure);
        const auto Result = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaDecodedProtobuf));
    }

    void TestGiaProtobufEncoderReportsModelMismatchDeterministically()
    {
        const auto Graph = MakeAuthenticResolved();
        MPP_CHECK(Graph.has_value());
        GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure::ModelMismatch);
        const auto First = GiaProtobufEncoder::Encode(*Graph);
        GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure::ModelMismatch);
        const auto Second = GiaProtobufEncoder::Encode(*Graph);
        MPP_CHECK(!First.has_value() && !Second.has_value());
        MPP_CHECK(First.error().size() == Second.error().size());
        MPP_CHECK(First.error()[0U].Code == DiagnosticCode::GiaProtobufModelMismatch);
        MPP_CHECK(First.error()[0U].Message == Second.error()[0U].Message);
    }

    void TestGiaProtobufEncoderPreservesCanonicalConnectionOrder()
    {
        const auto FirstGraph = MakeThreeResolved(false);
        const auto SecondGraph = MakeThreeResolved(true);
        MPP_CHECK(FirstGraph.has_value() && SecondGraph.has_value());
        const auto First = GiaProtobufEncoder::Encode(*FirstGraph);
        const auto Second = GiaProtobufEncoder::Encode(*SecondGraph);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(First->GetBytes().size() == Second->GetBytes().size());
        MPP_CHECK(std::equal(First->GetBytes().begin(), First->GetBytes().end(), Second->GetBytes().begin()));
        TestSnapshot Decoded;
        MPP_CHECK(ParseRoot(*First, Decoded));
        MPP_CHECK(FindPin(FindNode(Decoded, 1), TestPinKind::OutFlow, 10).Connections[0U].PeerNodeIndex == 2);
        MPP_CHECK(FindPin(FindNode(Decoded, 2), TestPinKind::OutFlow, 13).Connections[0U].PeerNodeIndex == 3);
    }
}

int main()
{
    TestGiaProtobufEncoderAcceptsAuthenticFirstFixture();
    TestGiaProtobufEncoderMapsRootAndGraphHeader();
    TestGiaProtobufEncoderPreservesConcreteZeroPresence();
    TestGiaProtobufEncoderMapsNodePropertiesAndPositions();
    TestGiaProtobufEncoderMapsParameterPinsAndSecondaryIndexes();
    TestGiaProtobufEncoderMapsFlowPins();
    TestGiaProtobufEncoderEncodesBooleanDescriptorDefault();
    TestGiaProtobufEncoderEncodesEnumDescriptorDefault();
    TestGiaProtobufEncoderEncodesExplicitLiteral();
    TestGiaProtobufEncoderEncodesConnectedInputWithoutLiteral();
    TestGiaProtobufEncoderEncodesDataConnections();
    TestGiaProtobufEncoderEncodesControlConnections();
    TestGiaProtobufEncoderOmitsUnsupportedOptionalSections();
    TestGiaProtobufEncoderDecodesIntoFreshMessage();
    TestGiaProtobufEncoderValidatesDecodedStructureIndependently();
    TestGiaProtobufEncoderRejectsInvalidResolvedModel();
    TestGiaProtobufEncoderRejectsGraphUnitIdRangeFailure();
    TestGiaProtobufEncoderReturnsNoBytesOnSerializationOrDecodeFailure();
    TestGiaProtobufEncoderProducesDeterministicBytes();
    TestGiaProtobufEncoderOwnsBytesAfterInputExpires();
    TestGiaProtobufEncoderDoesNotMutateResolvedGraph();
    TestGiaProtobufEncoderReportsMalformedDecodedStructure();
    TestGiaProtobufEncoderReportsModelMismatchDeterministically();
    TestGiaProtobufEncoderPreservesCanonicalConnectionOrder();
    return EXIT_SUCCESS;
}
