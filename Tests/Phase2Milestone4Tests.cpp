#include <cstdlib>
#include <string>
#include <variant>

#include "MiliastraPlusPlusGraphIR.h"

using namespace MiliastraPlusPlus;

namespace
{
    void Check(bool Condition)
    {
        if (!Condition)
        {
            std::abort();
        }
    }
}

int main()
{
    const NodeInstance FirstNode{NodeInstanceId(1U), NodeDescriptorId(10U)};
    const NodeInstance SecondNode{NodeInstanceId(2U), NodeDescriptorId(10U)};
    const GraphVariable Variable{
        GraphVariableId(1U),
        "Score",
        TypeDesc::Integer(),
        LiteralValue(LiteralValue::Data{std::int64_t{42}})
    };

    Check(FirstNode.IsValid());
    Check(!NodeInstance{}.IsValid());
    Check(Variable.IsValid());
    Check(!GraphVariable{}.IsValid());

    GraphIR Graph;
    Graph.AddNode(FirstNode);
    Graph.AddNode(SecondNode);
    Graph.AddVariable(Variable);
    Check(Graph.GetNodeCount() == 2U);
    Check(Graph.FindNode(NodeInstanceId(1U)) != nullptr);
    Check(Graph.FindNode(NodeInstanceId(99U)) == nullptr);
    Check(Graph.FindNode(NodeInstanceId(1U))->Descriptor == NodeDescriptorId(10U));
    Check(Graph.GetVariableCount() == 1U);
    Check(Graph.FindVariable(GraphVariableId(1U)) != nullptr);
    Check(Graph.FindVariable(GraphVariableId(99U)) == nullptr);

    Graph.BindInput(
        FirstNode.Identifier,
        PinIndex(0U),
        LiteralValue(LiteralValue::Data{std::int64_t{7}})
    );
    Graph.BindInput(
        FirstNode.Identifier,
        PinIndex(1U),
        OutputReference{SecondNode.Identifier, PinIndex(0U)}
    );
    Graph.BindInput(
        SecondNode.Identifier,
        PinIndex(0U),
        GraphVariableReference{Variable.Identifier}
    );

    Check(Graph.GetInputBindingCount() == 3U);
    const InputBinding* LiteralBinding = Graph.GetInputBinding(FirstNode.Identifier, PinIndex(0U));
    const InputBinding* OutputBinding = Graph.GetInputBinding(FirstNode.Identifier, PinIndex(1U));
    const InputBinding* VariableBinding = Graph.GetInputBinding(SecondNode.Identifier, PinIndex(0U));
    Check(LiteralBinding != nullptr && std::holds_alternative<LiteralValue>(*LiteralBinding));
    Check(OutputBinding != nullptr && std::get<OutputReference>(*OutputBinding).IsValid());
    Check(VariableBinding != nullptr && std::get<GraphVariableReference>(*VariableBinding).IsValid());
    Check(Graph.GetInputBinding(SecondNode.Identifier, PinIndex(9U)) == nullptr);

    Graph.AddControlEdge(ControlEdge{
        FirstNode.Identifier,
        PinIndex(2U),
        SecondNode.Identifier,
        PinIndex(3U)
    });
    Graph.AddControlEdge(ControlEdge{
        SecondNode.Identifier,
        PinIndex(4U),
        FirstNode.Identifier,
        PinIndex(5U)
    });
    Check(Graph.GetControlEdgeCount() == 2U);
    Check(Graph.GetControlEdges()[0].SourceNode == FirstNode.Identifier);
    Check(Graph.GetControlEdges()[0].DestinationInputPin == PinIndex(3U));
    Check(Graph.GetNodes().size() == 2U);
    Check(Graph.GetVariables().front().Name == "Score");

    GraphIR IndependentGraph;
    Check(IndependentGraph.GetNodeCount() == 0U);
    Check(IndependentGraph.GetVariableCount() == 0U);

    return 0;
}
