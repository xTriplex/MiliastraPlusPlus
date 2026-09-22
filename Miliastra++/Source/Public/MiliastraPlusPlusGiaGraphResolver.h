#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaResolvedBackendGraph.h"

namespace MiliastraPlusPlus
{
    namespace GiaGraphResolverDetail
    {
        enum class ValidationStage
        {
            InputModelValidation,
            NodeIdentityResolution,
            PinResolution,
            TargetSemanticResolution,
            ConnectionResolution,
            LayoutResolution,
            ModelValidation
        };

        struct PendingDiagnostic
        {
            ValidationStage Stage;
            DiagnosticCode Code;
            std::optional<NodeInstanceId> Node;
            std::optional<PinIndex> Pin;
            std::string ExternalIdentityKey;
            std::string Message;
            std::optional<SourceProvenance> PrimaryProvenance;
            std::optional<SourceProvenance> RelatedProvenance;
        };

        [[nodiscard]] inline bool IsEarlier(const PendingDiagnostic& Left,
            const PendingDiagnostic& Right)
        {
            return std::tuple{
                static_cast<int>(Left.Stage),
                static_cast<int>(Left.Code),
                Left.Node,
                Left.Pin,
                Left.ExternalIdentityKey,
                Left.Message,
                Left.PrimaryProvenance,
                Left.RelatedProvenance
            } < std::tuple{
                static_cast<int>(Right.Stage),
                static_cast<int>(Right.Code),
                Right.Node,
                Right.Pin,
                Right.ExternalIdentityKey,
                Right.Message,
                Right.PrimaryProvenance,
                Right.RelatedProvenance
            };
        }

        [[nodiscard]] inline DiagnosticCollection Materialize(std::vector<PendingDiagnostic> Diagnostics)
        {
            std::stable_sort(Diagnostics.begin(), Diagnostics.end(), IsEarlier);

            DiagnosticCollection Result;
            Result.reserve(Diagnostics.size());
            for (PendingDiagnostic& Pending : Diagnostics)
            {
                Result.push_back(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = Pending.Code,
                    .Message = std::move(Pending.Message),
                    .SourceNodeIdentifier = std::nullopt,
                    .DestinationNodeIdentifier = std::nullopt,
                    .SourcePinReference = std::nullopt,
                    .DestinationPinReference = std::nullopt,
                    .PrimarySourceProvenance = std::move(
                        Pending.PrimaryProvenance
                    ),
                    .RelatedSourceProvenance = std::move(
                        Pending.RelatedProvenance
                    ),
                    .ExternalIdentityKey = std::move(
                        Pending.ExternalIdentityKey.empty()
                            ? std::optional<std::string>{}
                            : std::optional<std::string>(
                                std::move(Pending.ExternalIdentityKey)
                            )
                    )
                });
            }
            return Result;
        }

        inline void Add(std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            DiagnosticCode Code,
            std::string Message,
            std::optional<NodeInstanceId> Node = std::nullopt,
            std::optional<PinIndex> Pin = std::nullopt,
            std::string ExternalIdentityKey = {},
            std::optional<SourceProvenance> PrimaryProvenance = std::nullopt,
            std::optional<SourceProvenance> RelatedProvenance = std::nullopt)
        {
            Diagnostics.push_back(PendingDiagnostic{
                Stage,
                Code,
                Node,
                Pin,
                std::move(ExternalIdentityKey),
                std::move(Message),
                std::move(PrimaryProvenance),
                std::move(RelatedProvenance)
            });
        }

