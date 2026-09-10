#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusValueTypes.h"

namespace MiliastraPlusPlus
{
    struct NodeInstance
    {
        NodeInstanceId Identifier;
        NodeDescriptorId Descriptor;

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
    };

    struct ControlEdge
    {
        NodeInstanceId SourceNode;
        PinIndex SourceOutputPin;
        NodeInstanceId DestinationNode;
        PinIndex DestinationInputPin;
    };

    class GraphIR
    {
    public:
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
            InputBinding Binding
        )
        {
            m_InputBindings.push_back(InputBindingRecord{
                DestinationNode,
                DestinationInputPin,
                std::move(Binding)
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

    private:
        std::vector<NodeInstance> m_Nodes;
        std::vector<GraphVariable> m_Variables;
        std::vector<InputBindingRecord> m_InputBindings;
        std::vector<ControlEdge> m_ControlEdges;
    };
}
