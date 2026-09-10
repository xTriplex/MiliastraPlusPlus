#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

#include "MiliastraPlusPlusCompiler.h"

using namespace MiliastraPlusPlus;

namespace
{
    struct TestPinSet
    {
        PinReference Output;
        PinReference Input;
    };

    std::unique_ptr<Node> MakeNode(
        NodeIdentifier Identifier,
        EPinCategory Category,
        EPinType Type,
        std::int32_t TypeGroupIdentifier = -1
    )
    {
        auto GraphNode = std::make_unique<Node>(Identifier, "Test Node");
        const auto OutputResult = GraphNode->AddPin(std::make_unique<Pin>(
            PinIdentifier(1U), "Output", Category, Type, EPinKind::Output,
            std::nullopt, TypeGroupIdentifier
        ));
        const auto InputResult = GraphNode->AddPin(std::make_unique<Pin>(
            PinIdentifier(2U), "Input", Category, Type, EPinKind::Input,
            std::nullopt, TypeGroupIdentifier
        ));
        if (!OutputResult.has_value() || !InputResult.has_value())
        {
            return nullptr;
        }
        return GraphNode;
    }

    TestPinSet GetPins(NodeIdentifier Identifier)
    {
        return {
            .Output = { Identifier, PinIdentifier(1U) },
            .Input = { Identifier, PinIdentifier(2U) }
        };
    }

    Link MakeLink(
        std::uint32_t Identifier,
        const PinReference& SourcePinReference,
        const PinReference& DestinationPinReference
    )
    {
        return Link(LinkIdentifier(Identifier), SourcePinReference, DestinationPinReference);
    }

    bool HasDiagnostic(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        for (const Diagnostic& CurrentDiagnostic : Diagnostics)
        {
            if (CurrentDiagnostic.Code == Code)
            {
                return true;
            }
        }
        return false;
    }

    void Check(bool Condition, int FailureCode)
    {
        if (!Condition)
        {
            std::exit(FailureCode);
        }
    }
}

