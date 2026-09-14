#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusValueTypes.h"

namespace MiliastraPlusPlus
{
    /// One graph-local node instance; optional region membership is persisted for
    /// structured execution.
    struct NodeInstance
    {
        NodeInstanceId Identifier;
        NodeDescriptorId Descriptor;
        std::optional<ExecutionRegionId> ExecutionRegion;

        [[nodiscard]] bool IsValid() const
        {
            return Identifier.IsValid() && Descriptor.IsValid();
        }

        auto operator<=>(const NodeInstance&) const = default;
    };

    struct GraphVariable
    {
        GraphVariableId Identifier;
        std::string Name;
        TypeDesc Type;
        std::optional<LiteralValue> DefaultValue;

        [[nodiscard]] bool IsValid() const
        {
            return Identifier.IsValid() && !Name.empty() && Type.IsValid();
        }
    };

    struct InputBindingRecord
    {
        NodeInstanceId DestinationNode;
        PinIndex DestinationInputPin;
        InputBinding Binding;
        // Output intent belongs to this binding, not to the OutputReference itself.
        std::optional<TypeDesc> OutputTypeConstraint;
    };

    /// The sole persisted node-to-node execution topology.
    struct ControlEdge
    {
        NodeInstanceId SourceNode;
        PinIndex SourceOutputPin;
        NodeInstanceId DestinationNode;
        PinIndex DestinationInputPin;
    };

    /// Unstructured graphs carry no inferred Entry or region ownership.
    enum class ExecutionModel
    {
        Unstructured,
        Structured
    };

    enum class ExecutionRegionKind
    {
        Entry,
        BranchArm,
        LoopBody
    };

    struct ExecutionEntry
    {
        ExecutionEntryId Identifier;
        NodeInstanceId RootNode;

        auto operator<=>(const ExecutionEntry&) const = default;
    };

    struct ExecutionRegion
    {
        ExecutionRegionId Identifier;
        ExecutionEntryId Entry;
        ExecutionRegionKind Kind;
        // Parent records lexical containment; owner fields identify the construct
        // output that creates the region.
        std::optional<ExecutionRegionId> Parent;
        std::optional<NodeInstanceId> OwnerNode;
        std::optional<PinIndex> OwnerOutputPin;

        auto operator<=>(const ExecutionRegion&) const = default;
    };

    /// Canonical persisted graph representation used by validation and serialization.
    class GraphIR
    {
    public:
        [[nodiscard]] ExecutionModel GetExecutionModel() const
        {
            return m_ExecutionModel;
        }

        void SetExecutionModel(ExecutionModel Model)
        {
            m_ExecutionModel = Model;
        }

        void AddExecutionEntry(ExecutionEntry Entry)
        {
            m_ExecutionEntries.push_back(std::move(Entry));
        }

        [[nodiscard]] const std::vector<ExecutionEntry>& GetExecutionEntries() const
        {
            return m_ExecutionEntries;
        }

        [[nodiscard]] const ExecutionEntry* FindExecutionEntry(ExecutionEntryId Identifier) const
        {
            for (const ExecutionEntry& Entry : m_ExecutionEntries)
            {
                if (Entry.Identifier == Identifier)
                {
                    return &Entry;
                }
            }
            return nullptr;
        }

        void AddExecutionRegion(ExecutionRegion Region)
        {
            m_ExecutionRegions.push_back(std::move(Region));
        }

        [[nodiscard]] const std::vector<ExecutionRegion>& GetExecutionRegions() const
        {
            return m_ExecutionRegions;
        }

        [[nodiscard]] const ExecutionRegion* FindExecutionRegion(ExecutionRegionId Identifier) const
        {
            for (const ExecutionRegion& Region : m_ExecutionRegions)
            {
                if (Region.Identifier == Identifier)
                {
                    return &Region;
                }
            }
            return nullptr;
        }

        void AddNode(NodeInstance Node)
        {
            m_Nodes.push_back(std::move(Node));
        }

        [[nodiscard]] const NodeInstance* FindNode(NodeInstanceId Identifier) const
        {
            for (const NodeInstance& Node : m_Nodes)
            {
                if (Node.Identifier == Identifier)
                {
                    return &Node;
                }
            }
            return nullptr;
        }

        void AddVariable(GraphVariable Variable)
        {
            m_Variables.push_back(std::move(Variable));
        }

        [[nodiscard]] const GraphVariable* FindVariable(GraphVariableId Identifier) const
        {
            for (const GraphVariable& Variable : m_Variables)
            {
                if (Variable.Identifier == Identifier)
                {
                    return &Variable;
                }
            }
            return nullptr;
        }

        void BindInput(
            NodeInstanceId DestinationNode,
            PinIndex DestinationInputPin,
            InputBinding Binding,
            std::optional<TypeDesc> OutputTypeConstraint = std::nullopt
        )
        {
            m_InputBindings.push_back(InputBindingRecord{
                DestinationNode,
                DestinationInputPin,
                std::move(Binding),
                std::move(OutputTypeConstraint)
            });
        }

        [[nodiscard]] const InputBinding* GetInputBinding(
            NodeInstanceId DestinationNode,
            PinIndex DestinationInputPin
        ) const
        {
            for (const InputBindingRecord& Record : m_InputBindings)
            {
                if (Record.DestinationNode == DestinationNode &&
                    Record.DestinationInputPin == DestinationInputPin)
                {
                    return &Record.Binding;
                }
            }
            return nullptr;
        }

        void AddControlEdge(ControlEdge Edge)
        {
            m_ControlEdges.push_back(std::move(Edge));
        }

        [[nodiscard]] const std::vector<NodeInstance>& GetNodes() const
        {
            return m_Nodes;
        }

        [[nodiscard]] const std::vector<GraphVariable>& GetVariables() const
        {
            return m_Variables;
        }

        [[nodiscard]] const std::vector<InputBindingRecord>& GetInputBindings() const
        {
            return m_InputBindings;
        }

        [[nodiscard]] const std::vector<ControlEdge>& GetControlEdges() const
        {
            return m_ControlEdges;
        }

        [[nodiscard]] std::size_t GetNodeCount() const
        {
            return m_Nodes.size();
        }

        [[nodiscard]] std::size_t GetVariableCount() const
        {
            return m_Variables.size();
        }

        [[nodiscard]] std::size_t GetInputBindingCount() const
        {
            return m_InputBindings.size();
        }

        [[nodiscard]] std::size_t GetControlEdgeCount() const
        {
            return m_ControlEdges.size();
        }

        [[nodiscard]] std::size_t GetExecutionEntryCount() const
        {
            return m_ExecutionEntries.size();
        }

        [[nodiscard]] std::size_t GetExecutionRegionCount() const
        {
            return m_ExecutionRegions.size();
        }

    private:
        ExecutionModel m_ExecutionModel = ExecutionModel::Unstructured;
        std::vector<NodeInstance> m_Nodes;
        std::vector<GraphVariable> m_Variables;
        std::vector<InputBindingRecord> m_InputBindings;
        std::vector<ControlEdge> m_ControlEdges;
        std::vector<ExecutionEntry> m_ExecutionEntries;
        std::vector<ExecutionRegion> m_ExecutionRegions;
    };
}
