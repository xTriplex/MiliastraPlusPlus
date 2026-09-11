#include <cstdlib>
#include <cstdio>
#include <source_location>
#include <type_traits>

#include "MiliastraPlusPlusGraphBuilder.h"

using namespace MiliastraPlusPlus;

namespace
{
    [[noreturn]] void ReportFailure(
        const char* Expression,
        const std::source_location& Location
    )
    {
        std::fprintf(
            stderr,
            "Check failed: %s (%s:%u, %s)\n",
            Expression,
            Location.file_name(),
            Location.line(),
            Location.function_name()
        );
        std::abort();
    }

    void Check(
        bool Condition,
        const char* Expression,
        const std::source_location& Location = std::source_location::current()
    )
    {
        if (!Condition)
        {
            ReportFailure(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    constexpr NodeDescriptorId DescriptorId(901U);
    constexpr NodeDescriptorId OtherDescriptorId(902U);

    NodeDescriptor MakeDescriptor(NodeDescriptorId Identifier, const char* Name)
    {
        return NodeDescriptor(
            Identifier,
            Name,
            {NodeAvailability::Server},
            {}
        );
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Descriptors;
        MPP_CHECK(Descriptors.Register(MakeDescriptor(DescriptorId, "BuilderNode")).has_value());
        MPP_CHECK(Descriptors.Register(MakeDescriptor(OtherDescriptorId, "OtherNode")).has_value());
        return Descriptors;
    }

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        for (const Diagnostic& Diagnostic : Diagnostics)
        {
            if (Diagnostic.Code == Code)
            {
                return true;
            }
        }
        return false;
    }
}

static_assert(std::is_copy_constructible_v<NodeHandle>);
static_assert(!std::is_copy_constructible_v<GraphBuilder>);
static_assert(std::is_move_constructible_v<GraphBuilder>);
static_assert(!std::is_move_assignable_v<GraphBuilder>);

int main()
{
    NodeDescriptorRegistry InvalidRegistry;
    const auto InvalidRegistration = InvalidRegistry.Register(NodeDescriptor(
        NodeDescriptorId{},
        "",
        {},
        {}
    ));
    MPP_CHECK(!InvalidRegistration.has_value());
    MPP_CHECK(InvalidRegistration.error().Code == DiagnosticCode::InvalidNodeDescriptor);

    NodeDescriptorRegistry Descriptors = MakeRegistry();

    NodeHandle DefaultHandle;
    MPP_CHECK(!DefaultHandle.IsValid());

    GraphBuilder Builder(Descriptors);
    MPP_CHECK(Builder.Validate().empty());

    const auto InvalidDescriptor = Builder.AddNode(NodeDescriptorId{});
    MPP_CHECK(!InvalidDescriptor.has_value());
    MPP_CHECK(HasCode(InvalidDescriptor.error(), DiagnosticCode::InvalidNodeDescriptor));

    const auto MissingDescriptor = Builder.AddNode(NodeDescriptorId(999U));
    MPP_CHECK(!MissingDescriptor.has_value());
    MPP_CHECK(HasCode(MissingDescriptor.error(), DiagnosticCode::MissingDescriptor));

    const auto FirstNodeResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(FirstNodeResult.has_value());
    const NodeHandle FirstNode = *FirstNodeResult;
    MPP_CHECK(FirstNode.IsValid());
    MPP_CHECK(FirstNode.GetIdentifier() == NodeInstanceId(1U));
    MPP_CHECK(FirstNode.GetDescriptor() == DescriptorId);
    MPP_CHECK(Builder.IsHandleUsable(FirstNode));

    const NodeHandle CopiedNode = FirstNode;
    MPP_CHECK(CopiedNode.IsValid());
    MPP_CHECK(CopiedNode.GetIdentifier() == FirstNode.GetIdentifier());
    MPP_CHECK(Builder.IsHandleUsable(CopiedNode));

    const auto SecondNodeResult = Builder.AddNode(OtherDescriptorId);
    MPP_CHECK(SecondNodeResult.has_value());
    MPP_CHECK(SecondNodeResult->GetIdentifier() == NodeInstanceId(2U));

    const auto ThirdNodeResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(ThirdNodeResult.has_value());
    MPP_CHECK(ThirdNodeResult->GetIdentifier() == NodeInstanceId(3U));
    MPP_CHECK(Builder.Validate().empty());

    auto BuildEquivalentGraph = [&Descriptors]()
    {
        GraphBuilder EquivalentBuilder(Descriptors);
        MPP_CHECK(EquivalentBuilder.AddNode(DescriptorId).has_value());
        MPP_CHECK(EquivalentBuilder.AddNode(OtherDescriptorId).has_value());
        auto Result = std::move(EquivalentBuilder).Finalize();
        MPP_CHECK(Result.has_value());
        return std::move(*Result);
    };

    const GraphIR FirstEquivalentGraph = BuildEquivalentGraph();
    const GraphIR SecondEquivalentGraph = BuildEquivalentGraph();
    MPP_CHECK(FirstEquivalentGraph.GetNodes() == SecondEquivalentGraph.GetNodes());

    GraphIR FinalGraph;
    {
        GraphBuilder MoveSource(Descriptors);
        const auto MovedHandleResult = MoveSource.AddNode(DescriptorId);
        MPP_CHECK(MovedHandleResult.has_value());
        const NodeHandle MovedHandle = *MovedHandleResult;

        GraphBuilder MoveDestination(std::move(MoveSource));
        MPP_CHECK(MoveDestination.IsHandleUsable(MovedHandle));
        MPP_CHECK(!MoveSource.IsHandleUsable(MovedHandle));
        const auto MovedFromAddNode = MoveSource.AddNode(DescriptorId);
        MPP_CHECK(!MovedFromAddNode.has_value());
        MPP_CHECK(HasCode(MovedFromAddNode.error(), DiagnosticCode::InvalidGraphBuilderState));

        const auto MovedNodeResult = MoveDestination.AddNode(OtherDescriptorId);
        MPP_CHECK(MovedNodeResult.has_value());
        MPP_CHECK(MovedNodeResult->GetIdentifier() == NodeInstanceId(2U));

        auto MoveFinalized = std::move(MoveDestination).Finalize();
        MPP_CHECK(MoveFinalized.has_value());
        FinalGraph = std::move(*MoveFinalized);
        MPP_CHECK(!MovedHandle.IsValid());
        MPP_CHECK(!MoveDestination.IsHandleUsable(MovedHandle));
        MPP_CHECK(!MoveDestination.AddNode(DescriptorId).has_value());
    }

    MPP_CHECK(FinalGraph.GetNodeCount() == 2U);
    MPP_CHECK((FinalGraph.GetNodes()[0] == NodeInstance{
        NodeInstanceId(1U), DescriptorId
    }));
    MPP_CHECK((FinalGraph.GetNodes()[1] == NodeInstance{
        NodeInstanceId(2U), OtherDescriptorId
    }));
    MPP_CHECK(GraphIRValidator::Validate(FinalGraph, Descriptors).empty());

    {
        GraphBuilder BuilderA(Descriptors);
        GraphBuilder BuilderB(Descriptors);
        const auto ForeignHandleResult = BuilderA.AddNode(DescriptorId);
        MPP_CHECK(ForeignHandleResult.has_value());
        MPP_CHECK(!BuilderB.IsHandleUsable(*ForeignHandleResult));
        MPP_CHECK(!BuilderB.AddNode(NodeDescriptorId(999U)).has_value());
    }

    {
        NodeHandle DestroyedHandle;
        {
            GraphBuilder TemporaryBuilder(Descriptors);
            const auto HandleResult = TemporaryBuilder.AddNode(DescriptorId);
            MPP_CHECK(HandleResult.has_value());
            DestroyedHandle = *HandleResult;
            MPP_CHECK(DestroyedHandle.IsValid());
        }
        MPP_CHECK(!DestroyedHandle.IsValid());
    }

    {
        GraphBuilder TerminalBuilder(Descriptors);
        MPP_CHECK(TerminalBuilder.AddNode(DescriptorId).has_value());
        auto Result = std::move(TerminalBuilder).Finalize();
        MPP_CHECK(Result.has_value());
        const DiagnosticCollection ClosedValidation = TerminalBuilder.Validate();
        MPP_CHECK(HasCode(ClosedValidation, DiagnosticCode::InvalidGraphBuilderState));
        const auto RepeatedFinalization = std::move(TerminalBuilder).Finalize();
        MPP_CHECK(!RepeatedFinalization.has_value());
        MPP_CHECK(HasCode(RepeatedFinalization.error(), DiagnosticCode::InvalidGraphBuilderState));

        const auto ClosedAddNode = TerminalBuilder.AddNode(DescriptorId);
        MPP_CHECK(!ClosedAddNode.has_value());
        MPP_CHECK(HasCode(ClosedAddNode.error(), DiagnosticCode::InvalidGraphBuilderState));
    }

    {
        GraphBuilder EmptyBuilder(Descriptors);
        auto Result = std::move(EmptyBuilder).Finalize();
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetNodeCount() == 0U);
    }

    MPP_CHECK(Descriptors.Size() == 2U);
    MPP_CHECK(Descriptors.Find(DescriptorId) != nullptr);
    MPP_CHECK(Descriptors.Find(OtherDescriptorId) != nullptr);

    return 0;
}

#undef MPP_CHECK
