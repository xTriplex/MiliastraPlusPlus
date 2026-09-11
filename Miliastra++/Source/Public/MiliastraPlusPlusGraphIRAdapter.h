#pragma once

#include <expected>
#include <unordered_set>
#include <string>
#include <vector>

#include "MiliastraPlusPlusGraph.h"
#include "MiliastraPlusPlusGraphIR.h"

namespace MiliastraPlusPlus
{
    struct GraphIRPinMapping
    {
        PinIdentifier SourcePin;
        PinIndex GraphIRPin;
    };

    struct GraphIRNodeMapping
    {
        NodeIdentifier SourceNode;
        NodeDescriptorId Descriptor;
        std::vector<GraphIRPinMapping> Pins;
    };

    struct GraphIRAdapterMapping
    {
        std::vector<GraphIRNodeMapping> Nodes;
    };

    class GraphIRAdapter
    {
    public:
        [[nodiscard]] static std::expected<GraphIR, DiagnosticCollection> Convert(
            const Graph& SourceGraph,
            const GraphIRAdapterMapping& Mapping
        )
        {
            DiagnosticCollection Diagnostics;
            GraphIR Result;

            for (const GraphIRNodeMapping& NodeMapping : Mapping.Nodes)
            {
                const Node* SourceNode = SourceGraph.FindNodeByIdentifier(NodeMapping.SourceNode);
                if (SourceNode == nullptr || !NodeMapping.Descriptor.IsValid())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterMapping,
                        "The adapter mapping references a missing node or invalid descriptor.");
                    continue;
                }

