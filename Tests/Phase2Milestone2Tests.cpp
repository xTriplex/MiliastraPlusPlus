#include <cstdlib>
#include <cstdint>

#include "MiliastraPlusPlusTypeDesc.h"

using namespace MiliastraPlusPlus;

namespace
{
    void Check(bool Condition, int FailureCode)
    {
        if (!Condition)
        {
            std::exit(FailureCode);
        }
    }

    void CheckUnifies(const TypeDesc& FirstType, const TypeDesc& SecondType, const TypeDesc& ExpectedType,
        int FailureCode)
    {
        const auto Result = FirstType.Unify(SecondType);
        Check(Result.has_value() && Result.value() == ExpectedType, FailureCode);
    }
}

int main()
{
    Check(!TypeDesc().IsValid(), 1);

    const TypeDesc PrimitiveTypes[] = {
        TypeDesc::Boolean(), TypeDesc::Integer(), TypeDesc::Float(), TypeDesc::String(),
        TypeDesc::Flow(), TypeDesc::Entity(), TypeDesc::GUID(), TypeDesc::Vector3(),
        TypeDesc::PrefabId(), TypeDesc::ConfigId(), TypeDesc::Faction()
    };
    for (const TypeDesc& PrimitiveType : PrimitiveTypes)
    {
        Check(PrimitiveType.IsValid(), 2);
    }

    const TypeDesc GenericType = TypeDesc::Generic(GenericParameterId(1U));
    const TypeDesc OtherGenericType = TypeDesc::Generic(GenericParameterId(2U));
    Check(GenericType.IsValid(), 3);
    Check(!TypeDesc::Generic(GenericParameterId()).IsValid(), 4);
    Check(GenericType == TypeDesc::Generic(GenericParameterId(1U)), 5);
    Check(GenericType != OtherGenericType, 6);

    const TypeDesc IntegerList = TypeDesc::List(TypeDesc::Integer());
    const TypeDesc AnotherIntegerList = TypeDesc::List(TypeDesc::Integer());
    const TypeDesc FloatList = TypeDesc::List(TypeDesc::Float());
    Check(IntegerList.IsValid(), 7);
    Check(IntegerList.GetElementType()->GetKind() == TypeDesc::Kind::Integer, 8);
    Check(IntegerList == AnotherIntegerList, 9);
    Check(IntegerList != FloatList, 10);
    Check(!TypeDesc::List(TypeDesc()).IsValid(), 11);

    const TypeDesc StringFloatDictionary = TypeDesc::Dictionary(
        TypeDesc::String(), TypeDesc::Float()
    );
    Check(StringFloatDictionary.IsValid(), 12);
    Check(StringFloatDictionary.GetKeyType()->GetKind() == TypeDesc::Kind::String, 13);
    Check(StringFloatDictionary.GetValueType()->GetKind() == TypeDesc::Kind::Float, 14);
    Check(StringFloatDictionary == TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Float()), 15);
    Check(StringFloatDictionary != TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Integer()), 16);

    const TypeDesc ObjectType = TypeDesc::StructObject(StructTypeId(1U));
    Check(ObjectType.IsValid(), 17);
    Check(ObjectType.GetKind() == TypeDesc::Kind::StructObject, 18);
    Check(ObjectType != TypeDesc::Integer(), 19);
    Check(!TypeDesc::StructObject(StructTypeId()).IsValid(), 20);
    Check(ObjectType.IsCompatibleWith(ObjectType), 38);
    Check(!ObjectType.IsCompatibleWith(TypeDesc::StructObject(StructTypeId(2U))), 39);
    Check(!ObjectType.IsCompatibleWith(TypeDesc::Integer()), 40);
    CheckUnifies(ObjectType, ObjectType, ObjectType, 41);
    Check(!ObjectType.Unify(TypeDesc::StructObject(StructTypeId(2U))).has_value(), 42);
    Check(!ObjectType.Unify(TypeDesc::Integer()).has_value(), 43);

    Check(TypeDesc::Integer().IsCompatibleWith(TypeDesc::Integer()), 21);
    Check(!TypeDesc::Integer().IsCompatibleWith(TypeDesc::Float()), 22);
    Check(GenericType.IsCompatibleWith(TypeDesc::Integer()), 23);
    Check(TypeDesc::Integer().IsCompatibleWith(GenericType), 24);
    Check(IntegerList.IsCompatibleWith(AnotherIntegerList), 25);
    Check(!IntegerList.IsCompatibleWith(FloatList), 26);
    Check(StringFloatDictionary.IsCompatibleWith(
        TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Float())), 27);
    Check(!StringFloatDictionary.IsCompatibleWith(
        TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Integer())), 28);

    CheckUnifies(GenericType, TypeDesc::Integer(), TypeDesc::Integer(), 29);
    CheckUnifies(GenericType, TypeDesc::Float(), TypeDesc::Float(), 30);
    CheckUnifies(GenericType, GenericType, GenericType, 31);
    CheckUnifies(IntegerList, TypeDesc::List(GenericType), IntegerList, 32);
    CheckUnifies(
        StringFloatDictionary,
        TypeDesc::Dictionary(TypeDesc::String(), GenericType),
        StringFloatDictionary,
        33
    );
    Check(!GenericType.Unify(OtherGenericType).has_value(), 34);
    Check(!TypeDesc::Integer().Unify(TypeDesc::Float()).has_value(), 35);
    Check(!IntegerList.Unify(FloatList).has_value(), 36);
    Check(!TypeDesc().Unify(TypeDesc::Integer()).has_value(), 37);

    return EXIT_SUCCESS;
}
