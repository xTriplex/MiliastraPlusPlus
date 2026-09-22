#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGiaGraphResolver.h"
#include "MiliastraPlusPlusGiaGraphLowerer.h"
#include "MiliastraPlusPlusGiaExportContext.h"
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

    constexpr char CombinedSourceRevision[] =
        "genshin-ts@26bdf2a9a3fadba934423940489236f0b53eb3ea;"
        "resources/client_node_metadata.json@93237c724f6453650ae9394077620c6e0fddb3d3;"
        "resources/client_node_modes.json@b7e14a0dd7102ccd682235cf958a2d3d36378033;"
        "src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/"
        "node_data/client_enum_values.ts@17b7d80dcd414739bd42093c6142f93b2af2cf6c;"
        "src/definitions/client_enums.ts@b23535b9054b60d2069e88c13316958124aaa8a8";

    constexpr char P6_3Evidence[] =
        "resources/client_node_static_metadata.json@17d0290993c1fef2d84d5105b2f23b7429a77ba3;"
        "src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/node_data/client_node_metadata.ts@657589f1ebd11078e2c78ad1da5c2726032c36f5;"
        "src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/node_data/node_pin_records.ts@4cd90e4b2cb6147da442af2d79ff8abe61b6ee62;"
        "src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/gia_gen/client_basic.ts@7fad6e1f4d04d991667840694d598ac590a75dab;"
        "src/compiler/ir_to_gia_transform/client_graph.ts@5daf0088c7bb8f90e54077cbbb904d293f0a56fa;"
        "src/compiler/ir_to_gia_transform/layout.ts@e5af968fe4cb6d2ea9b0f678f497213245a91e73;"
        "src/thirdparty/Genshin-Impact-Miliastra-Wonderland-Code-Node-Editor-Pack/protobuf/gia.proto@233bbb264fa89abb1dc708d4d0ad1142dc213b86;"
        "scripts/client-nodegraph/smoke-client-exec-bindings.ts@faa3007db173590052812a3ad05e945ecbf1d607;"
        "scripts/client-nodegraph/smoke-send-signal.ts@e2f2a8e3dbafb7c3c617fbd55ba688b62ef82a9c;"
        "scripts/client-nodegraph/smoke-ordered-start-pins.ts@6d33fde53db84a6d95a6ae17b742a3439e2b3920";

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
        std::vector<NormalizedPinRecord> Pins,
        std::optional<ExecutionControlSchema> ControlSchema = std::nullopt
    )
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(Identity)),
            "P6.3 fixture descriptor",
            {NodeAvailability::Client},
            std::move(Pins),
            std::move(ControlSchema),
            SourceProvenance("p63.fixture", "record")
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

    NormalizedNodeDescriptorRecord MakeBooleanOutputRecord(std::string Identity)
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "Output",
                    TypeDesc::Boolean(),
                    PinDirection::Output,
                    PinCategory::Data
                )
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeLegacyRecord(std::uint32_t Index)
    {
        return MakeBooleanInputRecord(
            "legacy-" + std::to_string(Index),
            LiteralValue(LiteralValue::Data{false})
        );
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

    NormalizedNodeDescriptorRecord MakeFlowPairRecord(std::string Identity)
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "InputFlow",
                    TypeDesc::Flow(),
                    PinDirection::Input,
                    PinCategory::Execution
                ),
                NormalizedPinRecord(
                    "OutputFlow",
                    TypeDesc::Flow(),
                    PinDirection::Output,
                    PinCategory::Execution
                )
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
                    NormalizedPinRecord(
                        "OutputValue",
                        TypeDesc::Boolean(),
                        PinDirection::Output,
                        PinCategory::Data
                    ),
                    NormalizedPinRecord(
                        "OutputFlow",
                        TypeDesc::Flow(),
                        PinDirection::Output,
                        PinCategory::Execution
                    )
                },
                EntryControlSchema{PinIndex(1U)}
            ),
            MakeRecord(
                "three-intermediate",
                {
                    NormalizedPinRecord(
                        "InputValue",
                        TypeDesc::Boolean(),
                        PinDirection::Input,
                        PinCategory::Data,
                        PinCardinality::Single,
                        true
                    ),
                    NormalizedPinRecord(
                        "OutputValue",
                        TypeDesc::Boolean(),
                        PinDirection::Output,
                        PinCategory::Data
                    ),
                    NormalizedPinRecord(
                        "InputFlow",
                        TypeDesc::Flow(),
                        PinDirection::Input,
                        PinCategory::Execution
                    ),
                    NormalizedPinRecord(
                        "OutputFlow",
                        TypeDesc::Flow(),
                        PinDirection::Output,
                        PinCategory::Execution
                    )
                },
                SequenceControlSchema{PinIndex(2U), PinIndex(3U)}
            ),
            MakeRecord(
                "three-destination",
                {
                    NormalizedPinRecord(
                        "InputValue",
                        TypeDesc::Boolean(),
                        PinDirection::Input,
                        PinCategory::Data,
                        PinCardinality::Single,
                        true
                    ),
                    NormalizedPinRecord(
                        "InputFlow",
                        TypeDesc::Flow(),
                        PinDirection::Input,
                        PinCategory::Execution
                    )
                }
            )
        };
    }

    GiaExportConfiguration MakeConfiguration(std::int64_t GraphIdentifierValue = 63001, std::int64_t UniqueIdentifierValue = 63002)
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(GraphIdentifierValue),
            "P6.3 Fixture Graph",
            GiaUniqueIdentifier(UniqueIdentifierValue),
            0.5
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    GiaBackendPinMapping MakeInputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendTypeCode = 5,
        GiaLiteralEncodingKind Encoding = GiaLiteralEncodingKind::Boolean,
        GiaPinEmissionPolicy EmissionPolicy = GiaPinEmissionPolicy::Emit,
        bool Connectable = true,
        std::int32_t BackendIndex = -1,
        std::optional<std::int32_t> SecondaryIndex = std::nullopt
    )
    {
        const std::int32_t EffectiveBackendIndex = BackendIndex < 0
            ? static_cast<std::int32_t>(SemanticPin)
            : BackendIndex;
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::InputParameter,
            GiaPinIndex(EffectiveBackendIndex),
            SecondaryIndex.has_value()
                ? std::optional<GiaPinIndex>(GiaPinIndex(*SecondaryIndex))
                : std::nullopt,
            GiaBackendTypeCode(BackendTypeCode),
            Encoding,
            EmissionPolicy,
            Connectable
        );
    }

    GiaBackendPinMapping MakeOutputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendIndex,
        std::optional<std::int32_t> SecondaryIndex = std::nullopt
    )
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::OutputParameter,
            GiaPinIndex(BackendIndex),
            SecondaryIndex.has_value()
                ? std::optional<GiaPinIndex>(GiaPinIndex(*SecondaryIndex))
                : std::nullopt,
            GiaBackendTypeCode(5),
            GiaLiteralEncodingKind::Boolean,
            GiaPinEmissionPolicy::Emit,
            true
        );
    }

    GiaBackendPinMapping MakeFlowMapping(
        std::uint32_t SemanticPin,
        GiaPinKind Kind,
        std::int32_t BackendIndex,
        GiaPinEmissionPolicy EmissionPolicy = GiaPinEmissionPolicy::Emit
    )
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            Kind,
            GiaPinIndex(BackendIndex),
            std::nullopt,
            GiaBackendTypeCode(1),
            GiaLiteralEncodingKind::None,
            EmissionPolicy,
            false
        );
    }

    template<typename MappingFactory>
    std::expected<GiaExportContext, DiagnosticCollection> MakeContext(
        std::vector<NormalizedNodeDescriptorRecord> Records,
        MappingFactory MappingFactoryFunction,
        std::string SourceNamespace = "p63.fixture",
        std::string SourceRevision = "p63.fixture@1"
    )
    {
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            std::move(SourceNamespace),
            std::move(SourceRevision),
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
        const auto Registry = DescriptorCatalogueRegistryContext::Materialize(
            *Snapshot
        );
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

    std::expected<GiaExportContext, DiagnosticCollection> MakeAuthenticContext()
    {
        const auto SourceRecord =
            GenshinClientBooleanFilterResultNodeSourceAdapter::Adapt(
                NodeMetadataJson,
                ModesJson,
                EnumEvidenceJson
            );
        MPP_CHECK(SourceRecord.has_value());
        auto Records = MakeLegacyRecords();
        Records.push_back(*SourceRecord);
        return MakeContext(
            std::move(Records),
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("200000"),
                        GiaNodeGenericId(200000),
                        GiaNodeConcreteId(0),
                        {
                            MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean),
                            MakeInputMapping(1U, 13, GiaLiteralEncodingKind::Enum)
                        },
                        SourceProvenance("p63.graph-encoding", "200000")
                    )
                };
            },
            "genshin.client-bool-filter-descriptor-source",
            CombinedSourceRevision
        );
    }

    NodeDescriptorId GetDescriptorId(const GiaExportContext& Context, std::string_view Identity)
    {
        const auto* Entry = Context.GetRegistryContext().GetCatalogue().
            FindByExternalIdentity(ExternalNodeIdentity(std::string(Identity)));
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

    std::expected<GiaBackendGraph, DiagnosticCollection> Lower(const GraphIR& Graph, const GiaExportContext& Context)
    {
        return GiaGraphLowerer::Lower(Graph, Context);
    }

    std::expected<GiaResolvedBackendGraph, DiagnosticCollection> Resolve(const GiaBackendGraph& Graph)
    {
        return GiaGraphResolver::Resolve(Graph);
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeThreeNodeRecordsWithDefaults()
    {
        auto Records = MakeThreeNodeRecords();
        return Records;
    }

    std::vector<GiaBackendNodeMapping> MakeThreeNodeMappings(const DescriptorCatalogueIdentity&, bool Reverse)
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
                    MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean,
                        GiaPinEmissionPolicy::Emit, true, 4, 7),
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
                    MakeInputMapping(0U),
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

    std::expected<GiaExportContext, DiagnosticCollection> MakeThreeNodeContext(bool ReverseMappings = false)
    {
        return MakeContext(
            MakeThreeNodeRecordsWithDefaults(),
            [ReverseMappings](const DescriptorCatalogueIdentity& Identity)
            {
                return MakeThreeNodeMappings(Identity, ReverseMappings);
            },
            "p63.three-node",
            "p63.three-node@1"
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

        auto AddBindings = [&Graph](bool Reverse)
        {
            if (Reverse)
            {
                Graph.BindInput(
                    NodeInstanceId(3U),
                    PinIndex(0U),
                    OutputReference{NodeInstanceId(2U), PinIndex(1U)}
                );
                Graph.BindInput(
                    NodeInstanceId(2U),
                    PinIndex(0U),
                    OutputReference{NodeInstanceId(1U), PinIndex(0U)}
                );
            }
            else
            {
                Graph.BindInput(
                    NodeInstanceId(2U),
                    PinIndex(0U),
                    OutputReference{NodeInstanceId(1U), PinIndex(0U)}
                );
                Graph.BindInput(
                    NodeInstanceId(3U),
                    PinIndex(0U),
                    OutputReference{NodeInstanceId(2U), PinIndex(1U)}
                );
            }
        };
        AddBindings(ReverseBindings);

        auto AddControlEdges = [&Graph](bool Reverse)
        {
            const ControlEdge First{
                NodeInstanceId(1U),
                PinIndex(1U),
                NodeInstanceId(2U),
                PinIndex(2U)
            };
            const ControlEdge Second{
                NodeInstanceId(2U),
                PinIndex(3U),
                NodeInstanceId(3U),
                PinIndex(1U)
            };
            if (Reverse)
            {
                Graph.AddControlEdge(Second);
                Graph.AddControlEdge(First);
            }
            else
            {
                Graph.AddControlEdge(First);
                Graph.AddControlEdge(Second);
            }
        };
        AddControlEdges(ReverseControlEdges);
        return Graph;
    }

    GiaBackendGraph MakeManualFlowCycleGraph(const GiaBackendGraph& Template)
    {
        const NodeDescriptorId Descriptor =
            Template.GetNodes()[0U].Trace.Descriptor;
        const auto MakeNode = [Descriptor](NodeInstanceId GraphNode, std::string Identity, GiaNodeGenericId Generic)
        {
            return GiaBackendNode{
                .Trace = GiaBackendNodeTrace{
                    GraphNode,
                    Descriptor,
                    ExternalNodeIdentity(std::move(Identity)),
                    SourceProvenance("p63.synthetic", "cycle-descriptor"),
                    SourceProvenance("p63.synthetic", "cycle-mapping")
                },
                .Mapping = GiaBackendNodeMapping(
                    ExternalNodeIdentity(GraphNode.GetValue() == 1U
                        ? "cycle-a"
                        : "cycle-b"),
                    Generic,
                    GiaNodeConcreteId(0),
                    {
                        MakeFlowMapping(0U, GiaPinKind::InputFlow, 7),
                        MakeFlowMapping(1U, GiaPinKind::OutputFlow, 9)
                    }
                ),
                .Inputs = {}
            };
        };

        std::vector<GiaBackendNode> Nodes{
            MakeNode(NodeInstanceId(1U), "cycle-a", GiaNodeGenericId(401)),
            MakeNode(NodeInstanceId(2U), "cycle-b", GiaNodeGenericId(402))
        };
        std::vector<GiaBackendControlConnection> Connections{
            {
                NodeInstanceId(1U),
                PinIndex(1U),
                NodeInstanceId(2U),
                PinIndex(0U)
            },
            {
                NodeInstanceId(2U),
                PinIndex(1U),
                NodeInstanceId(1U),
                PinIndex(0U)
            }
        };
        return GiaBackendGraphDetail::CreateForTesting(
            Template.GetHeader(),
            std::move(Nodes),
            {},
            std::move(Connections)
        );
    }

    GiaBackendGraph MakeUnsupportedKindGraph(const GiaBackendGraph& Template, GiaPinKind Kind)
    {
        const GiaBackendNode& Original = Template.GetNodes()[0U];
        const GiaBackendPinMapping Mapping(
            PinIndex(0U),
            Kind,
            GiaPinIndex(0),
            std::nullopt,
            GiaBackendTypeCode(1),
            GiaLiteralEncodingKind::None,
            GiaPinEmissionPolicy::Emit,
            false
        );
        GiaBackendNode Node{
            Original.Trace,
            GiaBackendNodeMapping(
                ExternalNodeIdentity("unsupported-kind"),
                GiaNodeGenericId(450),
                GiaNodeConcreteId(0),
                {Mapping}
            ),
            {}
        };
        Node.Trace.ExternalIdentity = ExternalNodeIdentity("unsupported-kind");
        return GiaBackendGraphDetail::CreateForTesting(
            Template.GetHeader(),
            {std::move(Node)},
            {},
            {}
        );
    }

    GiaBackendGraph MakeDiagnosticConflictGraph(const GiaBackendGraph& Template)
    {
        const NodeDescriptorId Descriptor = Template.GetNodes()[0U].Trace.Descriptor;
        GiaBackendNode Source{
            GiaBackendNodeTrace{
                NodeInstanceId(1U),
                Descriptor,
                ExternalNodeIdentity("diagnostic-source"),
                std::nullopt,
                std::nullopt
            },
            GiaBackendNodeMapping(
                ExternalNodeIdentity("diagnostic-source"),
                GiaNodeGenericId(501),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(0U, GiaPinKind::OutputFlow, 2)}
            ),
            {}
        };
        GiaBackendNode OmittedDestination{
            GiaBackendNodeTrace{
                NodeInstanceId(2U),
                Descriptor,
                ExternalNodeIdentity("diagnostic-destination"),
                std::nullopt,
                std::nullopt
            },
            GiaBackendNodeMapping(
                ExternalNodeIdentity("diagnostic-destination"),
                GiaNodeGenericId(502),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(
                    0U,
                    GiaPinKind::InputFlow,
                    3,
                    GiaPinEmissionPolicy::Omit
                )}
            ),
            {}
        };
        GiaBackendNode MissingInput{
            GiaBackendNodeTrace{
                NodeInstanceId(3U),
                Descriptor,
                ExternalNodeIdentity("diagnostic-input"),
                std::nullopt,
                std::nullopt
            },
            GiaBackendNodeMapping(
                ExternalNodeIdentity("diagnostic-input"),
                GiaNodeGenericId(503),
                GiaNodeConcreteId(0),
                {MakeInputMapping(0U)}
            ),
            {}
        };
        return GiaBackendGraphDetail::CreateForTesting(
            Template.GetHeader(),
            {
                std::move(Source),
                std::move(OmittedDestination),
                std::move(MissingInput)
            },
            {},
            {
                {
                    NodeInstanceId(1U),
                    PinIndex(0U),
                    NodeInstanceId(2U),
                    PinIndex(0U)
                }
            }
        );
    }

    GiaBackendGraph MakeNonAdjacentControlDestinationGraph(const GiaBackendGraph& Template)
    {
        std::vector<GiaBackendNode> Nodes = Template.GetNodes();
        GiaBackendNode FourthSource = Nodes[0U];
        FourthSource.Trace.GraphNode = NodeInstanceId(4U);
        GiaBackendNode FifthSource = Nodes[0U];
        FifthSource.Trace.GraphNode = NodeInstanceId(5U);
        Nodes.push_back(std::move(FourthSource));
        Nodes.push_back(std::move(FifthSource));
        std::sort(
            Nodes.begin(),
            Nodes.end(),
            [](const GiaBackendNode& Left, const GiaBackendNode& Right)
            {
                return Left.Trace.GraphNode < Right.Trace.GraphNode;
            }
        );

        std::vector<GiaBackendControlConnection> Connections{
            {
                NodeInstanceId(1U),
                PinIndex(1U),
                NodeInstanceId(2U),
                PinIndex(2U)
            },
            {
                NodeInstanceId(4U),
                PinIndex(1U),
                NodeInstanceId(3U),
                PinIndex(1U)
            },
            {
                NodeInstanceId(5U),
                PinIndex(1U),
                NodeInstanceId(2U),
                PinIndex(2U)
            }
        };
        std::sort(
            Connections.begin(),
            Connections.end()
        );
        return GiaBackendGraphDetail::CreateForTesting(
            Template.GetHeader(),
            std::move(Nodes),
            Template.GetDataConnections(),
            std::move(Connections)
        );
    }

    void ReplaceResolvedPin(GiaResolvedNode& Node, PinIndex SemanticPin, const std::function<void(GiaResolvedPin&)>& Mutator)
    {
        for (GiaResolvedPin& Pin : Node.Pins)
        {
            if (Pin.SemanticPin == SemanticPin)
            {
                Mutator(Pin);
                return;
            }
        }
        MPP_CHECK(false);
    }

    void TestGiaGraphResolverAcceptsAuthenticFirstFixture()
    {
        MPP_CHECK(std::string_view(P6_3Evidence).find("gia.proto") != std::string_view::npos);
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetNodes().size() == 1U);
        MPP_CHECK(Result->GetDataConnections().empty());
        MPP_CHECK(Result->GetControlConnections().empty());
    }

    void TestGiaGraphResolverCopiesResolvedHeaderFacts()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const GiaResolvedGraphHeader& Header = Result->GetHeader();
        MPP_CHECK(Header.TargetProfile == GiaExportTargetProfile::ClientBooleanFilter);
        MPP_CHECK(Header.Mode == GiaExportMode::Beyond);
        MPP_CHECK(Header.GraphIdentifier.GetValue() == 63001);
        MPP_CHECK(Header.GraphName == "P6.3 Fixture Graph");
        MPP_CHECK(Header.UniqueIdentifier.GetValue() == 63002);
        MPP_CHECK(Header.EvaluationInterval == 0.5F);
        MPP_CHECK(Header.GraphType == 20001);
        MPP_CHECK(Header.GraphWhich == 10);
        MPP_CHECK(Header.RootClass == GiaResolvedGraphUnitClass::Node);
        MPP_CHECK(Header.RootType == GiaResolvedGraphUnitType::ClientGraph);
        MPP_CHECK(Header.InnerClass == GiaResolvedNodeGraphClass::UserDefined);
        MPP_CHECK(Header.InnerKind == GiaResolvedNodeGraphKind::NodeGraph);
        MPP_CHECK(Header.EntrySlotIndex == 1);
        MPP_CHECK(!Header.RootModeFlag.has_value());
    }

    void TestGiaGraphResolverPreservesNodeIdentityAndConcreteZero()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const GiaResolvedNode& Node = Result->GetNodes()[0U];
        MPP_CHECK(Node.GraphNode == NodeInstanceId(1U));
        MPP_CHECK(Node.NodeIndex == GiaResolvedNodeIndex(1));
        MPP_CHECK(Node.ExternalIdentity.GetKey() == "200000");
        MPP_CHECK(Node.GenericNodeIdentifier == GiaNodeGenericId(200000));
        MPP_CHECK(Node.ConcreteNodeIdentifier.has_value());
        MPP_CHECK(Node.ConcreteNodeIdentifier->GetValue() == 0);
    }

    void TestGiaGraphResolverResolvesFinalPinKindAndIndices()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const auto& Pins = Result->GetNodes()[0U].Pins;
        MPP_CHECK(Pins.size() == 2U);
        MPP_CHECK(Pins[0U].Kind == GiaPinKind::InputParameter);
        MPP_CHECK(Pins[1U].Kind == GiaPinKind::InputParameter);
        MPP_CHECK(Pins[0U].PrimaryIndex == GiaPinIndex(0));
        MPP_CHECK(Pins[0U].SecondaryIndex == GiaPinIndex(0));
        MPP_CHECK(Pins[1U].PrimaryIndex == GiaPinIndex(1));
        MPP_CHECK(Pins[1U].SecondaryIndex == GiaPinIndex(1));
    }

    void TestGiaGraphResolverResolvesDescriptorDefaults()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const auto& Pins = Result->GetNodes()[0U].Pins;
        MPP_CHECK(Pins[0U].InputValue->SourceKind ==
            GiaBackendInputValueSourceKind::DescriptorDefault);
        MPP_CHECK(Pins[0U].InputValue->Literal->Is<bool>());
        MPP_CHECK(*Pins[0U].InputValue->Literal->TryGet<bool>() == false);
        MPP_CHECK(Pins[1U].InputValue->SourceKind ==
            GiaBackendInputValueSourceKind::DescriptorDefault);
        const auto* Enum = Pins[1U].InputValue->Literal->TryGet<EnumLiteralValue>();
        MPP_CHECK(Enum != nullptr);
        MPP_CHECK(Enum->GetEnumTypeIdentity() == EnumTypeIdentity("filter_return_type"));
        MPP_CHECK(Enum->GetValue() == 1000010);
    }

    void TestGiaGraphResolverResolvesExplicitLiterals()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            LiteralValue(LiteralValue::Data{true})
        );
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(1U),
            LiteralValue(EnumLiteralValue(
                EnumTypeIdentity("filter_return_type"),
                10000011
            ))
        );
        const auto Backend = Lower(Graph, *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const auto& Pins = Result->GetNodes()[0U].Pins;
        MPP_CHECK(Pins[0U].InputValue->SourceKind ==
            GiaBackendInputValueSourceKind::ExplicitLiteral);
        MPP_CHECK(*Pins[0U].InputValue->Literal->TryGet<bool>());
        MPP_CHECK(Pins[1U].InputValue->SourceKind ==
            GiaBackendInputValueSourceKind::ExplicitLiteral);
        MPP_CHECK(Pins[1U].InputValue->Literal->TryGet<EnumLiteralValue>() != nullptr);
        MPP_CHECK(Pins[1U].InputValue->Literal->TryGet<EnumLiteralValue>()->GetValue() ==
            10000011);
    }

    void TestGiaGraphResolverResolvesSemanticDataConnection()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetDataConnections().size() == 2U);
        const auto& First = Result->GetDataConnections()[0U];
        MPP_CHECK(First.Source.Node == GiaResolvedNodeIndex(1));
        MPP_CHECK(First.Source.Kind == GiaPinKind::OutputParameter);
        MPP_CHECK(First.Source.PrimaryIndex == GiaPinIndex(8));
        MPP_CHECK(First.Destination.Node == GiaResolvedNodeIndex(2));
        MPP_CHECK(First.Destination.PrimaryIndex == GiaPinIndex(4));
        MPP_CHECK(First.Destination.SecondaryIndex == GiaPinIndex(7));
    }

    void TestGiaGraphResolverResolvesSemanticControlConnection()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetControlConnections().size() == 2U);
        MPP_CHECK(Result->GetControlConnections()[0U].Source.Kind == GiaPinKind::OutputFlow);
        MPP_CHECK(Result->GetControlConnections()[0U].Destination.Kind == GiaPinKind::InputFlow);
        MPP_CHECK(Result->GetControlConnections()[0U].Source.PrimaryIndex == GiaPinIndex(10));
        MPP_CHECK(Result->GetControlConnections()[0U].Destination.PrimaryIndex == GiaPinIndex(12));
    }

    void TestGiaGraphResolverRejectsUnsupportedClientExecutionAndSignalPins()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        for (const GiaPinKind Kind : {GiaPinKind::ClientExecution, GiaPinKind::ClientSignal})
        {
            const GiaBackendGraph Invalid = MakeUnsupportedKindGraph(*Backend, Kind);
            MPP_CHECK(Invalid.IsValid());
            const auto Result = Resolve(Invalid);
            MPP_CHECK(!Result.has_value());
            MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaResolvedPinKind));
        }
    }

    void TestGiaGraphResolverRejectsInvalidOmittedOrUnconnectableEndpoints()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const GiaBackendGraph Invalid = MakeDiagnosticConflictGraph(*Backend);
        MPP_CHECK(Invalid.IsValid());
        const auto Result = Resolve(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaResolvedPinEmission));
    }

    void TestGiaGraphResolverResolvesExplicitSecondaryIndex()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const auto& Intermediate = Result->GetNodes()[1U];
        const GiaResolvedPin* Pin = nullptr;
        for (const GiaResolvedPin& Candidate : Intermediate.Pins)
        {
            if (Candidate.SemanticPin == PinIndex(0U))
            {
                Pin = &Candidate;
            }
        }
        MPP_CHECK(Pin != nullptr);
        MPP_CHECK(Pin->PrimaryIndex == GiaPinIndex(4));
        MPP_CHECK(Pin->SecondaryIndex == GiaPinIndex(7));
    }

    void TestGiaGraphResolverRejectsInvalidInputModel()
    {
        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            GiaBackendGraphHeader{},
            {},
            {},
            {}
        );
        const auto Result = Resolve(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(Result.error().size() == 1U);
        MPP_CHECK(Result.error()[0U].Code == DiagnosticCode::InvalidGiaResolutionInput);

        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        GiaBackendGraphHeader InvalidIntervalHeader = Backend->GetHeader();
        InvalidIntervalHeader.EvaluationInterval =
            std::numeric_limits<double>::max();
        const GiaBackendGraph InvalidInterval =
            GiaBackendGraphDetail::CreateForTesting(
                std::move(InvalidIntervalHeader),
                Backend->GetNodes(),
                Backend->GetDataConnections(),
                Backend->GetControlConnections()
            );
        MPP_CHECK(InvalidInterval.IsValid());
        const auto InvalidIntervalResult = Resolve(InvalidInterval);
        MPP_CHECK(!InvalidIntervalResult.has_value());
        MPP_CHECK(InvalidIntervalResult.error().size() == 1U);
        MPP_CHECK(
            InvalidIntervalResult.error()[0U].Code ==
                DiagnosticCode::InvalidGiaResolutionInput
        );
        MPP_CHECK(!InvalidIntervalResult.error()[0U].SourceNodeIdentifier.has_value());
        MPP_CHECK(!InvalidIntervalResult.error()[0U].DestinationNodeIdentifier.has_value());
        MPP_CHECK(!InvalidIntervalResult.error()[0U].SourcePinReference.has_value());
        MPP_CHECK(!InvalidIntervalResult.error()[0U].DestinationPinReference.has_value());
        MPP_CHECK(!InvalidIntervalResult.error()[0U].ExternalIdentityKey.has_value());
    }

    void TestGiaGraphResolverOrdersNodesPinsAndConnections()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(
            MakeThreeNodeGraph(*Context, true, true, true),
            *Context
        );
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodes()[0U].GraphNode == NodeInstanceId(1U));
        MPP_CHECK(Result->GetNodes()[1U].GraphNode == NodeInstanceId(2U));
        MPP_CHECK(Result->GetNodes()[2U].GraphNode == NodeInstanceId(3U));
        for (const GiaResolvedNode& Node : Result->GetNodes())
        {
            for (std::size_t Index = 1U; Index < Node.Pins.size(); ++Index)
            {
                MPP_CHECK(GiaResolvedBackendGraphDetail::PinOrderKey(
                    Node.Pins[Index - 1U]
                ) < GiaResolvedBackendGraphDetail::PinOrderKey(Node.Pins[Index]));
            }
        }
        for (std::size_t Index = 1U; Index < Result->GetDataConnections().size(); ++Index)
        {
            MPP_CHECK(GiaResolvedBackendGraphDetail::DataConnectionOrderKey(
                Result->GetDataConnections()[Index - 1U]
            ) < GiaResolvedBackendGraphDetail::DataConnectionOrderKey(
                Result->GetDataConnections()[Index]
            ));
        }
        for (std::size_t Index = 1U; Index < Result->GetControlConnections().size(); ++Index)
        {
            MPP_CHECK(GiaResolvedBackendGraphDetail::ControlConnectionOrderKey(
                Result->GetControlConnections()[Index - 1U]
            ) < GiaResolvedBackendGraphDetail::ControlConnectionOrderKey(
                Result->GetControlConnections()[Index]
            ));
        }
    }

    void TestGiaGraphResolverIsInvariantToNodePermutation()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto FirstBackend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        const auto SecondBackend = Lower(
            MakeThreeNodeGraph(*Context, true, false, false),
            *Context
        );
        MPP_CHECK(FirstBackend.has_value() && SecondBackend.has_value());
        const auto First = Resolve(*FirstBackend);
        const auto Second = Resolve(*SecondBackend);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
    }

    void TestGiaGraphResolverIsInvariantToInputAndConnectionPermutation()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto FirstBackend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        const auto SecondBackend = Lower(
            MakeThreeNodeGraph(*Context, false, true, true),
            *Context
        );
        MPP_CHECK(FirstBackend.has_value() && SecondBackend.has_value());
        const auto First = Resolve(*FirstBackend);
        const auto Second = Resolve(*SecondBackend);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
    }

    void TestGiaGraphResolverIsInvariantToMappingPackagePermutation()
    {
        const auto ForwardContext = MakeThreeNodeContext(false);
        const auto ReverseContext = MakeThreeNodeContext(true);
        MPP_CHECK(ForwardContext.has_value() && ReverseContext.has_value());
        const GraphIR ForwardGraph = MakeThreeNodeGraph(
            *ForwardContext,
            false,
            true,
            true
        );
        const GraphIR ReverseGraph = MakeThreeNodeGraph(
            *ReverseContext,
            true,
            false,
            false
        );
        const auto FirstBackend = Lower(ForwardGraph, *ForwardContext);
        const auto SecondBackend = Lower(ReverseGraph, *ReverseContext);
        MPP_CHECK(FirstBackend.has_value() && SecondBackend.has_value());
        MPP_CHECK(*FirstBackend == *SecondBackend);
        const auto First = Resolve(*FirstBackend);
        const auto Second = Resolve(*SecondBackend);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
    }

    void TestGiaGraphResolverPlacesAuthenticNodeAtOrigin()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        const GiaResolvedNodePosition Origin{0.0F, 0.0F};
        MPP_CHECK(Result->GetNodes()[0U].Position == Origin);
    }

    void TestGiaGraphResolverPlacesDisconnectedNodesDeterministically()
    {
        const auto Context = MakeContext(
            {
                MakeBooleanInputRecord("disconnected-a", LiteralValue(LiteralValue::Data{false})),
                MakeBooleanInputRecord("disconnected-b", LiteralValue(LiteralValue::Data{true}))
            },
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("disconnected-a"),
                        GiaNodeGenericId(601),
                        GiaNodeConcreteId(0),
                        {MakeInputMapping(0U)}
                    ),
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("disconnected-b"),
                        GiaNodeGenericId(602),
                        GiaNodeConcreteId(0),
                        {MakeInputMapping(0U)}
                    )
                };
            },
            "p63.disconnected",
            "p63.disconnected@1"
        );
        MPP_CHECK(Context.has_value());
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(2U), GetDescriptorId(*Context, "disconnected-b"), std::nullopt});
        Graph.AddNode({NodeInstanceId(1U), GetDescriptorId(*Context, "disconnected-a"), std::nullopt});
        const auto Backend = Lower(Graph, *Context);
        MPP_CHECK(Backend.has_value());
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodes()[0U].GraphNode == NodeInstanceId(1U));
        const GiaResolvedNodePosition FirstPosition{0.0F, 0.0F};
        const GiaResolvedNodePosition SecondPosition{0.0F, 600.0F};
        MPP_CHECK(Result->GetNodes()[0U].Position == FirstPosition);
        MPP_CHECK(Result->GetNodes()[1U].Position == SecondPosition);
    }

    void TestGiaGraphResolverPlacesCyclicControlGraphDeterministically()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const GiaBackendGraph Cycle = MakeManualFlowCycleGraph(*Backend);
        MPP_CHECK(Cycle.IsValid());
        const auto First = Resolve(Cycle);
        const auto Second = Resolve(Cycle);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
        const GiaResolvedNodePosition FirstPosition{0.0F, 0.0F};
        const GiaResolvedNodePosition SecondPosition{0.0F, 600.0F};
        MPP_CHECK(First->GetNodes()[0U].Position == FirstPosition);
        MPP_CHECK(First->GetNodes()[1U].Position == SecondPosition);
    }

    void TestGiaGraphResolverRejectsInvalidNodeIndex()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        std::vector<GiaBackendNode> Nodes = Backend->GetNodes();
        Nodes[0U].Trace.GraphNode = NodeInstanceId(
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1U
        );
        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            Backend->GetHeader(),
            std::move(Nodes),
            Backend->GetDataConnections(),
            Backend->GetControlConnections()
        );
        MPP_CHECK(Invalid.IsValid());
        const auto Result = Resolve(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaResolvedNodeIdentity));
    }

    void TestGiaGraphResolverRejectsDanglingOrDuplicateConnections()
    {
        const auto Context = MakeThreeNodeContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(
            MakeThreeNodeGraph(*Context, false, false, false),
            *Context
        );
        MPP_CHECK(Backend.has_value());
        std::vector<GiaBackendDataConnection> Connections =
            Backend->GetDataConnections();
        Connections.push_back(Connections.back());
        std::sort(
            Connections.begin(),
            Connections.end(),
            [](const GiaBackendDataConnection& Left, const GiaBackendDataConnection& Right)
            {
                return std::tie(
                    Left.SourceNode,
                    Left.SourcePin,
                    Left.DestinationNode,
                    Left.DestinationPin
                ) < std::tie(
                    Right.SourceNode,
                    Right.SourcePin,
                    Right.DestinationNode,
                    Right.DestinationPin
                );
            }
        );
        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            Backend->GetHeader(),
            Backend->GetNodes(),
            std::move(Connections),
            Backend->GetControlConnections()
        );
        MPP_CHECK(!Invalid.IsValid());
        const auto Result = Resolve(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(Result.error().size() == 1U);
        MPP_CHECK(Result.error()[0U].Code == DiagnosticCode::InvalidGiaResolutionInput);

        const GiaBackendGraph InvalidControl =
            MakeNonAdjacentControlDestinationGraph(*Backend);
        MPP_CHECK(InvalidControl.IsValid());
        const auto ControlResult = Resolve(InvalidControl);
        MPP_CHECK(!ControlResult.has_value());
        MPP_CHECK(HasCode(
            ControlResult.error(),
            DiagnosticCode::InvalidGiaResolvedConnection
        ));
        MPP_CHECK(std::any_of(
            ControlResult.error().begin(),
            ControlResult.error().end(),
            [](const Diagnostic& DiagnosticValue)
            {
                return DiagnosticValue.Code ==
                        DiagnosticCode::InvalidGiaResolvedConnection &&
                    DiagnosticValue.Message.find(
                        "more than one incoming ordinary control connection"
                    ) != std::string::npos &&
                    DiagnosticValue.ExternalIdentityKey.has_value() &&
                    DiagnosticValue.ExternalIdentityKey.value() ==
                        "three-intermediate";
            }
        ));
    }

    void TestGiaGraphResolverReturnsNoPartialModelOnFailure()
    {
        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            GiaBackendGraphHeader{},
            {},
            {},
            {}
        );
        const auto Result = Resolve(Invalid);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(!Result.error().empty());
    }

    void TestGiaGraphResolverOwnsResultAfterInputsExpire()
    {
        std::optional<GiaBackendGraph> Backend;
        {
            const auto Context = MakeAuthenticContext();
            MPP_CHECK(Context.has_value());
            auto Graph = MakeAuthenticGraph(*Context);
            const auto Lowered = Lower(Graph, *Context);
            MPP_CHECK(Lowered.has_value());
            Backend = *Lowered;
        }
        MPP_CHECK(Backend.has_value());
        auto Resolved = Resolve(*Backend);
        MPP_CHECK(Resolved.has_value());
        Backend.reset();
        MPP_CHECK(Resolved->IsValid());
        MPP_CHECK(Resolved->GetNodes()[0U].ExternalIdentity.GetKey() == "200000");
        MPP_CHECK(Resolved->GetNodes()[0U].Pins[1U].InputValue->Literal.has_value());
    }

    void TestGiaGraphResolverDoesNotMutateBackendGraph()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const GiaBackendGraph Before = *Backend;
        const auto Result = Resolve(*Backend);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(*Backend == Before);
    }

    void TestGiaGraphResolverOrdersDiagnosticsDeterministically()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const GiaBackendGraph Invalid = MakeDiagnosticConflictGraph(*Backend);
        MPP_CHECK(Invalid.IsValid());
        const auto First = Resolve(Invalid);
        const auto Second = Resolve(Invalid);
        MPP_CHECK(!First.has_value() && !Second.has_value());
        MPP_CHECK(First.error().size() == 2U);
        MPP_CHECK(First.error()[0U].Code == DiagnosticCode::InvalidGiaResolvedPinEmission);
        MPP_CHECK(First.error()[1U].Code == DiagnosticCode::InvalidGiaResolvedPinEmission);
        MPP_CHECK(First.error()[0U].Message.find("emitted input mapping") != std::string::npos);
        MPP_CHECK(First.error()[1U].Message.find("semantic control connection") != std::string::npos);
        MPP_CHECK(First.error().size() == Second.error().size());
        for (std::size_t Index = 0U; Index < First.error().size(); ++Index)
        {
            MPP_CHECK(First.error()[Index].Code == Second.error()[Index].Code);
            MPP_CHECK(First.error()[Index].Message == Second.error()[Index].Message);
            MPP_CHECK(First.error()[Index].ExternalIdentityKey ==
                Second.error()[Index].ExternalIdentityKey);
        }
    }

    void TestGiaResolvedBackendGraphValidityRejectsIncompleteState()
    {
        const GiaResolvedBackendGraph Invalid =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                GiaResolvedGraphHeader{},
                {},
                {},
                {}
            );
        MPP_CHECK(!Invalid.IsValid());

        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Backend = Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Backend.has_value());
        const auto Authentic = Resolve(*Backend);
        MPP_CHECK(Authentic.has_value());
        MPP_CHECK(Authentic->IsValid());

        auto BooleanWrongEncoding = Authentic->GetNodes();
        ReplaceResolvedPin(
            BooleanWrongEncoding[0U],
            PinIndex(0U),
            [](GiaResolvedPin& Pin)
            {
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Enum;
            }
        );
        const auto InvalidBooleanEncoding = GiaResolvedBackendGraphDetail::CreateForTesting(
            Authentic->GetHeader(),
            std::move(BooleanWrongEncoding),
            Authentic->GetDataConnections(),
            Authentic->GetControlConnections()
        );
        MPP_CHECK(!InvalidBooleanEncoding.IsValid());

        auto BooleanWrongCode = Authentic->GetNodes();
        ReplaceResolvedPin(
            BooleanWrongCode[0U],
            PinIndex(0U),
            [](GiaResolvedPin& Pin)
            {
                Pin.BackendTypeCode = GiaBackendTypeCode(13);
            }
        );
        const auto InvalidBooleanCode = GiaResolvedBackendGraphDetail::CreateForTesting(
            Authentic->GetHeader(),
            std::move(BooleanWrongCode),
            Authentic->GetDataConnections(),
            Authentic->GetControlConnections()
        );
        MPP_CHECK(!InvalidBooleanCode.IsValid());

        auto EnumWrongEncoding = Authentic->GetNodes();
        ReplaceResolvedPin(
            EnumWrongEncoding[0U],
            PinIndex(1U),
            [](GiaResolvedPin& Pin)
            {
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Boolean;
            }
        );
        const auto InvalidEnumEncoding = GiaResolvedBackendGraphDetail::CreateForTesting(
            Authentic->GetHeader(),
            std::move(EnumWrongEncoding),
            Authentic->GetDataConnections(),
            Authentic->GetControlConnections()
        );
        MPP_CHECK(!InvalidEnumEncoding.IsValid());

        auto EnumWrongCode = Authentic->GetNodes();
        ReplaceResolvedPin(
            EnumWrongCode[0U],
            PinIndex(1U),
            [](GiaResolvedPin& Pin)
            {
                Pin.BackendTypeCode = GiaBackendTypeCode(5);
            }
        );
        const auto InvalidEnumCode = GiaResolvedBackendGraphDetail::CreateForTesting(
            Authentic->GetHeader(),
            std::move(EnumWrongCode),
            Authentic->GetDataConnections(),
            Authentic->GetControlConnections()
        );
        MPP_CHECK(!InvalidEnumCode.IsValid());

        auto WrongFamily = Authentic->GetNodes();
        ReplaceResolvedPin(
            WrongFamily[0U],
            PinIndex(1U),
            [](GiaResolvedPin& Pin)
            {
                Pin.InputValue->SemanticType = TypeDesc::Enum(
                    EnumTypeIdentity("other_enum")
                );
                Pin.InputValue->Literal = LiteralValue(EnumLiteralValue(
                    EnumTypeIdentity("other_enum"),
                    1
                ));
            }
        );
        const auto InvalidFamily = GiaResolvedBackendGraphDetail::CreateForTesting(
            Authentic->GetHeader(),
            std::move(WrongFamily),
            Authentic->GetDataConnections(),
            Authentic->GetControlConnections()
        );
        MPP_CHECK(!InvalidFamily.IsValid());

        const auto ThreeContext = MakeThreeNodeContext();
        MPP_CHECK(ThreeContext.has_value());
        const auto ThreeBackend = Lower(
            MakeThreeNodeGraph(*ThreeContext, false, false, false),
            *ThreeContext
        );
        MPP_CHECK(ThreeBackend.has_value());
        const auto ThreeResolved = Resolve(*ThreeBackend);
        MPP_CHECK(ThreeResolved.has_value());
        auto InvalidDataTupleNodes = ThreeResolved->GetNodes();
        ReplaceResolvedPin(
            InvalidDataTupleNodes[1U],
            PinIndex(0U),
            [](GiaResolvedPin& Pin)
            {
                Pin.BackendTypeCode = GiaBackendTypeCode(13);
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Enum;
                Pin.InputValue->BackendTypeCode = GiaBackendTypeCode(13);
                Pin.InputValue->LiteralEncoding = GiaLiteralEncodingKind::Enum;
            }
        );
        const auto InvalidDataTuple = GiaResolvedBackendGraphDetail::CreateForTesting(
            ThreeResolved->GetHeader(),
            std::move(InvalidDataTupleNodes),
            ThreeResolved->GetDataConnections(),
            ThreeResolved->GetControlConnections()
        );
        MPP_CHECK(!InvalidDataTuple.IsValid());

        auto NonAdjacentDuplicatePins = Authentic->GetNodes();
        GiaResolvedPin DuplicatePin = NonAdjacentDuplicatePins[0U].Pins[0U];
        DuplicatePin.PrimaryIndex = GiaPinIndex(2);
        DuplicatePin.SecondaryIndex = GiaPinIndex(2);
        NonAdjacentDuplicatePins[0U].Pins.push_back(std::move(DuplicatePin));
        std::sort(
            NonAdjacentDuplicatePins[0U].Pins.begin(),
            NonAdjacentDuplicatePins[0U].Pins.end(),
            [](const GiaResolvedPin& Left, const GiaResolvedPin& Right)
            {
                return GiaResolvedBackendGraphDetail::PinOrderKey(Left) <
                    GiaResolvedBackendGraphDetail::PinOrderKey(Right);
            }
        );
        const auto InvalidNonAdjacentDuplicatePins =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                Authentic->GetHeader(),
                std::move(NonAdjacentDuplicatePins),
                Authentic->GetDataConnections(),
                Authentic->GetControlConnections()
            );
        MPP_CHECK(!InvalidNonAdjacentDuplicatePins.IsValid());

        auto InvalidInputFlowEncodingNodes = ThreeResolved->GetNodes();
        ReplaceResolvedPin(
            InvalidInputFlowEncodingNodes[1U],
            PinIndex(2U),
            [](GiaResolvedPin& Pin)
            {
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Boolean;
            }
        );
        const auto InvalidInputFlowEncoding =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                ThreeResolved->GetHeader(),
                std::move(InvalidInputFlowEncodingNodes),
                ThreeResolved->GetDataConnections(),
                ThreeResolved->GetControlConnections()
            );
        MPP_CHECK(!InvalidInputFlowEncoding.IsValid());

        auto InvalidOutputFlowEncodingNodes = ThreeResolved->GetNodes();
        ReplaceResolvedPin(
            InvalidOutputFlowEncodingNodes[1U],
            PinIndex(3U),
            [](GiaResolvedPin& Pin)
            {
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Enum;
            }
        );
        const auto InvalidOutputFlowEncoding =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                ThreeResolved->GetHeader(),
                std::move(InvalidOutputFlowEncodingNodes),
                ThreeResolved->GetDataConnections(),
                ThreeResolved->GetControlConnections()
            );
        MPP_CHECK(!InvalidOutputFlowEncoding.IsValid());

        auto InvalidOmitEncodingNodes = Authentic->GetNodes();
        ReplaceResolvedPin(
            InvalidOmitEncodingNodes[0U],
            PinIndex(0U),
            [](GiaResolvedPin& Pin)
            {
                Pin.EmissionPolicy = GiaPinEmissionPolicy::Omit;
                Pin.IsConnectable = false;
                Pin.InputValue.reset();
                Pin.LiteralEncoding = GiaLiteralEncodingKind::Boolean;
            }
        );
        const auto InvalidOmitEncoding =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                Authentic->GetHeader(),
                std::move(InvalidOmitEncodingNodes),
                Authentic->GetDataConnections(),
                Authentic->GetControlConnections()
            );
        MPP_CHECK(!InvalidOmitEncoding.IsValid());

        auto InvalidLiteralEncodingNodes = ThreeResolved->GetNodes();
        ReplaceResolvedPin(
            InvalidLiteralEncodingNodes[0U],
            PinIndex(0U),
            [](GiaResolvedPin& Pin)
            {
                Pin.LiteralEncoding = static_cast<GiaLiteralEncodingKind>(99);
            }
        );
        const auto InvalidLiteralEncoding =
            GiaResolvedBackendGraphDetail::CreateForTesting(
                ThreeResolved->GetHeader(),
                std::move(InvalidLiteralEncodingNodes),
                ThreeResolved->GetDataConnections(),
                ThreeResolved->GetControlConnections()
            );
        MPP_CHECK(!InvalidLiteralEncoding.IsValid());
    }
}