                std::unordered_set<std::uint64_t> SourcePins;
                std::unordered_set<std::uint32_t> GraphIRPins;
                for (const GraphIRPinMapping& PinMapping : NodeMapping.Pins)
                {
                    const Pin* SourcePin = SourceNode->GetPinByIdentifier(PinMapping.SourcePin);
                    if (SourcePin == nullptr || !PinMapping.GraphIRPin.IsValid() ||
                        !SourcePins.insert(PinMapping.SourcePin.GetValue()).second ||
                        !GraphIRPins.insert(PinMapping.GraphIRPin.GetValue()).second)
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterMapping,
                            "The adapter mapping contains a missing, invalid, or duplicate pin mapping.");
                    }
                }
            }

            std::unordered_set<std::uint64_t> SourceNodes;
            for (const GraphIRNodeMapping& NodeMapping : Mapping.Nodes)
            {
                if (!SourceNodes.insert(NodeMapping.SourceNode.GetValue()).second)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterMapping,
                        "The adapter mapping contains duplicate source node mappings.");
                }
            }

            for (const std::unique_ptr<Node>& SourceNode : SourceGraph.GetNodes())
            {
                const GraphIRNodeMapping* NodeMapping = FindNodeMapping(
                    Mapping, SourceNode->GetIdentifier());
                if (NodeMapping == nullptr || !NodeMapping->Descriptor.IsValid())
                {
                    Add(Diagnostics, DiagnosticCode::MissingAdapterNodeMapping,
                        "The Phase 1 node has no valid GraphIR descriptor mapping.");
                    continue;
                }

                Result.AddNode(NodeInstance{
                    NodeInstanceId(SourceNode->GetIdentifier().GetValue()),
                    NodeMapping->Descriptor
                });
            }

            for (const Link& SourceLink : SourceGraph.GetLinks())
            {
                const PinReference& SourceReference = SourceLink.GetSourcePinReference();
                const PinReference& DestinationReference = SourceLink.GetDestinationPinReference();
                const Node* SourceNode = SourceGraph.FindNodeByIdentifier(
                    SourceReference.OwningNodeIdentifier);
                const Node* DestinationNode = SourceGraph.FindNodeByIdentifier(
                    DestinationReference.OwningNodeIdentifier);
                const Pin* SourcePin = SourceGraph.FindPinByReference(SourceReference);
                const Pin* DestinationPin = SourceGraph.FindPinByReference(DestinationReference);
                const GraphIRNodeMapping* SourceMapping = FindNodeMapping(
                    Mapping, SourceReference.OwningNodeIdentifier);
                const GraphIRNodeMapping* DestinationMapping = FindNodeMapping(
                    Mapping, DestinationReference.OwningNodeIdentifier);

                const PinIndex* SourceIndex = FindPinMapping(
                    SourceMapping, SourceReference.LocalPinIdentifier);
                const PinIndex* DestinationIndex = FindPinMapping(
                    DestinationMapping, DestinationReference.LocalPinIdentifier);
                if (SourceNode == nullptr || DestinationNode == nullptr ||
                    SourcePin == nullptr || DestinationPin == nullptr)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterLink,
                        "A Phase 1 link cannot be represented with the supplied GraphIR mapping.");
                    continue;
                }
                if (SourceMapping == nullptr || DestinationMapping == nullptr ||
                    SourceIndex == nullptr || DestinationIndex == nullptr)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterMapping,
                        "A Phase 1 link has no complete explicit GraphIR pin mapping.");
                    continue;
                }

                const NodeInstanceId SourceNodeId(SourceNode->GetIdentifier().GetValue());
                const NodeInstanceId DestinationNodeId(DestinationNode->GetIdentifier().GetValue());
                const bool IsExecutionLink =
                    SourcePin->GetPinCategory() == EPinCategory::Execution &&
                    DestinationPin->GetPinCategory() == EPinCategory::Execution &&
                    SourcePin->GetPinKind() == EPinKind::Output &&
                    DestinationPin->GetPinKind() == EPinKind::Input &&
                    SourcePin->GetPinType() == EPinType::Flow &&
                    DestinationPin->GetPinType() == EPinType::Flow;
                const bool IsDataLink =
                    SourcePin->GetPinCategory() == EPinCategory::Data &&
                    DestinationPin->GetPinCategory() == EPinCategory::Data &&
                    SourcePin->GetPinKind() == EPinKind::Output &&
                    DestinationPin->GetPinKind() == EPinKind::Input;
                if (IsExecutionLink)
                {
                    Result.AddControlEdge(ControlEdge{
                        SourceNodeId,
                        *SourceIndex,
                        DestinationNodeId,
                        *DestinationIndex
                    });
                }
                else if (IsDataLink)
                {
                    Result.BindInput(
                        DestinationNodeId,
                        *DestinationIndex,
                        OutputReference{SourceNodeId, *SourceIndex}
                    );
                }
                else
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRAdapterLink,
                        "A Phase 1 link has an unsupported direction, category, or execution type.");
                }
            }

            if (!Diagnostics.empty())
            {
                return std::unexpected(std::move(Diagnostics));
            }
            return Result;
        }

    private:
        static const GraphIRNodeMapping* FindNodeMapping(
            const GraphIRAdapterMapping& Mapping,
            NodeIdentifier Identifier
        )
        {
            for (const GraphIRNodeMapping& NodeMapping : Mapping.Nodes)
            {
                if (NodeMapping.SourceNode == Identifier)
                {
                    return &NodeMapping;
                }
            }
            return nullptr;
        }

        static const PinIndex* FindPinMapping(
            const GraphIRNodeMapping* NodeMapping,
            PinIdentifier Identifier
        )
        {
            if (NodeMapping == nullptr)
            {
                return nullptr;
            }
            for (const GraphIRPinMapping& PinMapping : NodeMapping->Pins)
            {
                if (PinMapping.SourcePin == Identifier)
                {
                    return &PinMapping.GraphIRPin;
                }
            }
            return nullptr;
        }

        static void Add(
            DiagnosticCollection& Diagnostics,
            DiagnosticCode Code,
            const char* Message
        )
        {
            Diagnostics.push_back(Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = Message
            });
        }
    };
}
