#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "MiliastraPlusPlusDescriptors.h"

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

    NodeDescriptor MakeDescriptor(NodeDescriptorId Identifier, std::string Name)
    {
        return NodeDescriptor(
            Identifier,
            std::move(Name),
            { NodeAvailability::Server, NodeAvailability::Client },
            {
                PinSchema(
                    "Input", TypeDesc::Integer(), PinDirection::Input,
                    PinCategory::Data, PinCardinality::Optional, true,
                    LiteralValue(LiteralValue::Data(std::int64_t(7)))
                ),
                PinSchema(
                    "Output", TypeDesc::Integer(), PinDirection::Output,
                    PinCategory::Data, PinCardinality::Single
                )
            }
        );
    }
}

int main()
{
    Check(!NodeDescriptorId().IsValid(), 1);
    Check(NodeDescriptorId(1U).IsValid(), 2);

    const PinSchema InputSchema(
        "Input", TypeDesc::Integer(), PinDirection::Input, PinCategory::Data,
        PinCardinality::Optional, true,
        LiteralValue(LiteralValue::Data(std::int64_t(7)))
    );
    Check(InputSchema.GetDirection() == PinDirection::Input, 3);
    Check(InputSchema.GetCategory() == PinCategory::Data, 4);
    Check(InputSchema.GetCardinality() == PinCardinality::Optional, 5);
    Check(InputSchema.GetType() == TypeDesc::Integer(), 6);
    Check(InputSchema.AllowsLiteral(), 7);
    Check(InputSchema.GetDefaultValue().has_value(), 8);

    const PinSchema OutputSchema(
        "Output", TypeDesc::List(TypeDesc::Integer()), PinDirection::Output,
        PinCategory::Data, PinCardinality::Multiple
    );
    Check(OutputSchema.GetDirection() == PinDirection::Output, 9);
    Check(OutputSchema.GetCardinality() == PinCardinality::Multiple, 10);
    Check(!OutputSchema.AllowsLiteral(), 11);
    Check(!OutputSchema.GetDefaultValue().has_value(), 12);

    const NodeDescriptor Descriptor = MakeDescriptor(NodeDescriptorId(10U), "SampleNode");
    Check(Descriptor.IsValid(), 13);
    Check(Descriptor.GetIdentifier() == NodeDescriptorId(10U), 14);
    Check(Descriptor.GetName() == "SampleNode", 15);
    Check(Descriptor.IsAvailableOn(NodeAvailability::Server), 16);
    Check(Descriptor.IsAvailableOn(NodeAvailability::Client), 17);
    Check(Descriptor.GetPins().size() == 2U, 18);
    Check(Descriptor.GetPins()[0].GetName() == "Input", 19);
    Check(Descriptor.GetPins()[1].GetName() == "Output", 20);
    Check(Descriptor.FindPinByName("Input") != nullptr, 21);
    Check(Descriptor.FindPinByName("Missing") == nullptr, 22);

    NodeDescriptorRegistry Registry;
    Check(Registry.Size() == 0U, 23);
    Check(Registry.Register(Descriptor).has_value(), 24);
    Check(Registry.Size() == 1U, 25);
    Check(Registry.Find(NodeDescriptorId(10U)) != nullptr, 26);
    Check(Registry.Get(NodeDescriptorId(10U)).has_value(), 27);
    Check(Registry.Register(MakeDescriptor(NodeDescriptorId(10U), "Duplicate")).error().Code ==
        DiagnosticCode::DuplicateDescriptor, 28);
    Check(!Registry.Get(NodeDescriptorId(99U)).has_value(), 29);
    Check(Registry.Get(NodeDescriptorId(99U)).error().Code == DiagnosticCode::MissingDescriptor, 30);

    NodeDescriptorRegistry IndependentRegistry;
    Check(IndependentRegistry.Register(MakeDescriptor(NodeDescriptorId(10U), "Independent")).has_value(), 31);
    Check(IndependentRegistry.Size() == 1U, 32);
    Check(Registry.Size() == 1U, 33);
    Check(IndependentRegistry.Find(NodeDescriptorId(10U))->GetName() == "Independent", 34);

    Check(!NodeDescriptor(NodeDescriptorId(), "Invalid", {}, {}).IsValid(), 35);
    Check(!NodeDescriptor(NodeDescriptorId(11U), "", {}, {}).IsValid(), 36);

    const NodeDescriptor InvalidPinTypeDescriptor(
        NodeDescriptorId(12U),
        "InvalidPinType",
        {},
        {
            PinSchema("Invalid", TypeDesc(), PinDirection::Output, PinCategory::Data)
        }
    );
    Check(!InvalidPinTypeDescriptor.IsValid(), 37);

    const NodeDescriptor InvalidNestedPinTypeDescriptor(
        NodeDescriptorId(13U),
        "InvalidNestedPinType",
        {},
        {
            PinSchema(
                "InvalidNested",
                TypeDesc::List(TypeDesc::Generic(GenericParameterId{})),
                PinDirection::Output,
                PinCategory::Data
            )
        }
    );
    Check(!InvalidNestedPinTypeDescriptor.IsValid(), 38);

    NodeDescriptorRegistry InvalidPinTypeRegistry;
    const auto InvalidPinTypeRegistration = InvalidPinTypeRegistry.Register(
        InvalidNestedPinTypeDescriptor
    );
    Check(!InvalidPinTypeRegistration.has_value(), 39);
    Check(InvalidPinTypeRegistration.error().Code == DiagnosticCode::InvalidNodeDescriptor, 40);

    const NodeDescriptor FlowDataPinDescriptor(
        NodeDescriptorId(14U),
        "FlowDataPin",
        {},
        {
            PinSchema(
                "FlowData",
                TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Flow()),
                PinDirection::Output,
                PinCategory::Data
            )
        }
    );
    Check(!FlowDataPinDescriptor.IsValid(), 41);

    return EXIT_SUCCESS;
}
