#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaExportContext.h"

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
    bool HasCode(
        const DiagnosticCollection& Diagnostics,
        DiagnosticCode Code
    )
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

    const Diagnostic* FindDiagnostic(
        const DiagnosticCollection& Diagnostics,
        DiagnosticCode Code
    )
    {
        for (const Diagnostic& DiagnosticValue : Diagnostics)
        {
            if (DiagnosticValue.Code == Code)
            {
                return &DiagnosticValue;
            }
        }
        return nullptr;
    }

    bool DiagnosticsEqual(
        const DiagnosticCollection& Left,
        const DiagnosticCollection& Right
    )
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
                Left[Index].ExternalIdentityKey != Right[Index].ExternalIdentityKey)
            {
                return false;
            }
        }
        return true;
    }

    struct RegistryFixture
    {
        DescriptorCatalogueIdentity Identity;
        DescriptorCatalogueBinding Binding;
        DescriptorCatalogueRegistryContext Context;

        RegistryFixture(
            DescriptorCatalogueIdentity IdentityValue,
            DescriptorCatalogueRegistryContext ContextValue
        )
            : Identity(std::move(IdentityValue))
            , Binding(Identity)
            , Context(std::move(ContextValue))
        {
        }
    };

    RegistryFixture MakeRegistryFixture(std::string SourceRevision)
    {
        std::vector<NormalizedNodeDescriptorRecord> Records;
        Records.emplace_back(
            ExternalNodeIdentity("p6.1.synthetic.node"),
            "P6.1 Synthetic Node",
            std::vector<NodeAvailability>{NodeAvailability::Client},
            std::vector<NormalizedPinRecord>{
                NormalizedPinRecord(
                    "Value",
                    TypeDesc::Boolean(),
                    PinDirection::Input,
                    PinCategory::Data,
                    PinCardinality::Single,
                    true,
                    LiteralValue(LiteralValue::Data{true})
                )
            }
        );

        const auto Catalogue = DescriptorCatalogueBuilder::Build(
            "p6.1.synthetic",
            std::move(SourceRevision),
            CurrentDescriptorCatalogueSemanticSchemaVersion,
            std::move(Records)
        );
        MPP_CHECK(Catalogue.has_value());

        const auto Snapshot = DescriptorCatalogueSnapshot::Create(*Catalogue);
        MPP_CHECK(Snapshot.has_value());

        const auto Context = DescriptorCatalogueRegistryContext::Materialize(
            *Snapshot
        );
        MPP_CHECK(Context.has_value());
        MPP_CHECK(Context->IsValid());

        return RegistryFixture(
            Context->GetCatalogueIdentity(),
            std::move(*Context)
        );
    }

    GiaExportConfiguration MakeConfiguration()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1001),
            "P6.1 Synthetic Graph",
            GiaUniqueIdentifier(2001)
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    GiaBackendPinMapping MakeBooleanPin(
        std::int32_t SemanticPin,
        std::int32_t BackendPin
    )
    {
        return GiaBackendPinMapping(
            PinIndex(static_cast<std::uint32_t>(SemanticPin)),
            GiaPinKind::InputParameter,
            GiaPinIndex(BackendPin),
            std::nullopt,
            GiaBackendTypeCode(5),
            GiaLiteralEncodingKind::Boolean,
            GiaPinEmissionPolicy::Emit,
            true
        );
    }

    GiaBackendNodeMapping MakeNodeMapping(
        std::string Identity,
        std::int32_t GenericIdentifier = 200000,
        std::optional<GiaNodeConcreteId> ConcreteIdentifier =
            GiaNodeConcreteId(0),
        std::vector<GiaBackendPinMapping> PinMappings = {
            MakeBooleanPin(0, 0)
        }
    )
    {
        return GiaBackendNodeMapping(
            ExternalNodeIdentity(std::move(Identity)),
            GiaNodeGenericId(GenericIdentifier),
            std::move(ConcreteIdentifier),
            std::move(PinMappings)
        );
    }

    GiaBackendNodeMapping MakeTwoPinNode(std::string Identity = "node_graph_end_boolean")
    {
        std::vector<GiaBackendPinMapping> PinMappings;
        PinMappings.emplace_back(MakeBooleanPin(0, 0));
        PinMappings.emplace_back(
            GiaBackendPinMapping(
                PinIndex(1U),
                GiaPinKind::InputParameter,
                GiaPinIndex(1),
                std::nullopt,
                GiaBackendTypeCode(13),
                GiaLiteralEncodingKind::Enum,
                GiaPinEmissionPolicy::Emit,
                true
            )
        );
        return MakeNodeMapping(
            std::move(Identity),
            200000,
            GiaNodeConcreteId(0),
            std::move(PinMappings)
        );
    }

    GiaBackendMappingIdentity MakeMappingIdentity(
        const RegistryFixture& Fixture,
        GiaExportTargetProfile Profile = GiaExportTargetProfile::ClientBooleanFilter,
        GiaExportMode Mode = GiaExportMode::Beyond,
        std::uint32_t SchemaVersion = 1U
    )
    {
        return GiaBackendMappingIdentity(
            Fixture.Identity,
            GiaBackendMappingSchemaVersion(SchemaVersion),
            Profile,
            Mode
        );
    }

    GiaBackendMappingPackage MakePackage(
        const RegistryFixture& Fixture,
        std::vector<GiaBackendNodeMapping> NodeMappings = {
            MakeTwoPinNode()
        }
    )
    {
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            std::move(NodeMappings)
        );
        MPP_CHECK(Result.has_value());
        return *Result;
    }

    std::expected<GiaExportContext, DiagnosticCollection> MakeContext(
        const RegistryFixture& Fixture,
        GiaBackendMappingPackage Package
    )
    {
        return GiaExportContext::Create(
            Fixture.Binding,
            Fixture.Context,
            MakeConfiguration(),
            std::move(Package)
        );
    }

    void TestGiaExportConfigurationAcceptsClientBooleanFilterBeyond()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2),
            0.5
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetTargetProfile() == GiaExportTargetProfile::ClientBooleanFilter);
        MPP_CHECK(Result->GetMode() == GiaExportMode::Beyond);
        MPP_CHECK(Result->GetGraphIdentifier().GetValue() == 1);
        MPP_CHECK(Result->GetGraphName() == "Graph");
        MPP_CHECK(Result->GetUniqueIdentifier().GetValue() == 2);
        MPP_CHECK(Result->GetEvaluationInterval() == 0.5);
    }

    void TestGiaExportConfigurationUsesDeterministicEvaluationIntervalDefault()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2)
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->GetEvaluationInterval() == 0.3);
    }

    void TestGiaExportConfigurationRejectsInvalidTargetAndMode()
    {
        const auto InvalidTarget = GiaExportConfiguration::Create(
            static_cast<GiaExportTargetProfile>(99),
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2)
        );
        MPP_CHECK(!InvalidTarget.has_value());
        MPP_CHECK(HasCode(InvalidTarget.error(), DiagnosticCode::UnsupportedGiaExportTarget));

        const auto InvalidMode = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            static_cast<GiaExportMode>(99),
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2)
        );
        MPP_CHECK(!InvalidMode.has_value());
        MPP_CHECK(HasCode(InvalidMode.error(), DiagnosticCode::UnsupportedGiaExportTarget));
    }

    void TestGiaExportConfigurationRejectsInvalidGraphIdentityAndInterval()
    {
        const auto Result = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(0),
            "",
            GiaUniqueIdentifier(0),
            -1.0
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaExportConfiguration));

        const auto NanResult = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2),
            std::numeric_limits<double>::quiet_NaN()
        );
        MPP_CHECK(!NanResult.has_value());
        MPP_CHECK(HasCode(NanResult.error(), DiagnosticCode::InvalidGiaExportConfiguration));

        const auto InfiniteResult = GiaExportConfiguration::Create(
            GiaExportTargetProfile::ClientBooleanFilter,
            GiaExportMode::Beyond,
            GiaGraphIdentifier(1),
            "Graph",
            GiaUniqueIdentifier(2),
            std::numeric_limits<double>::infinity()
        );
        MPP_CHECK(!InfiniteResult.has_value());
        MPP_CHECK(HasCode(InfiniteResult.error(), DiagnosticCode::InvalidGiaExportConfiguration));
    }

    void TestGiaBackendMappingPackageAcceptsCanonicalTwoPinMapping()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.success");
        const GiaBackendMappingPackage Package = MakePackage(Fixture);
        MPP_CHECK(Package.IsValid());
        MPP_CHECK(Package.GetNodeMappingCount() == 1U);
        const GiaBackendNodeMapping* Node = Package.FindByExternalIdentity(
            ExternalNodeIdentity("node_graph_end_boolean")
        );
        MPP_CHECK(Node != nullptr);
        MPP_CHECK(Node->GetGenericNodeIdentifier().GetValue() == 200000);
        MPP_CHECK(Node->GetConcreteNodeIdentifier().has_value());
        MPP_CHECK(Node->GetConcreteNodeIdentifier()->GetValue() == 0);
        MPP_CHECK(Node->GetPinMappings().size() == 2U);
        MPP_CHECK(Node->GetPinMappings()[0U].GetSemanticPinIndex().GetValue() == 0U);
        MPP_CHECK(Node->GetPinMappings()[1U].GetSemanticPinIndex().GetValue() == 1U);
    }

    void TestGiaBackendMappingPackageRejectsUnsupportedSchemaVersion()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.version");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture, GiaExportTargetProfile::ClientBooleanFilter, GiaExportMode::Beyond, 2U),
            {MakeTwoPinNode()}
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::UnsupportedGiaBackendMappingSchemaVersion));
    }

    void TestGiaBackendMappingPackageRejectsDuplicateExternalIdentity()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.duplicate.identity");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping("same", 200000, GiaNodeConcreteId(0)),
                MakeNodeMapping("same", 200001, GiaNodeConcreteId(1))
            }
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::DuplicateExternalNodeIdentity));
    }

    void TestGiaBackendMappingPackageRejectsInvalidGenericIdentifier()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.invalid.generic");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {MakeNodeMapping("invalid", 0)}
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaBackendNodeMapping));
    }

    void TestGiaBackendMappingPackageAcceptsPresentConcreteZero()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.concrete.zero");
        const GiaBackendMappingPackage Package = MakePackage(Fixture);
        const GiaBackendNodeMapping* Node = Package.FindByExternalIdentity(
            ExternalNodeIdentity("node_graph_end_boolean")
        );
        MPP_CHECK(Node != nullptr);
        MPP_CHECK(Node->GetConcreteNodeIdentifier().has_value());
        MPP_CHECK(Node->GetConcreteNodeIdentifier()->IsValid());
        MPP_CHECK(Node->GetConcreteNodeIdentifier()->GetValue() == 0);
    }

    void TestGiaBackendMappingPackageRejectsDuplicateSemanticPin()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.duplicate.semantic");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "duplicate-semantic",
                    200000,
                    GiaNodeConcreteId(0),
                    {MakeBooleanPin(0, 0), MakeBooleanPin(0, 1)}
                )
            }
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::DuplicateGiaBackendPinMapping));
    }

    void TestGiaBackendMappingPackageRejectsDuplicateBackendCoordinate()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.duplicate.backend");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "duplicate-backend",
                    200000,
                    GiaNodeConcreteId(0),
                    {MakeBooleanPin(0, 0), MakeBooleanPin(1, 0)}
                )
            }
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::DuplicateGiaBackendPinMapping));
    }

    void TestGiaBackendMappingPackageRejectsInvalidPinKindTypeAndLiteralCombination()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.invalid.pin");
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "invalid-pin",
                    200000,
                    GiaNodeConcreteId(0),
                    {
                        GiaBackendPinMapping(
                            PinIndex(0U),
                            GiaPinKind::InputFlow,
                            GiaPinIndex(0),
                            std::nullopt,
                            GiaBackendTypeCode(5),
                            GiaLiteralEncodingKind::Boolean,
                            GiaPinEmissionPolicy::Emit,
                            true
                        )
                    }
                )
            }
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::InvalidGiaBackendPinMapping));

        const auto InvalidKindResult = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "invalid-kind",
                    200000,
                    GiaNodeConcreteId(0),
                    {
                        GiaBackendPinMapping(
                            PinIndex(0U),
                            GiaPinKind::Unknown,
                            GiaPinIndex(0),
                            std::nullopt,
                            GiaBackendTypeCode(5),
                            GiaLiteralEncodingKind::Boolean,
                            GiaPinEmissionPolicy::Emit,
                            true
                        )
                    }
                )
            }
        );
        MPP_CHECK(!InvalidKindResult.has_value());
        MPP_CHECK(HasCode(InvalidKindResult.error(), DiagnosticCode::InvalidGiaBackendPinMapping));

        const auto InvalidTypeResult = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "invalid-type",
                    200000,
                    GiaNodeConcreteId(0),
                    {
                        GiaBackendPinMapping(
                            PinIndex(0U),
                            GiaPinKind::InputParameter,
                            GiaPinIndex(0),
                            std::nullopt,
                            GiaBackendTypeCode(0),
                            GiaLiteralEncodingKind::Boolean,
                            GiaPinEmissionPolicy::Emit,
                            true
                        )
                    }
                )
            }
        );
        MPP_CHECK(!InvalidTypeResult.has_value());
        MPP_CHECK(HasCode(InvalidTypeResult.error(), DiagnosticCode::InvalidGiaBackendPinMapping));

        const auto InvalidEmissionResult = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {
                MakeNodeMapping(
                    "invalid-emission",
                    200000,
                    GiaNodeConcreteId(0),
                    {
                        GiaBackendPinMapping(
                            PinIndex(0U),
                            GiaPinKind::InputParameter,
                            GiaPinIndex(0),
                            std::nullopt,
                            GiaBackendTypeCode(5),
                            GiaLiteralEncodingKind::Boolean,
                            GiaPinEmissionPolicy::Omit,
                            true
                        )
                    }
                )
            }
        );
        MPP_CHECK(!InvalidEmissionResult.has_value());
        MPP_CHECK(HasCode(InvalidEmissionResult.error(), DiagnosticCode::InvalidGiaBackendPinMapping));
    }

    void TestGiaBackendMappingPackageSortsIndependentlyOfInputOrder()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.order");
        const auto First = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {MakeNodeMapping("z"), MakeNodeMapping("a")}
        );
        const auto Second = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {MakeNodeMapping("a"), MakeNodeMapping("z")}
        );
        MPP_CHECK(First.has_value() && Second.has_value());
        MPP_CHECK(*First == *Second);
        MPP_CHECK(First->GetNodeMappings()[0U].GetExternalIdentity().GetKey() == "a");
        MPP_CHECK(First->GetNodeMappings()[1U].GetExternalIdentity().GetKey() == "z");
    }

    void TestGiaBackendMappingPackageFindsByOpaqueExternalIdentityOnly()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.opaque.lookup");
        const std::string Key = "family=example;concrete=0;variant=bytes";
        const GiaBackendMappingPackage Package = MakePackage(
            Fixture,
            {MakeNodeMapping(Key)}
        );
        MPP_CHECK(Package.FindByExternalIdentity(ExternalNodeIdentity(Key)) != nullptr);
        MPP_CHECK(
            Package.FindByExternalIdentity(
                ExternalNodeIdentity("family=example;concrete=1;variant=bytes")
            ) == nullptr
        );
        MPP_CHECK(
            Package.FindByExternalIdentity(
                ExternalNodeIdentity("0")
            ) == nullptr
        );
    }

    void TestGiaBackendMappingPackageDoesNotMutateInputRecords()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("mapping.input");
        std::vector<GiaBackendNodeMapping> Input = {
            MakeNodeMapping(
                "z",
                200000,
                GiaNodeConcreteId(0),
                {MakeBooleanPin(1, 1), MakeBooleanPin(0, 0)}
            ),
            MakeNodeMapping("a")
        };
        MPP_CHECK(Input[0U].GetExternalIdentity().GetKey() == "z");
        MPP_CHECK(Input[0U].GetPinMappings()[0U].GetSemanticPinIndex().GetValue() == 1U);
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            Input
        );
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Input[0U].GetExternalIdentity().GetKey() == "z");
        MPP_CHECK(Input[0U].GetPinMappings()[0U].GetSemanticPinIndex().GetValue() == 1U);
    }

    void TestGiaExportContextAcceptsMatchingBindingRegistryAndMapping()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.success");
        const auto Result = MakeContext(Fixture, MakePackage(Fixture));
        MPP_CHECK(Result.has_value());
        MPP_CHECK(Result->IsValid());
        MPP_CHECK(Result->GetCatalogueBinding().GetIdentity() == Fixture.Identity);
        MPP_CHECK(Result->GetRegistryContext().GetCatalogueIdentity() == Fixture.Identity);
        MPP_CHECK(Result->GetMappingPackage().IsValid());
    }

    void TestGiaExportContextRejectsCatalogueBindingMismatchBeforeAssociation()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.binding.a");
        const RegistryFixture OtherFixture = MakeRegistryFixture("context.binding.b");
        const auto Result = GiaExportContext::Create(
            OtherFixture.Binding,
            Fixture.Context,
            MakeConfiguration(),
            MakePackage(Fixture)
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::DescriptorCatalogueMismatch));
    }

    void TestGiaExportContextRejectsMappingCatalogueMismatch()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.mapping.a");
        const RegistryFixture OtherFixture = MakeRegistryFixture("context.mapping.b");
        const auto Result = GiaExportContext::Create(
            Fixture.Binding,
            Fixture.Context,
            MakeConfiguration(),
            MakePackage(OtherFixture)
        );
        MPP_CHECK(!Result.has_value());
        MPP_CHECK(HasCode(Result.error(), DiagnosticCode::IncompatibleGiaBackendMappingPackage));
    }

    void TestGiaExportContextRejectsTargetModeMismatch()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.target");
        const auto InvalidTargetPackage = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(
                Fixture,
                static_cast<GiaExportTargetProfile>(99),
                GiaExportMode::Beyond
            ),
            {MakeTwoPinNode()}
        );
        MPP_CHECK(!InvalidTargetPackage.has_value());
        MPP_CHECK(HasCode(InvalidTargetPackage.error(), DiagnosticCode::UnsupportedGiaExportTarget));

        const auto InvalidModePackage = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(
                Fixture,
                GiaExportTargetProfile::ClientBooleanFilter,
                static_cast<GiaExportMode>(99)
            ),
            {MakeTwoPinNode()}
        );
        MPP_CHECK(!InvalidModePackage.has_value());
        MPP_CHECK(HasCode(InvalidModePackage.error(), DiagnosticCode::UnsupportedGiaExportTarget));
    }

    void TestGiaExportContextPreservesValueCopyAndMoveSemantics()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.value");
        const auto Created = MakeContext(Fixture, MakePackage(Fixture));
        MPP_CHECK(Created.has_value());
        GiaExportContext Copy = *Created;
        MPP_CHECK(Copy.IsValid());
        MPP_CHECK(Copy.GetConfiguration() == Created->GetConfiguration());
        GiaExportContext Moved = std::move(Copy);
        MPP_CHECK(Moved.IsValid());
        MPP_CHECK(Moved.GetMappingPackage() == Created->GetMappingPackage());
    }

    void TestGiaExportContextFailureAtomicityAndDiagnosticOrdering()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("context.atomicity");
        const GiaBackendMappingIdentity Identity = MakeMappingIdentity(Fixture);
        const std::vector<GiaBackendNodeMapping> FirstInput = {
            MakeNodeMapping("b", 0),
            MakeNodeMapping("a", 0),
            MakeNodeMapping("a", 200000)
        };
        const std::vector<GiaBackendNodeMapping> SecondInput = {
            MakeNodeMapping("a", 200000),
            MakeNodeMapping("a", 0),
            MakeNodeMapping("b", 0)
        };
        const auto First = GiaBackendMappingPackage::Create(Identity, FirstInput);
        const auto Second = GiaBackendMappingPackage::Create(Identity, SecondInput);
        MPP_CHECK(!First.has_value() && !Second.has_value());
        MPP_CHECK(!First.error().empty() && !Second.error().empty());
        MPP_CHECK(DiagnosticsEqual(First.error(), Second.error()));

        const auto Context = GiaExportContext::Create(
            Fixture.Binding,
            Fixture.Context,
            MakeConfiguration(),
            MakePackage(Fixture)
        );
        MPP_CHECK(Context.has_value());
        MPP_CHECK(Context->IsValid());
    }

    void TestGiaDiagnosticsCarryExactOpaqueIdentityContext()
    {
        const RegistryFixture Fixture = MakeRegistryFixture("diagnostic.opaque");
        const std::string Key = "family=diagnostic;concrete=0;variant=opaque";
        const auto Result = GiaBackendMappingPackage::Create(
            MakeMappingIdentity(Fixture),
            {MakeNodeMapping(Key, 0)}
        );
        MPP_CHECK(!Result.has_value());
        const Diagnostic* DiagnosticValue = FindDiagnostic(
            Result.error(),
            DiagnosticCode::InvalidGiaBackendNodeMapping
        );
        MPP_CHECK(DiagnosticValue != nullptr);
        MPP_CHECK(DiagnosticValue->ExternalIdentityKey.has_value());
        MPP_CHECK(DiagnosticValue->ExternalIdentityKey.value() == Key);
    }
}

