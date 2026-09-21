#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGiaBackendGraph.h"
#include "MiliastraPlusPlusGiaGraphLowerer.h"
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
            "P6.2 fixture descriptor",
            {NodeAvailability::Client},
            std::move(Pins),
            std::move(ControlSchema),
            SourceProvenance("p62.fixture", "record")
        );
    }

    NormalizedNodeDescriptorRecord MakeBooleanInputRecord(
        std::string Identity = "boolean-input"
    )
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
                    true
                )
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeTwoBooleanInputRecord(
        std::string Identity = "two-input"
    )
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "First",
                    TypeDesc::Boolean(),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    LiteralValue(LiteralValue::Data{false})
                ),
                NormalizedPinRecord(
                    "Second",
                    TypeDesc::Boolean(),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    LiteralValue(LiteralValue::Data{true})
                )
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeBooleanOutputRecord(
        std::string Identity = "boolean-output"
    )
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

    NormalizedNodeDescriptorRecord MakeBooleanInputOutputRecord(
        std::string Identity = "boolean-input-output"
    )
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
                    true
                ),
                NormalizedPinRecord(
                    "Output",
                    TypeDesc::Boolean(),
                    PinDirection::Output,
                    PinCategory::Data
                )
            }
        );
    }

    NormalizedNodeDescriptorRecord MakeFlowOutputRecord(
        std::string Identity = "flow-output"
    )
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "FlowOutput",
                    TypeDesc::Flow(),
                    PinDirection::Output,
                    PinCategory::Execution
                )
            },
            EntryControlSchema{PinIndex(0U)}
        );
    }

    NormalizedNodeDescriptorRecord MakeFlowInputRecord(
        std::string Identity = "flow-input"
    )
    {
        return MakeRecord(
            std::move(Identity),
            {
                NormalizedPinRecord(
                    "FlowInput",
                    TypeDesc::Flow(),
                    PinDirection::Input,
                    PinCategory::Execution
                )
            },
            ReturnControlSchema{PinIndex(0U)}
        );
    }

   std::vector<NormalizedNodeDescriptorRecord> MakeLegacyRecords()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.reserve(25U);
        for (std::uint32_t Index = 1U; Index <= 25U; ++Index)
        {
            Records.push_back(MakeRecord(
                "legacy-" + std::to_string(Index),
                {
                    NormalizedPinRecord(
                        "Value",
                        TypeDesc::Boolean(),
                        PinDirection::Input,
                        PinCategory::Data,
                        PinCardinality::Single,
                        true,
                        LiteralValue(LiteralValue::Data{false})
                    )
                }
            ));
        }
        return Records;
    }

   GiaExportConfiguration MakeConfiguration()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(61001),
            "P6.2 Fixture Graph",
            GiaUniqueIdentifier(61002),
            0.5
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    GiaBackendPinMapping MakeInputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendTypeCode,
        GiaLiteralEncodingKind Encoding,
        GiaPinEmissionPolicy EmissionPolicy = GiaPinEmissionPolicy::Emit,
        bool Connectable = true
    )
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::InputParameter,
            GiaPinIndex(static_cast<std::int32_t>(SemanticPin)),
            std::nullopt,
            GiaBackendTypeCode(BackendTypeCode),
            Encoding,
            EmissionPolicy,
            Connectable
        );
    }

    GiaBackendPinMapping MakeOutputMapping(
        std::uint32_t SemanticPin,
        std::int32_t BackendTypeCode = 5
    )
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            GiaPinKind::OutputParameter,
            GiaPinIndex(static_cast<std::int32_t>(SemanticPin)),
            std::nullopt,
            GiaBackendTypeCode(BackendTypeCode),
            BackendTypeCode == 13
                ? GiaLiteralEncodingKind::Enum
                : GiaLiteralEncodingKind::Boolean,
            GiaPinEmissionPolicy::Emit,
            true
        );
    }

    GiaBackendPinMapping MakeFlowMapping(
        std::uint32_t SemanticPin,
        GiaPinKind Kind
    )
    {
        return GiaBackendPinMapping(
            PinIndex(SemanticPin),
            Kind,
            GiaPinIndex(static_cast<std::int32_t>(SemanticPin)),
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
        std::string SourceNamespace = "p62.fixture",
        std::string SourceRevision = "p62.fixture@1"
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
        bool IncludeConcrete = true,
        bool IncludePinOne = true,
        bool WrongTuple = false,
        bool IncludeMapping = true,
        bool WrongLiteralEncoding = false
    )
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
            [=](const DescriptorCatalogueIdentity&) {
                if (!IncludeMapping)
                {
                    return std::vector<GiaBackendNodeMapping>{};
                }

                std::vector<GiaBackendPinMapping> Pins;
                Pins.push_back(MakeInputMapping(
                    0U,
                    WrongTuple ? 13 : 5,
                    WrongTuple || WrongLiteralEncoding
                        ? GiaLiteralEncodingKind::Enum
                        : GiaLiteralEncodingKind::Boolean
                ));
                if (IncludePinOne)
                {
                    Pins.push_back(MakeInputMapping(
                        1U,
                        13,
                        GiaLiteralEncodingKind::Enum
                    ));
                }
                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("200000"),
                        GiaNodeGenericId(200000),
                        IncludeConcrete
                            ? std::optional<GiaNodeConcreteId>(GiaNodeConcreteId(0))
                            : std::nullopt,
                        std::move(Pins),
                        SourceProvenance("p62.graph-encoding", "200000")
                    )
                };
            },
            "genshin.client-bool-filter-descriptor-source",
            CombinedSourceRevision
        );
    }

    GraphIR MakeAuthenticGraph(const GiaExportContext& Context)
    {
        const auto* Entry = Context.GetRegistryContext().GetCatalogue().FindByExternalIdentity(
            ExternalNodeIdentity("200000")
        );
        MPP_CHECK(Entry != nullptr);
        GraphIR Graph;
        Graph.AddNode(NodeInstance{
            NodeInstanceId(1U),
            Entry->GetDescriptorIdentifier(),
            std::nullopt
        });
        return Graph;
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeSimpleContext(
        std::vector<NormalizedNodeDescriptorRecord> Records,
        std::vector<GiaBackendNodeMapping> (*Factory)(const DescriptorCatalogueIdentity&)
    )
    {
        return MakeContext(std::move(Records), Factory);
    }

    std::vector<GiaBackendNodeMapping> MakeMappingsForSimpleData(
        const DescriptorCatalogueIdentity&
    )
    {
        std::vector<GiaBackendNodeMapping> Result;
        Result.emplace_back(
            ExternalNodeIdentity("boolean-output"),
            GiaNodeGenericId(10),
            GiaNodeConcreteId(0),
            std::vector<GiaBackendPinMapping>{MakeOutputMapping(0U)},
            SourceProvenance("p62.synthetic", "boolean-output")
        );
        Result.emplace_back(
            ExternalNodeIdentity("boolean-input"),
            GiaNodeGenericId(11),
            GiaNodeConcreteId(0),
            std::vector<GiaBackendPinMapping>{MakeInputMapping(
                0U,
                5,
                GiaLiteralEncodingKind::Boolean
            )},
            SourceProvenance("p62.synthetic", "boolean-input")
        );
        return Result;
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeOutputReferenceContext()
    {
        return MakeSimpleContext(
            {
                MakeBooleanOutputRecord(),
                MakeBooleanInputRecord()
            },
            MakeMappingsForSimpleData
        );
    }

    std::vector<NormalizedNodeDescriptorRecord> MakeOrderingRecords()
    {
        return {
            MakeBooleanOutputRecord("data-output-a"),
            MakeBooleanInputRecord("data-input-a"),
            MakeBooleanOutputRecord("data-output-b"),
            MakeBooleanInputRecord("data-input-b"),
            MakeFlowOutputRecord("flow-output-a"),
            MakeFlowInputRecord("flow-input-a"),
            MakeFlowOutputRecord("flow-output-b"),
            MakeFlowInputRecord("flow-input-b")
        };
    }

    std::vector<GiaBackendNodeMapping> MakeOrderingMappingsForward(
        const DescriptorCatalogueIdentity&
    )
    {
        return {
            GiaBackendNodeMapping(
                ExternalNodeIdentity("data-output-a"),
                GiaNodeGenericId(100),
                GiaNodeConcreteId(0),
                {MakeOutputMapping(0U)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("data-input-a"),
                GiaNodeGenericId(101),
                GiaNodeConcreteId(0),
                {MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("data-output-b"),
                GiaNodeGenericId(102),
                GiaNodeConcreteId(0),
                {MakeOutputMapping(0U)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("data-input-b"),
                GiaNodeGenericId(103),
                GiaNodeConcreteId(0),
                {MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("flow-output-a"),
                GiaNodeGenericId(104),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(0U, GiaPinKind::OutputFlow)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("flow-input-a"),
                GiaNodeGenericId(105),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(0U, GiaPinKind::InputFlow)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("flow-output-b"),
                GiaNodeGenericId(106),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(0U, GiaPinKind::OutputFlow)}
            ),
            GiaBackendNodeMapping(
                ExternalNodeIdentity("flow-input-b"),
                GiaNodeGenericId(107),
                GiaNodeConcreteId(0),
                {MakeFlowMapping(0U, GiaPinKind::InputFlow)}
            )
        };
    }

    std::vector<GiaBackendNodeMapping> MakeOrderingMappingsReverse(
        const DescriptorCatalogueIdentity& Identity
    )
    {
        auto Result = MakeOrderingMappingsForward(Identity);
        std::reverse(Result.begin(), Result.end());
        return Result;
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeOrderingContext(
        bool ReverseMappings
    )
    {
        return MakeSimpleContext(
            MakeOrderingRecords(),
            ReverseMappings
                ? &MakeOrderingMappingsReverse
                : &MakeOrderingMappingsForward
        );
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeTwoInputContext()
    {
        return MakeSimpleContext(
            {MakeTwoBooleanInputRecord()},
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{GiaBackendNodeMapping(
                    ExternalNodeIdentity("two-input"),
                    GiaNodeGenericId(120),
                    GiaNodeConcreteId(0),
                    {
                        MakeInputMapping(1U, 5, GiaLiteralEncodingKind::Boolean),
                        MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean)
                    }
                )};
            }
        );
    }

    NodeDescriptorId GetDescriptorId(
        const GiaExportContext& Context,
        std::string_view Identity
    )
    {
        const auto* Entry = Context.GetRegistryContext().GetCatalogue().
            FindByExternalIdentity(ExternalNodeIdentity(std::string(Identity)));
        MPP_CHECK(Entry != nullptr);
        return Entry->GetDescriptorIdentifier();
    }

    GraphIR MakeOrderingGraph(
        const GiaExportContext& Context,
        bool ReverseNodes,
        bool ReverseBindings,
        bool ReverseControlEdges
    )
    {
        const std::vector<NodeInstance> Nodes{
            {NodeInstanceId(1U), GetDescriptorId(Context, "data-output-a"), std::nullopt},
            {NodeInstanceId(2U), GetDescriptorId(Context, "data-input-a"), std::nullopt},
            {NodeInstanceId(3U), GetDescriptorId(Context, "data-output-b"), std::nullopt},
            {NodeInstanceId(4U), GetDescriptorId(Context, "data-input-b"), std::nullopt},
            {NodeInstanceId(5U), GetDescriptorId(Context, "flow-output-a"), std::nullopt},
            {NodeInstanceId(6U), GetDescriptorId(Context, "flow-input-a"), std::nullopt},
            {NodeInstanceId(7U), GetDescriptorId(Context, "flow-output-b"), std::nullopt},
            {NodeInstanceId(8U), GetDescriptorId(Context, "flow-input-b"), std::nullopt}
        };

        GraphIR Graph;
        if (ReverseNodes)
        {
            for (auto Node = Nodes.rbegin(); Node != Nodes.rend(); ++Node)
            {
                Graph.AddNode(*Node);
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
            Graph.BindInput(
                NodeInstanceId(4U),
                PinIndex(0U),
                OutputReference{NodeInstanceId(3U), PinIndex(0U)}
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
                NodeInstanceId(4U),
                PinIndex(0U),
                OutputReference{NodeInstanceId(3U), PinIndex(0U)}
            );
        }

        if (ReverseControlEdges)
        {
            Graph.AddControlEdge({
                NodeInstanceId(7U),
                PinIndex(0U),
                NodeInstanceId(8U),
                PinIndex(0U)
            });
            Graph.AddControlEdge({
                NodeInstanceId(5U),
                PinIndex(0U),
                NodeInstanceId(6U),
                PinIndex(0U)
            });
        }
        else
        {
            Graph.AddControlEdge({
                NodeInstanceId(5U),
                PinIndex(0U),
                NodeInstanceId(6U),
                PinIndex(0U)
            });
            Graph.AddControlEdge({
                NodeInstanceId(7U),
                PinIndex(0U),
                NodeInstanceId(8U),
                PinIndex(0U)
            });
        }

        return Graph;
    }

    GiaBackendNode ReplaceNodePinMapping(
        const GiaBackendNode& Original,
        PinIndex SemanticPin,
        GiaBackendTypeCode BackendTypeCode,
        GiaLiteralEncodingKind LiteralEncoding
    )
    {
        std::vector<GiaBackendPinMapping> PinMappings =
            Original.Mapping.GetPinMappings();
        for (GiaBackendPinMapping& PinMapping : PinMappings)
        {
            if (PinMapping.GetSemanticPinIndex() == SemanticPin)
            {
                PinMapping = GiaBackendPinMapping(
                    PinMapping.GetSemanticPinIndex(),
                    PinMapping.GetPinKind(),
                    PinMapping.GetBackendIndex(),
                    PinMapping.GetSecondaryIndex(),
                    BackendTypeCode,
                    LiteralEncoding,
                    PinMapping.GetEmissionPolicy(),
                    PinMapping.IsConnectable()
                );
            }
        }

        return GiaBackendNode{
            Original.Trace,
            GiaBackendNodeMapping(
                Original.Mapping.GetExternalIdentity(),
                Original.Mapping.GetGenericNodeIdentifier(),
                Original.Mapping.GetConcreteNodeIdentifier(),
                std::move(PinMappings),
                Original.Mapping.GetSourceProvenance()
            ),
            Original.Inputs
        };
    }

    GiaBackendGraph ReplaceModelNode(
        const GiaBackendGraph& Original,
        NodeInstanceId Node,
        GiaBackendNode Replacement
    )
    {
        std::vector<GiaBackendNode> Nodes = Original.GetNodes();
        for (GiaBackendNode& Candidate : Nodes)
        {
            if (Candidate.Trace.GraphNode == Node)
            {
                Candidate = std::move(Replacement);
            }
        }
        return GiaBackendGraphDetail::CreateForTesting(
            Original.GetHeader(),
            std::move(Nodes),
            Original.GetDataConnections(),
            Original.GetControlConnections()
        );
    }

    std::pair<NodeDescriptorId, NodeDescriptorId> GetDescriptorPair(
        const GiaExportContext& Context,
        std::string_view First,
        std::string_view Second
    )
    {
        const auto* FirstEntry = Context.GetRegistryContext().GetCatalogue().FindByExternalIdentity(
            ExternalNodeIdentity(std::string(First))
        );
        const auto* SecondEntry = Context.GetRegistryContext().GetCatalogue().FindByExternalIdentity(
            ExternalNodeIdentity(std::string(Second))
        );
        MPP_CHECK(FirstEntry != nullptr && SecondEntry != nullptr);
        return {
            FirstEntry->GetDescriptorIdentifier(),
            SecondEntry->GetDescriptorIdentifier()
        };
    }

    void TestGiaGraphLowererAcceptsAuthenticFirstFixture()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(
            MakeAuthenticGraph(*Context),
            *Context
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Context->GetRegistryContext().GetCatalogue().GetEntryCount() == 26U);
        MPP_CHECK(Context->GetRegistryContext().GetCatalogueIdentity().GetSourceNamespace() ==
            "genshin.client-bool-filter-descriptor-source");
        MPP_CHECK(Context->GetRegistryContext().GetCatalogueIdentity().GetSourceRevision() ==
            CombinedSourceRevision);
        MPP_CHECK(Result->GetNodes().size() == 1U);
        MPP_CHECK(Result->GetDataConnections().empty());
        MPP_CHECK(Result->GetControlConnections().empty());
    }

    void TestGiaGraphLowererCopiesGraphHeaderAndTargetEncoding()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Result.has_value());
        const GiaBackendGraphHeader& Header = Result->GetHeader();
        MPP_CHECK(Header.TargetProfile == GiaExportTargetProfile::ClientBooleanFilter);
        MPP_CHECK(Header.Mode == GiaExportMode::Beyond);
        MPP_CHECK(Header.GraphIdentifier == GiaGraphIdentifier(61001));
        MPP_CHECK(Header.GraphName == "P6.2 Fixture Graph");
        MPP_CHECK(Header.UniqueIdentifier == GiaUniqueIdentifier(61002));
        MPP_CHECK(Header.EvaluationInterval == 0.5);
        MPP_CHECK(Header.GraphType == 20001);
        MPP_CHECK(Header.GraphWhich == 10);
        MPP_CHECK(Header.MappingSchemaVersion.GetValue() == 1U);
    }

    void TestGiaGraphLowererResolvesCatalogueAndOpaqueMapping()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Result.has_value());
        const GiaBackendNode& Node = Result->GetNodes()[0U];
        MPP_CHECK(Node.Trace.ExternalIdentity.GetKey() == "200000");
        MPP_CHECK(Node.Trace.Descriptor.IsValid());
        MPP_CHECK(Node.Mapping.GetExternalIdentity().GetKey() == "200000");
        MPP_CHECK(Node.Mapping.GetGenericNodeIdentifier().GetValue() == 200000);
        MPP_CHECK(Node.Mapping.GetPinMappings().size() == 2U);
        MPP_CHECK(Node.Mapping.GetPinMappings()[0U].GetSemanticPinIndex() == PinIndex(0U));
        MPP_CHECK(Node.Mapping.GetPinMappings()[0U].GetBackendTypeCode().GetValue() == 5);
        MPP_CHECK(Node.Mapping.GetPinMappings()[1U].GetSemanticPinIndex() == PinIndex(1U));
        MPP_CHECK(Node.Mapping.GetPinMappings()[1U].GetBackendTypeCode().GetValue() == 13);
    }

    void TestGiaGraphLowererPreservesConcreteZero()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodes()[0U].Mapping.GetConcreteNodeIdentifier().has_value());
        MPP_CHECK(Result->GetNodes()[0U].Mapping.GetConcreteNodeIdentifier()->GetValue() == 0);
    }

    void TestGiaGraphLowererMaterializesDescriptorDefaults()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(Result.has_value());
        const auto& Inputs = Result->GetNodes()[0U].Inputs;
        MPP_CHECK(Inputs.size() == 2U);
        MPP_CHECK(Inputs[0U].SourceKind == GiaBackendInputValueSourceKind::DescriptorDefault);
        MPP_CHECK(Inputs[0U].Literal->Is<bool>());
        MPP_CHECK(!*Inputs[0U].Literal->TryGet<bool>());
        MPP_CHECK(Inputs[1U].SourceKind == GiaBackendInputValueSourceKind::DescriptorDefault);
        MPP_CHECK(Inputs[1U].Literal->Is<EnumLiteralValue>());
        MPP_CHECK(Inputs[1U].SemanticType == TypeDesc::Enum(
            EnumTypeIdentity("filter_return_type")
        ));
        MPP_CHECK(Inputs[1U].Literal->TryGet<EnumLiteralValue>()->GetValue() == 1000010);
    }

    void TestGiaGraphLowererAcceptsExplicitBooleanAndEnumLiterals()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        const EnumTypeIdentity Identity("filter_return_type");
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            LiteralValue(LiteralValue::Data{true})
        );
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(1U),
            LiteralValue(LiteralValue::Data{
                EnumLiteralValue(Identity, 10000011)
            })
        );
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodes()[0U].Inputs[0U].SourceKind ==
            GiaBackendInputValueSourceKind::ExplicitLiteral);
        MPP_CHECK(*Result->GetNodes()[0U].Inputs[0U].Literal->TryGet<bool>());
        MPP_CHECK(Result->GetNodes()[0U].Inputs[1U].SourceKind ==
            GiaBackendInputValueSourceKind::ExplicitLiteral);
        MPP_CHECK(Result->GetNodes()[0U].Inputs[1U].Literal->TryGet<EnumLiteralValue>()->GetValue() ==
            10000011);
    }

    void TestGiaGraphLowererRejectsMissingDefault()
    {
        const auto Context = MakeSimpleContext(
            {MakeBooleanInputRecord("no-default")},
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{GiaBackendNodeMapping(
                    ExternalNodeIdentity("no-default"),
                    GiaNodeGenericId(20),
                    GiaNodeConcreteId(0),
                    {MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean)}
                )};
            }
        );
        MPP_CHECK(Context.has_value());
        const auto Id = Context->GetRegistryContext().GetCatalogue().GetEntries()[0U].GetDescriptorIdentifier();
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), Id, std::nullopt});
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaBackendInputResolution));
    }

    void TestGiaGraphLowererRejectsMissingNodeMapping()
    {
        const auto Context = MakeAuthenticContext(true, true, false, false);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingGiaBackendNodeMapping));

        const auto MissingConcreteContext = MakeAuthenticContext(false);
        MPP_CHECK(MissingConcreteContext.has_value());
        const auto MissingConcrete = GiaGraphLowerer::Lower(
            MakeAuthenticGraph(*MissingConcreteContext),
            *MissingConcreteContext
        );
        MPP_CHECK(!MissingConcrete.has_value());
        MPP_CHECK(HasCode(
            MissingConcrete.error(),
            DiagnosticCode::UnresolvedGiaBackendConcreteIdentity
        ));
    }

    void TestGiaGraphLowererRejectsMissingPinMapping()
    {
        const auto Context = MakeAuthenticContext(true, false);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingGiaBackendPinMapping));
    }

    void TestGiaGraphLowererRejectsMappingTypeAndLiteralMismatches()
    {
        const auto Context = MakeAuthenticContext(true, true, true);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaBackendType));

        const auto LiteralContext = MakeAuthenticContext(true, true, false, true, true);
        MPP_CHECK(LiteralContext.has_value());
        const auto LiteralResult = GiaGraphLowerer::Lower(
            MakeAuthenticGraph(*LiteralContext),
            *LiteralContext
        );
        MPP_CHECK(!LiteralResult.has_value());
        MPP_CHECK(HasCode(LiteralResult.error(), DiagnosticCode::UnsupportedGiaBackendValue));
    }

    void TestGiaGraphLowererRejectsGraphVariablesAndReferences()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        Graph.AddVariable({
            GraphVariableId(1U),
            "ReturnType",
            TypeDesc::Boolean(),
            LiteralValue(LiteralValue::Data{false})
        });
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            GraphVariableReference{GraphVariableId(1U)}
        );
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaTargetGraphFeature));
    }

    void TestGiaGraphLowererRejectsUnsupportedInputBinding()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        Graph.AddVariable({
            GraphVariableId(2U),
            "ReturnType",
            TypeDesc::Enum(EnumTypeIdentity("filter_return_type")),
            std::nullopt
        });
        Graph.BindInput(
            NodeInstanceId(1U),
            PinIndex(1U),
            GraphVariableReference{GraphVariableId(2U)}
        );
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaTargetGraphFeature));
    }

    void TestGiaGraphLowererLowersSemanticOutputReferences()
    {
        const auto Context = MakeOutputReferenceContext();
        MPP_CHECK(Context.has_value());
        const auto Ids = GetDescriptorPair(*Context, "boolean-output", "boolean-input");
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(2U), Ids.second, std::nullopt});
        Graph.AddNode({NodeInstanceId(1U), Ids.first, std::nullopt});
        Graph.BindInput(
            NodeInstanceId(2U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)}
        );
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetDataConnections().size() == 1U);
        MPP_CHECK(Result->GetDataConnections()[0U].SourceNode == NodeInstanceId(1U));
        MPP_CHECK(Result->GetDataConnections()[0U].DestinationNode == NodeInstanceId(2U));
        MPP_CHECK(Result->GetNodes()[1U].Inputs[0U].SourceKind ==
            GiaBackendInputValueSourceKind::DataConnection);
        MPP_CHECK(!Result->GetNodes()[1U].Inputs[0U].Literal.has_value());
    }

    void TestGiaGraphLowererLowersSeparateDataAndControlConnections()
    {
        const auto Context = MakeSimpleContext(
            {
                MakeBooleanOutputRecord("data-source"),
                MakeBooleanInputRecord("data-destination"),
                MakeFlowOutputRecord("flow-source"),
                MakeFlowInputRecord("flow-destination")
            },
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("data-source"), GiaNodeGenericId(30), GiaNodeConcreteId(0),
                        {MakeOutputMapping(0U)}),
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("data-destination"), GiaNodeGenericId(31), GiaNodeConcreteId(0),
                        {MakeInputMapping(0U, 5, GiaLiteralEncodingKind::Boolean)}),
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("flow-source"), GiaNodeGenericId(32), GiaNodeConcreteId(0),
                        {MakeFlowMapping(0U, GiaPinKind::OutputFlow)}),
                    GiaBackendNodeMapping(
                        ExternalNodeIdentity("flow-destination"), GiaNodeGenericId(33), GiaNodeConcreteId(0),
                        {MakeFlowMapping(0U, GiaPinKind::InputFlow)})
                };
            }
        );
        MPP_CHECK(Context.has_value());
        const auto DataIds = GetDescriptorPair(*Context, "data-source", "data-destination");
        const auto FlowIds = GetDescriptorPair(*Context, "flow-source", "flow-destination");
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), DataIds.first, std::nullopt});
        Graph.AddNode({NodeInstanceId(2U), DataIds.second, std::nullopt});
        Graph.AddNode({NodeInstanceId(3U), FlowIds.first, std::nullopt});
        Graph.AddNode({NodeInstanceId(4U), FlowIds.second, std::nullopt});
        Graph.BindInput(NodeInstanceId(2U), PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)});
        Graph.AddControlEdge({NodeInstanceId(3U), PinIndex(0U), NodeInstanceId(4U), PinIndex(0U)});
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetDataConnections().size() == 1U);
        MPP_CHECK(Result->GetControlConnections().size() == 1U);
    }

    void TestGiaGraphLowererRejectsUnmappedControlTopology()
    {
        const auto Context = MakeSimpleContext(
            {MakeFlowOutputRecord("flow-source"), MakeFlowInputRecord("flow-destination")},
            [](const DescriptorCatalogueIdentity&) {
                return std::vector<GiaBackendNodeMapping>{GiaBackendNodeMapping(
                    ExternalNodeIdentity("flow-source"), GiaNodeGenericId(40), GiaNodeConcreteId(0),
                    {})};
            }
        );
        MPP_CHECK(Context.has_value());
        const auto Ids = GetDescriptorPair(*Context, "flow-source", "flow-destination");
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), Ids.first, std::nullopt});
        Graph.AddNode({NodeInstanceId(2U), Ids.second, std::nullopt});
        Graph.AddControlEdge({NodeInstanceId(1U), PinIndex(0U), NodeInstanceId(2U), PinIndex(0U)});
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingGiaBackendPinMapping));
    }

    void TestGiaGraphLowererOrdersNodesPinsAndConnections()
    {
        const auto Context = MakeOrderingContext(true);
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeOrderingGraph(*Context, true, true, true);
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodes().size() == 8U);
        for (std::size_t Index = 0U; Index < Result->GetNodes().size(); ++Index)
        {
            MPP_CHECK(Result->GetNodes()[Index].Trace.GraphNode ==
                NodeInstanceId(static_cast<std::uint32_t>(Index + 1U)));
        }
        MPP_CHECK(Result->GetDataConnections().size() == 2U);
        MPP_CHECK(Result->GetDataConnections()[0U].SourceNode == NodeInstanceId(1U));
        MPP_CHECK(Result->GetDataConnections()[0U].DestinationNode == NodeInstanceId(2U));
        MPP_CHECK(Result->GetDataConnections()[1U].SourceNode == NodeInstanceId(3U));
        MPP_CHECK(Result->GetDataConnections()[1U].DestinationNode == NodeInstanceId(4U));
        MPP_CHECK(Result->GetControlConnections().size() == 2U);
        MPP_CHECK(Result->GetControlConnections()[0U].SourceNode == NodeInstanceId(5U));
        MPP_CHECK(Result->GetControlConnections()[0U].DestinationNode == NodeInstanceId(6U));
        MPP_CHECK(Result->GetControlConnections()[1U].SourceNode == NodeInstanceId(7U));
        MPP_CHECK(Result->GetControlConnections()[1U].DestinationNode == NodeInstanceId(8U));

        const auto TwoInputContext = MakeTwoInputContext();
        MPP_CHECK(TwoInputContext.has_value());
        GraphIR TwoInputGraph;
        TwoInputGraph.AddNode({
            NodeInstanceId(1U),
            GetDescriptorId(*TwoInputContext, "two-input"),
            std::nullopt
        });
        TwoInputGraph.BindInput(
            NodeInstanceId(1U),
            PinIndex(1U),
            LiteralValue(LiteralValue::Data{true})
        );
        TwoInputGraph.BindInput(
            NodeInstanceId(1U),
            PinIndex(0U),
            LiteralValue(LiteralValue::Data{false})
        );
        const auto TwoInputResult = GiaGraphLowerer::Lower(
            TwoInputGraph,
            *TwoInputContext
        );
        MPP_CHECK(TwoInputResult.has_value());
        MPP_CHECK(TwoInputResult->GetNodes()[0U].Inputs.size() == 2U);
        MPP_CHECK(TwoInputResult->GetNodes()[0U].Inputs[0U].SemanticPin == PinIndex(0U));
        MPP_CHECK(TwoInputResult->GetNodes()[0U].Inputs[1U].SemanticPin == PinIndex(1U));
        MPP_CHECK(TwoInputResult->GetNodes()[0U].Inputs[0U].Literal->Is<bool>());
        MPP_CHECK(TwoInputResult->GetNodes()[0U].Inputs[1U].Literal->Is<bool>());
        MPP_CHECK(*TwoInputResult->GetNodes()[0U].Inputs[0U].Literal->TryGet<bool>() == false);
        MPP_CHECK(*TwoInputResult->GetNodes()[0U].Inputs[1U].Literal->TryGet<bool>() == true);
    }

    void TestGiaGraphLowererIsInvariantToInputPermutation()
    {
        const auto Context = MakeOutputReferenceContext();
        MPP_CHECK(Context.has_value());
        const auto Ids = GetDescriptorPair(*Context, "boolean-output", "boolean-input");
        GraphIR First;
        First.AddNode({NodeInstanceId(1U), Ids.first, std::nullopt});
        First.AddNode({NodeInstanceId(2U), Ids.second, std::nullopt});
        First.BindInput(NodeInstanceId(2U), PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)});
        GraphIR Second;
        Second.AddNode({NodeInstanceId(2U), Ids.second, std::nullopt});
        Second.AddNode({NodeInstanceId(1U), Ids.first, std::nullopt});
        Second.BindInput(NodeInstanceId(2U), PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)});
        const auto FirstResult = GiaGraphLowerer::Lower(First, *Context);
        const auto SecondResult = GiaGraphLowerer::Lower(Second, *Context);
        MPP_CHECK(FirstResult.has_value() && SecondResult.has_value());
        MPP_CHECK(*FirstResult == *SecondResult);
    }

    void TestGiaGraphLowererIsInvariantToInputBindingPermutation()
    {
        const auto Context = MakeOrderingContext(false);
        MPP_CHECK(Context.has_value());
        const auto First = GiaGraphLowerer::Lower(
            MakeOrderingGraph(*Context, false, false, false),
            *Context
        );
        const auto Second = GiaGraphLowerer::Lower(
            MakeOrderingGraph(*Context, false, true, false),
            *Context
        );
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
    }

    void TestGiaGraphLowererIsInvariantToControlEdgePermutation()
    {
        const auto Context = MakeOrderingContext(false);
        MPP_CHECK(Context.has_value());
        const auto First = GiaGraphLowerer::Lower(
            MakeOrderingGraph(*Context, false, false, false),
            *Context
        );
        const auto Second = GiaGraphLowerer::Lower(
            MakeOrderingGraph(*Context, false, false, true),
            *Context
        );
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
    }

    void TestGiaGraphLowererIsInvariantToMappingPackagePermutation()
    {
        const auto ForwardContext = MakeOrderingContext(false);
        const auto ReverseContext = MakeOrderingContext(true);
        MPP_CHECK(ForwardContext.has_value() && ReverseContext.has_value());
        const GraphIR Graph = MakeOrderingGraph(
            *ForwardContext,
            false,
            false,
            false
        );
        const auto Forward = GiaGraphLowerer::Lower(Graph, *ForwardContext);
        const auto Reverse = GiaGraphLowerer::Lower(
            Graph,
            *ReverseContext
        );
        MPP_CHECK(Forward.has_value() && Reverse.has_value());
        MPP_CHECK(*Forward == *Reverse);
    }

    void TestGiaBackendGraphRejectsDuplicateDataDestination()
    {
        const auto Context = MakeOrderingContext(false);
        MPP_CHECK(Context.has_value());
        const auto Lowered = GiaGraphLowerer::Lower(
            MakeOrderingGraph(*Context, false, false, false),
            *Context
        );
        MPP_CHECK(Lowered.has_value());
        MPP_CHECK(Lowered->IsValid());

        const std::vector<GiaBackendDataConnection> Connections{
            Lowered->GetDataConnections()[0U],
            GiaBackendDataConnection{
                NodeInstanceId(3U),
                PinIndex(0U),
                NodeInstanceId(2U),
                PinIndex(0U),
                TypeDesc::Boolean()
            },
            Lowered->GetDataConnections()[1U]
        };
        MPP_CHECK(Connections[0U].IsValid());
        MPP_CHECK(Connections[1U].IsValid());
        MPP_CHECK(Connections[2U].IsValid());
        MPP_CHECK(Connections[0U] < Connections[1U]);
        MPP_CHECK(Connections[1U] < Connections[2U]);

        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            Lowered->GetHeader(),
            Lowered->GetNodes(),
            Connections,
            Lowered->GetControlConnections()
        );
        MPP_CHECK(!Invalid.IsValid());
    }

    void TestGiaBackendGraphRejectsInvalidInputMappingTuples()
    {
        const auto AuthenticContext = MakeAuthenticContext();
        MPP_CHECK(AuthenticContext.has_value());
        const auto Authentic = GiaGraphLowerer::Lower(
            MakeAuthenticGraph(*AuthenticContext),
            *AuthenticContext
        );
        MPP_CHECK(Authentic.has_value());
        MPP_CHECK(Authentic->IsValid());

        const GiaBackendNode BadBooleanNode = ReplaceNodePinMapping(
            Authentic->GetNodes()[0U],
            PinIndex(0U),
            GiaBackendTypeCode(13),
            GiaLiteralEncodingKind::Enum
        );
        MPP_CHECK(!ReplaceModelNode(
            *Authentic,
            NodeInstanceId(1U),
            BadBooleanNode
        ).IsValid());

        const GiaBackendNode BadBooleanEncodingNode = ReplaceNodePinMapping(
            Authentic->GetNodes()[0U],
            PinIndex(0U),
            GiaBackendTypeCode(5),
            GiaLiteralEncodingKind::Enum
        );
        MPP_CHECK(!ReplaceModelNode(
            *Authentic,
            NodeInstanceId(1U),
            BadBooleanEncodingNode
        ).IsValid());

        const GiaBackendNode BadEnumNode = ReplaceNodePinMapping(
            Authentic->GetNodes()[0U],
            PinIndex(1U),
            GiaBackendTypeCode(5),
            GiaLiteralEncodingKind::Boolean
        );
        MPP_CHECK(!ReplaceModelNode(
            *Authentic,
            NodeInstanceId(1U),
            BadEnumNode
        ).IsValid());

        const GiaBackendNode BadEnumEncodingNode = ReplaceNodePinMapping(
            Authentic->GetNodes()[0U],
            PinIndex(1U),
            GiaBackendTypeCode(13),
            GiaLiteralEncodingKind::Boolean
        );
        MPP_CHECK(!ReplaceModelNode(
            *Authentic,
            NodeInstanceId(1U),
            BadEnumEncodingNode
        ).IsValid());

        GiaBackendNode WrongFamilyNode = Authentic->GetNodes()[0U];
        WrongFamilyNode.Inputs[1U].SemanticType =
            TypeDesc::Enum(EnumTypeIdentity("other_family"));
        WrongFamilyNode.Inputs[1U].Literal = LiteralValue(
            LiteralValue::Data{
                EnumLiteralValue(EnumTypeIdentity("other_family"), 1000010)
            }
        );
        MPP_CHECK(!ReplaceModelNode(
            *Authentic,
            NodeInstanceId(1U),
            std::move(WrongFamilyNode)
        ).IsValid());

        const auto DataContext = MakeOutputReferenceContext();
        MPP_CHECK(DataContext.has_value());
        const auto DataIds = GetDescriptorPair(
            *DataContext,
            "boolean-output",
            "boolean-input"
        );
        GraphIR DataGraph;
        DataGraph.AddNode({NodeInstanceId(1U), DataIds.first, std::nullopt});
        DataGraph.AddNode({NodeInstanceId(2U), DataIds.second, std::nullopt});
        DataGraph.BindInput(
            NodeInstanceId(2U),
            PinIndex(0U),
            OutputReference{NodeInstanceId(1U), PinIndex(0U)}
        );
        const auto DataResult = GiaGraphLowerer::Lower(DataGraph, *DataContext);
        MPP_CHECK(DataResult.has_value());
        MPP_CHECK(DataResult->IsValid());
        const GiaBackendNode BadDataDestination = ReplaceNodePinMapping(
            DataResult->GetNodes()[1U],
            PinIndex(0U),
            GiaBackendTypeCode(13),
            GiaLiteralEncodingKind::Enum
        );
        MPP_CHECK(!ReplaceModelNode(
            *DataResult,
            NodeInstanceId(2U),
            BadDataDestination
        ).IsValid());
    }

    void TestGiaGraphLowererDoesNotMutateGraphIR()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        const Json Before = GraphIRJson::Serialize(Graph);
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(GraphIRJson::Serialize(Graph) == Before);
    }

    void TestGiaGraphLowererOwnsResultAfterInputsExpire()
    {
        std::optional<GiaBackendGraph> Result;
        {
            const auto Context = MakeAuthenticContext();
            MPP_CHECK(Context.has_value());
            GraphIR Graph = MakeAuthenticGraph(*Context);
            const auto Lowered = GiaGraphLowerer::Lower(Graph, *Context);
            MPP_CHECK(Lowered.has_value());
            Result = *Lowered;
        }
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetNodes()[0U].Trace.ExternalIdentity.GetKey() == "200000");
        MPP_CHECK(Result->GetNodes()[0U].Inputs[1U].Literal->TryGet<EnumLiteralValue>()->GetValue() ==
            1000010);
    }

    void TestGiaGraphLowererReturnsNoPartialModelOnFailure()
    {
        const auto Context = MakeAuthenticContext(true, false);
        MPP_CHECK(Context.has_value());
        const auto Result = GiaGraphLowerer::Lower(MakeAuthenticGraph(*Context), *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(!Result.error().empty());
    }

    void TestGiaGraphLowererRejectsUnresolvedDescriptor()
    {
        const auto Context = MakeAuthenticContext();
        MPP_CHECK(Context.has_value());
        GraphIR Graph;
        Graph.AddNode({NodeInstanceId(1U), NodeDescriptorId(999999U), std::nullopt});
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::MissingDescriptor));
        MPP_CHECK(!HasCode(Result.error(), DiagnosticCode::MissingGiaBackendNodeMapping));
    }

    void TestGiaGraphLowererUsesTrustedValidationBeforeMappingLookup()
    {
        const auto Context = MakeAuthenticContext(true, true, false, false);
        MPP_CHECK(Context.has_value());
        GraphIR Graph = MakeAuthenticGraph(*Context);
        Graph.BindInput(NodeInstanceId(999U), PinIndex(0U),
            LiteralValue(LiteralValue::Data{false}));
        const auto Result = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidInputBinding));
        MPP_CHECK(!HasCode(Result.error(), DiagnosticCode::MissingGiaBackendNodeMapping));
    }

    void TestGiaGraphLowererOrdersDiagnosticsDeterministically()
    {
        const auto Context = MakeAuthenticContext(
            true,
            true,
            false,
            true,
            true
        );
        MPP_CHECK(Context.has_value());
        const GraphIR Graph = MakeAuthenticGraph(*Context);
        const auto First = GiaGraphLowerer::Lower(Graph, *Context);
        const auto Second = GiaGraphLowerer::Lower(Graph, *Context);
        MPP_CHECK(!First.has_value() && !Second.has_value());
        MPP_CHECK(First.error().size() == 2U);
        MPP_CHECK(First.error()[0U].Code == DiagnosticCode::UnsupportedGiaBackendValue);
        MPP_CHECK(First.error()[1U].Code == DiagnosticCode::UnsupportedGiaBackendValue);
        MPP_CHECK(First.error()[0U].Message.find(
            "the input does not have the required target literal encoding"
        ) != std::string::npos);
        MPP_CHECK(First.error()[1U].Message.find(
            "the descriptor default is unsupported by the backend tuple"
        ) != std::string::npos);
        MPP_CHECK(First.error().size() == Second.error().size());
        for (std::size_t Index = 0U; Index < First.error().size(); ++Index)
        {
            MPP_CHECK(First.error()[Index].Code == Second.error()[Index].Code);
            MPP_CHECK(First.error()[Index].Message == Second.error()[Index].Message);
            MPP_CHECK(First.error()[Index].ExternalIdentityKey ==
                Second.error()[Index].ExternalIdentityKey);
        }
    }

    void TestGiaBackendGraphValidityRejectsIncompleteState()
    {
        const GiaBackendGraph Invalid = GiaBackendGraphDetail::CreateForTesting(
            GiaBackendGraphHeader{},
            {},
            {},
            {}
        );
        MPP_CHECK(!Invalid.IsValid());
    }
}

