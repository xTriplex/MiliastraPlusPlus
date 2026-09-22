#include <algorithm>
#include <array>
#include <bit>
#include <compare>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "MiliastraPlusPlusDescriptorCatalogue.h"

using namespace MiliastraPlusPlus;

namespace
{
    [[noreturn]] void Fail(const char* Expression, const std::source_location& Location)
    {
        std::fprintf(stderr, "Check failed: %s (%s:%u)\n", Expression,
            Location.file_name(), Location.line());
        std::exit(EXIT_FAILURE);
    }

    void Check(bool Condition, const char* Expression, const std::source_location& Location = std::source_location::current())
    {
        if (!Condition)
        {
            Fail(Expression, Location);
        }
    }

#define MPP_CHECK(Condition) Check((Condition), #Condition, std::source_location::current())

    template<typename Type>
    concept HasPublicThreeWayComparison = requires(
        const Type& Left,
        const Type& Right
    )
    {
        Left <=> Right;
    };

    static_assert(!HasPublicThreeWayComparison<NormalizedPinRecord>);

    template<typename Type>
    LiteralValue MakeLiteral(Type Value)
    {
        using ValueType = std::decay_t<Type>;
        return LiteralValue(LiteralValue::Data(
            std::in_place_type<ValueType>,
            std::move(Value)
        ));
    }

    SourceProvenance MakeProvenance(std::string SourceDocumentIdentifier, std::string SourceRecordIdentifier)
    {
        return SourceProvenance(
            std::move(SourceDocumentIdentifier),
            std::move(SourceRecordIdentifier)
        );
    }

    NormalizedPinRecord MakeDataPin(
        std::string Name,
        TypeDesc Type,
        bool AllowsLiteral = false,
        std::optional<LiteralValue> DefaultValue = std::nullopt,
        PinCardinality Cardinality = PinCardinality::Single,
        PinDirection Direction = PinDirection::Input
    )
    {
        return NormalizedPinRecord(
            std::move(Name),
            std::move(Type),
            Direction,
            PinCategory::Data,
            Cardinality,
            AllowsLiteral,
            std::move(DefaultValue)
        );
    }

    NormalizedPinRecord MakeExecutionPin(std::string Name, PinDirection Direction, PinCardinality Cardinality = PinCardinality::Single)
    {
        return NormalizedPinRecord(
            std::move(Name),
            TypeDesc::Flow(),
            Direction,
            PinCategory::Execution,
            Cardinality
        );
    }

