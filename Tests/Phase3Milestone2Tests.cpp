#include <cstdlib>
#include <cstdio>
#include <map>
#include <source_location>
#include <string>
#include <type_traits>
#include <vector>

#include "MiliastraPlusPlusGraphBuilder.h"
#include "MiliastraPlusPlusGraphIRJson.h"

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
    constexpr NodeDescriptorId GenericListDescriptorId(1004U);

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
                    PinCategory::Data),
                PinSchema("BooleanOutput", TypeDesc::Boolean(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("BooleanInput", TypeDesc::Boolean(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("StringOutput", TypeDesc::String(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("StringInput", TypeDesc::String(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("GuidOutput", TypeDesc::GUID(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("GuidInput", TypeDesc::GUID(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("Vector3Output", TypeDesc::Vector3(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("Vector3Input", TypeDesc::Vector3(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("PrefabOutput", TypeDesc::PrefabId(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("PrefabInput", TypeDesc::PrefabId(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("ConfigOutput", TypeDesc::ConfigId(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("ConfigInput", TypeDesc::ConfigId(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("FactionOutput", TypeDesc::Faction(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("FactionInput", TypeDesc::Faction(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Single, true),
                PinSchema("EntityOutput", TypeDesc::Entity(), PinDirection::Output,
                    PinCategory::Data),
                PinSchema("EntityInput", TypeDesc::Entity(), PinDirection::Input,
                    PinCategory::Data),
                PinSchema("EntityListOutput", TypeDesc::List(TypeDesc::Entity()),
                    PinDirection::Output, PinCategory::Data),
                PinSchema("EntityListInput", TypeDesc::List(TypeDesc::Entity()),
                    PinDirection::Input, PinCategory::Data)
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

    NodeDescriptor MakeGenericListDescriptor()
    {
        return NodeDescriptor(
            GenericListDescriptorId,
            "GenericListNode",
            {NodeAvailability::Server},
            {
                PinSchema("ListGenericOutput",
                    TypeDesc::List(TypeDesc::Generic(GenericParameterId(1U))),
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
        MPP_CHECK(Descriptors.Register(MakeGenericListDescriptor()).has_value());
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

    template<typename T>
    void CheckTypedScalarBridge(
        const NodeDescriptorRegistry& Descriptors,
        const TypeDesc& ExpectedType,
        PinIndex OutputPin,
        PinIndex InputPin,
        const char* VariableName,
        T Value
    )
    {
        const auto TypeResult = GetCppTypeDesc<T>();
        MPP_CHECK(TypeResult.has_value());
        MPP_CHECK(*TypeResult == ExpectedType);

        const auto LiteralResult = MakeLiteralValue(std::move(Value));
        MPP_CHECK(LiteralResult.has_value());

        GraphBuilder Builder(Descriptors);
        const auto Source = Builder.AddNode(DescriptorId);
        const auto OutputTarget = Builder.AddNode(DescriptorId);
        const auto VariableTarget = Builder.AddNode(DescriptorId);
        const auto LiteralTarget = Builder.AddNode(DescriptorId);
        MPP_CHECK(Source.has_value());
        MPP_CHECK(OutputTarget.has_value());
        MPP_CHECK(VariableTarget.has_value());
        MPP_CHECK(LiteralTarget.has_value());

        const auto Output = Builder.GetOutput<T>(*Source, OutputPin);
        MPP_CHECK(Output.has_value());
        MPP_CHECK(Builder.BindInput(
            *OutputTarget,
            InputPin,
            ValueOrExpr<T>(*Output)
        ).has_value());

        const auto Variable = Builder.DeclareVariable<T>(VariableName, *LiteralResult);
        MPP_CHECK(Variable.has_value());
        MPP_CHECK(Builder.BindInput(
            *VariableTarget,
            InputPin,
            Variable->AsInput()
        ).has_value());
        MPP_CHECK(Builder.BindInput(
            *LiteralTarget,
            InputPin,
            ValueOrExpr<T>(*LiteralResult)
        ).has_value());
        MPP_CHECK(std::move(Builder).Finalize().has_value());
    }

    struct UnsupportedType
    {
    };
}

static_assert(std::is_copy_constructible_v<Output<int>>);
static_assert(std::is_copy_constructible_v<Variable<int>>);
static_assert(CppTypeDesc<int>::IsSupported);
static_assert(CppTypeDesc<std::vector<int>>::IsSupported);
static_assert(CppTypeDesc<EntityTypeTag>::IsSupported);
static_assert(CppTypeDesc<std::vector<EntityTypeTag>>::IsSupported);
static_assert(!std::is_constructible_v<LiteralValue::Data, EntityTypeTag>);
static_assert(!CppTypeDesc<std::map<int, int>>::IsSupported);
static_assert(!CppTypeDesc<GenericParameterId>::IsSupported);
static_assert(!CppTypeDesc<StructTypeId>::IsSupported);
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
    MPP_CHECK(GetCppTypeDesc<EntityTypeTag>()->GetKind() == TypeDesc::Kind::Entity);
    const auto EntityListType = GetCppTypeDesc<std::vector<EntityTypeTag>>();
    MPP_CHECK(EntityListType.has_value());
    MPP_CHECK(EntityListType->GetKind() == TypeDesc::Kind::List);
    MPP_CHECK(EntityListType->GetElementType()->GetKind() == TypeDesc::Kind::Entity);
    const auto ListType = GetCppTypeDesc<std::vector<int>>();
    MPP_CHECK(ListType.has_value());
    MPP_CHECK(ListType->GetKind() == TypeDesc::Kind::List);
    MPP_CHECK(!GetCppTypeDesc<UnsupportedType>().has_value());
    MPP_CHECK(HasCode(GetCppTypeDesc<UnsupportedType>().error(), DiagnosticCode::UnsupportedCppType));
    MPP_CHECK(!MakeLiteralValue(std::vector<int>{1, 2, 3}).has_value());
    const auto EntityLiteral = MakeLiteralValue(EntityTypeTag{});
    MPP_CHECK(!EntityLiteral.has_value());
    MPP_CHECK(HasCode(EntityLiteral.error(), DiagnosticCode::UnsupportedCppType));
    const auto EntityListLiteral = MakeLiteralValue(std::vector<EntityTypeTag>{});
    MPP_CHECK(!EntityListLiteral.has_value());
    MPP_CHECK(HasCode(EntityListLiteral.error(), DiagnosticCode::UnsupportedCppType));
    MPP_CHECK(HasCode(
        GetCppTypeDesc<std::map<int, int>>().error(),
        DiagnosticCode::UnsupportedCppType
    ));

    CheckTypedScalarBridge<bool>(Descriptors, TypeDesc::Boolean(),
        PinIndex(10U), PinIndex(11U), "Boolean", true);
    CheckTypedScalarBridge<int>(Descriptors, TypeDesc::Integer(),
        PinIndex(0U), PinIndex(3U), "Integer", 8);
    CheckTypedScalarBridge<std::int64_t>(Descriptors, TypeDesc::Integer(),
        PinIndex(0U), PinIndex(3U), "Integer64", std::int64_t{9});
    CheckTypedScalarBridge<float>(Descriptors, TypeDesc::Float(),
        PinIndex(1U), PinIndex(4U), "Float", 1.5F);
    CheckTypedScalarBridge<double>(Descriptors, TypeDesc::Float(),
        PinIndex(1U), PinIndex(4U), "Double", 2.5);
    CheckTypedScalarBridge<std::string>(Descriptors, TypeDesc::String(),
        PinIndex(12U), PinIndex(13U), "String", std::string("text"));
    CheckTypedScalarBridge<GuidValue>(Descriptors, TypeDesc::GUID(),
        PinIndex(14U), PinIndex(15U), "Guid", GuidValue{1U});
    CheckTypedScalarBridge<Vector3Value>(Descriptors, TypeDesc::Vector3(),
        PinIndex(16U), PinIndex(17U), "Vector3", Vector3Value{1.0F, 2.0F, 3.0F});
    CheckTypedScalarBridge<PrefabIdValue>(Descriptors, TypeDesc::PrefabId(),
        PinIndex(18U), PinIndex(19U), "Prefab", PrefabIdValue{1U});
    CheckTypedScalarBridge<ConfigIdValue>(Descriptors, TypeDesc::ConfigId(),
        PinIndex(20U), PinIndex(21U), "Config", ConfigIdValue{1U});
    CheckTypedScalarBridge<FactionValue>(Descriptors, TypeDesc::Faction(),
        PinIndex(22U), PinIndex(23U), "Faction", FactionValue{1U});

    {
        GraphBuilder EntityBuilder(Descriptors);
        const auto Source = EntityBuilder.AddNode(DescriptorId);
        const auto EntityTarget = EntityBuilder.AddNode(DescriptorId);
        const auto VariableTarget = EntityBuilder.AddNode(DescriptorId);
        const auto EntityListTarget = EntityBuilder.AddNode(DescriptorId);
        const auto EntityListVariableTarget = EntityBuilder.AddNode(DescriptorId);
        const auto BadLiteralTarget = EntityBuilder.AddNode(DescriptorId);
        MPP_CHECK(Source.has_value());
        MPP_CHECK(EntityTarget.has_value());
        MPP_CHECK(VariableTarget.has_value());
        MPP_CHECK(EntityListTarget.has_value());
        MPP_CHECK(EntityListVariableTarget.has_value());
        MPP_CHECK(BadLiteralTarget.has_value());

        const auto EntityOutput = EntityBuilder.GetOutput<EntityTypeTag>(
            *Source,
            PinIndex(24U)
        );
        MPP_CHECK(EntityOutput.has_value());
        const auto WrongEntityView = EntityBuilder.GetOutput<int>(
            *Source,
            PinIndex(24U)
        );
        MPP_CHECK(!WrongEntityView.has_value());
        MPP_CHECK(HasCode(WrongEntityView.error(), DiagnosticCode::IncompatibleGraphIRTypes));
        MPP_CHECK(EntityBuilder.BindInput(
            *EntityTarget,
            PinIndex(25U),
            ValueOrExpr<EntityTypeTag>(*EntityOutput)
        ).has_value());

        const auto EntityVariable = EntityBuilder.DeclareVariable<EntityTypeTag>("Actor");
        MPP_CHECK(EntityVariable.has_value());
        MPP_CHECK(EntityBuilder.BindInput(
            *VariableTarget,
            PinIndex(25U),
            EntityVariable->AsInput()
        ).has_value());
        const auto EntityVariableWithDefault = EntityBuilder.DeclareVariable<EntityTypeTag>(
            "ActorWithDefault",
            Literal(7)
        );
        MPP_CHECK(!EntityVariableWithDefault.has_value());
        MPP_CHECK(HasCode(
            EntityVariableWithDefault.error(),
            DiagnosticCode::IncompatibleGraphIRTypes
        ));

        const auto EntityListOutput = EntityBuilder.GetOutput<std::vector<EntityTypeTag>>(
            *Source,
            PinIndex(26U)
        );
        MPP_CHECK(EntityListOutput.has_value());
        MPP_CHECK(EntityBuilder.BindInput(
            *EntityListTarget,
            PinIndex(27U),
            ValueOrExpr<std::vector<EntityTypeTag>>(*EntityListOutput)
        ).has_value());

        const auto EntityListVariable = EntityBuilder.DeclareVariable<
            std::vector<EntityTypeTag>>("Actors");
        MPP_CHECK(EntityListVariable.has_value());
        MPP_CHECK(EntityBuilder.BindInput(
            *EntityListVariableTarget,
            PinIndex(27U),
            EntityListVariable->AsInput()
        ).has_value());

        const auto RawEntityLiteral = EntityBuilder.BindInput(
            *BadLiteralTarget,
            PinIndex(25U),
            ValueOrExpr<EntityTypeTag>(Literal(7))
        );
        MPP_CHECK(!RawEntityLiteral.has_value());
        MPP_CHECK(HasCode(
            RawEntityLiteral.error(),
            DiagnosticCode::InvalidInputBinding
        ));

        const auto FinalizedEntityGraph = std::move(EntityBuilder).Finalize();
        MPP_CHECK(FinalizedEntityGraph.has_value());
        MPP_CHECK(FinalizedEntityGraph->GetVariables().front().Type == TypeDesc::Entity());
        MPP_CHECK(FinalizedEntityGraph->GetVariables().front().DefaultValue == std::nullopt);
        MPP_CHECK(FinalizedEntityGraph->GetVariables()[1U].Type ==
            TypeDesc::List(TypeDesc::Entity()));
        MPP_CHECK(FinalizedEntityGraph->GetVariables()[1U].DefaultValue == std::nullopt);
        MPP_CHECK(FinalizedEntityGraph->GetInputBindings()[0U].OutputTypeConstraint ==
            TypeDesc::Entity());
        MPP_CHECK(FinalizedEntityGraph->GetInputBindings()[1U].OutputTypeConstraint ==
            std::nullopt);
        MPP_CHECK(FinalizedEntityGraph->GetInputBindings()[2U].OutputTypeConstraint ==
            TypeDesc::List(TypeDesc::Entity()));
        MPP_CHECK(FinalizedEntityGraph->GetInputBindings()[3U].OutputTypeConstraint ==
            std::nullopt);
        MPP_CHECK(GraphIRValidator::Validate(*FinalizedEntityGraph, Descriptors).empty());

        const auto EntityJson = GraphIRJson::Serialize(*FinalizedEntityGraph);
        MPP_CHECK(EntityJson["variables"][0U]["type"]["kind"] == "Entity");
        MPP_CHECK(!EntityJson["variables"][0U].contains("default"));
        MPP_CHECK(EntityJson["variables"][1U]["type"]["kind"] == "List");
        MPP_CHECK(EntityJson["variables"][1U]["type"]["element"]["kind"] == "Entity");
        MPP_CHECK(!EntityJson["variables"][1U].contains("default"));
        const auto EntityRoundTrip = GraphIRJson::Deserialize(EntityJson);
        MPP_CHECK(EntityRoundTrip.has_value());
        MPP_CHECK(EntityRoundTrip->GetVariables().front().Type == TypeDesc::Entity());
        MPP_CHECK(EntityRoundTrip->GetVariables()[1U].Type ==
            TypeDesc::List(TypeDesc::Entity()));
        MPP_CHECK(EntityRoundTrip->GetVariables()[1U].DefaultValue == std::nullopt);
        MPP_CHECK(EntityRoundTrip->GetInputBindings()[0U].OutputTypeConstraint ==
            TypeDesc::Entity());
        MPP_CHECK(EntityRoundTrip->GetInputBindings()[2U].OutputTypeConstraint ==
            TypeDesc::List(TypeDesc::Entity()));
        MPP_CHECK(GraphIRValidator::Validate(*EntityRoundTrip, Descriptors).empty());
    }

    {
        GraphBuilder GenericEntityBuilder(Descriptors);
        const auto GenericSource = GenericEntityBuilder.AddNode(GenericDescriptorId);
        const auto EntityTarget = GenericEntityBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(EntityTarget.has_value());
        const auto EntityOutput = GenericEntityBuilder.GetOutput<EntityTypeTag>(
            *GenericSource,
            PinIndex(1U)
        );
        MPP_CHECK(EntityOutput.has_value());
        MPP_CHECK(GenericEntityBuilder.BindInput(
            *EntityTarget,
            PinIndex(25U),
            ValueOrExpr<EntityTypeTag>(*EntityOutput)
        ).has_value());
        const auto Finalized = std::move(GenericEntityBuilder).Finalize();
        MPP_CHECK(Finalized.has_value());
        MPP_CHECK(Finalized->GetInputBindings().front().OutputTypeConstraint ==
            TypeDesc::Entity());
    }

    {
        GraphBuilder GenericEntityListBuilder(Descriptors);
        const auto GenericSource = GenericEntityListBuilder.AddNode(GenericListDescriptorId);
        const auto EntityListTarget = GenericEntityListBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(EntityListTarget.has_value());
        const auto EntityListOutput = GenericEntityListBuilder.GetOutput<
            std::vector<EntityTypeTag>>(*GenericSource, PinIndex(0U));
        MPP_CHECK(EntityListOutput.has_value());
        MPP_CHECK(GenericEntityListBuilder.BindInput(
            *EntityListTarget,
            PinIndex(27U),
            ValueOrExpr<std::vector<EntityTypeTag>>(*EntityListOutput)
        ).has_value());
        const auto Finalized = std::move(GenericEntityListBuilder).Finalize();
        MPP_CHECK(Finalized.has_value());
        MPP_CHECK(Finalized->GetInputBindings().front().OutputTypeConstraint ==
            TypeDesc::List(TypeDesc::Entity()));
    }

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

    const auto TypedLiteralMismatchTarget = Builder.AddNode(DescriptorId);
    MPP_CHECK(TypedLiteralMismatchTarget.has_value());
    const auto TypedLiteralMismatch = Builder.BindInput(
        *TypedLiteralMismatchTarget,
        PinIndex(4U),
        ValueOrExpr<int>(Literal(3.5))
    );
    MPP_CHECK(!TypedLiteralMismatch.has_value());
    MPP_CHECK(HasCode(
        TypedLiteralMismatch.error(),
        DiagnosticCode::IncompatibleGraphIRTypes
    ));

    const auto UnsupportedLiteralTarget = Builder.AddNode(DescriptorId);
    MPP_CHECK(UnsupportedLiteralTarget.has_value());
    const auto UnsupportedLiteralBinding = Builder.BindInput(
        *UnsupportedLiteralTarget,
        PinIndex(3U),
        ValueOrExpr<UnsupportedType>(Literal(7))
    );
    MPP_CHECK(!UnsupportedLiteralBinding.has_value());
    MPP_CHECK(HasCode(
        UnsupportedLiteralBinding.error(),
        DiagnosticCode::UnsupportedCppType
    ));

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
        GraphBuilder GenericFloatConflictBuilder(Descriptors);
        const auto GenericSource = GenericFloatConflictBuilder.AddNode(GenericDescriptorId);
        const auto FloatTarget = GenericFloatConflictBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(FloatTarget.has_value());
        const auto TypedIntegerOutput = GenericFloatConflictBuilder.GetOutput<int>(
            *GenericSource,
            PinIndex(1U)
        );
        MPP_CHECK(TypedIntegerOutput.has_value());
        MPP_CHECK(GenericFloatConflictBuilder.BindInput(
            *FloatTarget,
            PinIndex(4U),
            ValueOrExpr<int>(*TypedIntegerOutput)
        ).has_value());
        const auto FinalizedConflict = std::move(GenericFloatConflictBuilder).Finalize();
        MPP_CHECK(!FinalizedConflict.has_value());
        MPP_CHECK(HasCode(
            FinalizedConflict.error(),
            DiagnosticCode::GenericConstraintConflict
        ));
    }

    {
        GraphBuilder GenericFloatBuilder(Descriptors);
        const auto GenericSource = GenericFloatBuilder.AddNode(GenericDescriptorId);
        const auto FloatTarget = GenericFloatBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(FloatTarget.has_value());
        const auto TypedFloatOutput = GenericFloatBuilder.GetOutput<float>(
            *GenericSource,
            PinIndex(1U)
        );
        MPP_CHECK(TypedFloatOutput.has_value());
        MPP_CHECK(GenericFloatBuilder.BindInput(
            *FloatTarget,
            PinIndex(4U),
            ValueOrExpr<float>(*TypedFloatOutput)
        ).has_value());
        MPP_CHECK(std::move(GenericFloatBuilder).Finalize().has_value());
    }

    {
        GraphBuilder GenericDestinationBuilder(Descriptors);
        const auto GenericSource = GenericDestinationBuilder.AddNode(GenericDescriptorId);
        const auto GenericDestination = GenericDestinationBuilder.AddNode(GenericDescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(GenericDestination.has_value());
        const auto TypedIntegerOutput = GenericDestinationBuilder.GetOutput<int>(
            *GenericSource,
            PinIndex(1U)
        );
        MPP_CHECK(TypedIntegerOutput.has_value());
        MPP_CHECK(GenericDestinationBuilder.BindInput(
            *GenericDestination,
            PinIndex(0U),
            ValueOrExpr<int>(*TypedIntegerOutput)
        ).has_value());
        MPP_CHECK(std::move(GenericDestinationBuilder).Finalize().has_value());
    }

    {
        GraphBuilder GenericListBuilder(Descriptors);
        const auto GenericListSource = GenericListBuilder.AddNode(GenericListDescriptorId);
        const auto ListTarget = GenericListBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericListSource.has_value());
        MPP_CHECK(ListTarget.has_value());
        const auto TypedListOutput = GenericListBuilder.GetOutput<std::vector<int>>(
            *GenericListSource,
            PinIndex(0U)
        );
        MPP_CHECK(TypedListOutput.has_value());
        MPP_CHECK(GenericListBuilder.BindInput(
            *ListTarget,
            PinIndex(7U),
            ValueOrExpr<std::vector<int>>(*TypedListOutput)
        ).has_value());
        const auto FinalizedGenericList = std::move(GenericListBuilder).Finalize();
        MPP_CHECK(FinalizedGenericList.has_value());
        MPP_CHECK(FinalizedGenericList->GetInputBindings()[0U].OutputTypeConstraint ==
            TypeDesc::List(TypeDesc::Integer()));
        MPP_CHECK(GraphIRValidator::Validate(*FinalizedGenericList, Descriptors).empty());
    }

    {
        GraphBuilder MultipleOutputUseBuilder(Descriptors);
        const auto GenericSource = MultipleOutputUseBuilder.AddNode(GenericDescriptorId);
        const auto MultipleTarget = MultipleOutputUseBuilder.AddNode(DescriptorId);
        MPP_CHECK(GenericSource.has_value());
        MPP_CHECK(MultipleTarget.has_value());
        const auto IntegerView = MultipleOutputUseBuilder.GetOutput<int>(
            *GenericSource,
            PinIndex(1U)
        );
        const auto FloatView = MultipleOutputUseBuilder.GetOutput<float>(
            *GenericSource,
            PinIndex(1U)
        );
        MPP_CHECK(IntegerView.has_value());
        MPP_CHECK(FloatView.has_value());
        MPP_CHECK(MultipleOutputUseBuilder.BindInput(
            *MultipleTarget,
            PinIndex(5U),
            ValueOrExpr<int>(*IntegerView)
        ).has_value());
        MPP_CHECK(MultipleOutputUseBuilder.BindInput(
            *MultipleTarget,
            PinIndex(5U),
            ValueOrExpr<float>(*FloatView)
        ).has_value());
        const auto MultipleUseConflict = std::move(MultipleOutputUseBuilder).Finalize();
        MPP_CHECK(!MultipleUseConflict.has_value());
        MPP_CHECK(HasCode(
            MultipleUseConflict.error(),
            DiagnosticCode::GenericConstraintConflict
        ));
    }

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
    bool FoundGenericOutputConstraint = false;
    for (const InputBindingRecord& Record : Finalized->GetInputBindings())
    {
        const OutputReference* OutputReferenceValue =
            std::get_if<OutputReference>(&Record.Binding);
        MPP_CHECK(Record.OutputTypeConstraint.has_value() ==
            (OutputReferenceValue != nullptr));
        if (OutputReferenceValue != nullptr &&
            OutputReferenceValue->SourceNode == GenericNodeResult->GetIdentifier())
        {
            FoundGenericOutputConstraint =
                Record.OutputTypeConstraint == TypeDesc::Integer();
        }
    }
    MPP_CHECK(FoundGenericOutputConstraint);
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

    {
        NodeDescriptorRegistry MutableDescriptors = MakeRegistry();
        GraphBuilder MissingOutputBuilder(MutableDescriptors);
        const auto Source = MissingOutputBuilder.AddNode(DescriptorId);
        MPP_CHECK(Source.has_value());
        MutableDescriptors = NodeDescriptorRegistry{};
        const auto MissingOutput = MissingOutputBuilder.GetOutput<int>(
            *Source,
            PinIndex(0U)
        );
        MPP_CHECK(!MissingOutput.has_value());
        MPP_CHECK(HasCode(MissingOutput.error(), DiagnosticCode::MissingDescriptor));
    }

    {
        NodeDescriptorRegistry MutableDescriptors = MakeRegistry();
        GraphBuilder MissingDestinationBuilder(MutableDescriptors);
        const auto Source = MissingDestinationBuilder.AddNode(DescriptorId);
        const auto Destination = MissingDestinationBuilder.AddNode(OtherDescriptorId);
        MPP_CHECK(Source.has_value());
        MPP_CHECK(Destination.has_value());
        const auto Output = MissingDestinationBuilder.GetOutput<int>(
            *Source,
            PinIndex(0U)
        );
        MPP_CHECK(Output.has_value());

        NodeDescriptorRegistry Replacement;
        MPP_CHECK(Replacement.Register(MakeDescriptor(DescriptorId, "SourceOnly")).has_value());
        MutableDescriptors = std::move(Replacement);
        const auto Binding = MissingDestinationBuilder.BindInput(
            *Destination,
            PinIndex(3U),
            ValueOrExpr<int>(*Output)
        );
        MPP_CHECK(!Binding.has_value());
        MPP_CHECK(HasCode(Binding.error(), DiagnosticCode::MissingDescriptor));
    }

    {
        NodeDescriptorRegistry MutableDescriptors = MakeRegistry();
        GraphBuilder MissingSourceBuilder(MutableDescriptors);
        const auto Source = MissingSourceBuilder.AddNode(DescriptorId);
        const auto Destination = MissingSourceBuilder.AddNode(OtherDescriptorId);
        MPP_CHECK(Source.has_value());
        MPP_CHECK(Destination.has_value());
        const auto Output = MissingSourceBuilder.GetOutput<int>(
            *Source,
            PinIndex(0U)
        );
        MPP_CHECK(Output.has_value());

        NodeDescriptorRegistry Replacement;
        MPP_CHECK(Replacement.Register(MakeDescriptor(OtherDescriptorId, "DestinationOnly")).has_value());
        MutableDescriptors = std::move(Replacement);
        const auto Binding = MissingSourceBuilder.BindInput(
            *Destination,
            PinIndex(3U),
            ValueOrExpr<int>(*Output)
        );
        MPP_CHECK(!Binding.has_value());
        MPP_CHECK(HasCode(Binding.error(), DiagnosticCode::MissingDescriptor));
    }

    return 0;
}

#undef MPP_CHECK