int main()
{
    TestGiaGraphResolverAcceptsAuthenticFirstFixture();
    TestGiaGraphResolverCopiesResolvedHeaderFacts();
    TestGiaGraphResolverPreservesNodeIdentityAndConcreteZero();
    TestGiaGraphResolverResolvesFinalPinKindAndIndices();
    TestGiaGraphResolverResolvesDescriptorDefaults();
    TestGiaGraphResolverResolvesExplicitLiterals();
    TestGiaGraphResolverResolvesSemanticDataConnection();
    TestGiaGraphResolverResolvesSemanticControlConnection();
    TestGiaGraphResolverRejectsUnsupportedClientExecutionAndSignalPins();
    TestGiaGraphResolverRejectsInvalidOmittedOrUnconnectableEndpoints();
    TestGiaGraphResolverResolvesExplicitSecondaryIndex();
    TestGiaGraphResolverRejectsInvalidInputModel();
    TestGiaGraphResolverOrdersNodesPinsAndConnections();
    TestGiaGraphResolverIsInvariantToNodePermutation();
    TestGiaGraphResolverIsInvariantToInputAndConnectionPermutation();
    TestGiaGraphResolverIsInvariantToMappingPackagePermutation();
    TestGiaGraphResolverPlacesAuthenticNodeAtOrigin();
    TestGiaGraphResolverPlacesDisconnectedNodesDeterministically();
    TestGiaGraphResolverPlacesCyclicControlGraphDeterministically();
    TestGiaGraphResolverRejectsInvalidNodeIndex();
    TestGiaGraphResolverRejectsDanglingOrDuplicateConnections();
    TestGiaGraphResolverReturnsNoPartialModelOnFailure();
    TestGiaGraphResolverOwnsResultAfterInputsExpire();
    TestGiaGraphResolverDoesNotMutateBackendGraph();
    TestGiaGraphResolverOrdersDiagnosticsDeterministically();
    TestGiaResolvedBackendGraphValidityRejectsIncompleteState();
    return EXIT_SUCCESS;
}
