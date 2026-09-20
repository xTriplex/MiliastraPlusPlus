#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <expected>
#include <initializer_list>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDescriptorCatalogueSnapshot.h"
#include "MiliastraPlusPlusGraphBuilder.h"

using namespace MiliastraPlusPlus;

#define MPP_CHECK(Expression)                                                   \
    do                                                                          \
    {                                                                           \
        if (!(Expression))                                                      \
        {                                                                       \
            std::fprintf(stderr, "Check failed at line %d.\n", __LINE__);      \
            std::abort();                                                       \
        }                                                                       \
    } while (false)

namespace
{
    using JsonValue = nlohmann::json;

    bool HasCode(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        for (const Diagnostic& DiagnosticValue : Diagnostics)
        {
            if (DiagnosticValue.Code == Code)
            {
                return true;
            }
        }

        return false;
    }

    void RequireReadCode(
        const std::string& SnapshotJson,
        DiagnosticCode Code
    )
    {
        const auto Result = DescriptorCatalogueSnapshotPersistence::Read(
            SnapshotJson
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), Code));
    }

    bool HasExactMembers(
        const JsonValue& Value,
        std::initializer_list<const char*> Names
    )
    {
        if (!Value.is_object() || Value.size() != Names.size())
        {
            return false;
        }

        for (const char* Name : Names)
        {
            if (!Value.contains(Name))
            {
                return false;
            }
        }

        return true;
    }

    JsonValue* FindJsonEntry(
        JsonValue& Document,
        const std::string& Identity
    )
    {
        for (JsonValue& Entry : Document["entries"])
        {
            if (Entry["record"]["externalIdentity"] == Identity)
            {
                return &Entry;
            }
        }

        return nullptr;
    }

    const DescriptorCatalogueEntry* FindCatalogueEntry(
        const DescriptorCatalogue& Catalogue,
        const std::string& Identity
    )
    {
        return Catalogue.FindByExternalIdentity(ExternalNodeIdentity(Identity));
    }

    NormalizedNodeDescriptorRecord MakeRecord(
        std::string Identity,
        std::string Name,
        std::vector<NormalizedPinRecord> Pins,
        std::optional<ExecutionControlSchema> Control = std::nullopt,
        std::optional<SourceProvenance> Provenance = std::nullopt,
        std::vector<NodeAvailability> Availability = {NodeAvailability::Client}
    )
    {
        return NormalizedNodeDescriptorRecord(
            ExternalNodeIdentity(std::move(Identity)),
            std::move(Name),
            std::move(Availability),
            std::move(Pins),
            std::move(Control),
            std::move(Provenance)
        );
    }

    NormalizedPinRecord MakeInput(
        std::string Name,
        TypeDesc Type,
        PinCardinality Cardinality = PinCardinality::Single,
        bool AllowsLiteral = false,
        std::optional<LiteralValue> Default = std::nullopt
    )
    {
        return NormalizedPinRecord(
            std::move(Name),
            std::move(Type),
            PinDirection::Input,
            PinCategory::Data,
            Cardinality,
            AllowsLiteral,
            std::move(Default)
        );
    }

    NormalizedPinRecord MakeExecution(
        std::string Name,
        PinDirection Direction,
        PinCardinality Cardinality = PinCardinality::Single
    )
    {
        return NormalizedPinRecord(
            std::move(Name),
            TypeDesc::Flow(),
            Direction,
            PinCategory::Execution,
            Cardinality
        );
    }

    struct SnapshotFixture
    {
        DescriptorCatalogue Catalogue;
        DescriptorSpecializationResult Specialization;
        DescriptorCatalogueSnapshot Snapshot;
        std::string Json;

        SnapshotFixture(
            DescriptorCatalogue CatalogueValue,
            DescriptorSpecializationResult SpecializationValue,
            DescriptorCatalogueSnapshot SnapshotValue,
            std::string JsonValue
        )
            : Catalogue(std::move(CatalogueValue))
            , Specialization(std::move(SpecializationValue))
            , Snapshot(std::move(SnapshotValue))
            , Json(std::move(JsonValue))
        {
        }
    };

    SnapshotFixture MakeFixture()
    {
        std::vector<DescriptorSpecializationPin> FamilyAPins;
        FamilyAPins.emplace_back(
            "Fixed",
            TypeDesc::Integer(),
            false,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            true,
            LiteralValue(LiteralValue::Data{std::int64_t(7)})
        );
        FamilyAPins.emplace_back(
            "Reflected",
            std::nullopt,
            true,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Optional,
            false
        );
        FamilyAPins.emplace_back(
            "ReflectedOutput",
            std::nullopt,
            true,
            PinDirection::Output,
            PinCategory::Data,
            PinCardinality::Multiple,
            false
        );

        DescriptorSpecializationVariant VariantA(
            ExternalNodeIdentity("concrete-a"),
            "opaque-key-a",
            {
                DescriptorSpecializationPinBinding(PinIndex(2U), TypeDesc::Float()),
                DescriptorSpecializationPinBinding(PinIndex(1U), TypeDesc::Integer())
            }
        );
        DescriptorSpecializationVariant VariantB(
            ExternalNodeIdentity("concrete-b"),
            "opaque-key-b",
            {
                DescriptorSpecializationPinBinding(PinIndex(2U), TypeDesc::Vector3()),
                DescriptorSpecializationPinBinding(PinIndex(1U), TypeDesc::String())
            }
        );
        DescriptorSpecializationFamily FamilyA(
            ExternalNodeIdentity("family-a"),
            "Specialized A",
            {NodeAvailability::Server, NodeAvailability::Client},
            FamilyAPins,
            std::nullopt,
            SourceProvenance("logical.document", "family-a"),
            {VariantB, VariantA}
        );

        std::vector<DescriptorSpecializationPin> FamilyBPins;
        FamilyBPins.emplace_back(
            "FixedB",
            TypeDesc::Boolean(),
            false,
            PinDirection::Input,
            PinCategory::Data,
            PinCardinality::Single,
            true,
            LiteralValue(LiteralValue::Data{true})
        );
        FamilyBPins.emplace_back(
            "ReflectedB",
            std::nullopt,
            true,
            PinDirection::Output,
            PinCategory::Data,
            PinCardinality::Single,
            false
        );
        DescriptorSpecializationFamily FamilyB(
            ExternalNodeIdentity("family-b"),
            "Specialized B",
            {NodeAvailability::Client},
            FamilyBPins,
            std::nullopt,
            std::nullopt,
            {
                DescriptorSpecializationVariant(
                    ExternalNodeIdentity("concrete-c"),
                    "opaque-key-c",
                    {
                        DescriptorSpecializationPinBinding(
                            PinIndex(1U),
                            TypeDesc::GUID()
                        )
                    }
                )
            }
        );

        const auto SpecializationResult =
            DescriptorFamilySpecializer::Specialize({FamilyB, FamilyA});
        MPP_CHECK(SpecializationResult.has_value());

        std::vector<NormalizedNodeDescriptorRecord> Records =
            SpecializationResult->GetConcreteRecords();
        Records.push_back(
            MakeRecord(
                "ordinary",
                "Ordinary",
                {MakeInput("Value", TypeDesc::Float())}
            )
        );
        const auto CatalogueResult = DescriptorCatalogueBuilder::Build(
            "snapshot.tests",
            "snapshot.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
        MPP_CHECK(CatalogueResult.has_value());

        const auto SnapshotResult = DescriptorCatalogueSnapshot::Create(
            *CatalogueResult,
            *SpecializationResult
        );
        MPP_CHECK(SnapshotResult.has_value());

        const auto JsonResult =
            DescriptorCatalogueSnapshotPersistence::Write(*SnapshotResult);
        MPP_CHECK(JsonResult.has_value());

        return SnapshotFixture(
            *CatalogueResult,
            *SpecializationResult,
            *SnapshotResult,
            *JsonResult
        );
    }

    std::string WriteFixture(const SnapshotFixture& Fixture)
    {
        const auto Result =
            DescriptorCatalogueSnapshotPersistence::Write(Fixture.Snapshot);
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    DescriptorCatalogue MakeTypeCatalogue()
    {
        std::vector<TypeDesc> Types = {
            TypeDesc::Boolean(),
            TypeDesc::Integer(),
            TypeDesc::Float(),
            TypeDesc::String(),
            TypeDesc::Flow(),
            TypeDesc::Entity(),
            TypeDesc::GUID(),
            TypeDesc::Vector3(),
            TypeDesc::PrefabId(),
            TypeDesc::ConfigId(),
            TypeDesc::Faction(),
            TypeDesc::Generic(GenericParameterId(1U)),
            TypeDesc::List(TypeDesc::Integer()),
            TypeDesc::Dictionary(
                TypeDesc::String(),
                TypeDesc::List(TypeDesc::Integer())
            ),
            TypeDesc::StructObject(StructTypeId(2U))
        };

        std::vector<NormalizedNodeDescriptorRecord> Records;
        for (std::size_t Index = 0U; Index < Types.size(); ++Index)
        {
            const bool IsFlow = Types[Index].GetKind() == TypeDesc::Kind::Flow;
            Records.push_back(
                MakeRecord(
                    "type-" + std::to_string(Index),
                    "Type",
                    {
                        IsFlow
                            ? MakeExecution("Value", PinDirection::Output)
                            : MakeInput("Value", Types[Index])
                    }
                )
            );
        }

        const auto Result = DescriptorCatalogueBuilder::Build(
            "type.tests",
            "type.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    DescriptorCatalogue MakeLiteralCatalogue()
    {
        const auto Result = DescriptorCatalogueBuilder::Build(
            "literal.tests",
            "literal.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::vector<NormalizedNodeDescriptorRecord>{
                MakeRecord(
                    "literal-bool",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::Boolean(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{true}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-int",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::Integer(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{std::int64_t(-9)}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-float",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::Float(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{
                                    std::bit_cast<double>(
                                        std::uint64_t(0x3ff8000000000000ULL)
                                    )
                                }
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-string",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::String(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{std::string("value")}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-guid",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::GUID(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{GuidValue{7U}}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-vector",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::Vector3(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{
                                    Vector3Value{1.0F, -2.0F, 3.0F}
                                }
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-prefab",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::PrefabId(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{PrefabIdValue{8U}}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-config",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::ConfigId(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{ConfigIdValue{9U}}
                            )
                        )
                    }
                ),
                MakeRecord(
                    "literal-faction",
                    "Literal",
                    {
                        MakeInput(
                            "Value",
                            TypeDesc::Faction(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{FactionValue{10U}}
                            )
                        )
                    }
                )
            }
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    NormalizedNodeDescriptorRecord MakeControlRecord(
        std::string Identity,
        std::vector<NormalizedPinRecord> Pins,
        ExecutionControlSchema Control,
        std::optional<SourceProvenance> Provenance = std::nullopt,
        std::vector<NodeAvailability> Availability = {
            NodeAvailability::Client
        }
    )
    {
        return MakeRecord(
            std::move(Identity),
            "Control",
            std::move(Pins),
            std::move(Control),
            std::move(Provenance),
            std::move(Availability)
        );
    }

    DescriptorCatalogue MakeControlCatalogue()
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.push_back(
            MakeControlRecord(
                "control-entry",
                {
                    MakeExecution("Out", PinDirection::Output)
                },
                EntryControlSchema{PinIndex(0U)},
                SourceProvenance("control.document", "entry"),
                {NodeAvailability::Server, NodeAvailability::Client}
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-sequence",
                {
                    MakeExecution("In", PinDirection::Input),
                    MakeExecution("Out", PinDirection::Output)
                },
                SequenceControlSchema{PinIndex(0U), PinIndex(1U)}
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-join",
                {
                    NormalizedPinRecord(
                        "In",
                        TypeDesc::Flow(),
                        PinDirection::Input,
                        PinCategory::Execution,
                        PinCardinality::Multiple
                    ),
                    MakeExecution("Out", PinDirection::Output)
                },
                JoinControlSchema{PinIndex(0U), PinIndex(1U)}
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-branch",
                {
                    MakeExecution("In", PinDirection::Input),
                    NormalizedPinRecord(
                        "Condition",
                        TypeDesc::Boolean(),
                        PinDirection::Input,
                        PinCategory::Data
                    ),
                    MakeExecution("True", PinDirection::Output),
                    MakeExecution("False", PinDirection::Output)
                },
                BranchControlSchema{
                    PinIndex(0U),
                    PinIndex(1U),
                    PinIndex(2U),
                    PinIndex(3U)
                }
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-loop-conditional",
                {
                    MakeExecution("In", PinDirection::Input),
                    MakeExecution("Body", PinDirection::Output),
                    MakeExecution("Exit", PinDirection::Output),
                    MakeExecution(
                        "Repeat",
                        PinDirection::Input,
                        PinCardinality::Multiple
                    ),
                    MakeExecution(
                        "Break",
                        PinDirection::Input,
                        PinCardinality::Multiple
                    ),
                    NormalizedPinRecord(
                        "Condition",
                        TypeDesc::Boolean(),
                        PinDirection::Input,
                        PinCategory::Data
                    )
                },
                LoopControlSchema{
                    PinIndex(0U),
                    PinIndex(1U),
                    PinIndex(2U),
                    PinIndex(3U),
                    PinIndex(4U),
                    LoopExitPolicy::Conditional,
                    PinIndex(5U)
                }
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-loop-unconditional",
                {
                    MakeExecution("In", PinDirection::Input),
                    MakeExecution("Body", PinDirection::Output),
                    MakeExecution("Exit", PinDirection::Output),
                    MakeExecution(
                        "Repeat",
                        PinDirection::Input,
                        PinCardinality::Multiple
                    ),
                    MakeExecution(
                        "Break",
                        PinDirection::Input,
                        PinCardinality::Multiple
                    )
                },
                LoopControlSchema{
                    PinIndex(0U),
                    PinIndex(1U),
                    PinIndex(2U),
                    PinIndex(3U),
                    PinIndex(4U),
                    LoopExitPolicy::Unconditional,
                    std::nullopt
                }
            )
        );
        Records.push_back(
            MakeControlRecord(
                "control-return",
                {
                    MakeExecution("In", PinDirection::Input)
                },
                ReturnControlSchema{PinIndex(0U)}
            )
        );

        const auto Result = DescriptorCatalogueBuilder::Build(
            "control.tests",
            "control.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    DescriptorSpecializationResult MakeMismatchedSpecialization(
        const SnapshotFixture& Fixture
    )
    {
        const DescriptorSpecializationFamily& Family =
            Fixture.Specialization.GetFamilies().front();
        const DescriptorSpecializationVariant& Variant =
            Family.GetVariants().front();
        DescriptorSpecializationVariant MismatchedVariant(
            ExternalNodeIdentity("missing-concrete"),
            "mismatch-key",
            Variant.GetPinBindings()
        );
        DescriptorSpecializationFamily MismatchedFamily(
            Family.GetFamilyExternalIdentity(),
            Family.GetDisplayName(),
            Family.GetAvailability(),
            Family.GetPins(),
            Family.GetExecutionControlSchema(),
            Family.GetSourceProvenance(),
            {MismatchedVariant}
        );
        const auto Result =
            DescriptorFamilySpecializer::Specialize({MismatchedFamily});
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    void CheckCatalogueFields(
        const DescriptorCatalogue& Expected,
        const DescriptorCatalogue& Actual
    )
    {
        MPP_CHECK(Expected == Actual);
        MPP_CHECK(
            Expected.GetIdentity().GetSourceNamespace() ==
            Actual.GetIdentity().GetSourceNamespace()
        );
        MPP_CHECK(
            Expected.GetIdentity().GetSourceRevision() ==
            Actual.GetIdentity().GetSourceRevision()
        );
        MPP_CHECK(
            Expected.GetIdentity().GetSemanticSchemaVersion() ==
            Actual.GetIdentity().GetSemanticSchemaVersion()
        );
        MPP_CHECK(
            Expected.GetIdentity().GetCatalogueContentIdentifier() ==
            Actual.GetIdentity().GetCatalogueContentIdentifier()
        );
        MPP_CHECK(Expected.GetEntryCount() == Actual.GetEntryCount());

        for (std::size_t Index = 0U; Index < Expected.GetEntries().size(); ++Index)
        {
            MPP_CHECK(
                Expected.GetEntries()[Index] == Actual.GetEntries()[Index]
            );
            MPP_CHECK(
                Expected.GetEntries()[Index].GetDescriptorIdentifier() ==
                Actual.GetEntries()[Index].GetDescriptorIdentifier()
            );
            MPP_CHECK(
                Expected.GetEntries()[Index].GetRecord() ==
                Actual.GetEntries()[Index].GetRecord()
            );
        }
    }

    void CheckNodeDescriptor(
        const NodeDescriptor& Actual,
        const DescriptorCatalogueEntry& Expected
    )
    {
        const NormalizedNodeDescriptorRecord& Record = Expected.GetRecord();
        MPP_CHECK(Actual.GetIdentifier() == Expected.GetDescriptorIdentifier());
        MPP_CHECK(Actual.GetName() == Record.GetDisplayName());
        MPP_CHECK(Actual.GetAvailability() == Record.GetAvailability());
        MPP_CHECK(Actual.GetPins().size() == Record.GetPins().size());
        MPP_CHECK(
            Actual.GetExecutionControlSchema().has_value() ==
            Record.GetExecutionControlSchema().has_value()
        );

        for (std::size_t Index = 0U; Index < Record.GetPins().size(); ++Index)
        {
            const PinSchema& ActualPin = Actual.GetPins()[Index];
            const NormalizedPinRecord& ExpectedPin = Record.GetPins()[Index];
            MPP_CHECK(ActualPin.GetName() == ExpectedPin.GetName());
            MPP_CHECK(ActualPin.GetType() == ExpectedPin.GetType());
            MPP_CHECK(ActualPin.GetDirection() == ExpectedPin.GetDirection());
            MPP_CHECK(ActualPin.GetCategory() == ExpectedPin.GetCategory());
            MPP_CHECK(
                ActualPin.GetCardinality() == ExpectedPin.GetCardinality()
            );
            MPP_CHECK(ActualPin.AllowsLiteral() == ExpectedPin.AllowsLiteral());
            MPP_CHECK(
                ActualPin.GetDefaultValue().has_value() ==
                ExpectedPin.GetDefaultValue().has_value()
            );
            if (ActualPin.GetDefaultValue().has_value())
            {
                MPP_CHECK(
                    DescriptorCatalogueDetail::AreNormalizedLiteralValuesEqual(
                        *ActualPin.GetDefaultValue(),
                        *ExpectedPin.GetDefaultValue()
                    )
                );
            }
        }

        if (Record.GetExecutionControlSchema().has_value())
        {
            MPP_CHECK(
                DescriptorCatalogueDetail::AreExecutionControlSchemasEqual(
                    *Actual.GetExecutionControlSchema(),
                    *Record.GetExecutionControlSchema()
                )
            );
        }
    }

    void TestDescriptorCatalogueSnapshotWriteSuccess()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto Snapshot = DescriptorCatalogueSnapshot::Create(
            Fixture.Catalogue
        );
        MPP_CHECK(Snapshot.has_value());
        MPP_CHECK(Snapshot->IsValid());

        const auto Written =
            DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Written.has_value());
        const JsonValue Document = JsonValue::parse(*Written);

        MPP_CHECK(
            HasExactMembers(
                Document,
                {"catalogue", "entries", "snapshotFormatVersion", "specialization"}
            )
        );
        MPP_CHECK(Document["snapshotFormatVersion"] == 2U);
        MPP_CHECK(Document["catalogue"].is_object());
        MPP_CHECK(
            HasExactMembers(
                Document["catalogue"],
                {
                    "contentIdentifier",
                    "semanticSchemaVersion",
                    "sourceNamespace",
                    "sourceRevision"
                }
            )
        );

        const auto& Identity = Fixture.Catalogue.GetIdentity();
        MPP_CHECK(
            Document["catalogue"]["contentIdentifier"] ==
            Identity.GetCatalogueContentIdentifier().GetValue()
        );
        MPP_CHECK(
            Document["catalogue"]["semanticSchemaVersion"] ==
            Identity.GetSemanticSchemaVersion().GetValue()
        );
        MPP_CHECK(
            Document["catalogue"]["sourceNamespace"] ==
            Identity.GetSourceNamespace()
        );
        MPP_CHECK(
            Document["catalogue"]["sourceRevision"] ==
            Identity.GetSourceRevision()
        );
        MPP_CHECK(Document["entries"].is_array());
        MPP_CHECK(
            Document["entries"].size() == Fixture.Catalogue.GetEntryCount()
        );
        MPP_CHECK(Document["specialization"].is_null());

        const auto& CatalogueEntry = Fixture.Catalogue.GetEntries().front();
        const JsonValue& SerializedEntry = Document["entries"].front();
        MPP_CHECK(
            HasExactMembers(SerializedEntry, {"nodeDescriptorId", "record"})
        );
        MPP_CHECK(
            SerializedEntry["nodeDescriptorId"] ==
            CatalogueEntry.GetDescriptorIdentifier().GetValue()
        );

        const JsonValue& Record = SerializedEntry["record"];
        MPP_CHECK(
            HasExactMembers(
                Record,
                {
                    "availability",
                    "displayName",
                    "executionControl",
                    "externalIdentity",
                    "pins",
                    "provenance"
                }
            )
        );
        MPP_CHECK(
            Record["externalIdentity"] ==
            CatalogueEntry.GetExternalIdentity().GetKey()
        );
        MPP_CHECK(
            Record["displayName"] == CatalogueEntry.GetRecord().GetDisplayName()
        );
        MPP_CHECK(Record["executionControl"].is_null());
        MPP_CHECK(Record["provenance"].is_object());
        MPP_CHECK(
            Record["provenance"]["sourceDocumentIdentifier"] ==
            "logical.document"
        );
        MPP_CHECK(
            Record["provenance"]["sourceRecordIdentifier"] == "family-a"
        );
        MPP_CHECK(Record["availability"].size() == 2U);
        MPP_CHECK(Record["availability"][0U] == "Server");
        MPP_CHECK(Record["availability"][1U] == "Client");
        MPP_CHECK(Record["pins"].size() == 3U);
        MPP_CHECK(Record["pins"][0U]["name"] == "Fixed");
        MPP_CHECK(Record["pins"][0U]["type"]["kind"] == "Integer");
        MPP_CHECK(Record["pins"][0U]["direction"] == "Input");
        MPP_CHECK(Record["pins"][0U]["category"] == "Data");
        MPP_CHECK(Record["pins"][0U]["cardinality"] == "Single");
        MPP_CHECK(Record["pins"][0U]["allowsLiteral"] == true);
        MPP_CHECK(Record["pins"][0U]["default"]["kind"] == "Integer");
        MPP_CHECK(Record["pins"][0U]["default"]["value"] == 7);
    }

    void TestDescriptorCatalogueSnapshotReadSuccess()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto Snapshot = DescriptorCatalogueSnapshot::Create(
            Fixture.Catalogue
        );
        MPP_CHECK(Snapshot.has_value());
        const auto Written =
            DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Written.has_value());
        const auto Read =
            DescriptorCatalogueSnapshotPersistence::Read(*Written);
        MPP_CHECK(Read.has_value());
        MPP_CHECK(Read->IsValid());
        CheckCatalogueFields(Fixture.Catalogue, Read->GetCatalogue());
        MPP_CHECK(!Read->GetSpecializationResult().has_value());

        const auto& ExpectedIdentity = Fixture.Catalogue.GetIdentity();
        const auto& ActualIdentity = Read->GetCatalogue().GetIdentity();
        MPP_CHECK(
            ActualIdentity.GetSourceNamespace() ==
            ExpectedIdentity.GetSourceNamespace()
        );
        MPP_CHECK(
            ActualIdentity.GetSourceRevision() ==
            ExpectedIdentity.GetSourceRevision()
        );
        MPP_CHECK(
            ActualIdentity.GetSemanticSchemaVersion() ==
            ExpectedIdentity.GetSemanticSchemaVersion()
        );
        MPP_CHECK(
            ActualIdentity.GetCatalogueContentIdentifier() ==
            ExpectedIdentity.GetCatalogueContentIdentifier()
        );
        MPP_CHECK(
            Read->GetCatalogue().GetEntryCount() ==
            Fixture.Catalogue.GetEntryCount()
        );
        for (std::size_t Index = 0U; Index < Fixture.Catalogue.GetEntries().size(); ++Index)
        {
            MPP_CHECK(
                Read->GetCatalogue().GetEntries()[Index].GetDescriptorIdentifier() ==
                Fixture.Catalogue.GetEntries()[Index].GetDescriptorIdentifier()
            );
            MPP_CHECK(
                Read->GetCatalogue().GetEntries()[Index].GetRecord() ==
                Fixture.Catalogue.GetEntries()[Index].GetRecord()
            );
        }
    }

    void TestDescriptorCatalogueSnapshotDeterministicBytes()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const SnapshotFixture Independent = MakeFixture();
        MPP_CHECK(WriteFixture(Fixture) == WriteFixture(Fixture));
        MPP_CHECK(Fixture.Json == Independent.Json);

        std::vector<NormalizedNodeDescriptorRecord> PermutedRecords;
        for (auto Iterator = Fixture.Catalogue.GetEntries().rbegin();
             Iterator != Fixture.Catalogue.GetEntries().rend();
             ++Iterator)
        {
            PermutedRecords.push_back(Iterator->GetRecord());
        }
        const auto PermutedCatalogue = DescriptorCatalogueBuilder::Build(
            Fixture.Catalogue.GetIdentity().GetSourceNamespace(),
            Fixture.Catalogue.GetIdentity().GetSourceRevision(),
            Fixture.Catalogue.GetIdentity().GetSemanticSchemaVersion(),
            std::move(PermutedRecords)
        );
        MPP_CHECK(PermutedCatalogue.has_value());
        const auto PermutedSnapshot =
            DescriptorCatalogueSnapshot::Create(*PermutedCatalogue);
        MPP_CHECK(PermutedSnapshot.has_value());
        const auto PermutedJson =
            DescriptorCatalogueSnapshotPersistence::Write(*PermutedSnapshot);
        MPP_CHECK(PermutedJson.has_value());

        const auto CanonicalNoSidecar =
            DescriptorCatalogueSnapshot::Create(Fixture.Catalogue);
        MPP_CHECK(CanonicalNoSidecar.has_value());
        const auto CanonicalJson =
            DescriptorCatalogueSnapshotPersistence::Write(*CanonicalNoSidecar);
        MPP_CHECK(CanonicalJson.has_value());
        MPP_CHECK(*PermutedJson == *CanonicalJson);

        const JsonValue Document = JsonValue::parse(Fixture.Json);
        MPP_CHECK(
            Document["entries"][0U]["record"]["externalIdentity"] ==
            Fixture.Catalogue.GetEntries()[0U].GetExternalIdentity().GetKey()
        );
        MPP_CHECK(
            Document["entries"][0U]["record"]["externalIdentity"] <
            Document["entries"][1U]["record"]["externalIdentity"]
        );
    }

    void TestDescriptorCatalogueSnapshotRoundTripEquality()
    {
        const SnapshotFixture Fixture = MakeFixture();

        const auto NoSidecar =
            DescriptorCatalogueSnapshot::Create(Fixture.Catalogue);
        MPP_CHECK(NoSidecar.has_value());
        const auto NoSidecarJson =
            DescriptorCatalogueSnapshotPersistence::Write(*NoSidecar);
        MPP_CHECK(NoSidecarJson.has_value());
        const auto NoSidecarRead =
            DescriptorCatalogueSnapshotPersistence::Read(*NoSidecarJson);
        MPP_CHECK(NoSidecarRead.has_value());
        MPP_CHECK(*NoSidecarRead == *NoSidecar);
        CheckCatalogueFields(
            NoSidecar->GetCatalogue(),
            NoSidecarRead->GetCatalogue()
        );
        MPP_CHECK(
            NoSidecarRead->GetSpecializationResult().has_value() ==
            NoSidecar->GetSpecializationResult().has_value()
        );

        const auto WithSidecar =
            DescriptorCatalogueSnapshot::Create(
                Fixture.Catalogue,
                Fixture.Specialization
            );
        MPP_CHECK(WithSidecar.has_value());
        const auto WithSidecarJson =
            DescriptorCatalogueSnapshotPersistence::Write(*WithSidecar);
        MPP_CHECK(WithSidecarJson.has_value());
        const auto WithSidecarRead =
            DescriptorCatalogueSnapshotPersistence::Read(*WithSidecarJson);
        MPP_CHECK(WithSidecarRead.has_value());
        MPP_CHECK(*WithSidecarRead == *WithSidecar);
        CheckCatalogueFields(
            WithSidecar->GetCatalogue(),
            WithSidecarRead->GetCatalogue()
        );
        MPP_CHECK(WithSidecarRead->GetSpecializationResult().has_value());
        MPP_CHECK(
            *WithSidecarRead->GetSpecializationResult() ==
            *WithSidecar->GetSpecializationResult()
        );
        MPP_CHECK(
            WithSidecarRead->GetSpecializationResult()->GetConcreteRecords().size() ==
            Fixture.Specialization.GetConcreteRecords().size()
        );
    }

    void TestDescriptorCatalogueSnapshotIdentityAndContentValidation()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto SuccessfulRead =
            DescriptorCatalogueSnapshotPersistence::Read(Fixture.Json);
        MPP_CHECK(SuccessfulRead.has_value());
        const auto& ExpectedIdentity = Fixture.Catalogue.GetIdentity();
        const auto& ActualIdentity = SuccessfulRead->GetCatalogue().GetIdentity();
        MPP_CHECK(
            ActualIdentity.GetSourceNamespace() ==
            ExpectedIdentity.GetSourceNamespace()
        );
        MPP_CHECK(
            ActualIdentity.GetSourceRevision() ==
            ExpectedIdentity.GetSourceRevision()
        );
        MPP_CHECK(
            ActualIdentity.GetSemanticSchemaVersion() ==
            ExpectedIdentity.GetSemanticSchemaVersion()
        );
        MPP_CHECK(
            ActualIdentity.GetCatalogueContentIdentifier() ==
            ExpectedIdentity.GetCatalogueContentIdentifier()
        );

        const auto InvalidCatalogue =
            DescriptorCatalogueSnapshot::Create(DescriptorCatalogue{});
        MPP_CHECK(!InvalidCatalogue.has_value());
        MPP_CHECK(
            HasCode(
                InvalidCatalogue.error(),
                DiagnosticCode::DescriptorCatalogueMismatch
            )
        );

        JsonValue Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"]["contentIdentifier"] = "";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::InvalidDescriptorCatalogueIdentity
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"]["contentIdentifier"] = "sha256:tampered";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::DescriptorCatalogueSnapshotContentMismatch
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"]["semanticSchemaVersion"] = 999U;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSemanticSchemaVersion
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"]["sourceNamespace"] = "";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::InvalidDescriptorCatalogueIdentity
        );
    }

    void TestDescriptorCatalogueSnapshotVersionAndSchemaDiagnostics()
    {
        const SnapshotFixture Fixture = MakeFixture();
        JsonValue Json = JsonValue::parse(Fixture.Json);

        Json.erase("snapshotFormatVersion");
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["snapshotFormatVersion"] = "1";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["snapshotFormatVersion"] = 0U;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSnapshotVersion
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["snapshotFormatVersion"] = 3U;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::UnsupportedDescriptorCatalogueSnapshotVersion
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["unknown"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json.erase("catalogue");
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"] = nullptr;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["entries"] = nullptr;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"] = JsonValue::array();
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );
    }

    void TestDescriptorCatalogueSnapshotNormalizedRecordRoundTrip()
    {
        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            "pin.tests",
            "pin.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::vector<NormalizedNodeDescriptorRecord>{
                MakeRecord(
                    "pins",
                    "Pins",
                    {
                        MakeInput(
                            "Zeta",
                            TypeDesc::Integer(),
                            PinCardinality::Optional,
                            true,
                            LiteralValue(
                                LiteralValue::Data{std::int64_t(11)}
                            )
                        ),
                        NormalizedPinRecord(
                            "Alpha",
                            TypeDesc::Float(),
                            PinDirection::Output,
                            PinCategory::Data,
                            PinCardinality::Multiple
                        ),
                        MakeExecution("Middle", PinDirection::Input)
                    }
                )
            }
        );
        MPP_CHECK(Catalogue.has_value());

        const auto Snapshot = DescriptorCatalogueSnapshot::Create(*Catalogue);
        MPP_CHECK(Snapshot.has_value());
        const auto Json =
            DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Json.has_value());

        const JsonValue Document = JsonValue::parse(*Json);
        const JsonValue& Pins =
            Document["entries"][0U]["record"]["pins"];
        MPP_CHECK(Pins.size() == 3U);
        MPP_CHECK(Pins[0U]["name"] == "Zeta");
        MPP_CHECK(Pins[0U]["type"]["kind"] == "Integer");
        MPP_CHECK(Pins[0U]["direction"] == "Input");
        MPP_CHECK(Pins[0U]["category"] == "Data");
        MPP_CHECK(Pins[0U]["cardinality"] == "Optional");
        MPP_CHECK(Pins[0U]["allowsLiteral"] == true);
        MPP_CHECK(Pins[0U]["default"]["kind"] == "Integer");
        MPP_CHECK(Pins[0U]["default"]["value"] == 11);
        MPP_CHECK(Pins[1U]["name"] == "Alpha");
        MPP_CHECK(Pins[1U]["direction"] == "Output");
        MPP_CHECK(Pins[1U]["category"] == "Data");
        MPP_CHECK(Pins[1U]["cardinality"] == "Multiple");
        MPP_CHECK(Pins[1U]["default"].is_null());
        MPP_CHECK(Pins[2U]["name"] == "Middle");
        MPP_CHECK(Pins[2U]["type"]["kind"] == "Flow");
        MPP_CHECK(Pins[2U]["direction"] == "Input");
        MPP_CHECK(Pins[2U]["category"] == "Execution");
        MPP_CHECK(Pins[2U]["cardinality"] == "Single");

        const auto Read =
            DescriptorCatalogueSnapshotPersistence::Read(*Json);
        MPP_CHECK(Read.has_value());
        const auto& ReadPins =
            Read->GetCatalogue().GetEntries()[0U].GetRecord().GetPins();
        MPP_CHECK(ReadPins.size() == 3U);
        MPP_CHECK(ReadPins[0U].GetName() == "Zeta");
        MPP_CHECK(ReadPins[1U].GetName() == "Alpha");
        MPP_CHECK(ReadPins[2U].GetName() == "Middle");
        MPP_CHECK(ReadPins[0U].GetCardinality() == PinCardinality::Optional);
        MPP_CHECK(ReadPins[1U].GetCardinality() == PinCardinality::Multiple);
        MPP_CHECK(ReadPins[2U].GetCategory() == PinCategory::Execution);
        MPP_CHECK(ReadPins[0U].GetDefaultValue().has_value());
        MPP_CHECK(
            ReadPins[0U].GetDefaultValue()->TryGet<std::int64_t>() != nullptr
        );
    }

    void TestDescriptorCatalogueSnapshotTypeAndLiteralRoundTrip()
    {
        const auto TypeCatalogue = MakeTypeCatalogue();
        const auto TypeSnapshot =
            DescriptorCatalogueSnapshot::Create(TypeCatalogue);
        MPP_CHECK(TypeSnapshot.has_value());
        const auto TypeJson =
            DescriptorCatalogueSnapshotPersistence::Write(*TypeSnapshot);
        MPP_CHECK(TypeJson.has_value());
        const JsonValue TypeDocument = JsonValue::parse(*TypeJson);
        const std::vector<std::string> TypeKinds = {
            "Boolean",
            "Integer",
            "Float",
            "String",
            "Flow",
            "Entity",
            "GUID",
            "Vector3",
            "PrefabId",
            "ConfigId",
            "Faction",
            "Generic",
            "List",
            "Dictionary",
            "StructObject"
        };
        MPP_CHECK(TypeDocument["entries"].size() == TypeKinds.size());
        for (std::size_t Index = 0U; Index < TypeKinds.size(); ++Index)
        {
            const JsonValue* Entry = FindJsonEntry(
                const_cast<JsonValue&>(TypeDocument),
                "type-" + std::to_string(Index)
            );
            MPP_CHECK(Entry != nullptr);
            MPP_CHECK(
                (*Entry)["record"]["pins"][0U]["type"]["kind"] ==
                TypeKinds[Index]
            );
        }

        const auto TypeRead =
            DescriptorCatalogueSnapshotPersistence::Read(*TypeJson);
        MPP_CHECK(TypeRead.has_value());
        MPP_CHECK(*TypeRead == *TypeSnapshot);

        const auto LiteralCatalogue = MakeLiteralCatalogue();
        const auto LiteralSnapshot =
            DescriptorCatalogueSnapshot::Create(LiteralCatalogue);
        MPP_CHECK(LiteralSnapshot.has_value());
        const auto LiteralJson =
            DescriptorCatalogueSnapshotPersistence::Write(*LiteralSnapshot);
        MPP_CHECK(LiteralJson.has_value());
        const JsonValue LiteralDocument = JsonValue::parse(*LiteralJson);
        MPP_CHECK(
            LiteralDocument["entries"].size() == 9U
        );
        const JsonValue& FloatLiteral =
            FindJsonEntry(
                const_cast<JsonValue&>(LiteralDocument),
                "literal-float"
            )->at("record").at("pins").at(0U).at("default");
        MPP_CHECK(FloatLiteral["kind"] == "Float");
        MPP_CHECK(FloatLiteral["bits"] == "3ff8000000000000");
        const JsonValue& VectorLiteral =
            FindJsonEntry(
                const_cast<JsonValue&>(LiteralDocument),
                "literal-vector"
            )->at("record").at("pins").at(0U).at("default");
        MPP_CHECK(VectorLiteral["xBits"] == "3f800000");
        MPP_CHECK(VectorLiteral["yBits"] == "c0000000");
        MPP_CHECK(VectorLiteral["zBits"] == "40400000");

        const auto LiteralRead =
            DescriptorCatalogueSnapshotPersistence::Read(*LiteralJson);
        MPP_CHECK(LiteralRead.has_value());
        MPP_CHECK(*LiteralRead == *LiteralSnapshot);

        JsonValue Mutated = JsonValue::parse(*TypeJson);
        FindJsonEntry(Mutated, "type-0")->at("record").at("pins").at(0U).at("type")["kind"] = "Unknown";
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*TypeJson);
        FindJsonEntry(Mutated, "type-11")->at("record").at("pins").at(0U).at("type")["parameter"] = 0U;
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*TypeJson);
        FindJsonEntry(Mutated, "type-14")->at("record").at("pins").at(0U).at("type")["structType"] = 0U;
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*LiteralJson);
        FindJsonEntry(Mutated, "literal-float")->at("record").at("pins").at(0U).at("default")["bits"] = "3FF8000000000000";
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*LiteralJson);
        FindJsonEntry(Mutated, "literal-float")->at("record").at("pins").at(0U).at("default")["bits"] = "123";
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*LiteralJson);
        FindJsonEntry(Mutated, "literal-float")->at("record").at("pins").at(0U).at("default")["bits"] = "3ff800000000000g";
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Mutated = JsonValue::parse(*LiteralJson);
        FindJsonEntry(Mutated, "literal-bool")->at("record").at("pins").at(0U).at("default")["kind"] = "Unknown";
        RequireReadCode(
            Mutated.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );
    }

    void TestDescriptorCatalogueSnapshotControlAndProvenanceRoundTrip()
    {
        const auto Catalogue = MakeControlCatalogue();
        const auto Snapshot = DescriptorCatalogueSnapshot::Create(Catalogue);
        MPP_CHECK(Snapshot.has_value());
        const auto Json =
            DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Json.has_value());
        const JsonValue Document = JsonValue::parse(*Json);

        const std::vector<std::pair<std::string, std::string>> Kinds = {
            {"control-entry", "Entry"},
            {"control-sequence", "Sequence"},
            {"control-branch", "Branch"},
            {"control-join", "Join"},
            {"control-loop-conditional", "Loop"},
            {"control-loop-unconditional", "Loop"},
            {"control-return", "Return"}
        };
        for (const auto& [Identity, Kind] : Kinds)
        {
            JsonValue* Entry = FindJsonEntry(
                const_cast<JsonValue&>(Document),
                Identity
            );
            MPP_CHECK(Entry != nullptr);
            MPP_CHECK(
                (*Entry)["record"]["executionControl"]["kind"] == Kind
            );
        }
        MPP_CHECK(
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-sequence"
            )->at("record").at("executionControl").at("kind") == "Sequence"
        );
        MPP_CHECK(
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-join"
            )->at("record").at("executionControl").at("kind") == "Join"
        );

        const JsonValue& ConditionalControl =
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-loop-conditional"
            )->at("record").at("executionControl");
        MPP_CHECK(ConditionalControl["exitPolicy"] == "Conditional");
        MPP_CHECK(ConditionalControl["conditionInput"].is_number_unsigned());
        const JsonValue& UnconditionalControl =
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-loop-unconditional"
            )->at("record").at("executionControl");
        MPP_CHECK(UnconditionalControl["exitPolicy"] == "Unconditional");
        MPP_CHECK(UnconditionalControl["conditionInput"].is_null());

        const JsonValue& EntryRecord =
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-entry"
            )->at("record");
        MPP_CHECK(EntryRecord["availability"][0U] == "Server");
        MPP_CHECK(EntryRecord["availability"][1U] == "Client");
        MPP_CHECK(EntryRecord["provenance"]["sourceDocumentIdentifier"] == "control.document");
        MPP_CHECK(EntryRecord["provenance"]["sourceRecordIdentifier"] == "entry");
        const JsonValue& SequenceRecord =
            FindJsonEntry(
                const_cast<JsonValue&>(Document),
                "control-sequence"
            )->at("record");
        MPP_CHECK(SequenceRecord["provenance"].is_null());

        const auto Read =
            DescriptorCatalogueSnapshotPersistence::Read(*Json);
        MPP_CHECK(Read.has_value());
        const auto* SequenceEntry =
            FindCatalogueEntry(Read->GetCatalogue(), "control-sequence");
        const auto* JoinEntry =
            FindCatalogueEntry(Read->GetCatalogue(), "control-join");
        const auto* ConditionalEntry =
            FindCatalogueEntry(Read->GetCatalogue(), "control-loop-conditional");
        const auto* UnconditionalEntry =
            FindCatalogueEntry(Read->GetCatalogue(), "control-loop-unconditional");
        MPP_CHECK(SequenceEntry != nullptr && JoinEntry != nullptr);
        MPP_CHECK(ConditionalEntry != nullptr && UnconditionalEntry != nullptr);
        MPP_CHECK(
            std::holds_alternative<SequenceControlSchema>(
                *SequenceEntry->GetRecord().GetExecutionControlSchema()
            )
        );
        MPP_CHECK(
            std::holds_alternative<JoinControlSchema>(
                *JoinEntry->GetRecord().GetExecutionControlSchema()
            )
        );
        const LoopControlSchema& Conditional =
            std::get<LoopControlSchema>(
                *ConditionalEntry->GetRecord().GetExecutionControlSchema()
            );
        const LoopControlSchema& Unconditional =
            std::get<LoopControlSchema>(
                *UnconditionalEntry->GetRecord().GetExecutionControlSchema()
            );
        MPP_CHECK(Conditional.ExitPolicy == LoopExitPolicy::Conditional);
        MPP_CHECK(
            Conditional.ConditionInput.has_value() &&
            Conditional.ConditionInput->GetValue() == 5U
        );
        MPP_CHECK(Unconditional.ExitPolicy == LoopExitPolicy::Unconditional);
        MPP_CHECK(!Unconditional.ConditionInput.has_value());
        MPP_CHECK(
            Read->GetCatalogue().FindByExternalIdentity(
                ExternalNodeIdentity("control-entry")
            )->GetRecord().GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            "control.document"
        );
        MPP_CHECK(
            Read->GetCatalogue().FindByExternalIdentity(
                ExternalNodeIdentity("control-sequence")
            )->GetRecord().GetSourceProvenance().has_value() == false
        );
        const std::vector<NodeAvailability> ExpectedAvailability = {
            NodeAvailability::Server,
            NodeAvailability::Client
        };
        MPP_CHECK(
            Read->GetCatalogue().FindByExternalIdentity(
                ExternalNodeIdentity("control-entry")
            )->GetRecord().GetAvailability() == ExpectedAvailability
        );
    }

    void TestDescriptorCatalogueSnapshotSpecializationSidecarRoundTrip()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto Snapshot =
            DescriptorCatalogueSnapshot::Create(
                Fixture.Catalogue,
                Fixture.Specialization
            );
        MPP_CHECK(Snapshot.has_value());
        const auto Json =
            DescriptorCatalogueSnapshotPersistence::Write(*Snapshot);
        MPP_CHECK(Json.has_value());
        JsonValue Document = JsonValue::parse(*Json);

        const JsonValue& Specialization = Document["specialization"];
        MPP_CHECK(HasExactMembers(Specialization, {"families"}));
        MPP_CHECK(Specialization["families"].is_array());
        MPP_CHECK(Specialization["families"].size() == 2U);
        const JsonValue& Family = Specialization["families"][0U];
        MPP_CHECK(
            HasExactMembers(
                Family,
                {
                    "familyExternalIdentity",
                    "displayName",
                    "availability",
                    "pins",
                    "executionControl",
                    "provenance",
                    "variants"
                }
            )
        );
        MPP_CHECK(Family["familyExternalIdentity"] == "family-a");
        MPP_CHECK(Family["displayName"] == "Specialized A");
        MPP_CHECK(Family["availability"][0U] == "Server");
        MPP_CHECK(Family["availability"][1U] == "Client");
        MPP_CHECK(Family["executionControl"].is_null());
        MPP_CHECK(
            Family["provenance"]["sourceDocumentIdentifier"] ==
            "logical.document"
        );
        MPP_CHECK(
            Family["provenance"]["sourceRecordIdentifier"] == "family-a"
        );
        MPP_CHECK(Family["pins"].size() == 3U);
        const JsonValue& FixedPin = Family["pins"][0U];
        const JsonValue& ReflectedPin = Family["pins"][1U];
        MPP_CHECK(
            HasExactMembers(
                FixedPin,
                {
                    "name",
                    "fixedType",
                    "reflected",
                    "direction",
                    "category",
                    "cardinality",
                    "allowsLiteral",
                    "default"
                }
            )
        );
        MPP_CHECK(FixedPin["name"] == "Fixed");
        MPP_CHECK(FixedPin["fixedType"]["kind"] == "Integer");
        MPP_CHECK(FixedPin["reflected"] == false);
        MPP_CHECK(FixedPin["direction"] == "Input");
        MPP_CHECK(FixedPin["category"] == "Data");
        MPP_CHECK(FixedPin["cardinality"] == "Single");
        MPP_CHECK(FixedPin["allowsLiteral"] == true);
        MPP_CHECK(FixedPin["default"]["value"] == 7);
        MPP_CHECK(ReflectedPin["fixedType"].is_null());
        MPP_CHECK(ReflectedPin["reflected"] == true);
        MPP_CHECK(ReflectedPin["direction"] == "Input");
        MPP_CHECK(ReflectedPin["category"] == "Data");
        MPP_CHECK(ReflectedPin["cardinality"] == "Optional");
        MPP_CHECK(Family["variants"].size() == 2U);
        const JsonValue& Variant = Family["variants"][0U];
        MPP_CHECK(
            HasExactMembers(
                Variant,
                {
                    "concreteExternalIdentity",
                    "specializationKey",
                    "pinBindings"
                }
            )
        );
        MPP_CHECK(Variant["concreteExternalIdentity"] == "concrete-a");
        MPP_CHECK(Variant["specializationKey"] == "opaque-key-a");
        MPP_CHECK(Variant["pinBindings"].size() == 2U);
        MPP_CHECK(Variant["pinBindings"][0U]["familyPinIndex"] == 1U);
        MPP_CHECK(Variant["pinBindings"][0U]["concreteType"]["kind"] == "Integer");
        MPP_CHECK(Variant["pinBindings"][1U]["familyPinIndex"] == 2U);
        MPP_CHECK(Variant["pinBindings"][1U]["concreteType"]["kind"] == "Float");
        MPP_CHECK(!Variant.contains("record"));
        MPP_CHECK(!Family.contains("record"));
        MPP_CHECK(
            Specialization["families"][0U]["familyExternalIdentity"] <
            Specialization["families"][1U]["familyExternalIdentity"]
        );
        MPP_CHECK(
            Family["variants"][0U]["concreteExternalIdentity"] <
            Family["variants"][1U]["concreteExternalIdentity"]
        );

        const JsonValue& FamilyB = Specialization["families"][1U];
        MPP_CHECK(FamilyB["provenance"].is_null());
        MPP_CHECK(FamilyB["pins"][0U]["fixedType"]["kind"] == "Boolean");
        MPP_CHECK(FamilyB["pins"][0U]["reflected"] == false);
        MPP_CHECK(FamilyB["pins"][1U]["fixedType"].is_null());
        MPP_CHECK(FamilyB["pins"][1U]["reflected"] == true);
        MPP_CHECK(FamilyB["variants"][0U]["pinBindings"][0U]["familyPinIndex"] == 1U);

        const auto Read =
            DescriptorCatalogueSnapshotPersistence::Read(*Json);
        MPP_CHECK(Read.has_value());
        MPP_CHECK(Read->GetSpecializationResult().has_value());
        MPP_CHECK(
            *Read->GetSpecializationResult() == Fixture.Specialization
        );
        MPP_CHECK(Read->GetSpecializationResult()->IsValid());
        for (const auto& Record :
             Read->GetSpecializationResult()->GetConcreteRecords())
        {
            const DescriptorCatalogueEntry* Entry =
                Fixture.Catalogue.FindByExternalIdentity(
                    Record.GetExternalIdentity()
                );
            MPP_CHECK(Entry != nullptr);
            MPP_CHECK(Entry->GetRecord() == Record);
        }
        MPP_CHECK(
            Fixture.Catalogue.FindByExternalIdentity(
                ExternalNodeIdentity("ordinary")
            ) != nullptr
        );
        MPP_CHECK(
            Read->GetSpecializationResult()->GetConcreteRecords().size() == 3U
        );

        const auto Mismatched =
            MakeMismatchedSpecialization(Fixture);
        const auto MismatchSnapshot =
            DescriptorCatalogueSnapshot::Create(
                Fixture.Catalogue,
                Mismatched
            );
        MPP_CHECK(!MismatchSnapshot.has_value());
        MPP_CHECK(
            HasCode(
                MismatchSnapshot.error(),
                DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch
            )
        );
    }

    void TestDescriptorCatalogueSnapshotMalformedDataAndFailureAtomicity()
    {
        const SnapshotFixture Fixture = MakeFixture();

        RequireReadCode(
            "{",
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        std::string DuplicateRoot = Fixture.Json;
        const std::size_t RootPosition =
            DuplicateRoot.find("\"snapshotFormatVersion\":");
        MPP_CHECK(RootPosition != std::string::npos);
        DuplicateRoot.insert(
            RootPosition,
            "\"snapshotFormatVersion\":1,"
        );
        RequireReadCode(
            DuplicateRoot,
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        std::string DuplicateNested = Fixture.Json;
        const std::size_t NestedPosition =
            DuplicateNested.find("\"sourceNamespace\":");
        MPP_CHECK(NestedPosition != std::string::npos);
        DuplicateNested.insert(
            NestedPosition,
            "\"sourceNamespace\":\"duplicate\","
        );
        RequireReadCode(
            DuplicateNested,
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        JsonValue Json = JsonValue::parse(Fixture.Json);
        Json["unknown"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        FindJsonEntry(Json, "concrete-a")->at("record")["unknown"] = 1;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["unexpected"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["families"][0U]["unexpected"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["families"][0U]["pins"][0U]["unexpected"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["families"][0U]["variants"][0U]["unexpected"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["families"][0U]["variants"][0U]["pinBindings"][0U]["unexpected"] = true;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        FindJsonEntry(Json, "concrete-a")->at("record").at("displayName") = 7;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        std::swap(Json["entries"][0U], Json["entries"][1U]);
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        std::swap(
            Json["entries"][0U]["record"]["availability"][0U],
            Json["entries"][0U]["record"]["availability"][1U]
        );
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        std::swap(
            Json["specialization"]["families"][0U],
            Json["specialization"]["families"][1U]
        );
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        std::swap(
            Json["specialization"]["families"][0U]["variants"][0U],
            Json["specialization"]["families"][0U]["variants"][1U]
        );
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        std::swap(
            Json["specialization"]["families"][0U]["variants"][0U]["pinBindings"][0U],
            Json["specialization"]["families"][0U]["variants"][0U]["pinBindings"][1U]
        );
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        for (const char* Field : {"direction", "category", "cardinality"})
        {
            Json = JsonValue::parse(Fixture.Json);
            FindJsonEntry(Json, "concrete-a")->at("record").at("pins").at(0U)[Field] =
                "invalid";
            RequireReadCode(
                Json.dump(),
                DiagnosticCode::MalformedDescriptorCatalogueSnapshot
            );
        }

        const auto ControlCatalogue = MakeControlCatalogue();
        const auto ControlSnapshot =
            DescriptorCatalogueSnapshot::Create(ControlCatalogue);
        MPP_CHECK(ControlSnapshot.has_value());
        const auto ControlJson =
            DescriptorCatalogueSnapshotPersistence::Write(*ControlSnapshot);
        MPP_CHECK(ControlJson.has_value());

        JsonValue ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-entry"
        )->at("record").at("executionControl") =
            JsonValue{{"executionOutput", 0U}};
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-entry"
        )->at("record").at("executionControl")["kind"] = "Unknown";
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-entry"
        )->at("record").at("executionControl")["extra"] = true;
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-loop-conditional"
        )->at("record").at("executionControl")["exitPolicy"] = "Invalid";
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-loop-conditional"
        )->at("record").at("executionControl")["conditionInput"] = nullptr;
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        ControlDocument = JsonValue::parse(*ControlJson);
        FindJsonEntry(
            ControlDocument,
            "control-loop-unconditional"
        )->at("record").at("executionControl")["conditionInput"] = 5U;
        RequireReadCode(
            ControlDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        JsonValue TypeDocument = JsonValue::parse(
            *DescriptorCatalogueSnapshotPersistence::Write(
                *DescriptorCatalogueSnapshot::Create(MakeTypeCatalogue())
            )
        );
        FindJsonEntry(TypeDocument, "type-0")->at("record").at("pins").at(0U).at("type") =
            JsonValue{{"kind", "List"}};
        RequireReadCode(
            TypeDocument.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        FindJsonEntry(Json, "concrete-a")->at("record").at("pins").at(0U).at("default") =
            JsonValue{{"kind", "Float"}, {"bits", "not-hex"}};
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"] = JsonValue::array();
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::MalformedDescriptorCatalogueSnapshot
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["specialization"]["families"][0U]["variants"][0U]["concreteExternalIdentity"] =
            "concrete-aa";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::DescriptorCatalogueSnapshotSpecializationMismatch
        );

        Json = JsonValue::parse(Fixture.Json);
        const std::uint32_t OriginalId =
            Json["entries"][0U]["nodeDescriptorId"].get<std::uint32_t>();
        Json["entries"][0U]["nodeDescriptorId"] = OriginalId + 100U;
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::DescriptorCatalogueSnapshotIdentifierMismatch
        );

        Json = JsonValue::parse(Fixture.Json);
        Json["catalogue"]["contentIdentifier"] = "sha256:other";
        RequireReadCode(
            Json.dump(),
            DiagnosticCode::DescriptorCatalogueSnapshotContentMismatch
        );

        const auto ValidAfterFailures =
            DescriptorCatalogueSnapshotPersistence::Read(Fixture.Json);
        MPP_CHECK(ValidAfterFailures.has_value());
        MPP_CHECK(ValidAfterFailures->IsValid());
    }

    void TestDescriptorCatalogueRegistryMaterializationMapping()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto Snapshot =
            DescriptorCatalogueSnapshot::Create(Fixture.Catalogue);
        MPP_CHECK(Snapshot.has_value());
        const auto Context =
            DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        MPP_CHECK(Context.has_value());
        MPP_CHECK(Context->IsValid());
        MPP_CHECK(Context->GetSnapshot() == *Snapshot);
        CheckCatalogueFields(Fixture.Catalogue, Context->GetCatalogue());
        MPP_CHECK(
            Context->GetCatalogueIdentity() == Fixture.Catalogue.GetIdentity()
        );
        MPP_CHECK(
            Context->GetRegistry().Size() ==
            Fixture.Catalogue.GetEntryCount()
        );
        for (const auto& Entry : Fixture.Catalogue.GetEntries())
        {
            const NodeDescriptor* Descriptor =
                Context->GetRegistry().Find(Entry.GetDescriptorIdentifier());
            MPP_CHECK(Descriptor != nullptr);
            CheckNodeDescriptor(*Descriptor, Entry);
        }

        const auto ControlCatalogue = MakeControlCatalogue();
        const auto ControlSnapshot =
            DescriptorCatalogueSnapshot::Create(ControlCatalogue);
        MPP_CHECK(ControlSnapshot.has_value());
        const auto ControlContext =
            DescriptorCatalogueRegistryContext::Materialize(*ControlSnapshot);
        MPP_CHECK(ControlContext.has_value());
        for (const auto& Entry : ControlCatalogue.GetEntries())
        {
            const NodeDescriptor* Descriptor =
                ControlContext->GetRegistry().Find(
                    Entry.GetDescriptorIdentifier()
                );
            MPP_CHECK(Descriptor != nullptr);
            CheckNodeDescriptor(*Descriptor, Entry);
        }

        constexpr std::uint64_t DoubleNanBits = 0x7ff8000000000042ULL;
        constexpr std::uint32_t VectorNanBits = 0x7fc00042U;
        const double DoubleNan = std::bit_cast<double>(DoubleNanBits);
        const float VectorNan = std::bit_cast<float>(VectorNanBits);
        const auto NanCatalogueResult = DescriptorCatalogueBuilder::Build(
            "nan.tests",
            "nan.tests@1",
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::vector<NormalizedNodeDescriptorRecord>{
                MakeRecord(
                    "nan-double",
                    "NaN Double",
                    {
                        MakeInput(
                            "DoubleDefault",
                            TypeDesc::Float(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(LiteralValue::Data{DoubleNan})
                        )
                    }
                ),
                MakeRecord(
                    "nan-vector",
                    "NaN Vector",
                    {
                        MakeInput(
                            "VectorDefault",
                            TypeDesc::Vector3(),
                            PinCardinality::Single,
                            true,
                            LiteralValue(
                                LiteralValue::Data{
                                    Vector3Value{VectorNan, 1.0F, 2.0F}
                                }
                            )
                        )
                    }
                )
            }
        );
        MPP_CHECK(NanCatalogueResult.has_value());
        const auto NanSnapshot =
            DescriptorCatalogueSnapshot::Create(*NanCatalogueResult);
        MPP_CHECK(NanSnapshot.has_value());
        const auto NanContext =
            DescriptorCatalogueRegistryContext::Materialize(*NanSnapshot);
        MPP_CHECK(NanContext.has_value());
        MPP_CHECK(NanContext->IsValid());

        const auto* NanDoubleEntry =
            FindCatalogueEntry(*NanCatalogueResult, "nan-double");
        const auto* NanVectorEntry =
            FindCatalogueEntry(*NanCatalogueResult, "nan-vector");
        MPP_CHECK(NanDoubleEntry != nullptr && NanVectorEntry != nullptr);
        const NodeDescriptor* NanDoubleDescriptor =
            NanContext->GetRegistry().Find(
                NanDoubleEntry->GetDescriptorIdentifier()
            );
        const NodeDescriptor* NanVectorDescriptor =
            NanContext->GetRegistry().Find(
                NanVectorEntry->GetDescriptorIdentifier()
            );
        MPP_CHECK(NanDoubleDescriptor != nullptr && NanVectorDescriptor != nullptr);
        const auto& NanDoubleDefault =
            NanDoubleDescriptor->GetPins()[0U].GetDefaultValue();
        const auto& NanVectorDefault =
            NanVectorDescriptor->GetPins()[0U].GetDefaultValue();
        MPP_CHECK(NanDoubleDefault.has_value() && NanVectorDefault.has_value());
        const double* RetrievedDouble = NanDoubleDefault->TryGet<double>();
        const Vector3Value* RetrievedVector =
            NanVectorDefault->TryGet<Vector3Value>();
        MPP_CHECK(RetrievedDouble != nullptr && RetrievedVector != nullptr);
        MPP_CHECK(std::bit_cast<std::uint64_t>(*RetrievedDouble) == DoubleNanBits);
        MPP_CHECK(std::bit_cast<std::uint32_t>(RetrievedVector->X) == VectorNanBits);
        MPP_CHECK(std::bit_cast<std::uint32_t>(RetrievedVector->Y) == 0x3f800000U);
        MPP_CHECK(std::bit_cast<std::uint32_t>(RetrievedVector->Z) == 0x40000000U);

        GraphBuilder Builder(Context->GetRegistry());
        const auto Node =
            Builder.AddNode(Fixture.Catalogue.GetEntries()[0U].GetDescriptorIdentifier());
        MPP_CHECK(Node.has_value());
    }

    void TestDescriptorCatalogueRegistryMaterializationDeterminismAndFailure()
    {
        const SnapshotFixture Fixture = MakeFixture();
        const auto Snapshot =
            DescriptorCatalogueSnapshot::Create(
                Fixture.Catalogue,
                Fixture.Specialization
            );
        MPP_CHECK(Snapshot.has_value());
        const auto First =
            DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        const auto Second =
            DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(First->IsValid() && Second->IsValid());
        MPP_CHECK(First->GetSnapshot() == Second->GetSnapshot());
        MPP_CHECK(
            First->GetCatalogueIdentity() ==
            Second->GetCatalogueIdentity()
        );
        MPP_CHECK(
            First->GetRegistry().Size() == Second->GetRegistry().Size()
        );
        for (const auto& Entry : Fixture.Catalogue.GetEntries())
        {
            const NodeDescriptor* FirstDescriptor =
                First->GetRegistry().Find(Entry.GetDescriptorIdentifier());
            const NodeDescriptor* SecondDescriptor =
                Second->GetRegistry().Find(Entry.GetDescriptorIdentifier());
            MPP_CHECK(FirstDescriptor != nullptr && SecondDescriptor != nullptr);
            CheckNodeDescriptor(*FirstDescriptor, Entry);
            CheckNodeDescriptor(*SecondDescriptor, Entry);
        }

        const auto InvalidSnapshot =
            DescriptorCatalogueSnapshot::Create(DescriptorCatalogue{});
        MPP_CHECK(!InvalidSnapshot.has_value());
        MPP_CHECK(
            HasCode(
                InvalidSnapshot.error(),
                DiagnosticCode::DescriptorCatalogueMismatch
            )
        );
        const auto ValidAfterFailure =
            DescriptorCatalogueRegistryContext::Materialize(*Snapshot);
        MPP_CHECK(ValidAfterFailure.has_value());
        MPP_CHECK(ValidAfterFailure->IsValid());
    }

    template<typename Type>
    concept CanWriteAsSnapshotInput = requires(const Type& Value)
    {
        DescriptorCatalogueSnapshotPersistence::Write(Value);
    };

    void TestDescriptorCatalogueSnapshotGraphIRAndScopeBoundaries()
    {
        using CreateSignature = std::expected<
            DescriptorCatalogueSnapshot,
            DiagnosticCollection
        > (*)(
            DescriptorCatalogue,
            std::optional<DescriptorSpecializationResult>
        );
        using WriteSignature = std::expected<
            std::string,
            DiagnosticCollection
        > (*)(const DescriptorCatalogueSnapshot&);
        using ReadSignature = std::expected<
            DescriptorCatalogueSnapshot,
            DiagnosticCollection
        > (*)(std::string);
        using MaterializeSignature = std::expected<
            DescriptorCatalogueRegistryContext,
            DiagnosticCollection
        > (*)(DescriptorCatalogueSnapshot);
        using GetSnapshotSignature =
            const DescriptorCatalogueSnapshot& (DescriptorCatalogueRegistryContext::*)() const;
        using GetCatalogueSignature =
            const DescriptorCatalogue& (DescriptorCatalogueRegistryContext::*)() const;
        using GetIdentitySignature =
            const DescriptorCatalogueIdentity& (DescriptorCatalogueRegistryContext::*)() const;
        using GetRegistrySignature =
            const NodeDescriptorRegistry& (DescriptorCatalogueRegistryContext::*)() const;

        static_assert(
            std::is_same_v<
                decltype(static_cast<CreateSignature>(
                    &DescriptorCatalogueSnapshot::Create
                )),
                CreateSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<WriteSignature>(
                    &DescriptorCatalogueSnapshotPersistence::Write
                )),
                WriteSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<ReadSignature>(
                    &DescriptorCatalogueSnapshotPersistence::Read
                )),
                ReadSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<MaterializeSignature>(
                    &DescriptorCatalogueRegistryContext::Materialize
                )),
                MaterializeSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<GetSnapshotSignature>(
                    &DescriptorCatalogueRegistryContext::GetSnapshot
                )),
                GetSnapshotSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<GetCatalogueSignature>(
                    &DescriptorCatalogueRegistryContext::GetCatalogue
                )),
                GetCatalogueSignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<GetIdentitySignature>(
                    &DescriptorCatalogueRegistryContext::GetCatalogueIdentity
                )),
                GetIdentitySignature
            >
        );
        static_assert(
            std::is_same_v<
                decltype(static_cast<GetRegistrySignature>(
                    &DescriptorCatalogueRegistryContext::GetRegistry
                )),
                GetRegistrySignature
            >
        );
        static_assert(!CanWriteAsSnapshotInput<DescriptorCatalogue>);
        static_assert(
            !std::is_constructible_v<
                DescriptorCatalogueSnapshot,
                DescriptorCatalogue
            >
        );
        static_assert(!std::is_default_constructible_v<DescriptorCatalogueSnapshot>);
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueSnapshot>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueSnapshot>);
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueRegistryContext>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueRegistryContext>);

        MPP_CHECK(
            DescriptorCatalogueSnapshotPersistence::CurrentSnapshotFormatVersion ==
            2U
        );
    }
}

int main()
{
    TestDescriptorCatalogueSnapshotWriteSuccess();
    TestDescriptorCatalogueSnapshotReadSuccess();
    TestDescriptorCatalogueSnapshotDeterministicBytes();
    TestDescriptorCatalogueSnapshotRoundTripEquality();
    TestDescriptorCatalogueSnapshotIdentityAndContentValidation();
    TestDescriptorCatalogueSnapshotVersionAndSchemaDiagnostics();
    TestDescriptorCatalogueSnapshotNormalizedRecordRoundTrip();
    TestDescriptorCatalogueSnapshotTypeAndLiteralRoundTrip();
    TestDescriptorCatalogueSnapshotControlAndProvenanceRoundTrip();
    TestDescriptorCatalogueSnapshotSpecializationSidecarRoundTrip();
    TestDescriptorCatalogueSnapshotMalformedDataAndFailureAtomicity();
    TestDescriptorCatalogueRegistryMaterializationMapping();
    TestDescriptorCatalogueRegistryMaterializationDeterminismAndFailure();
    TestDescriptorCatalogueSnapshotGraphIRAndScopeBoundaries();
    return 0;
}