        [[nodiscard]] inline const GiaBackendPinMapping* FindMapping(const GiaBackendNode& Node,
            PinIndex SemanticPin)
        {
            for (const GiaBackendPinMapping& Mapping :
                Node.Mapping.GetPinMappings())
            {
                if (Mapping.GetSemanticPinIndex() == SemanticPin)
                {
                    return &Mapping;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline const GiaBackendInputValue* FindInput(const GiaBackendNode& Node,
            PinIndex SemanticPin)
        {
            for (const GiaBackendInputValue& Input : Node.Inputs)
            {
                if (Input.SemanticPin == SemanticPin)
                {
                    return &Input;
                }
            }
            return nullptr;
        }

        struct WorkingNode
        {
            const GiaBackendNode* BackendNode = nullptr;
            GiaResolvedNode Node;
        };

        [[nodiscard]] inline WorkingNode* FindNode(std::vector<WorkingNode>& Nodes,
            NodeInstanceId GraphNode)
        {
            for (WorkingNode& Node : Nodes)
            {
                if (Node.Node.GraphNode == GraphNode)
                {
                    return &Node;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline const GiaResolvedPin* FindResolvedPin(const GiaResolvedNode& Node,
            PinIndex SemanticPin)
        {
            for (const GiaResolvedPin& Pin : Node.Pins)
            {
                if (Pin.SemanticPin == SemanticPin)
                {
                    return &Pin;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline GiaResolvedPin* FindResolvedPin(GiaResolvedNode& Node,
            PinIndex SemanticPin)
        {
            for (GiaResolvedPin& Pin : Node.Pins)
            {
                if (Pin.SemanticPin == SemanticPin)
                {
                    return &Pin;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline bool HasErrors(const std::vector<PendingDiagnostic>& Diagnostics)
        {
            return !Diagnostics.empty();
        }

        [[nodiscard]] inline bool ResolveEvaluationInterval(double EvaluationInterval,
            float& Result)
        {
            if (!std::isfinite(EvaluationInterval) ||
                EvaluationInterval < 0.0 ||
                EvaluationInterval > static_cast<double>(
                    std::numeric_limits<float>::max()
                ))
            {
                return false;
            }

            Result = static_cast<float>(EvaluationInterval);
            return std::isfinite(Result) && Result >= 0.0F;
        }

        [[nodiscard]] inline std::optional<GiaResolvedNodeIndex> ResolveNodeIndex(NodeInstanceId GraphNode)
        {
            if (!GraphNode.IsValid() ||
                GraphNode.GetValue() > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int32_t>::max()
                ))
            {
                return std::nullopt;
            }

            const auto Value = static_cast<std::int32_t>(GraphNode.GetValue());
            if (Value <= 0)
            {
                return std::nullopt;
            }
            return GiaResolvedNodeIndex(Value);
        }

        [[nodiscard]] inline bool IsSupportedPinKind(GiaPinKind Kind)
        {
            return GiaResolvedBackendGraphDetail::IsSupportedPinKind(Kind);
        }

        [[nodiscard]] inline bool IsPinCanonical(const GiaResolvedPin& Left,
            const GiaResolvedPin& Right)
        {
            return GiaResolvedBackendGraphDetail::PinOrderKey(Left) <
                GiaResolvedBackendGraphDetail::PinOrderKey(Right);
        }

        [[nodiscard]] inline bool IsDataCanonical(const GiaResolvedDataConnection& Left,
            const GiaResolvedDataConnection& Right)
        {
            return GiaResolvedBackendGraphDetail::DataConnectionOrderKey(Left) <
                GiaResolvedBackendGraphDetail::DataConnectionOrderKey(Right);
        }

        [[nodiscard]] inline bool IsControlCanonical(const GiaResolvedControlConnection& Left,
            const GiaResolvedControlConnection& Right)
        {
            return GiaResolvedBackendGraphDetail::ControlConnectionOrderKey(Left) <
                GiaResolvedBackendGraphDetail::ControlConnectionOrderKey(Right);
        }

        [[nodiscard]] inline std::size_t FindWorkingNodeIndex(const std::vector<WorkingNode>& Nodes,
            GiaResolvedNodeIndex NodeIndex)
        {
            for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
            {
                if (Nodes[Index].Node.NodeIndex == NodeIndex)
                {
                    return Index;
                }
            }
            return Nodes.size();
        }

        [[nodiscard]] inline bool ResolveLayout(std::vector<WorkingNode>& Nodes,
            const std::vector<GiaResolvedControlConnection>& ControlConnections,
            std::vector<PendingDiagnostic>& Diagnostics)
        {
            const std::size_t NodeCount = Nodes.size();
            if (NodeCount == 0U)
            {
                Add(
                    Diagnostics,
                    ValidationStage::LayoutResolution,
                    DiagnosticCode::InvalidGiaResolvedLayout,
                    "The resolved graph has no nodes to place."
                );
                return false;
            }

            std::vector<std::vector<std::size_t>> Adjacency(NodeCount);
            for (const GiaResolvedControlConnection& Connection :
                ControlConnections)
            {
                const std::size_t SourceIndex = FindWorkingNodeIndex(
                    Nodes,
                    Connection.Source.Node
                );
                const std::size_t DestinationIndex = FindWorkingNodeIndex(
                    Nodes,
                    Connection.Destination.Node
                );
                if (SourceIndex == NodeCount || DestinationIndex == NodeCount)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::LayoutResolution,
                        DiagnosticCode::InvalidGiaResolvedLayout,
                        "A control connection references a node absent from layout."
                    );
                    return false;
                }
                Adjacency[SourceIndex].push_back(DestinationIndex);
            }

            for (std::vector<std::size_t>& Destinations : Adjacency)
            {
                std::sort(
                    Destinations.begin(),
                    Destinations.end(),
                    [&Nodes](std::size_t Left, std::size_t Right)
                    {
                        return Nodes[Left].Node.GraphNode <
                            Nodes[Right].Node.GraphNode;
                    }
                );
                Destinations.erase(
                    std::unique(Destinations.begin(), Destinations.end()),
                    Destinations.end()
                );
            }

            std::vector<int> DiscoveryIndex(NodeCount, -1);
            std::vector<int> LowLink(NodeCount, -1);
            std::vector<std::size_t> Stack;
            std::vector<bool> OnStack(NodeCount, false);
            std::vector<std::vector<std::size_t>> Components;
            int NextDiscoveryIndex = 0;

            std::function<void(std::size_t)> Visit =
                [&](std::size_t NodeIndex)
                {
                    DiscoveryIndex[NodeIndex] = NextDiscoveryIndex;
                    LowLink[NodeIndex] = NextDiscoveryIndex;
                    ++NextDiscoveryIndex;
                    Stack.push_back(NodeIndex);
                    OnStack[NodeIndex] = true;

                    for (const std::size_t Destination : Adjacency[NodeIndex])
                    {
                        if (DiscoveryIndex[Destination] == -1)
                        {
                            Visit(Destination);
                            LowLink[NodeIndex] = std::min(
                                LowLink[NodeIndex],
                                LowLink[Destination]
                            );
                        }
                        else if (OnStack[Destination])
                        {
                            LowLink[NodeIndex] = std::min(
                                LowLink[NodeIndex],
                                DiscoveryIndex[Destination]
                            );
                        }
                    }

                    if (LowLink[NodeIndex] == DiscoveryIndex[NodeIndex])
                    {
                        std::vector<std::size_t> Component;
                        while (true)
                        {
                            const std::size_t Member = Stack.back();
                            Stack.pop_back();
                            OnStack[Member] = false;
                            Component.push_back(Member);
                            if (Member == NodeIndex)
                            {
                                break;
                            }
                        }
                        std::sort(
                            Component.begin(),
                            Component.end(),
                            [&Nodes](std::size_t Left, std::size_t Right)
                            {
                                return Nodes[Left].Node.GraphNode <
                                    Nodes[Right].Node.GraphNode;
                            }
                        );
                        Components.push_back(std::move(Component));
                    }
                };

            for (std::size_t Index = 0U; Index < NodeCount; ++Index)
            {
                if (DiscoveryIndex[Index] == -1)
                {
                    Visit(Index);
                }
            }

            std::vector<std::size_t> ComponentForNode(NodeCount);
            for (std::size_t ComponentIndex = 0U;
                ComponentIndex < Components.size();
                ++ComponentIndex)
            {
                for (const std::size_t Member : Components[ComponentIndex])
                {
                    ComponentForNode[Member] = ComponentIndex;
                }
            }

            std::vector<std::vector<std::size_t>> ComponentEdges(
                Components.size()
            );
            std::vector<std::size_t> InDegree(Components.size(), 0U);
            for (std::size_t Source = 0U; Source < NodeCount; ++Source)
            {
                for (const std::size_t Destination : Adjacency[Source])
                {
                    const std::size_t SourceComponent =
                        ComponentForNode[Source];
                    const std::size_t DestinationComponent =
                        ComponentForNode[Destination];
                    if (SourceComponent != DestinationComponent)
                    {
                        ComponentEdges[SourceComponent].push_back(
                            DestinationComponent
                        );
                    }
                }
            }

            auto ComponentMinimumNode =
                [&Components, &Nodes](std::size_t ComponentIndex)
                {
                    return Nodes[Components[ComponentIndex].front()].Node.GraphNode;
                };

            for (std::vector<std::size_t>& Destinations : ComponentEdges)
            {
                std::sort(
                    Destinations.begin(),
                    Destinations.end(),
                    [&ComponentMinimumNode](std::size_t Left, std::size_t Right)
                    {
                        return ComponentMinimumNode(Left) <
                            ComponentMinimumNode(Right);
                    }
                );
                Destinations.erase(
                    std::unique(Destinations.begin(), Destinations.end()),
                    Destinations.end()
                );
            }

            for (const std::vector<std::size_t>& Destinations : ComponentEdges)
            {
                for (const std::size_t Destination : Destinations)
                {
                    ++InDegree[Destination];
                }
            }

            std::vector<std::size_t> ComponentRank(Components.size(), 0U);
            std::vector<bool> Processed(Components.size(), false);
            for (std::size_t ProcessedCount = 0U;
                ProcessedCount < Components.size();
                ++ProcessedCount)
            {
                std::vector<std::size_t> Ready;
                for (std::size_t Component = 0U;
                    Component < Components.size();
                    ++Component)
                {
                    if (!Processed[Component] && InDegree[Component] == 0U)
                    {
                        Ready.push_back(Component);
                    }
                }

                if (Ready.empty())
                {
                    Add(
                        Diagnostics,
                        ValidationStage::LayoutResolution,
                        DiagnosticCode::InvalidGiaResolvedLayout,
                        "The condensed control topology is not acyclic."
                    );
                    return false;
                }

                std::sort(
                    Ready.begin(),
                    Ready.end(),
                    [&ComponentMinimumNode](std::size_t Left, std::size_t Right)
                    {
                        return ComponentMinimumNode(Left) <
                            ComponentMinimumNode(Right);
                    }
                );
                const std::size_t Current = Ready.front();
                Processed[Current] = true;
                for (const std::size_t Destination : ComponentEdges[Current])
                {
                    ComponentRank[Destination] = std::max(
                        ComponentRank[Destination],
                        ComponentRank[Current] + 1U
                    );
                    --InDegree[Destination];
                }
            }

            std::size_t MaximumRank = 0U;
            for (const std::size_t Rank : ComponentRank)
            {
                MaximumRank = std::max(MaximumRank, Rank);
            }

            std::vector<std::vector<std::size_t>> RankBuckets(MaximumRank + 1U);
            for (std::size_t Component = 0U;
                Component < Components.size();
                ++Component)
            {
                RankBuckets[ComponentRank[Component]].push_back(Component);
            }

            for (std::vector<std::size_t>& Bucket : RankBuckets)
            {
                std::sort(
                    Bucket.begin(),
                    Bucket.end(),
                    [&ComponentMinimumNode](std::size_t Left, std::size_t Right)
                    {
                        return ComponentMinimumNode(Left) <
                            ComponentMinimumNode(Right);
                    }
                );
            }

            for (std::size_t Rank = 0U; Rank < RankBuckets.size(); ++Rank)
            {
                std::size_t Slot = 0U;
                for (const std::size_t Component : RankBuckets[Rank])
                {
                    for (const std::size_t Member : Components[Component])
                    {
                        const long double X = static_cast<long double>(Rank) *
                            800.0L;
                        const long double Y = static_cast<long double>(Slot) *
                            600.0L;
                        if (X > static_cast<long double>(
                                std::numeric_limits<float>::max()
                            ) ||
                            Y > static_cast<long double>(
                                std::numeric_limits<float>::max()
                            ))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::LayoutResolution,
                                DiagnosticCode::InvalidGiaResolvedLayout,
                                "Resolved node layout exceeds the finite float domain."
                            );
                            return false;
                        }

                        Nodes[Member].Node.Position = GiaResolvedNodePosition{
                            static_cast<float>(X),
                            static_cast<float>(Y)
                        };
                        ++Slot;
                    }
                }
            }

            return true;
        }
    }

    class GiaGraphResolver final
    {
    public:
        [[nodiscard]] static std::expected<
            GiaResolvedBackendGraph,
            DiagnosticCollection
        > Resolve(const GiaBackendGraph& Graph)
        {
            using namespace GiaGraphResolverDetail;

            std::vector<PendingDiagnostic> Diagnostics;
            if (!Graph.IsValid())
            {
                Add(
                    Diagnostics,
                    ValidationStage::InputModelValidation,
                    DiagnosticCode::InvalidGiaResolutionInput,
                    "The P6.2 semantic backend graph is invalid."
                );
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            GiaResolvedGraphHeader Header{
                .CatalogueIdentity = Graph.GetHeader().CatalogueIdentity,
                .MappingSchemaVersion = Graph.GetHeader().MappingSchemaVersion,
                .TargetProfile = Graph.GetHeader().TargetProfile,
                .Mode = Graph.GetHeader().Mode,
                .GraphIdentifier = Graph.GetHeader().GraphIdentifier,
                .GraphName = Graph.GetHeader().GraphName,
                .UniqueIdentifier = Graph.GetHeader().UniqueIdentifier,
                .EvaluationInterval = 0.0F,
                .GraphType = Graph.GetHeader().GraphType,
                .GraphWhich = Graph.GetHeader().GraphWhich,
                .RootClass = GiaResolvedGraphUnitClass::Node,
                .RootType = GiaResolvedGraphUnitType::ClientGraph,
                .InnerClass = GiaResolvedNodeGraphClass::UserDefined,
                .InnerKind = GiaResolvedNodeGraphKind::NodeGraph,
                .EntrySlotIndex = 1,
                .RootModeFlag = std::nullopt
            };

            if (!ResolveEvaluationInterval(
                    Graph.GetHeader().EvaluationInterval,
                    Header.EvaluationInterval
                ))
            {
                Add(
                    Diagnostics,
                    ValidationStage::TargetSemanticResolution,
                    DiagnosticCode::InvalidGiaResolutionInput,
                    "The evaluation interval cannot be represented by the target float domain."
                );
            }

            std::vector<const GiaBackendNode*> OrderedNodes;
            OrderedNodes.reserve(Graph.GetNodes().size());
            for (const GiaBackendNode& Node : Graph.GetNodes())
            {
                OrderedNodes.push_back(&Node);
            }
            std::sort(
                OrderedNodes.begin(),
                OrderedNodes.end(),
                [](const GiaBackendNode* Left, const GiaBackendNode* Right)
                {
                    return Left->Trace.GraphNode < Right->Trace.GraphNode;
                }
            );

            std::vector<WorkingNode> Nodes;
            Nodes.reserve(OrderedNodes.size());
            for (const GiaBackendNode* BackendNode : OrderedNodes)
            {
                const std::optional<GiaResolvedNodeIndex> NodeIndex =
                    ResolveNodeIndex(BackendNode->Trace.GraphNode);
                if (!NodeIndex.has_value())
                {
                    Add(
                        Diagnostics,
                        ValidationStage::NodeIdentityResolution,
                        DiagnosticCode::InvalidGiaResolvedNodeIdentity,
                        "The graph-local node identity is outside the signed target node-index domain.",
                        BackendNode->Trace.GraphNode,
                        std::nullopt,
                        BackendNode->Trace.ExternalIdentity.GetKey(),
                        BackendNode->Trace.DescriptorProvenance,
                        BackendNode->Trace.MappingProvenance
                    );
                    continue;
                }

                Nodes.push_back(WorkingNode{
                    BackendNode,
                    GiaResolvedNode{
                        .GraphNode = BackendNode->Trace.GraphNode,
                        .Descriptor = BackendNode->Trace.Descriptor,
                        .ExternalIdentity = BackendNode->Trace.ExternalIdentity,
                        .DescriptorProvenance = BackendNode->Trace.DescriptorProvenance,
                        .MappingProvenance = BackendNode->Trace.MappingProvenance,
                        .NodeIndex = *NodeIndex,
                        .GenericNodeIdentifier =
                            BackendNode->Mapping.GetGenericNodeIdentifier(),
                        .ConcreteNodeIdentifier =
                            BackendNode->Mapping.GetConcreteNodeIdentifier(),
                        .Pins = {},
                        .Position = GiaResolvedNodePosition{}
                    }
                });
            }

            if (HasErrors(Diagnostics))
            {
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            for (WorkingNode& Working : Nodes)
            {
                const GiaBackendNode& BackendNode = *Working.BackendNode;
                for (const GiaBackendPinMapping& Mapping :
                    BackendNode.Mapping.GetPinMappings())
                {
                    const std::string ExternalKey =
                        BackendNode.Trace.ExternalIdentity.GetKey();
                    if (!IsSupportedPinKind(Mapping.GetPinKind()))
                    {
                        Add(
                            Diagnostics,
                            ValidationStage::PinResolution,
                            DiagnosticCode::UnsupportedGiaResolvedPinKind,
                            "The copied mapping uses a pin kind unsupported by the bounded target.",
                            BackendNode.Trace.GraphNode,
                            Mapping.GetSemanticPinIndex(),
                            ExternalKey,
                            BackendNode.Trace.MappingProvenance
                        );
                        continue;
                    }

                    GiaResolvedPin Pin{
                        .SemanticPin = Mapping.GetSemanticPinIndex(),
                        .Kind = Mapping.GetPinKind(),
                        .PrimaryIndex = Mapping.GetBackendIndex(),
                        .SecondaryIndex = Mapping.GetSecondaryIndex().value_or(
                            Mapping.GetBackendIndex()
                        ),
                        .BackendTypeCode = Mapping.GetBackendTypeCode(),
                        .LiteralEncoding = Mapping.GetLiteralEncoding(),
                        .EmissionPolicy = Mapping.GetEmissionPolicy(),
                        .IsConnectable = Mapping.IsConnectable(),
                        .InputValue = std::nullopt
                    };

                    const GiaBackendInputValue* Input = GiaGraphResolverDetail::FindInput(
                        BackendNode,
                        Mapping.GetSemanticPinIndex()
                    );
                    if (Input != nullptr)
                    {
                        if (Mapping.GetPinKind() != GiaPinKind::InputParameter ||
                            Mapping.GetEmissionPolicy() == GiaPinEmissionPolicy::Omit)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::TargetSemanticResolution,
                                DiagnosticCode::InvalidGiaResolvedPinEmission,
                                "A semantic input value cannot be carried by this resolved pin policy.",
                                BackendNode.Trace.GraphNode,
                                Mapping.GetSemanticPinIndex(),
                                ExternalKey,
                                BackendNode.Trace.MappingProvenance
                            );
                        }
                        else
                        {
                            Pin.InputValue = GiaResolvedInputValue{
                                .SemanticType = Input->SemanticType,
                                .SourceKind = Input->SourceKind,
                                .Literal = Input->Literal,
                                .BackendTypeCode = Mapping.GetBackendTypeCode(),
                                .LiteralEncoding = Mapping.GetLiteralEncoding()
                            };
                        }
                    }
                    else if (Mapping.GetPinKind() == GiaPinKind::InputParameter &&
                        Mapping.GetEmissionPolicy() == GiaPinEmissionPolicy::Emit)
                    {
                        Add(
                            Diagnostics,
                            ValidationStage::TargetSemanticResolution,
                            DiagnosticCode::InvalidGiaResolvedPinEmission,
                            "An emitted input mapping has no semantic value or connection intent.",
                            BackendNode.Trace.GraphNode,
                            Mapping.GetSemanticPinIndex(),
                            ExternalKey,
                            BackendNode.Trace.MappingProvenance
                        );
                    }

                    Working.Node.Pins.push_back(std::move(Pin));
                }

                std::sort(
                    Working.Node.Pins.begin(),
                    Working.Node.Pins.end(),
                    IsPinCanonical
                );
                if (!Working.Node.IsValid())
                {
                    Add(
                        Diagnostics,
                        ValidationStage::PinResolution,
                        DiagnosticCode::InvalidGiaResolvedPinEndpoint,
                        "The copied mapping cannot form a valid resolved pin set.",
                        Working.Node.GraphNode,
                        std::nullopt,
                        Working.Node.ExternalIdentity.GetKey(),
                        Working.Node.MappingProvenance
                    );
                }
            }

            std::vector<GiaResolvedDataConnection> DataConnections;
            DataConnections.reserve(Graph.GetDataConnections().size());
            for (const GiaBackendDataConnection& Connection :
                Graph.GetDataConnections())
            {
                WorkingNode* Source = FindNode(Nodes, Connection.SourceNode);
                WorkingNode* Destination = FindNode(
                    Nodes,
                    Connection.DestinationNode
                );
                const GiaResolvedPin* SourcePin = Source == nullptr
                    ? nullptr
                    : FindResolvedPin(
                        Source->Node,
                        Connection.SourcePin
                    );
                const GiaResolvedPin* DestinationPin = Destination == nullptr
                    ? nullptr
                    : FindResolvedPin(
                        Destination->Node,
                        Connection.DestinationPin
                    );

                const bool Valid =
                    Source != nullptr && Destination != nullptr &&
                    SourcePin != nullptr && DestinationPin != nullptr &&
                    SourcePin->Kind == GiaPinKind::OutputParameter &&
                    DestinationPin->Kind == GiaPinKind::InputParameter &&
                    SourcePin->EmissionPolicy == GiaPinEmissionPolicy::Emit &&
                    DestinationPin->EmissionPolicy == GiaPinEmissionPolicy::Emit &&
                    DestinationPin->IsConnectable &&
                    DestinationPin->InputValue.has_value() &&
                    DestinationPin->InputValue->SourceKind ==
                        GiaBackendInputValueSourceKind::DataConnection &&
                    !DestinationPin->InputValue->Literal.has_value() &&
                    DestinationPin->InputValue->SemanticType == Connection.ValueType;
                if (!Valid)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::ConnectionResolution,
                        DiagnosticCode::InvalidGiaResolvedConnection,
                        "A semantic data connection cannot become a supported resolved endpoint pair.",
                        Connection.DestinationNode,
                        Connection.DestinationPin,
                        Destination == nullptr
                            ? std::string{}
                            : Destination->Node.ExternalIdentity.GetKey(),
                        Destination == nullptr
                            ? std::nullopt
                            : Destination->Node.MappingProvenance
                    );
                    continue;
                }

                DataConnections.push_back(GiaResolvedDataConnection{
                    GiaResolvedBackendGraphDetail::MakeEndpoint(
                        Source->Node,
                        *SourcePin
                    ),
                    GiaResolvedBackendGraphDetail::MakeEndpoint(
                        Destination->Node,
                        *DestinationPin
                    ),
                    Connection.ValueType
                });
            }

            std::sort(DataConnections.begin(), DataConnections.end(), IsDataCanonical);
            for (std::size_t Index = 1U; Index < DataConnections.size(); ++Index)
            {
                if (!IsDataCanonical(
                        DataConnections[Index - 1U],
                        DataConnections[Index]
                    ) ||
                    DataConnections[Index - 1U].Destination ==
                        DataConnections[Index].Destination)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::ConnectionResolution,
                        DiagnosticCode::InvalidGiaResolvedConnection,
                        "Resolved data connections are duplicate or violate single-cardinality destination rules."
                    );
                }
            }

            for (const WorkingNode& Working : Nodes)
            {
                for (const GiaResolvedPin& Pin : Working.Node.Pins)
                {
                    if (Pin.InputValue.has_value() &&
                        Pin.InputValue->SourceKind ==
                            GiaBackendInputValueSourceKind::DataConnection)
                    {
                        const GiaResolvedPinEndpoint Endpoint =
                            GiaResolvedBackendGraphDetail::MakeEndpoint(
                                Working.Node,
                                Pin
                            );
                        const std::size_t Count = static_cast<std::size_t>(
                            std::count_if(
                                DataConnections.begin(),
                                DataConnections.end(),
                                [&Endpoint](
                                    const GiaResolvedDataConnection& Candidate
                                )
                                {
                                    return Candidate.Destination == Endpoint;
                                }
                            )
                        );
                        if (Count != 1U)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ConnectionResolution,
                                DiagnosticCode::InvalidGiaResolvedConnection,
                                "A DataConnection input must resolve to exactly one destination connection.",
                                Working.Node.GraphNode,
                                Pin.SemanticPin,
                                Working.Node.ExternalIdentity.GetKey(),
                                Working.Node.MappingProvenance
                            );
                        }
                    }
                }
            }

            std::vector<GiaResolvedControlConnection> ControlConnections;
            ControlConnections.reserve(Graph.GetControlConnections().size());
            for (const GiaBackendControlConnection& Connection :
                Graph.GetControlConnections())
            {
                WorkingNode* Source = FindNode(Nodes, Connection.SourceNode);
                WorkingNode* Destination = FindNode(
                    Nodes,
                    Connection.DestinationNode
                );
                const GiaResolvedPin* SourcePin = Source == nullptr
                    ? nullptr
                    : FindResolvedPin(Source->Node, Connection.SourcePin);
                const GiaResolvedPin* DestinationPin = Destination == nullptr
                    ? nullptr
                    : FindResolvedPin(Destination->Node, Connection.DestinationPin);
                const bool Valid =
                    Source != nullptr && Destination != nullptr &&
                    SourcePin != nullptr && DestinationPin != nullptr &&
                    SourcePin->Kind == GiaPinKind::OutputFlow &&
                    DestinationPin->Kind == GiaPinKind::InputFlow &&
                    SourcePin->EmissionPolicy == GiaPinEmissionPolicy::Emit &&
                    DestinationPin->EmissionPolicy == GiaPinEmissionPolicy::Emit;
                if (!Valid)
                {
                    const DiagnosticCode Code =
                        SourcePin != nullptr && DestinationPin != nullptr &&
                        SourcePin->Kind == GiaPinKind::OutputFlow &&
                        DestinationPin->Kind == GiaPinKind::InputFlow &&
                        (SourcePin->EmissionPolicy != GiaPinEmissionPolicy::Emit ||
                            DestinationPin->EmissionPolicy != GiaPinEmissionPolicy::Emit)
                            ? DiagnosticCode::InvalidGiaResolvedPinEmission
                            : DiagnosticCode::InvalidGiaResolvedConnection;
                    Add(
                        Diagnostics,
                        ValidationStage::ConnectionResolution,
                        Code,
                        "A semantic control connection cannot become a supported resolved flow endpoint pair.",
                        Connection.DestinationNode,
                        Connection.DestinationPin,
                        Destination == nullptr
                            ? std::string{}
                            : Destination->Node.ExternalIdentity.GetKey(),
                        Destination == nullptr
                            ? std::nullopt
                            : Destination->Node.MappingProvenance
                    );
                    continue;
                }

                ControlConnections.push_back(GiaResolvedControlConnection{
                    GiaResolvedBackendGraphDetail::MakeEndpoint(
                        Source->Node,
                        *SourcePin
                    ),
                    GiaResolvedBackendGraphDetail::MakeEndpoint(
                        Destination->Node,
                        *DestinationPin
                    )
                });
            }

            std::sort(
                ControlConnections.begin(),
                ControlConnections.end(),
                IsControlCanonical
            );
            for (std::size_t Index = 1U; Index < ControlConnections.size(); ++Index)
            {
                if (!IsControlCanonical(
                        ControlConnections[Index - 1U],
                        ControlConnections[Index]
                    ))
                {
                    Add(
                        Diagnostics,
                        ValidationStage::ConnectionResolution,
                        DiagnosticCode::InvalidGiaResolvedConnection,
                        "Resolved control connections are duplicate or violate single-cardinality flow rules."
                    );
                }
            }

            for (const WorkingNode& Working : Nodes)
            {
                for (const GiaResolvedPin& Pin : Working.Node.Pins)
                {
                    if (Pin.Kind != GiaPinKind::InputFlow ||
                        Pin.EmissionPolicy != GiaPinEmissionPolicy::Emit)
                    {
                        continue;
                    }

                    const GiaResolvedPinEndpoint Endpoint =
                        GiaResolvedBackendGraphDetail::MakeEndpoint(
                            Working.Node,
                            Pin
                        );
                    const std::size_t Count = static_cast<std::size_t>(
                        std::count_if(
                            ControlConnections.begin(),
                            ControlConnections.end(),
                            [&Endpoint](
                                const GiaResolvedControlConnection& Candidate
                            )
                            {
                                return Candidate.Destination == Endpoint;
                            }
                        )
                    );
                    if (Count > 1U)
                    {
                        Add(
                            Diagnostics,
                            ValidationStage::ConnectionResolution,
                            DiagnosticCode::InvalidGiaResolvedConnection,
                            "An emitted InputFlow destination has more than one incoming ordinary control connection.",
                            Working.Node.GraphNode,
                            Pin.SemanticPin,
                            Working.Node.ExternalIdentity.GetKey(),
                            Working.Node.MappingProvenance
                        );
                    }
                }
            }

            if (HasErrors(Diagnostics))
            {
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            if (!ResolveLayout(Nodes, ControlConnections, Diagnostics))
            {
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            std::vector<GiaResolvedNode> ResolvedNodes;
            ResolvedNodes.reserve(Nodes.size());
            for (WorkingNode& Working : Nodes)
            {
                ResolvedNodes.push_back(std::move(Working.Node));
            }
            std::sort(
                ResolvedNodes.begin(),
                ResolvedNodes.end(),
                [](const GiaResolvedNode& Left, const GiaResolvedNode& Right)
                {
                    return Left.GraphNode < Right.GraphNode;
                }
            );

            GiaResolvedBackendGraph Result(
                std::move(Header),
                std::move(ResolvedNodes),
                std::move(DataConnections),
                std::move(ControlConnections)
            );
            if (!Result.IsValid())
            {
                Add(
                    Diagnostics,
                    ValidationStage::ModelValidation,
                    DiagnosticCode::InvalidGiaResolvedBackendModel,
                    "The complete resolved GIA backend model violates its invariants."
                );
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            return Result;
        }
    };
}