int main()
{
    TestGiaExportConfigurationAcceptsClientBooleanFilterBeyond();
    TestGiaExportConfigurationUsesDeterministicEvaluationIntervalDefault();
    TestGiaExportConfigurationRejectsInvalidTargetAndMode();
    TestGiaExportConfigurationRejectsInvalidGraphIdentityAndInterval();
    TestGiaBackendMappingPackageAcceptsCanonicalTwoPinMapping();
    TestGiaBackendMappingPackageRejectsUnsupportedSchemaVersion();
    TestGiaBackendMappingPackageRejectsDuplicateExternalIdentity();
    TestGiaBackendMappingPackageRejectsInvalidGenericIdentifier();
    TestGiaBackendMappingPackageAcceptsPresentConcreteZero();
    TestGiaBackendMappingPackageRejectsDuplicateSemanticPin();
    TestGiaBackendMappingPackageRejectsDuplicateBackendCoordinate();
    TestGiaBackendMappingPackageRejectsInvalidPinKindTypeAndLiteralCombination();
    TestGiaBackendMappingPackageSortsIndependentlyOfInputOrder();
    TestGiaBackendMappingPackageFindsByOpaqueExternalIdentityOnly();
    TestGiaBackendMappingPackageDoesNotMutateInputRecords();
    TestGiaExportContextAcceptsMatchingBindingRegistryAndMapping();
    TestGiaExportContextRejectsCatalogueBindingMismatchBeforeAssociation();
    TestGiaExportContextRejectsMappingCatalogueMismatch();
    TestGiaExportContextRejectsTargetModeMismatch();
    TestGiaExportContextPreservesValueCopyAndMoveSemantics();
    TestGiaExportContextFailureAtomicityAndDiagnosticOrdering();
    TestGiaDiagnosticsCarryExactOpaqueIdentityContext();
    return 0;
}
