#include <cstdlib>
#include <cstdio>
#include <source_location>
#include <string>
#include <type_traits>
#include <vector>

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

    constexpr NodeDescriptorId DescriptorId(1001U);
    constexpr NodeDescriptorId OtherDescriptorId(1002U);
    constexpr NodeDescriptorId GenericDescriptorId(1003U);

    NodeDescriptor MakeDescriptor(NodeDescriptorId Identifier, const char* Name)
    {
        return NodeDescriptor(
            Identifier,
            Name,
            {NodeAvailability::Server},
            {
                PinSchema("IntOutput", TypeDesc::Integer(), PinDirection::Output, PinCategory::Data),
                PinSchema("FloatOutput", TypeDesc::Float(), PinDirection::Output, PinCategory::Data),
                PinSchema("FlowOutput", TypeDesc::Flow(), PinDirection::Output, PinCategory::Execution),
                PinSchema("IntInput", TypeDesc::Integer(), PinDirection::Input, PinCategory::Data,
                    PinCardinality::Single, true),
                PinSchema("FloatInput", TypeDesc::Float(), PinDirection::Input, PinCategory::Data,
                    PinCardinality::Single, true),
                PinSchema("MultipleIntInput", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Multiple, true),
                PinSchema("ListOutput", TypeDesc::List(TypeDesc::Integer()), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("ListInput", TypeDesc::List(TypeDesc::Integer()), PinDirection::Input,
                    PinCategory::Data),
                PinSchema("FlowInput", TypeDesc::Flow(), PinDirection::Input, PinCategory::Execution)
                , PinSchema("NoLiteralInput", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data)
            }
        );
    }

    NodeDescriptor MakeGenericDescriptor()
    {
        return NodeDescriptor(
            GenericDescriptorId,
            "GenericNode",
            {NodeAvailability::Server},
            {
                PinSchema("GenericInput", TypeDesc::Generic(GenericParameterId(1U)),
                    PinDirection::Input, PinCategory::Data),
                PinSchema("GenericOutput", TypeDesc::Generic(GenericParameterId(1U)),
                    PinDirection::Output, PinCategory::Data)
            }
        );
    }

    NodeDescriptorRegistry MakeRegistry()
    {
        NodeDescriptorRegistry Descriptors;
        MPP_CHECK(Descriptors.Register(MakeDescriptor(DescriptorId, "TypedNode")).has_value());
        MPP_CHECK(Descriptors.Register(MakeDescriptor(OtherDescriptorId, "OtherTypedNode")).has_value());
        MPP_CHECK(Descriptors.Register(MakeGenericDescriptor()).has_value());
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

    template<typename T>
    LiteralValue Literal(T Value)
    {
        const auto Result = MakeLiteralValue(std::move(Value));
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    struct UnsupportedType
    {
    };
}

static_assert(std::is_copy_constructible_v<Output<int>>);
static_assert(std::is_copy_constructible_v<Variable<int>>);
static_assert(CppTypeDesc<int>::IsSupported);
static_assert(CppTypeDesc<std::vector<int>>::IsSupported);
static_assert(!CppTypeDesc<UnsupportedType>::IsSupported);

int main()
{
    NodeDescriptorRegistry Descriptors = MakeRegistry();

    MPP_CHECK(GetCppTypeDesc<bool>()->GetKind() == TypeDesc::Kind::Boolean);
    MPP_CHECK(GetCppTypeDesc<int>()->GetKind() == TypeDesc::Kind::Integer);
    MPP_CHECK(GetCppTypeDesc<float>()->GetKind() == TypeDesc::Kind::Float);
    MPP_CHECK(GetCppTypeDesc<double>()->GetKind() == TypeDesc::Kind::Float);
    MPP_CHECK(GetCppTypeDesc<std::string>()->GetKind() == TypeDesc::Kind::String);
    MPP_CHECK(GetCppTypeDesc<GuidValue>()->GetKind() == TypeDesc::Kind::GUID);
    MPP_CHECK(GetCppTypeDesc<Vector3Value>()->GetKind() == TypeDesc::Kind::Vector3);
    MPP_CHECK(GetCppTypeDesc<PrefabIdValue>()->GetKind() == TypeDesc::Kind::PrefabId);
    MPP_CHECK(GetCppTypeDesc<ConfigIdValue>()->GetKind() == TypeDesc::Kind::ConfigId);
    MPP_CHECK(GetCppTypeDesc<FactionValue>()->GetKind() == TypeDesc::Kind::Faction);
    const auto ListType = GetCppTypeDesc<std::vector<int>>();
    MPP_CHECK(ListType.has_value());
    MPP_CHECK(ListType->GetKind() == TypeDesc::Kind::List);
    MPP_CHECK(!GetCppTypeDesc<UnsupportedType>().has_value());
    MPP_CHECK(HasCode(GetCppTypeDesc<UnsupportedType>().error(), DiagnosticCode::UnsupportedCppType));
    MPP_CHECK(!MakeLiteralValue(std::vector<int>{1, 2, 3}).has_value());

    GraphBuilder Builder(Descriptors);
    const auto SourceResult = Builder.AddNode(DescriptorId);
    const auto TargetResult = Builder.AddNode(OtherDescriptorId);
    MPP_CHECK(SourceResult.has_value());
    MPP_CHECK(TargetResult.has_value());

    const auto IntOutputResult = Builder.GetOutput<int>(*SourceResult, PinIndex(0U));
    MPP_CHECK(IntOutputResult.has_value());
    MPP_CHECK(IntOutputResult->IsValid());
    MPP_CHECK(IntOutputResult->GetIdentifier() == SourceResult->GetIdentifier());
    MPP_CHECK(IntOutputResult->GetPin() == PinIndex(0U));

    MPP_CHECK(!Builder.GetOutput<float>(*SourceResult, PinIndex(0U)).has_value());
    MPP_CHECK(HasCode(
        Builder.GetOutput<float>(*SourceResult, PinIndex(0U)).error(),
        DiagnosticCode::IncompatibleGraphIRTypes));
    MPP_CHECK(!Builder.GetOutput<int>(*SourceResult, PinIndex(99U)).has_value());
    MPP_CHECK(HasCode(
        Builder.GetOutput<int>(*SourceResult, PinIndex(99U)).error(),
        DiagnosticCode::InvalidGraphIRPinReference));
    MPP_CHECK(!Builder.GetOutput<int>(*SourceResult, PinIndex(2U)).has_value());
    MPP_CHECK(HasCode(
        Builder.GetOutput<int>(*SourceResult, PinIndex(2U)).error(),
        DiagnosticCode::InvalidInputBinding));

    const auto IntExpression = ValueOrExpr<int>(*IntOutputResult);
    MPP_CHECK(Builder.BindInput(*TargetResult, PinIndex(3U), IntExpression).has_value());

    const auto DuplicateBinding = Builder.BindInput(
        *TargetResult,
        PinIndex(3U),
        ValueOrExpr<int>(Literal(7))
    );
    MPP_CHECK(!DuplicateBinding.has_value());
    MPP_CHECK(HasCode(DuplicateBinding.error(), DiagnosticCode::DuplicateInputBinding));

    const auto FloatTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(FloatTargetResult.has_value());
    const auto FloatMismatch = Builder.BindInput(
        *FloatTargetResult,
        PinIndex(4U),
        ValueOrExpr<int>(Literal(7))
    );
    MPP_CHECK(!FloatMismatch.has_value());
    MPP_CHECK(HasCode(FloatMismatch.error(), DiagnosticCode::IncompatibleGraphIRTypes));

    const auto TypedIntToFloatTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(TypedIntToFloatTargetResult.has_value());
    const auto TypedIntToFloatMismatch = Builder.BindInput(
        *TypedIntToFloatTargetResult,
        PinIndex(4U),
        ValueOrExpr<int>(*IntOutputResult)
    );
    MPP_CHECK(!TypedIntToFloatMismatch.has_value());
    MPP_CHECK(HasCode(
        TypedIntToFloatMismatch.error(),
        DiagnosticCode::IncompatibleGraphIRTypes
    ));

    const auto FloatOutputResult = Builder.GetOutput<float>(*SourceResult, PinIndex(1U));
    MPP_CHECK(FloatOutputResult.has_value());
    const auto TypedFloatToIntTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(TypedFloatToIntTargetResult.has_value());
    const auto TypedFloatToIntMismatch = Builder.BindInput(
        *TypedFloatToIntTargetResult,
        PinIndex(3U),
        ValueOrExpr<float>(*FloatOutputResult)
    );
    MPP_CHECK(!TypedFloatToIntMismatch.has_value());
    MPP_CHECK(HasCode(
        TypedFloatToIntMismatch.error(),
        DiagnosticCode::IncompatibleGraphIRTypes
    ));

    const auto LiteralTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(LiteralTargetResult.has_value());
    MPP_CHECK(Builder.BindInput(
        *LiteralTargetResult,
        PinIndex(3U),
        ValueOrExpr<int>(Literal(7))
    ).has_value());

    const auto FlowTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(FlowTargetResult.has_value());
    const auto FlowBinding = Builder.BindInput(
        *FlowTargetResult,
        PinIndex(8U),
        ValueOrExpr<int>(Literal(7))
    );
    MPP_CHECK(!FlowBinding.has_value());
    MPP_CHECK(HasCode(FlowBinding.error(), DiagnosticCode::InvalidInputBinding));

    const auto NoLiteralTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(NoLiteralTargetResult.has_value());
    const auto NoLiteralBinding = Builder.BindInput(
        *NoLiteralTargetResult,
        PinIndex(9U),
        ValueOrExpr<int>(Literal(7))
    );
    MPP_CHECK(!NoLiteralBinding.has_value());
    MPP_CHECK(HasCode(NoLiteralBinding.error(), DiagnosticCode::InvalidInputBinding));

    const auto MultipleTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(MultipleTargetResult.has_value());
    MPP_CHECK(Builder.BindInput(
        *MultipleTargetResult,
        PinIndex(5U),
        ValueOrExpr<int>(Literal(1))
    ).has_value());
    MPP_CHECK(Builder.BindInput(
        *MultipleTargetResult,
        PinIndex(5U),
        ValueOrExpr<int>(Literal(2))
    ).has_value());

    const auto ListOutputResult = Builder.GetOutput<std::vector<int>>(
        *SourceResult,
        PinIndex(6U)
    );
    MPP_CHECK(ListOutputResult.has_value());
    const auto ListTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(ListTargetResult.has_value());
    MPP_CHECK(Builder.BindInput(
        *ListTargetResult,
        PinIndex(7U),
        ValueOrExpr<std::vector<int>>(*ListOutputResult)
    ).has_value());
    const auto ScalarListTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(ScalarListTargetResult.has_value());
    const auto ScalarListMismatch = Builder.BindInput(
        *ScalarListTargetResult,
        PinIndex(3U),
        ValueOrExpr<std::vector<int>>(*ListOutputResult)
    );
    MPP_CHECK(!ScalarListMismatch.has_value());
    MPP_CHECK(HasCode(ScalarListMismatch.error(), DiagnosticCode::IncompatibleGraphIRTypes));

    const auto VariableResult = Builder.DeclareVariable<int>("Score", Literal(42));
    MPP_CHECK(VariableResult.has_value());
    MPP_CHECK(VariableResult->IsValid());
    MPP_CHECK(VariableResult->GetIdentifier() == GraphVariableId(1U));
    const auto VariableTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(VariableTargetResult.has_value());
    MPP_CHECK(Builder.BindInput(
        *VariableTargetResult,
        PinIndex(3U),
        VariableResult->AsInput()
    ).has_value());

    const auto DuplicateVariable = Builder.DeclareVariable<int>("Score");
    MPP_CHECK(!DuplicateVariable.has_value());
    MPP_CHECK(HasCode(DuplicateVariable.error(), DiagnosticCode::DuplicateGraphVariableName));
    const auto WrongDefault = Builder.DeclareVariable<int>("Wrong", Literal(1.0));
    MPP_CHECK(!WrongDefault.has_value());
    MPP_CHECK(HasCode(WrongDefault.error(), DiagnosticCode::IncompatibleGraphIRTypes));

    Output<int> DefaultOutput;
    Variable<int> DefaultVariable;
    const auto InvalidHandleTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(InvalidHandleTargetResult.has_value());
    MPP_CHECK(!Builder.BindInput(
        *InvalidHandleTargetResult,
        PinIndex(3U),
        ValueOrExpr<int>(DefaultOutput)
    ).has_value());
    MPP_CHECK(!Builder.BindInput(
        *InvalidHandleTargetResult,
        PinIndex(3U),
        ValueOrExpr<int>(DefaultVariable)
    ).has_value());

    const auto GenericNodeResult = Builder.AddNode(GenericDescriptorId);
    MPP_CHECK(GenericNodeResult.has_value());
    const auto GenericOutput = Builder.GetOutput<int>(*GenericNodeResult, PinIndex(1U));
    MPP_CHECK(GenericOutput.has_value());
    MPP_CHECK(Builder.BindInput(
        *GenericNodeResult,
        PinIndex(0U),
        ValueOrExpr<int>(*IntOutputResult)
    ).has_value());
    const auto GenericOutputTargetResult = Builder.AddNode(DescriptorId);
    MPP_CHECK(GenericOutputTargetResult.has_value());
    MPP_CHECK(Builder.BindInput(
        *GenericOutputTargetResult,
        PinIndex(3U),
        ValueOrExpr<int>(*GenericOutput)
    ).has_value());

    {
        GraphBuilder OtherBuilder(Descriptors);
        MPP_CHECK(!OtherBuilder.BindInput(
            *TargetResult,
            PinIndex(3U),
            IntExpression
        ).has_value());
        MPP_CHECK(!OtherBuilder.BindInput(
            *TargetResult,
            PinIndex(3U),
            VariableResult->AsInput()
        ).has_value());
    }

    auto Finalized = std::move(Builder).Finalize();
    MPP_CHECK(Finalized.has_value());
    MPP_CHECK(!IntOutputResult->IsValid());
    MPP_CHECK(!VariableResult->IsValid());
    MPP_CHECK(Finalized->GetVariableCount() == 1U);
    MPP_CHECK(Finalized->GetInputBindingCount() == 8U);
    MPP_CHECK(GraphIRValidator::Validate(*Finalized, Descriptors).empty());

    {
        NodeHandle SourceHandle;
        Output<int> DestroyedOutput;
        Variable<int> DestroyedVariable;
        {
            GraphBuilder TemporaryBuilder(Descriptors);
            const auto Node = TemporaryBuilder.AddNode(DescriptorId);
            MPP_CHECK(Node.has_value());
            SourceHandle = *Node;
            DestroyedOutput = *TemporaryBuilder.GetOutput<int>(*Node, PinIndex(0U));
            DestroyedVariable = *TemporaryBuilder.DeclareVariable<int>("Temporary");
        }
        MPP_CHECK(!SourceHandle.IsValid());
        MPP_CHECK(!DestroyedOutput.IsValid());
        MPP_CHECK(!DestroyedVariable.IsValid());
    }

    {
        GraphBuilder MoveSource(Descriptors);
        const auto Node = MoveSource.AddNode(DescriptorId);
        const auto VariableValue = MoveSource.DeclareVariable<int>("Moved");
        MPP_CHECK(Node.has_value());
        MPP_CHECK(VariableValue.has_value());
        const auto OutputValue = MoveSource.GetOutput<int>(*Node, PinIndex(0U));
        MPP_CHECK(OutputValue.has_value());
        GraphBuilder MoveDestination(std::move(MoveSource));
        MPP_CHECK(MoveDestination.IsHandleUsable(*Node));
        MPP_CHECK(MoveDestination.BindInput(
            *Node,
            PinIndex(3U),
            ValueOrExpr<int>(*OutputValue)
        ).has_value());
        MPP_CHECK(MoveDestination.BindInput(
            *Node,
            PinIndex(5U),
            VariableValue->AsInput()
        ).has_value());
        MPP_CHECK(!MoveSource.AddNode(DescriptorId).has_value());
        MPP_CHECK(OutputValue->IsValid());
        MPP_CHECK(VariableValue->IsValid());
    }

    return 0;
}

#undef MPP_CHECK