int main()
{
    Graph GraphInstance(GraphIdentifier(1U), "Test Graph");
    Check(GraphInstance.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 1);
    Check(GraphInstance.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 2);
    const auto DuplicateNodeResult = GraphInstance.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Integer
    ));
    Check(!DuplicateNodeResult.has_value() &&
        DuplicateNodeResult.error().Code == DiagnosticCode::DuplicateNodeIdentifier, 3);
    Check(GraphInstance.GetNodes().size() == 2U, 4);
    const auto NullNodeResult = GraphInstance.AddNode(nullptr);
    Check(!NullNodeResult.has_value() && NullNodeResult.error().Code == DiagnosticCode::NullNode, 5);
    Check(GraphInstance.GetNodes().size() == 2U, 6);

    const TestPinSet FirstPins = GetPins(NodeIdentifier(1U));
    const TestPinSet SecondPins = GetPins(NodeIdentifier(2U));
    Check(FirstPins.Output != SecondPins.Output, 7);
    Check(GraphInstance.FindPinByReference(FirstPins.Output) != nullptr, 8);
    Check(GraphInstance.FindPinByReference(FirstPins.Output) !=
        GraphInstance.FindPinByReference(SecondPins.Output), 9);
    Check(GraphInstance.FindNodeByIdentifier(FirstPins.Output.OwningNodeIdentifier)->
        GetPinByIdentifier(FirstPins.Output.LocalPinIdentifier) ==
        GraphInstance.FindPinByReference(FirstPins.Output), 10);
    Node PinValidationNode(NodeIdentifier(99U), "Pin Validation Node");
    const auto NullPinResult = PinValidationNode.AddPin(nullptr);
    Check(!NullPinResult.has_value() && NullPinResult.error().Code == DiagnosticCode::NullPin, 11);
    Check(PinValidationNode.AddPin(std::make_unique<Pin>(
        PinIdentifier(1U), "Output", EPinCategory::Data, EPinType::Integer, EPinKind::Output
    )).has_value(), 12);
    const auto DuplicatePinResult = PinValidationNode.AddPin(std::make_unique<Pin>(
        PinIdentifier(1U), "Duplicate", EPinCategory::Data, EPinType::Integer, EPinKind::Output
    ));
    Check(!DuplicatePinResult.has_value() &&
        DuplicatePinResult.error().Code == DiagnosticCode::DuplicatePinIdentifier, 13);
    Check(PinValidationNode.GetPins().size() == 1U, 14);

    const std::size_t InitialLinkCount = GraphInstance.GetLinks().size();
    Check(GraphInstance.AddLink(MakeLink(1U, FirstPins.Output, SecondPins.Input)).has_value(), 15);
    Check(GraphInstance.GetLinks().size() == InitialLinkCount + 1U, 16);

    Graph ExecutionGraph(GraphIdentifier(2U), "Execution Graph");
    Check(ExecutionGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Execution, EPinType::Flow
    )).has_value(), 17);
    Check(ExecutionGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Execution, EPinType::Flow
    )).has_value(), 18);
    Check(ExecutionGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 19);

    Graph InvalidExecutionGraph(GraphIdentifier(12U), "Invalid Execution Graph");
    Check(InvalidExecutionGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Execution, EPinType::Integer
    )).has_value(), 300);
    Check(InvalidExecutionGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Execution, EPinType::Flow
    )).has_value(), 301);
    const auto InvalidExecutionTypeResult = InvalidExecutionGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!InvalidExecutionTypeResult.has_value() &&
        InvalidExecutionTypeResult.error().Code == DiagnosticCode::ExecutionPinsMustUseFlowType, 302);

    const auto CheckRejectedLink = [&GraphInstance](
        std::uint32_t Identifier,
        const PinReference& SourcePinReference,
        const PinReference& DestinationPinReference,
        DiagnosticCode ExpectedCode,
        int FailureCode
    )
    {
        const std::size_t LinkCount = GraphInstance.GetLinks().size();
        const auto Result = GraphInstance.AddLink(MakeLink(
            Identifier, SourcePinReference, DestinationPinReference
        ));
        Check(!Result.has_value(), FailureCode);
        Check(Result.error().Code == ExpectedCode, FailureCode + 1);
        Check(GraphInstance.GetLinks().size() == LinkCount, FailureCode + 2);
        Check(Result.error().Severity == DiagnosticSeverity::Error, FailureCode + 3);
        Check(!Result.error().Message.empty(), FailureCode + 4);
        Check(Result.error().SourceNodeIdentifier.has_value(), FailureCode + 5);
        Check(Result.error().DestinationNodeIdentifier.has_value(), FailureCode + 6);
        Check(Result.error().SourcePinReference.has_value(), FailureCode + 7);
        Check(Result.error().DestinationPinReference.has_value(), FailureCode + 8);
    };

    CheckRejectedLink(2U, { NodeIdentifier(99U), PinIdentifier(1U) }, SecondPins.Input,
        DiagnosticCode::MissingSourcePin, 20);
    CheckRejectedLink(3U, FirstPins.Output, { NodeIdentifier(99U), PinIdentifier(2U) },
        DiagnosticCode::MissingDestinationPin, 30);
    CheckRejectedLink(4U, FirstPins.Input, SecondPins.Input,
        DiagnosticCode::SourcePinMustBeOutput, 40);
    CheckRejectedLink(5U, FirstPins.Output, SecondPins.Output,
        DiagnosticCode::DestinationPinMustBeInput, 50);

    Graph CrossGraph(GraphIdentifier(3U), "Cross Graph");
    Check(CrossGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 60);
    Check(CrossGraph.FindPinByReference(FirstPins.Output) != nullptr, 61);
    const std::size_t CrossGraphLinkCount = CrossGraph.GetLinks().size();
    const auto CrossGraphResult = CrossGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(99U)).Output, GetPins(NodeIdentifier(1U)).Input
    ));
    Check(!CrossGraphResult.has_value() &&
        CrossGraphResult.error().Code == DiagnosticCode::MissingSourcePin, 62);
    Check(CrossGraph.GetLinks().size() == CrossGraphLinkCount, 63);

    Graph CategoryGraph(GraphIdentifier(4U), "Category Graph");
    Check(CategoryGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Execution, EPinType::Flow
    )).has_value(), 70);
    Check(CategoryGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 71);
    const auto ExecutionToDataResult = CategoryGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!ExecutionToDataResult.has_value() &&
        ExecutionToDataResult.error().Code == DiagnosticCode::IncompatiblePinCategories, 72);
    const auto DataToExecutionResult = CategoryGraph.AddLink(MakeLink(
        2U, GetPins(NodeIdentifier(2U)).Output, GetPins(NodeIdentifier(1U)).Input
    ));
    Check(!DataToExecutionResult.has_value() &&
        DataToExecutionResult.error().Code == DiagnosticCode::IncompatiblePinCategories, 73);

    Graph TypeGraph(GraphIdentifier(5U), "Type Graph");
    Check(TypeGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 80);
    Check(TypeGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::String
    )).has_value(), 81);
    const auto IncompatibleResult = TypeGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!IncompatibleResult.has_value() &&
        IncompatibleResult.error().Code == DiagnosticCode::IncompatibleDataPinTypes, 82);
    Check(TypeGraph.GetLinks().empty(), 83);

    Graph DuplicateGraph(GraphIdentifier(6U), "Duplicate Graph");
    for (std::uint32_t Identifier = 1U; Identifier <= 3U; ++Identifier)
    {
        Check(DuplicateGraph.AddNode(MakeNode(
            NodeIdentifier(Identifier), EPinCategory::Data, EPinType::Integer
        )).has_value(), 90 + static_cast<int>(Identifier));
    }
    Check(DuplicateGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 94);
    const std::size_t DuplicateLinkCount = DuplicateGraph.GetLinks().size();
    const auto MultipleProducerResult = DuplicateGraph.AddLink(MakeLink(
        2U, GetPins(NodeIdentifier(3U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!MultipleProducerResult.has_value() &&
        MultipleProducerResult.error().Code == DiagnosticCode::MultipleDataInputProducers, 95);
    Check(DuplicateGraph.GetLinks().size() == DuplicateLinkCount, 96);
    const auto DuplicateResult = DuplicateGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!DuplicateResult.has_value() &&
        DuplicateResult.error().Code == DiagnosticCode::DuplicateLinkIdentifier, 97);
    const auto DuplicateEndpointResult = DuplicateGraph.AddLink(MakeLink(
        4U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    ));
    Check(!DuplicateEndpointResult.has_value() &&
        DuplicateEndpointResult.error().Code == DiagnosticCode::DuplicateLink, 98);
    const auto SelfLinkResult = DuplicateGraph.AddLink(MakeLink(
        3U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(1U)).Output
    ));
    Check(!SelfLinkResult.has_value() &&
        SelfLinkResult.error().Code == DiagnosticCode::SelfLink, 99);

    Check(DuplicateGraph.AddNode(MakeNode(
        NodeIdentifier(4U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 100);
    Check(DuplicateGraph.AddLink(MakeLink(
        5U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(4U)).Input
    )).has_value(), 101);

    Graph GenericGraph(GraphIdentifier(7U), "Generic Graph");
    Check(GenericGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 400);
    Check(GenericGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 401);
    Check(GenericGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 402);
    const DiagnosticCollection GenericDiagnostics = GenericGraph.PropagateTypes();
    Check(!ContainsError(GenericDiagnostics), 403);
    Check(GenericGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Integer, 404);
    Check(GenericGraph.PropagateTypes().empty(), 405);

    Graph StringGenericGraph(GraphIdentifier(13U), "String Generic Graph");
    Check(StringGenericGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 420);
    Check(StringGenericGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::String
    )).has_value(), 421);
    Check(StringGenericGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 422);
    Check(StringGenericGraph.PropagateTypes().empty(), 423);
    Check(StringGenericGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::String, 424);

    Graph GenericChainGraph(GraphIdentifier(14U), "Generic Chain Graph");
    Check(GenericChainGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 440);
    Check(GenericChainGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 441);
    Check(GenericChainGraph.AddNode(MakeNode(
        NodeIdentifier(3U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 442);
    Check(GenericChainGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 443);
    Check(GenericChainGraph.AddLink(MakeLink(
        2U, GetPins(NodeIdentifier(2U)).Output, GetPins(NodeIdentifier(3U)).Input
    )).has_value(), 444);
    Check(GenericChainGraph.PropagateTypes().empty(), 445);
    Check(GenericChainGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Integer, 446);
    Check(GenericChainGraph.FindPinByReference(GetPins(NodeIdentifier(2U)).Output)->
        GetEffectiveType() == EPinType::Integer, 447);

    Graph ConflictGraph(GraphIdentifier(8U), "Conflict Graph");
    Check(ConflictGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 460);
    Check(ConflictGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 461);
    Check(ConflictGraph.AddNode(MakeNode(
        NodeIdentifier(3U), EPinCategory::Data, EPinType::String
    )).has_value(), 462);
    Check(ConflictGraph.AddNode(MakeNode(
        NodeIdentifier(4U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 463);
    Check(ConflictGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 464);
    Check(ConflictGraph.AddLink(MakeLink(
        2U, GetPins(NodeIdentifier(4U)).Output, GetPins(NodeIdentifier(3U)).Input
    )).has_value(), 465);
    Check(ConflictGraph.AddLink(MakeLink(
        3U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(4U)).Input
    )).has_value(), 466);
    const DiagnosticCollection ConflictDiagnostics = ConflictGraph.PropagateTypes();
    Check(HasDiagnostic(ConflictDiagnostics, DiagnosticCode::TypePropagationConflict), 467);
    Check(ContainsError(ConflictDiagnostics), 468);
    Check(ConflictGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Generic, 469);

    Graph UnresolvedGraph(GraphIdentifier(9U), "Unresolved Graph");
    Check(UnresolvedGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic
    )).has_value(), 500);
    const DiagnosticCollection UnresolvedDiagnostics = UnresolvedGraph.PropagateTypes();
    Check(HasDiagnostic(UnresolvedDiagnostics, DiagnosticCode::UnresolvedGenericPinType), 501);
    Check(!ContainsError(UnresolvedDiagnostics), 502);

    Graph CompilerGraph(GraphIdentifier(10U), "Compiler Graph");
    Check(CompilerGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 600);
    Check(CompilerGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 601);
    Check(CompilerGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 602);
    Check(CompilerGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Generic, 603);
    Check(!Compiler::CompileToJSON(CompilerGraph).empty(), 604);
    Check(CompilerGraph.GetLinks().size() == 1U, 605);
    Check(CompilerGraph.GetNodes().size() == 2U, 606);
    Check(CompilerGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Generic, 607);
    DiagnosticCollection FileDiagnostics;
    Check(!Compiler::CompileToFile(
        CompilerGraph,
        "Z:\\MiliastraPlusPlus\\unavailable\\output.json",
        &FileDiagnostics
    ), 608);
    Check(HasDiagnostic(FileDiagnostics, DiagnosticCode::FileOpenFailure), 609);

    Graph InvalidCompilerGraph(GraphIdentifier(11U), "Invalid Compiler Graph");
    Check(InvalidCompilerGraph.AddNode(MakeNode(
        NodeIdentifier(1U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 700);
    Check(InvalidCompilerGraph.AddNode(MakeNode(
        NodeIdentifier(2U), EPinCategory::Data, EPinType::Integer
    )).has_value(), 701);
    Check(InvalidCompilerGraph.AddNode(MakeNode(
        NodeIdentifier(3U), EPinCategory::Data, EPinType::String
    )).has_value(), 702);
    Check(InvalidCompilerGraph.AddNode(MakeNode(
        NodeIdentifier(4U), EPinCategory::Data, EPinType::Generic, 1
    )).has_value(), 703);
    Check(InvalidCompilerGraph.AddLink(MakeLink(
        1U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(2U)).Input
    )).has_value(), 704);
    Check(InvalidCompilerGraph.AddLink(MakeLink(
        2U, GetPins(NodeIdentifier(4U)).Output, GetPins(NodeIdentifier(3U)).Input
    )).has_value(), 705);
    Check(InvalidCompilerGraph.AddLink(MakeLink(
        3U, GetPins(NodeIdentifier(1U)).Output, GetPins(NodeIdentifier(4U)).Input
    )).has_value(), 706);
    DiagnosticCollection CompilerDiagnostics;
    Check(Compiler::CompileToJSON(InvalidCompilerGraph, &CompilerDiagnostics).empty(), 707);
    Check(HasDiagnostic(CompilerDiagnostics, DiagnosticCode::TypePropagationConflict), 708);
    Check(InvalidCompilerGraph.GetLinks().size() == 3U, 709);
    Check(InvalidCompilerGraph.FindPinByReference(GetPins(NodeIdentifier(1U)).Output)->
        GetEffectiveType() == EPinType::Generic, 710);

    return EXIT_SUCCESS;
}