    NormalizedNodeDescriptorRecord MakeRecord(
        std::string ExternalKey,
        std::string DisplayName = "DisplayName",
        std::vector<NodeAvailability> Availability = {},
        std::vector<NormalizedPinRecord> Pins = {},
        std::optional<ExecutionControlSchema> ControlSchema = std::nullopt,
        std::optional<SourceProvenance> Provenance = std::nullopt
    )
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(ExternalKey)),
            std::move(DisplayName),
            std::move(Availability),
            std::move(Pins),
            std::move(ControlSchema),
            std::move(Provenance)
        );
    }

    std::optional<ExecutionControlSchema> MakeEntryControl(std::uint32_t ExecutionOutput)
    {
        return ExecutionControlSchema(EntryControlSchema{PinIndex(ExecutionOutput)});
    }

    std::optional<ExecutionControlSchema> MakeSequenceControl(std::uint32_t ExecutionInput, std::uint32_t ExecutionOutput)
    {
        return ExecutionControlSchema(SequenceControlSchema{
            PinIndex(ExecutionInput), PinIndex(ExecutionOutput)
        });
    }

    std::optional<ExecutionControlSchema> MakeBranchControl(std::uint32_t ExecutionInput, std::uint32_t ConditionInput, std::uint32_t TrueOutput, std::uint32_t FalseOutput)
    {
        return ExecutionControlSchema(BranchControlSchema{
            PinIndex(ExecutionInput),
            PinIndex(ConditionInput),
            PinIndex(TrueOutput),
            PinIndex(FalseOutput)
        });
    }

    std::optional<ExecutionControlSchema> MakeJoinControl(std::uint32_t ExecutionInput, std::uint32_t ExecutionOutput)
    {
        return ExecutionControlSchema(JoinControlSchema{
            PinIndex(ExecutionInput), PinIndex(ExecutionOutput)
        });
    }

    std::optional<ExecutionControlSchema> MakeLoopControl(bool Conditional, std::uint32_t ConditionInput = 0U)
    {
        return ExecutionControlSchema(LoopControlSchema{
            .ExecutionInput = PinIndex(0U),
            .BodyOutput = PinIndex(1U),
            .ExitOutput = PinIndex(2U),
            .RepeatInput = PinIndex(3U),
            .BreakInput = PinIndex(4U),
            .ExitPolicy = Conditional
                ? LoopExitPolicy::Conditional
                : LoopExitPolicy::Unconditional,
            .ConditionInput = Conditional
                ? std::optional<PinIndex>(PinIndex(ConditionInput))
                : std::nullopt
        });
    }

    std::optional<ExecutionControlSchema> MakeReturnControl(std::uint32_t ExecutionInput)
    {
        return ExecutionControlSchema(ReturnControlSchema{PinIndex(ExecutionInput)});
    }

    NormalizedNodeDescriptorRecord MakeLoopRecord(std::string ExternalKey, bool Conditional)
    {
        std::vector<NormalizedPinRecord> Pins;
        Pins.push_back(MakeExecutionPin("ExecutionInput", PinDirection::Input));
        Pins.push_back(MakeExecutionPin("BodyOutput", PinDirection::Output));
        Pins.push_back(MakeExecutionPin("ExitOutput", PinDirection::Output));
        Pins.push_back(MakeExecutionPin(
            "RepeatInput", PinDirection::Input, PinCardinality::Multiple));
        Pins.push_back(MakeExecutionPin(
            "BreakInput", PinDirection::Input, PinCardinality::Multiple));
        if (Conditional)
        {
            Pins.push_back(MakeDataPin("ConditionInput", TypeDesc::Boolean()));
        }

        return MakeRecord(
            std::move(ExternalKey),
            "DisplayName",
            {},
            std::move(Pins),
            MakeLoopControl(Conditional, 5U)
        );
    }

    bool HasDiagnosticCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return std::any_of(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& CurrentDiagnostic)
            {
                return CurrentDiagnostic.Code == Code;
            }
        );
    }

    std::size_t CountDiagnosticCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        return static_cast<std::size_t>(std::count_if(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& CurrentDiagnostic)
            {
                return CurrentDiagnostic.Code == Code;
            }
        ));
    }

    bool SameDiagnostics(const DiagnosticCollection& Left, const DiagnosticCollection& Right)
    {
        if (Left.size() != Right.size())
        {
            return false;
        }

        for (std::size_t Index = 0U; Index < Left.size(); ++Index)
        {
            if (Left[Index].Severity != Right[Index].Severity ||
                Left[Index].Code != Right[Index].Code ||
                Left[Index].Message != Right[Index].Message ||
                Left[Index].PrimarySourceProvenance != Right[Index].PrimarySourceProvenance ||
                Left[Index].RelatedSourceProvenance != Right[Index].RelatedSourceProvenance)
            {
                return false;
            }
        }

        return true;
    }

    void CheckDigest(const std::vector<NormalizedNodeDescriptorRecord>& Records, const char* ExpectedDigest)
    {
        const auto Result = DeriveDescriptorCatalogueContentIdentifier(Records);
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetValue() == ExpectedDigest);
    }

    std::uint32_t RotateRightForIndependentHash(std::uint32_t Value, std::uint32_t Shift)
    {
        return (Value >> Shift) | (Value << (32U - Shift));
    }

    std::string ComputeIndependentSha256(const std::string& Input)
    {
        static constexpr std::array<std::uint32_t, 64U> Constants = {
            0x428A2F98U, 0x71374491U, 0xB5C0FBCFU, 0xE9B5DBA5U,
            0x3956C25BU, 0x59F111F1U, 0x923F82A4U, 0xAB1C5ED5U,
            0xD807AA98U, 0x12835B01U, 0x243185BEU, 0x550C7DC3U,
            0x72BE5D74U, 0x80DEB1FEU, 0x9BDC06A7U, 0xC19BF174U,
            0xE49B69C1U, 0xEFBE4786U, 0x0FC19DC6U, 0x240CA1CCU,
            0x2DE92C6FU, 0x4A7484AAU, 0x5CB0A9DCU, 0x76F988DAU,
            0x983E5152U, 0xA831C66DU, 0xB00327C8U, 0xBF597FC7U,
            0xC6E00BF3U, 0xD5A79147U, 0x06CA6351U, 0x14292967U,
            0x27B70A85U, 0x2E1B2138U, 0x4D2C6DFCU, 0x53380D13U,
            0x650A7354U, 0x766A0ABBU, 0x81C2C92EU, 0x92722C85U,
            0xA2BFE8A1U, 0xA81A664BU, 0xC24B8B70U, 0xC76C51A3U,
            0xD192E819U, 0xD6990624U, 0xF40E3585U, 0x106AA070U,
            0x19A4C116U, 0x1E376C08U, 0x2748774CU, 0x34B0BCB5U,
            0x391C0CB3U, 0x4ED8AA4AU, 0x5B9CCA4FU, 0x682E6FF3U,
            0x748F82EEU, 0x78A5636FU, 0x84C87814U, 0x8CC70208U,
            0x90BEFFFAU, 0xA4506CEBU, 0xBEF9A3F7U, 0xC67178F2U
        };

        std::vector<std::uint8_t> Bytes;
        for (const char Character : Input)
        {
            Bytes.push_back(static_cast<std::uint8_t>(
                static_cast<unsigned char>(Character)));
        }
        Bytes.push_back(0x80U);
        while ((Bytes.size() + 8U) % 64U != 0U)
        {
            Bytes.push_back(0x00U);
        }
        const std::uint64_t BitLength = static_cast<std::uint64_t>(Input.size()) * 8U;
        for (std::uint32_t Shift = 56U;; Shift -= 8U)
        {
            Bytes.push_back(static_cast<std::uint8_t>((BitLength >> Shift) & 0xFFU));
            if (Shift == 0U)
            {
                break;
            }
        }

        std::array<std::uint32_t, 8U> Hash = {
            0x6A09E667U, 0xBB67AE85U, 0x3C6EF372U, 0xA54FF53AU,
            0x510E527FU, 0x9B05688CU, 0x1F83D9ABU, 0x5BE0CD19U
        };

        for (std::size_t Block = 0U; Block < Bytes.size(); Block += 64U)
        {
            std::array<std::uint32_t, 64U> Words{};
            for (std::size_t Index = 0U; Index < 16U; ++Index)
            {
                const std::size_t Offset = Block + Index * 4U;
                Words[Index] =
                    (static_cast<std::uint32_t>(Bytes[Offset]) << 24U) |
                    (static_cast<std::uint32_t>(Bytes[Offset + 1U]) << 16U) |
                    (static_cast<std::uint32_t>(Bytes[Offset + 2U]) << 8U) |
                    static_cast<std::uint32_t>(Bytes[Offset + 3U]);
            }
            for (std::size_t Index = 16U; Index < Words.size(); ++Index)
            {
                const std::uint32_t First = Words[Index - 15U];
                const std::uint32_t Second = Words[Index - 2U];
                const std::uint32_t SmallZero =
                    RotateRightForIndependentHash(First, 7U) ^
                    RotateRightForIndependentHash(First, 18U) ^ (First >> 3U);
                const std::uint32_t SmallOne =
                    RotateRightForIndependentHash(Second, 17U) ^
                    RotateRightForIndependentHash(Second, 19U) ^ (Second >> 10U);
                Words[Index] = Words[Index - 16U] + SmallZero +
                    Words[Index - 7U] + SmallOne;
            }

            std::uint32_t A = Hash[0U];
            std::uint32_t B = Hash[1U];
            std::uint32_t C = Hash[2U];
            std::uint32_t D = Hash[3U];
            std::uint32_t E = Hash[4U];
            std::uint32_t F = Hash[5U];
            std::uint32_t G = Hash[6U];
            std::uint32_t H = Hash[7U];
            for (std::size_t Index = 0U; Index < Words.size(); ++Index)
            {
                const std::uint32_t BigOne =
                    RotateRightForIndependentHash(E, 6U) ^
                    RotateRightForIndependentHash(E, 11U) ^
                    RotateRightForIndependentHash(E, 25U);
                const std::uint32_t Choose = (E & F) ^ ((~E) & G);
                const std::uint32_t First = H + BigOne + Choose + Constants[Index] +
                    Words[Index];
                const std::uint32_t BigZero =
                    RotateRightForIndependentHash(A, 2U) ^
                    RotateRightForIndependentHash(A, 13U) ^
                    RotateRightForIndependentHash(A, 22U);
                const std::uint32_t Majority = (A & B) ^ (A & C) ^ (B & C);
                const std::uint32_t Second = BigZero + Majority;

                H = G;
                G = F;
                F = E;
                E = D + First;
                D = C;
                C = B;
                B = A;
                A = First + Second;
            }

            Hash[0U] += A;
            Hash[1U] += B;
            Hash[2U] += C;
            Hash[3U] += D;
            Hash[4U] += E;
            Hash[5U] += F;
            Hash[6U] += G;
            Hash[7U] += H;
        }

        constexpr char HexDigits[] = "0123456789abcdef";
        std::string Result;
        Result.reserve(64U);
        for (const std::uint32_t Value : Hash)
        {
            for (std::uint32_t Shift = 24U;; Shift -= 8U)
            {
                const std::uint8_t Byte = static_cast<std::uint8_t>(Value >> Shift);
                Result.push_back(HexDigits[(Byte >> 4U) & 0x0FU]);
                Result.push_back(HexDigits[Byte & 0x0FU]);
                if (Shift == 0U)
                {
                    break;
                }
            }
        }
        return Result;
    }

    void TestNormalizedPinRecordValueSemantics()
    {
        static_assert(std::is_copy_constructible_v<NormalizedPinRecord>);
        static_assert(std::is_move_constructible_v<NormalizedPinRecord>);
        static_assert(std::is_copy_assignable_v<NormalizedPinRecord>);
        static_assert(std::is_move_assignable_v<NormalizedPinRecord>);

        const NormalizedPinRecord Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(Invalid.GetName().empty());
        MPP_CHECK(!Invalid.GetType().IsValid());
        MPP_CHECK(!Invalid.GetDefaultValue().has_value());

        std::string Name = "OwnedName";
        TypeDesc Type = TypeDesc::Integer();
        const NormalizedPinRecord Owned(
            Name,
            Type,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Optional,
            true,
            MakeLiteral(std::int64_t(7))
        );
        Name[0U] = 'X';
        MPP_CHECK(Owned.IsValid());
        MPP_CHECK(Owned.GetName() == "OwnedName");
        MPP_CHECK(Owned.GetType() == TypeDesc::Integer());
        MPP_CHECK(Owned.GetDirection() == PinDirection::Input);
        MPP_CHECK(Owned.GetCategory() == PinCategory::Data);
        MPP_CHECK(Owned.GetCardinality() == PinCardinality::Optional);
        MPP_CHECK(Owned.AllowsLiteral());
        MPP_CHECK(Owned.GetDefaultValue()->Is<std::int64_t>());
        MPP_CHECK(*Owned.GetDefaultValue()->TryGet<std::int64_t>() == 7);

        const NormalizedPinRecord Copy = Owned;
        NormalizedPinRecord Moved = std::move(Copy);
        MPP_CHECK(Moved == Owned);
        MPP_CHECK(Moved == Moved);
        MPP_CHECK(Moved != MakeDataPin("Other", TypeDesc::Integer()));

        MPP_CHECK(Moved != NormalizedPinRecord(
            "OwnedName", TypeDesc::Integer(), PinDirection::Input,
            PinCategory::Data, PinCardinality::Single, true,
            MakeLiteral(std::int64_t(7))));
        MPP_CHECK(Moved != NormalizedPinRecord(
            "OwnedName", TypeDesc::Float(), PinDirection::Input,
            PinCategory::Data, PinCardinality::Optional, true,
            MakeLiteral(std::int64_t(7))));

        const double PositiveZero = std::bit_cast<double>(std::uint64_t(0x0000000000000000ULL));
        const double NegativeZero = std::bit_cast<double>(std::uint64_t(0x8000000000000000ULL));
        const double FirstNaN = std::bit_cast<double>(std::uint64_t(0x7FF8000000000001ULL));
        const double SameNaN = std::bit_cast<double>(std::uint64_t(0x7FF8000000000001ULL));
        const double OtherNaN = std::bit_cast<double>(std::uint64_t(0xFFF8000000000001ULL));

        const auto PositiveZeroPin = MakeDataPin(
            "Value", TypeDesc::Float(), true, MakeLiteral(PositiveZero));
        const auto NegativeZeroPin = MakeDataPin(
            "Value", TypeDesc::Float(), true, MakeLiteral(NegativeZero));
        MPP_CHECK(PositiveZeroPin == PositiveZeroPin);
        MPP_CHECK(NegativeZeroPin == NegativeZeroPin);
        MPP_CHECK(PositiveZeroPin != NegativeZeroPin);

        const auto FirstNaNPin = MakeDataPin(
            "Value", TypeDesc::Float(), true, MakeLiteral(FirstNaN));
        const auto SameNaNPin = MakeDataPin(
            "Value", TypeDesc::Float(), true, MakeLiteral(SameNaN));
        const auto OtherNaNPin = MakeDataPin(
            "Value", TypeDesc::Float(), true, MakeLiteral(OtherNaN));
        MPP_CHECK(FirstNaNPin == FirstNaNPin);
        MPP_CHECK(FirstNaNPin == SameNaNPin);
        MPP_CHECK(FirstNaNPin != OtherNaNPin);

        const Vector3Value PositiveVector{0.0F, 1.0F, 2.0F};
        const Vector3Value SameVector{0.0F, 1.0F, 2.0F};
        const Vector3Value SignedZeroVector{
            std::bit_cast<float>(std::uint32_t(0x80000000U)), 1.0F, 2.0F
        };
        const auto PositiveVectorPin = MakeDataPin(
            "Value", TypeDesc::Vector3(), true, MakeLiteral(PositiveVector));
        const auto SameVectorPin = MakeDataPin(
            "Value", TypeDesc::Vector3(), true, MakeLiteral(SameVector));
        const auto SignedZeroVectorPin = MakeDataPin(
            "Value", TypeDesc::Vector3(), true, MakeLiteral(SignedZeroVector));
        MPP_CHECK(PositiveVectorPin == SameVectorPin);
        MPP_CHECK(PositiveVectorPin != SignedZeroVectorPin);

        const auto AbsentDefault = MakeDataPin("Value", TypeDesc::Integer());
        MPP_CHECK(AbsentDefault == AbsentDefault);
        MPP_CHECK(AbsentDefault != Owned);
    }

    void TestNormalizedNodeDescriptorRecordValueSemantics()
    {
        static_assert(std::is_copy_constructible_v<NormalizedNodeDescriptorRecord>);
        static_assert(std::is_move_constructible_v<NormalizedNodeDescriptorRecord>);
        static_assert(std::is_copy_assignable_v<NormalizedNodeDescriptorRecord>);
        static_assert(std::is_move_assignable_v<NormalizedNodeDescriptorRecord>);

        const NormalizedNodeDescriptorRecord Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(Invalid.GetAvailability().empty());
        MPP_CHECK(Invalid.GetPins().empty());
        MPP_CHECK(!Invalid.GetExecutionControlSchema().has_value());
        MPP_CHECK(!Invalid.GetSourceProvenance().has_value());

        const NormalizedNodeDescriptorRecord Empty = MakeRecord(
            "node", "DisplayName", {NodeAvailability::Client, NodeAvailability::Server});
        MPP_CHECK(Empty.IsValid());
        MPP_CHECK(Empty.GetAvailability().size() == 2U);
        MPP_CHECK(Empty.GetAvailability()[0U] == NodeAvailability::Server);
        MPP_CHECK(Empty.GetAvailability()[1U] == NodeAvailability::Client);

        const NormalizedNodeDescriptorRecord DuplicateAvailability = MakeRecord(
            "node", "DisplayName", {
                NodeAvailability::Client,
                NodeAvailability::Server,
                NodeAvailability::Server
            });
        MPP_CHECK(DuplicateAvailability.GetAvailability().size() == 3U);
        MPP_CHECK(DuplicateAvailability.GetAvailability()[0U] == NodeAvailability::Server);
        MPP_CHECK(DuplicateAvailability.GetAvailability()[1U] == NodeAvailability::Server);
        MPP_CHECK(DuplicateAvailability.GetAvailability()[2U] == NodeAvailability::Client);
        MPP_CHECK(!DuplicateAvailability.IsValid());

        const NormalizedNodeDescriptorRecord WithMetadata = MakeRecord(
            "node",
            "DisplayName",
            {},
            {MakeDataPin("Value", TypeDesc::Integer())},
            std::nullopt,
            MakeProvenance("document", "record")
        );
        const NormalizedNodeDescriptorRecord SameMetadata = WithMetadata;
        MPP_CHECK(WithMetadata == SameMetadata);
        MPP_CHECK(WithMetadata == WithMetadata);
        MPP_CHECK(WithMetadata != MakeRecord(
            "node", "OtherDisplayName", {},
            {MakeDataPin("Value", TypeDesc::Integer())},
            std::nullopt, MakeProvenance("document", "record")));
        MPP_CHECK(WithMetadata != MakeRecord(
            "node", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Integer())},
            std::nullopt, MakeProvenance("document", "other-record")));
        MPP_CHECK(WithMetadata != MakeRecord(
            "node", "DisplayName", {},
            {MakeDataPin("OtherValue", TypeDesc::Integer())},
            std::nullopt, MakeProvenance("document", "record")));

        const NormalizedNodeDescriptorRecord WithControl = MakeRecord(
            "entry",
            "Entry",
            {},
            {MakeExecutionPin("Exec", PinDirection::Output)},
            MakeEntryControl(0U)
        );
        MPP_CHECK(WithControl.IsValid());
        MPP_CHECK(WithControl == WithControl);
        MPP_CHECK(WithControl == MakeRecord(
            "entry",
            "Entry",
            {},
            {MakeExecutionPin("Exec", PinDirection::Output)},
            MakeEntryControl(0U)
        ));
    }

    void TestNormalizedDescriptorValidation()
    {
        const std::vector<TypeDesc> ValidTypes = {
            TypeDesc::Boolean(), TypeDesc::Integer(), TypeDesc::Float(),
            TypeDesc::String(), TypeDesc::GUID(), TypeDesc::Vector3(),
            TypeDesc::PrefabId(), TypeDesc::ConfigId(), TypeDesc::Faction(),
            TypeDesc::Entity(),
            TypeDesc::Generic(GenericParameterId(1U)),
            TypeDesc::List(TypeDesc::Integer()),
            TypeDesc::Dictionary(TypeDesc::String(), TypeDesc::Integer()),
            TypeDesc::StructObject(StructTypeId(1U))
        };
        for (std::size_t Index = 0U; Index < ValidTypes.size(); ++Index)
        {
            const NormalizedNodeDescriptorRecord Record = MakeRecord(
                "type-" + std::to_string(Index),
                "DisplayName",
                {},
                {MakeDataPin("Value", ValidTypes[Index])}
            );
            MPP_CHECK(Record.IsValid());
            MPP_CHECK(ValidateNormalizedDescriptorRecord(Record).has_value());
        }

        const NormalizedNodeDescriptorRecord ValidExecution = MakeRecord(
            "execution",
            "Execution",
            {},
            {MakeExecutionPin("Exec", PinDirection::Output)}
        );
        MPP_CHECK(ValidExecution.IsValid());

        const NormalizedNodeDescriptorRecord InvalidIdentity = MakeRecord("");
        const auto InvalidIdentityResult = ValidateNormalizedDescriptorRecord(InvalidIdentity);
        MPP_CHECK(!InvalidIdentityResult.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidIdentityResult.error(), DiagnosticCode::InvalidExternalNodeIdentity));

        const NormalizedNodeDescriptorRecord InvalidProvenance = MakeRecord(
            "invalid-provenance", "DisplayName", {}, {}, std::nullopt,
            MakeProvenance("", "record")
        );
        const auto InvalidProvenanceResult =
            ValidateNormalizedDescriptorRecord(InvalidProvenance);
        MPP_CHECK(!InvalidProvenanceResult.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidProvenanceResult.error(), DiagnosticCode::InvalidSourceProvenance));

        const NormalizedNodeDescriptorRecord InvalidDisplayName = MakeRecord(
            "invalid-display", "");
        MPP_CHECK(!InvalidDisplayName.IsValid());
        MPP_CHECK(HasDiagnosticCode(
            ValidateNormalizedDescriptorRecord(InvalidDisplayName).error(),
            DiagnosticCode::InvalidNormalizedDescriptorRecord));

        const NormalizedNodeDescriptorRecord InvalidAvailability = MakeRecord(
            "invalid-availability", "DisplayName", {
                static_cast<NodeAvailability>(99)
            });
        MPP_CHECK(!InvalidAvailability.IsValid());
        MPP_CHECK(!ValidateNormalizedDescriptorRecord(InvalidAvailability).has_value());

        const NormalizedNodeDescriptorRecord DuplicateAvailability = MakeRecord(
            "duplicate-availability", "DisplayName", {
                NodeAvailability::Server, NodeAvailability::Server
            });
        MPP_CHECK(!DuplicateAvailability.IsValid());

        const NormalizedNodeDescriptorRecord InvalidType = MakeRecord(
            "invalid-type", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc())}
        );
        MPP_CHECK(!InvalidType.IsValid());

        const NormalizedNodeDescriptorRecord InvalidNestedType = MakeRecord(
            "invalid-nested-type", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::List(TypeDesc()))}
        );
        MPP_CHECK(!InvalidNestedType.IsValid());

        const NormalizedNodeDescriptorRecord FlowData = MakeRecord(
            "flow-data", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::List(TypeDesc::Flow()))}
        );
        MPP_CHECK(!FlowData.IsValid());

        const NormalizedNodeDescriptorRecord InvalidExecution = MakeRecord(
            "invalid-execution", "DisplayName", {},
            {NormalizedPinRecord(
                "Exec", TypeDesc::Integer(), PinDirection::Output,
                PinCategory::Execution
            )}
        );
        MPP_CHECK(!InvalidExecution.IsValid());

        const NormalizedNodeDescriptorRecord ExecutionLiteral = MakeRecord(
            "execution-literal", "DisplayName", {},
            {NormalizedPinRecord(
                "Exec", TypeDesc::Flow(), PinDirection::Output,
                PinCategory::Execution, PinCardinality::Single, true
            )}
        );
        MPP_CHECK(!ExecutionLiteral.IsValid());

        const NormalizedNodeDescriptorRecord OutputDefault = MakeRecord(
            "output-default", "DisplayName", {},
            {MakeDataPin(
                "Value", TypeDesc::Integer(), true, MakeLiteral(std::int64_t(1)),
                PinCardinality::Single, PinDirection::Output
            )}
        );
        MPP_CHECK(!OutputDefault.IsValid());

        const NormalizedNodeDescriptorRecord WrongDefault = MakeRecord(
            "wrong-default", "DisplayName", {},
            {MakeDataPin(
                "Value", TypeDesc::Integer(), true, MakeLiteral(true)
            )}
        );
        MPP_CHECK(!WrongDefault.IsValid());

        const NormalizedNodeDescriptorRecord DuplicatePins = MakeRecord(
            "duplicate-pins", "DisplayName", {},
            {
                MakeDataPin("Value", TypeDesc::Integer()),
                MakeDataPin("Value", TypeDesc::Integer())
            }
        );
        MPP_CHECK(!DuplicatePins.IsValid());

        const NormalizedNodeDescriptorRecord Entry = MakeRecord(
            "valid-entry", "Entry", {},
            {MakeExecutionPin("Exec", PinDirection::Output)}, MakeEntryControl(0U)
        );
        const NormalizedNodeDescriptorRecord Sequence = MakeRecord(
            "valid-sequence", "Sequence", {},
            {
                MakeExecutionPin("In", PinDirection::Input),
                MakeExecutionPin("Out", PinDirection::Output)
            }, MakeSequenceControl(0U, 1U)
        );
        const NormalizedNodeDescriptorRecord Branch = MakeRecord(
            "valid-branch", "Branch", {},
            {
                MakeExecutionPin("In", PinDirection::Input),
                MakeDataPin("Condition", TypeDesc::Boolean()),
                MakeExecutionPin("True", PinDirection::Output),
                MakeExecutionPin("False", PinDirection::Output)
            }, MakeBranchControl(0U, 1U, 2U, 3U)
        );
        const NormalizedNodeDescriptorRecord Join = MakeRecord(
            "valid-join", "Join", {},
            {
                MakeExecutionPin("In", PinDirection::Input, PinCardinality::Multiple),
                MakeExecutionPin("Out", PinDirection::Output)
            }, MakeJoinControl(0U, 1U)
        );
        const NormalizedNodeDescriptorRecord ConditionalLoop = MakeLoopRecord(
            "valid-conditional-loop", true);
        const NormalizedNodeDescriptorRecord UnconditionalLoop = MakeLoopRecord(
            "valid-unconditional-loop", false);
        const NormalizedNodeDescriptorRecord Return = MakeRecord(
            "valid-return", "Return", {},
            {MakeExecutionPin("In", PinDirection::Input)}, MakeReturnControl(0U)
        );
        for (const NormalizedNodeDescriptorRecord& Record : {
            Entry, Sequence, Branch, Join, ConditionalLoop, UnconditionalLoop, Return
        })
        {
            MPP_CHECK(Record.IsValid());
        }

        const NormalizedNodeDescriptorRecord InvalidRole = MakeRecord(
            "invalid-role", "Entry", {},
            {MakeExecutionPin("Exec", PinDirection::Output)}, MakeEntryControl(1U)
        );
        MPP_CHECK(!InvalidRole.IsValid());

        const NormalizedNodeDescriptorRecord UncoveredExecution = MakeRecord(
            "uncovered-execution", "Entry", {},
            {
                MakeExecutionPin("First", PinDirection::Output),
                MakeExecutionPin("Second", PinDirection::Output)
            }, MakeEntryControl(0U)
        );
        MPP_CHECK(!UncoveredExecution.IsValid());

        const NormalizedNodeDescriptorRecord ConditionalWithoutCondition = MakeRecord(
            "conditional-without-condition", "Loop", {},
            {
                MakeExecutionPin("ExecutionInput", PinDirection::Input),
                MakeExecutionPin("BodyOutput", PinDirection::Output),
                MakeExecutionPin("ExitOutput", PinDirection::Output),
                MakeExecutionPin("RepeatInput", PinDirection::Input, PinCardinality::Multiple),
                MakeExecutionPin("BreakInput", PinDirection::Input, PinCardinality::Multiple)
            }, MakeLoopControl(true, 5U)
        );
        MPP_CHECK(!ConditionalWithoutCondition.IsValid());
    }

    void TestDescriptorCatalogueContentIdentifierVectors()
    {
        MPP_CHECK(ComputeIndependentSha256("abc") ==
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

        CheckDigest({},
            "439b5a7bb02e2f8acc21a160fbe007d87bfb7b9fce7846a9d1bdd3258c8b97e6");
        CheckDigest({MakeRecord("node")},
            "bb66dd12ba8fb92335c9d36135161542af4f9aa75f8e150e507e0a7b3052b117");
        CheckDigest({MakeRecord("node2")},
            "864cdf72a31813ad25252e97aaf3fdb0613e1b4fe78c42257e09756bebcc8063");
        CheckDigest({MakeRecord("b"), MakeRecord("a")},
            "9cf43283996a0a44191477571b3bc61b77a79096ac44cd4b156726ef718b2505");

        CheckDigest({MakeRecord(
            "bool", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Boolean(), true, MakeLiteral(false))})},
            "729f1e894db207070e9ed66b3f10a7b069042619e5d4d22d8f468f592cab9329");
        CheckDigest({MakeRecord(
            "bool", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Boolean(), true, MakeLiteral(true))})},
            "bcc445054b6d8ec88afbabdc9191ea68850197e177accce78b75361090eff57f");
        CheckDigest({MakeRecord(
            "integer", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Integer(), true, MakeLiteral(std::int64_t(1)))})},
            "2724ce504011015e82276c3368e9aac13a6633b73610cd4f0c9e2801b6094b5a");
        CheckDigest({MakeRecord(
            "integer", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Integer(), true, MakeLiteral(std::int64_t(0)))})},
            "e7056a517f19402f11551adf5ba60737a7998b73f857f029bf97c9b8a3a74c0b");
        CheckDigest({MakeRecord(
            "integer", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Integer(), true, MakeLiteral(std::int64_t(-1)))})},
            "d68981885ace10c18e6737209bacb6897eeb96338baaa71848eda7637e69dddc");
        CheckDigest({MakeRecord(
            "integer", "DisplayName", {},
            {MakeDataPin(
                "Value", TypeDesc::Integer(), true,
                MakeLiteral(std::numeric_limits<std::int64_t>::min()))})},
            "c8bbcf97ce6a7f022256c7059a15fc6bf2de9d6dc541702e69951dc806db0013");
        CheckDigest({MakeRecord(
            "integer", "DisplayName", {},
            {MakeDataPin(
                "Value", TypeDesc::Integer(), true,
                MakeLiteral(std::numeric_limits<std::int64_t>::max()))})},
            "4dd57caa8890f60765a59f7007d89dbc62ded4011f7e9a7a02e11b077db515df");

        CheckDigest({MakeRecord(
            "entry-control", "DisplayName", {},
            {MakeExecutionPin("Exec", PinDirection::Output)}, MakeEntryControl(0U))},
            "f48ca75e63800343245b957b22cc147c7ff732c40b490075bd06236e8ab512b2");
        CheckDigest({MakeLoopRecord("loop-conditional", true)},
            "b8d9ebf850b730311861e9a0568e662a9d2d68d51c86eb19a01a2bb2666394a4");
        CheckDigest({MakeLoopRecord("loop-unconditional", false)},
            "47d466e956f84e6e72cb7aa8e8732a4e755a92a309eaeb4d8e77bdcca2772059");

        const double PositiveZero = std::bit_cast<double>(std::uint64_t(0x0000000000000000ULL));
        const double NegativeZero = std::bit_cast<double>(std::uint64_t(0x8000000000000000ULL));
        const double QuietNaN = std::bit_cast<double>(std::uint64_t(0x7FF8000000000001ULL));
        CheckDigest({MakeRecord(
            "float", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(PositiveZero))})},
            "c45bfc5c3ba27443d6be6f7db25b43307f4045b371f4381504ecfb1a05771922");
        CheckDigest({MakeRecord(
            "float", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(NegativeZero))})},
            "9d5a2abe9f27554618b8650007e838bf8f6d4549dfce5a7640f13a54be55cafe");
        CheckDigest({MakeRecord(
            "float", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(QuietNaN))})},
            "43a5e0f5f424a0b150ca3365f208989225c22ad8b99fbf1caea3bc4cb6948dda");
        CheckDigest({MakeRecord(
            "vector3", "DisplayName", {},
            {MakeDataPin(
                "Value", TypeDesc::Vector3(), true,
                MakeLiteral(Vector3Value{
                    std::bit_cast<float>(std::uint32_t(0x00000000U)),
                    std::bit_cast<float>(std::uint32_t(0x80000000U)),
                    std::bit_cast<float>(std::uint32_t(0x3F800000U))
                })
            )})},
            "5a27b17e382b2b6395e031b39011818d0ac7dfdb568dab61882f6f33253c2614");
    }

    void TestDescriptorCatalogueContentIdentifierExclusions()
    {
        const NormalizedNodeDescriptorRecord Base = MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Integer())}
        );
        const auto BaseDigest = DeriveDescriptorCatalogueContentIdentifier({Base});
        MPP_CHECK(BaseDigest.has_value());

        const auto DisplayDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "OtherDisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Integer())}
        )});
        MPP_CHECK(DisplayDigest.has_value());
        MPP_CHECK(*BaseDigest == *DisplayDigest);

        const auto ProvenanceDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Integer())}, std::nullopt,
            MakeProvenance("document", "record")
        )});
        MPP_CHECK(ProvenanceDigest.has_value());
        MPP_CHECK(*BaseDigest == *ProvenanceDigest);

        const auto KeyDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "other", "DisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Integer())}
        )});
        MPP_CHECK(KeyDigest.has_value());
        MPP_CHECK(*BaseDigest != *KeyDigest);

        const auto AvailabilityDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Client},
            {MakeDataPin("Value", TypeDesc::Integer())}
        )});
        MPP_CHECK(AvailabilityDigest.has_value());
        MPP_CHECK(*BaseDigest != *AvailabilityDigest);

        const auto PinOrderDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {
                MakeDataPin("First", TypeDesc::Integer()),
                MakeDataPin("Second", TypeDesc::Integer())
            }
        )});
        const auto ReversedPinOrderDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {
                MakeDataPin("Second", TypeDesc::Integer()),
                MakeDataPin("First", TypeDesc::Integer())
            }
        )});
        MPP_CHECK(PinOrderDigest.has_value());
        MPP_CHECK(ReversedPinOrderDigest.has_value());
        MPP_CHECK(*PinOrderDigest != *ReversedPinOrderDigest);

        const auto TypeDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Float())}
        )});
        MPP_CHECK(TypeDigest.has_value());
        MPP_CHECK(*BaseDigest != *TypeDigest);

        const auto DefaultDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {MakeDataPin("Value", TypeDesc::Integer(), true, MakeLiteral(std::int64_t(1)))})});
        MPP_CHECK(DefaultDigest.has_value());
        MPP_CHECK(*BaseDigest != *DefaultDigest);

        const auto ControlDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {NodeAvailability::Server},
            {MakeExecutionPin("Exec", PinDirection::Output)}, MakeEntryControl(0U))});
        MPP_CHECK(ControlDigest.has_value());
        MPP_CHECK(*BaseDigest != *ControlDigest);

        const double PositiveZero = std::bit_cast<double>(std::uint64_t(0x0000000000000000ULL));
        const double NegativeZero = std::bit_cast<double>(std::uint64_t(0x8000000000000000ULL));
        const auto PositiveZeroDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(PositiveZero))})});
        const auto NegativeZeroDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(NegativeZero))})});
        MPP_CHECK(PositiveZeroDigest.has_value());
        MPP_CHECK(NegativeZeroDigest.has_value());
        MPP_CHECK(*PositiveZeroDigest != *NegativeZeroDigest);

        const double FirstNaN = std::bit_cast<double>(std::uint64_t(0x7FF8000000000001ULL));
        const double OtherNaN = std::bit_cast<double>(std::uint64_t(0x7FF8000000000002ULL));
        const auto FirstNaNDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(FirstNaN))})});
        const auto OtherNaNDigest = DeriveDescriptorCatalogueContentIdentifier({MakeRecord(
            "base", "DisplayName", {},
            {MakeDataPin("Value", TypeDesc::Float(), true, MakeLiteral(OtherNaN))})});
        MPP_CHECK(FirstNaNDigest.has_value());
        MPP_CHECK(OtherNaNDigest.has_value());
        MPP_CHECK(*FirstNaNDigest != *OtherNaNDigest);

        const auto FirstBuild = DescriptorCatalogueBuilder::Build(
            "source-a", "revision-a", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("base")}
        );
        const auto NamespaceBuild = DescriptorCatalogueBuilder::Build(
            "source-b", "revision-a", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("base")}
        );
        const auto RevisionBuild = DescriptorCatalogueBuilder::Build(
            "source-a", "revision-b", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("base")}
        );
        MPP_CHECK(FirstBuild.has_value());
        MPP_CHECK(NamespaceBuild.has_value());
        MPP_CHECK(RevisionBuild.has_value());
        MPP_CHECK(FirstBuild->GetIdentity().GetCatalogueContentIdentifier() ==
            NamespaceBuild->GetIdentity().GetCatalogueContentIdentifier());
        MPP_CHECK(FirstBuild->GetIdentity().GetCatalogueContentIdentifier() ==
            RevisionBuild->GetIdentity().GetCatalogueContentIdentifier());
        MPP_CHECK(FirstBuild->GetIdentity() != NamespaceBuild->GetIdentity());
        MPP_CHECK(FirstBuild->GetIdentity() != RevisionBuild->GetIdentity());

        const DescriptorCatalogueIdentity FirstSchemaIdentity(
            "source-a",
            "revision-a",
            DescriptorCatalogueSemanticSchemaVersion(1U),
            *BaseDigest
        );
        const DescriptorCatalogueIdentity SecondSchemaIdentity(
            "source-a",
            "revision-a",
            DescriptorCatalogueSemanticSchemaVersion(2U),
            *BaseDigest
        );
        MPP_CHECK(FirstSchemaIdentity.GetCatalogueContentIdentifier() ==
            SecondSchemaIdentity.GetCatalogueContentIdentifier());
        MPP_CHECK(FirstSchemaIdentity != SecondSchemaIdentity);

        const DescriptorCatalogueEntry FirstEntry(Base, NodeDescriptorId(1U));
        const DescriptorCatalogueEntry SecondEntry(Base, NodeDescriptorId(2U));
        MPP_CHECK(FirstEntry.IsValid());
        MPP_CHECK(SecondEntry.IsValid());
        MPP_CHECK(FirstEntry != SecondEntry);
        const auto FirstEntryDigest = DeriveDescriptorCatalogueContentIdentifier({
            FirstEntry.GetRecord()
        });
        const auto SecondEntryDigest = DeriveDescriptorCatalogueContentIdentifier({
            SecondEntry.GetRecord()
        });
        MPP_CHECK(FirstEntryDigest.has_value());
        MPP_CHECK(SecondEntryDigest.has_value());
        MPP_CHECK(*FirstEntryDigest == *BaseDigest);
        MPP_CHECK(*SecondEntryDigest == *BaseDigest);

        const auto InputOrderA = DescriptorCatalogueBuilder::Build(
            "source-a", "revision-a", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("b"), MakeRecord("a")}
        );
        const auto InputOrderB = DescriptorCatalogueBuilder::Build(
            "source-a", "revision-a", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("a"), MakeRecord("b")}
        );
        MPP_CHECK(InputOrderA.has_value());
        MPP_CHECK(InputOrderB.has_value());
        MPP_CHECK(*InputOrderA == *InputOrderB);
    }

    void TestDescriptorCatalogueBuildAndIdentity()
    {
        const std::vector<NormalizedNodeDescriptorRecord> Records = {
            MakeRecord("second", "Second"),
            MakeRecord(
                "first", "First", {}, {}, std::nullopt,
                MakeProvenance("document", "first")
            )
        };
        const auto Result = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion, Records
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetIdentity().IsValid());
        MPP_CHECK(Result->GetIdentity().GetSourceNamespace() == "source");
        MPP_CHECK(Result->GetIdentity().GetSourceRevision() == "revision");
        MPP_CHECK(Result->GetIdentity().GetSemanticSchemaVersion() ==
            CurrentDescriptorCatalogueSemanticSchemaVersion);
        MPP_CHECK(Result->GetEntryCount() == 2U);
        MPP_CHECK(Result->GetEntries()[0U].GetExternalIdentity().GetKey() == "first");
        MPP_CHECK(Result->GetEntries()[1U].GetExternalIdentity().GetKey() == "second");
        MPP_CHECK(Result->GetEntries()[0U].GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK(Result->GetEntries()[1U].GetDescriptorIdentifier() == NodeDescriptorId(2U));
        MPP_CHECK(Result->GetEntries()[0U].GetSourceProvenance()->GetSourceRecordIdentifier() ==
            "first");
        MPP_CHECK(Result->GetEntries()[0U].GetRecord().GetDisplayName() == "First");

        const DescriptorCatalogue Copy = *Result;
        DescriptorCatalogue Moved = std::move(Copy);
        MPP_CHECK(Moved == *Result);
        const DescriptorCatalogue Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(!DescriptorCatalogueEntry().IsValid());
    }

    void TestDescriptorCatalogueOrderingAndLookup()
    {
        const auto Result = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("charlie"), MakeRecord("alpha"), MakeRecord("bravo")}
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetEntries()[0U].GetExternalIdentity().GetKey() == "alpha");
        MPP_CHECK(Result->GetEntries()[1U].GetExternalIdentity().GetKey() == "bravo");
        MPP_CHECK(Result->GetEntries()[2U].GetExternalIdentity().GetKey() == "charlie");

        const DescriptorCatalogueEntry* Alpha = Result->FindByExternalIdentity(
            ExternalNodeIdentity("alpha"));
        MPP_CHECK(Alpha != nullptr);
        MPP_CHECK(Alpha->GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK(Result->FindByExternalIdentity(ExternalNodeIdentity("missing")) == nullptr);
        MPP_CHECK(Result->FindByDescriptorIdentifier(NodeDescriptorId(2U)) != nullptr);
        MPP_CHECK(Result->FindByDescriptorIdentifier(NodeDescriptorId(2U))
            ->GetExternalIdentity().GetKey() == "bravo");
        MPP_CHECK(Result->FindByDescriptorIdentifier(NodeDescriptorId()) == nullptr);
        MPP_CHECK(Result->FindByDescriptorIdentifier(NodeDescriptorId(99U)) == nullptr);
    }

    void TestDescriptorCatalogueAllocationDeterminism()
    {
        const auto First = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("z", "Z"), MakeRecord("a", "A"), MakeRecord("m", "M")
            }
        );
        const auto Permuted = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("m", "M"), MakeRecord("z", "Z"), MakeRecord("a", "A")
            }
        );
        const auto Repeated = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("z", "Z"), MakeRecord("a", "A"), MakeRecord("m", "M")
            }
        );
        MPP_CHECK(First.has_value());
        MPP_CHECK(Permuted.has_value());
        MPP_CHECK(Repeated.has_value());
        MPP_CHECK(*First == *Permuted);
        MPP_CHECK(*First == *Repeated);

        const auto WithProvenance = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("z", "Z", {}, {}, std::nullopt,
                    MakeProvenance("document", "z")),
                MakeRecord("a", "A", {}, {}, std::nullopt,
                    MakeProvenance("document", "a")),
                MakeRecord("m", "M", {}, {}, std::nullopt,
                    MakeProvenance("document", "m"))
            }
        );
        MPP_CHECK(WithProvenance.has_value());
        MPP_CHECK(WithProvenance->GetIdentity().GetCatalogueContentIdentifier() ==
            First->GetIdentity().GetCatalogueContentIdentifier());
        for (std::size_t Index = 0U; Index < First->GetEntryCount(); ++Index)
        {
            MPP_CHECK(First->GetEntries()[Index].GetDescriptorIdentifier() ==
                WithProvenance->GetEntries()[Index].GetDescriptorIdentifier());
        }

        MPP_CHECK(!NodeDescriptorId().IsValid());
        MPP_CHECK(First->GetEntries()[0U].GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK(First->GetEntries()[2U].GetDescriptorIdentifier() == NodeDescriptorId(3U));

        MPP_CHECK(DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(0U));
        MPP_CHECK(DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
            std::numeric_limits<std::uint32_t>::max()));
        if (std::numeric_limits<std::size_t>::max() >
            std::numeric_limits<std::uint32_t>::max())
        {
            MPP_CHECK(!DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
                static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1U));
        }
    }

    void TestDescriptorCatalogueDuplicateDiagnostics()
    {
        const auto TwoDuplicates = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("duplicate"), MakeRecord("duplicate")}
        );
        MPP_CHECK(!TwoDuplicates.has_value());
        MPP_CHECK(CountDiagnosticCode(
            TwoDuplicates.error(), DiagnosticCode::DuplicateExternalNodeIdentity) == 1U);

        const auto ThreeDuplicates = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("duplicate", "D3", {}, {}, std::nullopt,
                    MakeProvenance("document", "three")),
                MakeRecord("duplicate", "D1", {}, {}, std::nullopt,
                    MakeProvenance("document", "one")),
                MakeRecord("duplicate", "D2", {}, {}, std::nullopt,
                    MakeProvenance("document", "two"))
            }
        );
        MPP_CHECK(!ThreeDuplicates.has_value());
        MPP_CHECK(CountDiagnosticCode(
            ThreeDuplicates.error(), DiagnosticCode::DuplicateExternalNodeIdentity) == 2U);
        for (const Diagnostic& CurrentDiagnostic : ThreeDuplicates.error())
        {
            if (CurrentDiagnostic.Code == DiagnosticCode::DuplicateExternalNodeIdentity)
            {
                MPP_CHECK(CurrentDiagnostic.PrimarySourceProvenance.has_value());
                MPP_CHECK(CurrentDiagnostic.RelatedSourceProvenance.has_value());
                MPP_CHECK(CurrentDiagnostic.PrimarySourceProvenance->GetSourceRecordIdentifier() ==
                    "one");
            }
        }

        const std::vector<NormalizedNodeDescriptorRecord> PermutationA = {
            MakeRecord("z", "Z", {}, {}, std::nullopt,
                MakeProvenance("document", "z-2")),
            MakeRecord("a", "A", {}, {}, std::nullopt,
                MakeProvenance("document", "a-2")),
            MakeRecord("z", "Z", {}, {}, std::nullopt,
                MakeProvenance("document", "z-1")),
            MakeRecord("a", "A", {}, {}, std::nullopt,
                MakeProvenance("document", "a-1"))
        };
        const std::vector<NormalizedNodeDescriptorRecord> PermutationB = {
            PermutationA[3U], PermutationA[2U], PermutationA[1U], PermutationA[0U]
        };
        const auto DiagnosticsA = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            PermutationA
        );
        const auto DiagnosticsB = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            PermutationB
        );
        MPP_CHECK(!DiagnosticsA.has_value());
        MPP_CHECK(!DiagnosticsB.has_value());
        MPP_CHECK(SameDiagnostics(DiagnosticsA.error(), DiagnosticsB.error()));
        MPP_CHECK(DiagnosticsA.error().size() == 2U);
        MPP_CHECK(DiagnosticsA.error()[0U].Message.find("a") != std::string::npos);

        const auto InvalidDuplicate = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {
                MakeRecord("same", "", {}, {}, std::nullopt,
                    MakeProvenance("document", "invalid")),
                MakeRecord("same", "Valid", {}, {}, std::nullopt,
                    MakeProvenance("document", "valid"))
            }
        );
        MPP_CHECK(!InvalidDuplicate.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidDuplicate.error(), DiagnosticCode::InvalidNormalizedDescriptorRecord));
        MPP_CHECK(HasDiagnosticCode(
            InvalidDuplicate.error(), DiagnosticCode::DuplicateExternalNodeIdentity));
    }

    void TestDescriptorCatalogueFailureAtomicity()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records = {
            MakeRecord("valid"), MakeRecord("invalid", "")
        };
        const std::vector<NormalizedNodeDescriptorRecord> Original = Records;
        const auto InvalidResult = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion, Records
        );
        MPP_CHECK(!InvalidResult.has_value());
        MPP_CHECK(Records == Original);
        MPP_CHECK(InvalidResult.error().size() == 1U);
        MPP_CHECK(InvalidResult.error()[0U].Code ==
            DiagnosticCode::InvalidNormalizedDescriptorRecord);

        const std::vector<NormalizedNodeDescriptorRecord> DuplicateRecords = {
            MakeRecord("same"), MakeRecord("same")
        };
        const auto DuplicateResult = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            DuplicateRecords
        );
        MPP_CHECK(!DuplicateResult.has_value());
        MPP_CHECK(CountDiagnosticCode(
            DuplicateResult.error(), DiagnosticCode::DuplicateExternalNodeIdentity) == 1U);

        const auto InvalidContext = DescriptorCatalogueBuilder::Build(
            "", "", DescriptorCatalogueSemanticSchemaVersion(), {MakeRecord("valid")}
        );
        MPP_CHECK(!InvalidContext.has_value());
        MPP_CHECK(CountDiagnosticCode(
            InvalidContext.error(), DiagnosticCode::InvalidDescriptorCatalogueIdentity) == 3U);
        MPP_CHECK(InvalidContext.error()[0U].Message.find("version") != std::string::npos);
        MPP_CHECK(InvalidContext.error()[1U].Message.find("namespace") != std::string::npos);
        MPP_CHECK(InvalidContext.error()[2U].Message.find("revision") != std::string::npos);
    }

    void TestEmptyDescriptorCatalogue()
    {
        const auto Result = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion, {}
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetEntryCount() == 0U);
        MPP_CHECK(Result->GetEntries().empty());
        MPP_CHECK(Result->FindByExternalIdentity(ExternalNodeIdentity("missing")) == nullptr);
        MPP_CHECK(Result->FindByDescriptorIdentifier(NodeDescriptorId(1U)) == nullptr);
        MPP_CHECK(Result->GetIdentity().GetCatalogueContentIdentifier().GetValue() ==
            "439b5a7bb02e2f8acc21a160fbe007d87bfb7b9fce7846a9d1bdd3258c8b97e6");
    }

    void TestUnsupportedDescriptorCatalogueSemanticSchemaVersion()
    {
        const auto Unsupported = DescriptorCatalogueBuilder::Build(
            "source", "revision", DescriptorCatalogueSemanticSchemaVersion(3U), {}
        );
        MPP_CHECK(!Unsupported.has_value());
        MPP_CHECK(Unsupported.error().size() == 1U);
        MPP_CHECK(Unsupported.error()[0U].Code ==
            DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion);
        MPP_CHECK(!DescriptorCatalogueBuilder::Build(
            "source", "revision", DescriptorCatalogueSemanticSchemaVersion(), {}
        ).has_value());
        MPP_CHECK(DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion, {}
        ).has_value());
    }

    void TestDescriptorCatalogueProvenance()
    {
        const NormalizedNodeDescriptorRecord WithoutProvenance = MakeRecord("node");
        const NormalizedNodeDescriptorRecord WithProvenance = MakeRecord(
            "node", "DisplayName", {}, {}, std::nullopt,
            MakeProvenance("logical-document", "logical-record")
        );
        MPP_CHECK(WithoutProvenance.IsValid());
        MPP_CHECK(WithProvenance.IsValid());
        MPP_CHECK(WithoutProvenance != WithProvenance);

        const auto WithoutBuild = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {WithoutProvenance}
        );
        const auto WithBuild = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {WithProvenance}
        );
        MPP_CHECK(WithoutBuild.has_value());
        MPP_CHECK(WithBuild.has_value());
        MPP_CHECK(WithoutBuild->GetIdentity().GetCatalogueContentIdentifier() ==
            WithBuild->GetIdentity().GetCatalogueContentIdentifier());
        MPP_CHECK(!WithoutBuild->GetEntries()[0U].GetSourceProvenance().has_value());
        MPP_CHECK(WithBuild->GetEntries()[0U].GetSourceProvenance().has_value());
        MPP_CHECK(WithBuild->GetEntries()[0U].GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            "logical-document");

        const auto Invalid = DescriptorCatalogueBuilder::Build(
            "source", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("node", "DisplayName", {}, {}, std::nullopt,
                MakeProvenance("", "record"))}
        );
        MPP_CHECK(!Invalid.has_value());
        MPP_CHECK(Invalid.error()[0U].Code == DiagnosticCode::InvalidSourceProvenance);
        MPP_CHECK(!Invalid.error()[0U].PrimarySourceProvenance.has_value());
    }

    void TestP51CompatibilityRegression()
    {
        const Diagnostic LegacyDiagnostic{
            .Severity = DiagnosticSeverity::Error,
            .Code = DiagnosticCode::MissingDescriptor,
            .Message = "legacy diagnostic"
        };
        MPP_CHECK(!LegacyDiagnostic.PrimarySourceProvenance.has_value());
        MPP_CHECK(!LegacyDiagnostic.RelatedSourceProvenance.has_value());

        const DescriptorCatalogueBinding InvalidBinding;
        const DescriptorCatalogueIdentity InvalidIdentity;
        const auto Compatibility = ValidateDescriptorCatalogueCompatibility(
            InvalidBinding, InvalidIdentity
        );
        MPP_CHECK(!Compatibility.has_value());
        MPP_CHECK(Compatibility.error().size() == 2U);
        MPP_CHECK(Compatibility.error()[0U].Message ==
            "Available catalogue identity is invalid.");
        MPP_CHECK(Compatibility.error()[1U].Message ==
            "Graph binding contains an invalid descriptor catalogue identity.");

        const auto Assignments = DescriptorIdentifierAllocator::Allocate({
            DescriptorIdentifierAllocationCandidate(ExternalNodeIdentity("b")),
            DescriptorIdentifierAllocationCandidate(ExternalNodeIdentity("a"))
        });
        MPP_CHECK(Assignments.has_value());
        MPP_CHECK((*Assignments)[0U].GetExternalIdentity().GetKey() == "a");
        MPP_CHECK((*Assignments)[0U].GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK((*Assignments)[1U].GetExternalIdentity().GetKey() == "b");
        MPP_CHECK((*Assignments)[1U].GetDescriptorIdentifier() == NodeDescriptorId(2U));

        const auto FirstCatalogue = DescriptorCatalogueBuilder::Build(
            "catalogue-a", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("shared-node")}
        );
        const auto SecondCatalogue = DescriptorCatalogueBuilder::Build(
            "catalogue-b", "revision", CurrentDescriptorCatalogueSemanticSchemaVersion,
            {MakeRecord("shared-node")}
        );
        MPP_CHECK(FirstCatalogue.has_value());
        MPP_CHECK(SecondCatalogue.has_value());
        MPP_CHECK(FirstCatalogue->GetEntries()[0U].GetDescriptorIdentifier() ==
            NodeDescriptorId(1U));
        MPP_CHECK(SecondCatalogue->GetEntries()[0U].GetDescriptorIdentifier() ==
            NodeDescriptorId(1U));
        MPP_CHECK(FirstCatalogue->GetEntries()[0U].GetDescriptorIdentifier() ==
            SecondCatalogue->GetEntries()[0U].GetDescriptorIdentifier());
        MPP_CHECK(FirstCatalogue->GetIdentity() != SecondCatalogue->GetIdentity());

        const DescriptorCatalogueBinding FirstBinding(FirstCatalogue->GetIdentity());
        const auto Mismatch = ValidateDescriptorCatalogueCompatibility(
            FirstBinding,
            SecondCatalogue->GetIdentity()
        );
        MPP_CHECK(!Mismatch.has_value());
        MPP_CHECK(HasDiagnosticCode(
            Mismatch.error(), DiagnosticCode::DescriptorCatalogueMismatch));

        const auto MatchingCompatibility = ValidateDescriptorCatalogueCompatibility(
            FirstBinding,
            FirstCatalogue->GetIdentity()
        );
        MPP_CHECK(MatchingCompatibility.has_value());
    }
}

int main()
{
    TestNormalizedPinRecordValueSemantics();
    TestNormalizedNodeDescriptorRecordValueSemantics();
    TestNormalizedDescriptorValidation();
    TestDescriptorCatalogueContentIdentifierVectors();
    TestDescriptorCatalogueContentIdentifierExclusions();
    TestDescriptorCatalogueBuildAndIdentity();
    TestDescriptorCatalogueOrderingAndLookup();
    TestDescriptorCatalogueAllocationDeterminism();
    TestDescriptorCatalogueDuplicateDiagnostics();
    TestDescriptorCatalogueFailureAtomicity();
    TestEmptyDescriptorCatalogue();
    TestUnsupportedDescriptorCatalogueSemanticSchemaVersion();
    TestDescriptorCatalogueProvenance();
    TestP51CompatibilityRegression();
    return EXIT_SUCCESS;
}
