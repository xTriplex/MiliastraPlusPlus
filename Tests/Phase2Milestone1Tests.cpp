#include <cstdlib>
#include <cstdint>
#include <string>

#include "MiliastraPlusPlusValueTypes.h"

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
}

int main()
{
    Check(!GraphVariableId().IsValid(), 1);
    Check(GraphVariableId(7U).IsValid(), 2);
    Check(GraphVariableId(7U) == GraphVariableId(7U), 3);

    Check(!NodeInstanceId().IsValid(), 4);
    Check(NodeInstanceId(12U).IsValid(), 5);

    Check(!PinIndex().IsValid(), 6);
    Check(PinIndex(0U).IsValid(), 7);
    Check(PinIndex(2U) != PinIndex(3U), 8);

    const OutputReference ValidOutput{
        NodeInstanceId(12U),
        PinIndex(2U)
    };
    Check(ValidOutput.IsValid(), 9);
    Check(!OutputReference{ NodeInstanceId(), PinIndex(2U) }.IsValid(), 10);
    Check(!OutputReference{ NodeInstanceId(12U), PinIndex() }.IsValid(), 11);

    const GraphVariableReference ValidVariable{ GraphVariableId(4U) };
    Check(ValidVariable.IsValid(), 12);
    Check(!GraphVariableReference{ GraphVariableId() }.IsValid(), 13);

    const LiteralValue InvalidLiteral;
    Check(!InvalidLiteral.IsValid(), 14);

    const LiteralValue BooleanLiteral(LiteralValue::Data(true));
    const LiteralValue IntegerLiteral(LiteralValue::Data(std::int64_t(42)));
    const LiteralValue FloatLiteral(LiteralValue::Data(3.5));
    const LiteralValue StringLiteral(LiteralValue::Data(std::string("value")));
    const LiteralValue GuidLiteral(LiteralValue::Data(GuidValue{ 9U }));
    const LiteralValue VectorLiteral(LiteralValue::Data(Vector3Value{ 1.0F, 2.0F, 3.0F }));
    const LiteralValue PrefabLiteral(LiteralValue::Data(PrefabIdValue{ 10U }));
    const LiteralValue ConfigLiteral(LiteralValue::Data(ConfigIdValue{ 11U }));
    const LiteralValue FactionLiteral(LiteralValue::Data(FactionValue{ 12U }));

    Check(BooleanLiteral.Is<bool>(), 15);
    Check(IntegerLiteral.Is<std::int64_t>(), 16);
    Check(FloatLiteral.Is<double>(), 17);
    Check(StringLiteral.Is<std::string>(), 18);
    Check(GuidLiteral.Is<GuidValue>(), 19);
    Check(VectorLiteral.Is<Vector3Value>(), 20);
    Check(PrefabLiteral.Is<PrefabIdValue>(), 21);
    Check(ConfigLiteral.Is<ConfigIdValue>(), 22);
    Check(FactionLiteral.Is<FactionValue>(), 23);
    Check(IntegerLiteral.TryGet<std::int64_t>() != nullptr, 24);
    Check(IntegerLiteral.TryGet<std::string>() == nullptr, 25);
    Check(BooleanLiteral != IntegerLiteral, 26);

    const InputBinding LiteralBinding = IntegerLiteral;
    const InputBinding OutputBinding = ValidOutput;
    const InputBinding VariableBinding = ValidVariable;
    Check(std::holds_alternative<LiteralValue>(LiteralBinding), 27);
    Check(std::holds_alternative<OutputReference>(OutputBinding), 28);
    Check(std::holds_alternative<GraphVariableReference>(VariableBinding), 29);

    return EXIT_SUCCESS;
}