int main()
{
    TestGiaGraphLowererAcceptsAuthenticFirstFixture();
    TestGiaGraphLowererCopiesGraphHeaderAndTargetEncoding();
    TestGiaGraphLowererResolvesCatalogueAndOpaqueMapping();
    TestGiaGraphLowererPreservesConcreteZero();
    TestGiaGraphLowererMaterializesDescriptorDefaults();
    TestGiaGraphLowererAcceptsExplicitBooleanAndEnumLiterals();
    TestGiaGraphLowererRejectsMissingDefault();
    TestGiaGraphLowererRejectsMissingNodeMapping();
    TestGiaGraphLowererRejectsMissingPinMapping();
    TestGiaGraphLowererRejectsMappingTypeAndLiteralMismatches();
    TestGiaGraphLowererRejectsGraphVariablesAndReferences();
    TestGiaGraphLowererRejectsUnsupportedInputBinding();
    TestGiaGraphLowererLowersSemanticOutputReferences();
    TestGiaGraphLowererLowersSeparateDataAndControlConnections();
    TestGiaGraphLowererRejectsUnmappedControlTopology();
    TestGiaGraphLowererOrdersNodesPinsAndConnections();
    TestGiaGraphLowererIsInvariantToInputPermutation();
    TestGiaGraphLowererIsInvariantToInputBindingPermutation();
    TestGiaGraphLowererIsInvariantToControlEdgePermutation();
    TestGiaGraphLowererIsInvariantToMappingPackagePermutation();
    TestGiaBackendGraphRejectsDuplicateDataDestination();
    TestGiaBackendGraphRejectsInvalidInputMappingTuples();
    TestGiaGraphLowererDoesNotMutateGraphIR();
    TestGiaGraphLowererOwnsResultAfterInputsExpire();
    TestGiaGraphLowererReturnsNoPartialModelOnFailure();
    TestGiaGraphLowererRejectsUnresolvedDescriptor();
    TestGiaGraphLowererUsesTrustedValidationBeforeMappingLookup();
    TestGiaGraphLowererOrdersDiagnosticsDeterministically();
    TestGiaBackendGraphValidityRejectsIncompleteState();
    return EXIT_SUCCESS;
}
