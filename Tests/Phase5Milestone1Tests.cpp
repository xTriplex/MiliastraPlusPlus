#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <optional>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>
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

    DescriptorCatalogueIdentity MakeCatalogueIdentity(
        std::string SourceNamespace = "source",
        std::string SourceRevision = "build-1",
        std::uint32_t SemanticSchemaVersion = 1U,
        std::string CatalogueContentIdentifier = "content-a"
    )
    {
        return DescriptorCatalogueIdentity(
            std::move(SourceNamespace),
            std::move(SourceRevision),
            DescriptorCatalogueSemanticSchemaVersion(SemanticSchemaVersion),
            DescriptorCatalogueContentIdentifier(std::move(CatalogueContentIdentifier))
        );
    }

    SourceProvenance MakeSourceProvenance(std::string SourceDocumentIdentifier, std::string SourceRecordIdentifier)
    {
        return SourceProvenance(
            std::move(SourceDocumentIdentifier),
            std::move(SourceRecordIdentifier)
        );
    }

    DescriptorIdentifierAllocationCandidate MakeAllocationCandidate(
        std::string Key,
        std::optional<SourceProvenance> Provenance = std::nullopt
    )
    {
        return DescriptorIdentifierAllocationCandidate(
            ExternalNodeIdentity(std::move(Key)),
            std::move(Provenance)
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
                Left[Index].SourceNodeIdentifier != Right[Index].SourceNodeIdentifier ||
                Left[Index].DestinationNodeIdentifier != Right[Index].DestinationNodeIdentifier ||
                Left[Index].SourcePinReference != Right[Index].SourcePinReference ||
                Left[Index].DestinationPinReference != Right[Index].DestinationPinReference ||
                Left[Index].PrimarySourceProvenance != Right[Index].PrimarySourceProvenance ||
                Left[Index].RelatedSourceProvenance != Right[Index].RelatedSourceProvenance)
            {
                return false;
            }
        }

        return true;
    }

    const Diagnostic* FindDiagnostic(const DiagnosticCollection& Diagnostics, DiagnosticCode Code)
    {
        const auto Iterator = std::find_if(
            Diagnostics.begin(),
            Diagnostics.end(),
            [Code](const Diagnostic& CurrentDiagnostic)
            {
                return CurrentDiagnostic.Code == Code;
            }
        );
        return Iterator == Diagnostics.end() ? nullptr : &*Iterator;
    }

    void TestDescriptorCatalogueSemanticSchemaVersion()
    {
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueSemanticSchemaVersion>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueSemanticSchemaVersion>);

        const DescriptorCatalogueSemanticSchemaVersion Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(Invalid.GetValue() == 0U);

        const DescriptorCatalogueSemanticSchemaVersion Zero(0U);
        MPP_CHECK(!Zero.IsValid());

        const DescriptorCatalogueSemanticSchemaVersion First(1U);
        const DescriptorCatalogueSemanticSchemaVersion Second(2U);
        MPP_CHECK(First.IsValid());
        MPP_CHECK(First.GetValue() == 1U);
        MPP_CHECK(First == DescriptorCatalogueSemanticSchemaVersion(1U));
        MPP_CHECK(First < Second);
    }

    void TestDescriptorCatalogueContentIdentifier()
    {
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueContentIdentifier>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueContentIdentifier>);

        const DescriptorCatalogueContentIdentifier Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(Invalid.GetValue().empty());

        const DescriptorCatalogueContentIdentifier Empty{std::string()};
        MPP_CHECK(!Empty.IsValid());

        const DescriptorCatalogueContentIdentifier Upper("Content");
        const DescriptorCatalogueContentIdentifier Lower("content");
        MPP_CHECK(Upper.IsValid());
        MPP_CHECK(Upper.GetValue() == "Content");
        MPP_CHECK(Upper != Lower);
        MPP_CHECK(Upper < Lower);
    }

    void TestDescriptorCatalogueIdentity()
    {
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueIdentity>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueIdentity>);

        const DescriptorCatalogueIdentity Invalid;
        MPP_CHECK(!Invalid.IsValid());

        const DescriptorCatalogueIdentity Valid = MakeCatalogueIdentity();
        MPP_CHECK(Valid.IsValid());
        MPP_CHECK(Valid.GetSourceNamespace() == "source");
        MPP_CHECK(Valid.GetSourceRevision() == "build-1");
        MPP_CHECK(Valid.GetSemanticSchemaVersion() ==
            DescriptorCatalogueSemanticSchemaVersion(1U));
        MPP_CHECK(Valid.GetCatalogueContentIdentifier() ==
            DescriptorCatalogueContentIdentifier("content-a"));

        MPP_CHECK(!DescriptorCatalogueIdentity(
            "", "build-1", DescriptorCatalogueSemanticSchemaVersion(1U),
            DescriptorCatalogueContentIdentifier("content-a")).IsValid());
        MPP_CHECK(!DescriptorCatalogueIdentity(
            "source", "", DescriptorCatalogueSemanticSchemaVersion(1U),
            DescriptorCatalogueContentIdentifier("content-a")).IsValid());
        MPP_CHECK(!DescriptorCatalogueIdentity(
            "source", "build-1", DescriptorCatalogueSemanticSchemaVersion(),
            DescriptorCatalogueContentIdentifier("content-a")).IsValid());
        MPP_CHECK(!DescriptorCatalogueIdentity(
            "source", "build-1", DescriptorCatalogueSemanticSchemaVersion(1U),
            DescriptorCatalogueContentIdentifier()).IsValid());

        MPP_CHECK(Valid != MakeCatalogueIdentity("other-source"));
        MPP_CHECK(Valid != MakeCatalogueIdentity("source", "build-2"));
        MPP_CHECK(Valid != MakeCatalogueIdentity("source", "build-1", 2U));
        MPP_CHECK(Valid != MakeCatalogueIdentity("source", "build-1", 1U, "content-b"));
        MPP_CHECK(Valid < MakeCatalogueIdentity("source-z"));
        MPP_CHECK(Valid < MakeCatalogueIdentity("source", "build-2"));
        MPP_CHECK(Valid < MakeCatalogueIdentity("source", "build-1", 2U));
        MPP_CHECK(Valid < MakeCatalogueIdentity("source", "build-1", 1U, "content-b"));

        std::string SourceNamespace = "owned-source";
        std::string SourceRevision = "owned-build";
        std::string ContentIdentifier = "owned-content";
        const DescriptorCatalogueIdentity Owned(
            SourceNamespace,
            SourceRevision,
            DescriptorCatalogueSemanticSchemaVersion(3U),
            DescriptorCatalogueContentIdentifier(ContentIdentifier)
        );
        SourceNamespace[0U] = 'X';
        SourceRevision[0U] = 'X';
        ContentIdentifier[0U] = 'X';
        MPP_CHECK(Owned.GetSourceNamespace() == "owned-source");
        MPP_CHECK(Owned.GetSourceRevision() == "owned-build");
        MPP_CHECK(Owned.GetCatalogueContentIdentifier().GetValue() == "owned-content");
    }

    void TestExternalNodeIdentity()
    {
        static_assert(std::is_copy_constructible_v<ExternalNodeIdentity>);
        static_assert(std::is_move_constructible_v<ExternalNodeIdentity>);

        const ExternalNodeIdentity Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(!ExternalNodeIdentity(std::string()).IsValid());

        const ExternalNodeIdentity Upper("Node");
        const ExternalNodeIdentity Lower("node");
        MPP_CHECK(Upper.IsValid());
        MPP_CHECK(Upper.GetKey() == "Node");
        MPP_CHECK(Upper != Lower);
        MPP_CHECK(Upper < Lower);

        std::string Key = "owned-node";
        const ExternalNodeIdentity Owned(Key);
        Key[0U] = 'X';
        MPP_CHECK(Owned.GetKey() == "owned-node");
    }

    void TestSourceProvenance()
    {
        static_assert(std::is_copy_constructible_v<SourceProvenance>);
        static_assert(std::is_move_constructible_v<SourceProvenance>);

        const SourceProvenance Invalid;
        MPP_CHECK(!Invalid.IsValid());
        MPP_CHECK(!SourceProvenance("", "record").IsValid());
        MPP_CHECK(!SourceProvenance("document", "").IsValid());

        const SourceProvenance First = MakeSourceProvenance("document-a", "record-a");
        const SourceProvenance Second = MakeSourceProvenance("document-b", "record-a");
        MPP_CHECK(First.IsValid());
        MPP_CHECK(First.GetSourceDocumentIdentifier() == "document-a");
        MPP_CHECK(First.GetSourceRecordIdentifier() == "record-a");
        MPP_CHECK(First < Second);
        MPP_CHECK(First < MakeSourceProvenance("document-a", "record-b"));

        const std::optional<SourceProvenance> Absent = std::nullopt;
        const std::optional<SourceProvenance> Present = First;
        MPP_CHECK(!Absent.has_value());
        MPP_CHECK(Present.has_value() && Present->IsValid());

        std::string Document = "owned-document";
        std::string Record = "owned-record";
        const SourceProvenance Owned(Document, Record);
        Document[0U] = 'X';
        Record[0U] = 'X';
        MPP_CHECK(Owned.GetSourceDocumentIdentifier() == "owned-document");
        MPP_CHECK(Owned.GetSourceRecordIdentifier() == "owned-record");
    }

    void TestDescriptorCatalogueBinding()
    {
        static_assert(std::is_copy_constructible_v<DescriptorCatalogueBinding>);
        static_assert(std::is_move_constructible_v<DescriptorCatalogueBinding>);

        const DescriptorCatalogueBinding Invalid;
        MPP_CHECK(!Invalid.IsValid());

        const DescriptorCatalogueBinding InvalidIdentity{DescriptorCatalogueIdentity()};
        MPP_CHECK(!InvalidIdentity.IsValid());

        const DescriptorCatalogueBinding Valid(MakeCatalogueIdentity());
        MPP_CHECK(Valid.IsValid());
        MPP_CHECK(Valid.GetIdentity() == MakeCatalogueIdentity());
        MPP_CHECK(Valid == DescriptorCatalogueBinding(MakeCatalogueIdentity()));
        MPP_CHECK(Valid < DescriptorCatalogueBinding(MakeCatalogueIdentity("source-z")));
    }

    void TestCatalogueCompatibility()
    {
        const DescriptorCatalogueIdentity AvailableIdentity = MakeCatalogueIdentity();
        const DescriptorCatalogueBinding MatchingBinding(AvailableIdentity);
        const auto Matching = ValidateDescriptorCatalogueCompatibility(
            MatchingBinding,
            AvailableIdentity
        );
        MPP_CHECK(Matching.has_value());

        const std::vector<DescriptorCatalogueIdentity> Mismatches = {
            MakeCatalogueIdentity("other-source"),
            MakeCatalogueIdentity("source", "build-2"),
            MakeCatalogueIdentity("source", "build-1", 2U),
            MakeCatalogueIdentity("source", "build-1", 1U, "content-b")
        };
        for (const DescriptorCatalogueIdentity& MismatchIdentity : Mismatches)
        {
            const auto Mismatch = ValidateDescriptorCatalogueCompatibility(
                MatchingBinding,
                MismatchIdentity
            );
            MPP_CHECK(!Mismatch.has_value());
            MPP_CHECK(Mismatch.error().size() == 1U);
            MPP_CHECK(HasDiagnosticCode(
                Mismatch.error(), DiagnosticCode::DescriptorCatalogueMismatch));
        }

        const auto InvalidBinding = ValidateDescriptorCatalogueCompatibility(
            DescriptorCatalogueBinding(),
            AvailableIdentity
        );
        MPP_CHECK(!InvalidBinding.has_value());
        MPP_CHECK(InvalidBinding.error().size() == 1U);
        MPP_CHECK(HasDiagnosticCode(
            InvalidBinding.error(), DiagnosticCode::InvalidDescriptorCatalogueIdentity));

        const auto InvalidAvailable = ValidateDescriptorCatalogueCompatibility(
            MatchingBinding,
            DescriptorCatalogueIdentity()
        );
        MPP_CHECK(!InvalidAvailable.has_value());
        MPP_CHECK(InvalidAvailable.error().size() == 1U);
        MPP_CHECK(HasDiagnosticCode(
            InvalidAvailable.error(), DiagnosticCode::InvalidDescriptorCatalogueIdentity));

        const auto BothInvalid = ValidateDescriptorCatalogueCompatibility(
            DescriptorCatalogueBinding(),
            DescriptorCatalogueIdentity()
        );
        MPP_CHECK(!BothInvalid.has_value());
        MPP_CHECK(BothInvalid.error().size() == 2U);
        MPP_CHECK(BothInvalid.error()[0U].Code ==
            DiagnosticCode::InvalidDescriptorCatalogueIdentity);
        MPP_CHECK(BothInvalid.error()[0U].Message ==
            "Available catalogue identity is invalid.");
        MPP_CHECK(BothInvalid.error()[1U].Code ==
            DiagnosticCode::InvalidDescriptorCatalogueIdentity);
        MPP_CHECK(BothInvalid.error()[1U].Message ==
            "Graph binding contains an invalid descriptor catalogue identity.");
    }

    void TestDescriptorIdentifierAllocationCandidate()
    {
        static_assert(std::is_copy_constructible_v<DescriptorIdentifierAllocationCandidate>);
        static_assert(std::is_move_constructible_v<DescriptorIdentifierAllocationCandidate>);

        const DescriptorIdentifierAllocationCandidate Invalid;
        MPP_CHECK(!Invalid.IsValid());

        const DescriptorIdentifierAllocationCandidate InvalidIdentity{
            ExternalNodeIdentity()
        };
        MPP_CHECK(!InvalidIdentity.IsValid());

        const DescriptorIdentifierAllocationCandidate WithoutProvenance(
            ExternalNodeIdentity("node")
        );
        MPP_CHECK(WithoutProvenance.IsValid());
        MPP_CHECK(!WithoutProvenance.GetSourceProvenance().has_value());

        const SourceProvenance Provenance = MakeSourceProvenance("document", "record");
        const DescriptorIdentifierAllocationCandidate WithProvenance(
            ExternalNodeIdentity("node"),
            Provenance
        );
        MPP_CHECK(WithProvenance.IsValid());
        MPP_CHECK(WithProvenance.GetSourceProvenance() == Provenance);

        const DescriptorIdentifierAllocationCandidate InvalidProvenance(
            ExternalNodeIdentity("node"),
            SourceProvenance()
        );
        MPP_CHECK(!InvalidProvenance.IsValid());
        MPP_CHECK(WithProvenance != WithoutProvenance);
        MPP_CHECK(WithoutProvenance < WithProvenance);
        MPP_CHECK(MakeAllocationCandidate(
            "a", MakeSourceProvenance("document-z", "record-z")) <
            MakeAllocationCandidate("b", MakeSourceProvenance("document-a", "record-a")));
        MPP_CHECK(MakeAllocationCandidate(
            "node", MakeSourceProvenance("document-a", "record-a")) <
            MakeAllocationCandidate("node", MakeSourceProvenance("document-a", "record-b")));

        std::string Key = "owned-node";
        std::string Document = "owned-document";
        std::string Record = "owned-record";
        const DescriptorIdentifierAllocationCandidate Owned(
            ExternalNodeIdentity(Key),
            MakeSourceProvenance(Document, Record)
        );
        Key[0U] = 'X';
        Document[0U] = 'X';
        Record[0U] = 'X';
        MPP_CHECK(Owned.GetExternalIdentity().GetKey() == "owned-node");
        MPP_CHECK(Owned.GetSourceProvenance()->GetSourceDocumentIdentifier() ==
            "owned-document");
        MPP_CHECK(Owned.GetSourceProvenance()->GetSourceRecordIdentifier() ==
            "owned-record");
    }

    void TestDescriptorIdentifierAssignment()
    {
        static_assert(std::is_copy_constructible_v<DescriptorIdentifierAssignment>);
        static_assert(std::is_move_constructible_v<DescriptorIdentifierAssignment>);

        const DescriptorIdentifierAssignment Invalid;
        MPP_CHECK(!Invalid.IsValid());

        const DescriptorIdentifierAssignment InvalidIdentity(
            ExternalNodeIdentity(),
            NodeDescriptorId(1U)
        );
        MPP_CHECK(!InvalidIdentity.IsValid());

        const DescriptorIdentifierAssignment InvalidDescriptorIdentifier(
            ExternalNodeIdentity("node"),
            NodeDescriptorId()
        );
        MPP_CHECK(!InvalidDescriptorIdentifier.IsValid());

        const DescriptorIdentifierAssignment Valid(
            ExternalNodeIdentity("node"),
            NodeDescriptorId(1U)
        );
        MPP_CHECK(Valid.IsValid());
        MPP_CHECK(Valid.GetExternalIdentity() == ExternalNodeIdentity("node"));
        MPP_CHECK(Valid.GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK(Valid == DescriptorIdentifierAssignment(
            ExternalNodeIdentity("node"), NodeDescriptorId(1U)));
        MPP_CHECK(DescriptorIdentifierAssignment(
            ExternalNodeIdentity("a"), NodeDescriptorId(99U)) < Valid);
        MPP_CHECK(Valid < DescriptorIdentifierAssignment(
            ExternalNodeIdentity("node"), NodeDescriptorId(2U)));
    }

    void TestDeterministicDescriptorIdentifierAllocation()
    {
        const auto Empty = DescriptorIdentifierAllocator::Allocate({});
        MPP_CHECK(Empty.has_value());
        MPP_CHECK(Empty->empty());

        const std::vector<DescriptorIdentifierAllocationCandidate> Candidates = {
            MakeAllocationCandidate("z"),
            MakeAllocationCandidate("a"),
            MakeAllocationCandidate("m")
        };
        const auto First = DescriptorIdentifierAllocator::Allocate(Candidates);
        MPP_CHECK(First.has_value());
        MPP_CHECK(First->size() == 3U);
        MPP_CHECK((*First)[0U].GetExternalIdentity().GetKey() == "a");
        MPP_CHECK((*First)[1U].GetExternalIdentity().GetKey() == "m");
        MPP_CHECK((*First)[2U].GetExternalIdentity().GetKey() == "z");
        MPP_CHECK((*First)[0U].GetDescriptorIdentifier() == NodeDescriptorId(1U));
        MPP_CHECK((*First)[1U].GetDescriptorIdentifier() == NodeDescriptorId(2U));
        MPP_CHECK((*First)[2U].GetDescriptorIdentifier() == NodeDescriptorId(3U));
        MPP_CHECK(std::all_of(
            First->begin(),
            First->end(),
            [](const DescriptorIdentifierAssignment& Assignment)
            {
                return Assignment.GetDescriptorIdentifier().IsValid();
            }
        ));

        const std::vector<DescriptorIdentifierAllocationCandidate> Permuted = {
            MakeAllocationCandidate("m"),
            MakeAllocationCandidate("z"),
            MakeAllocationCandidate("a")
        };
        const auto Second = DescriptorIdentifierAllocator::Allocate(Permuted);
        const auto Third = DescriptorIdentifierAllocator::Allocate(Candidates);
        MPP_CHECK(Second.has_value());
        MPP_CHECK(Third.has_value());
        MPP_CHECK(*First == *Second);
        MPP_CHECK(*First == *Third);

        const auto WithFirstProvenance = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("a", MakeSourceProvenance("one", "a")),
            MakeAllocationCandidate("b", MakeSourceProvenance("one", "b"))
        });
        const auto WithDifferentProvenance = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("a", MakeSourceProvenance("two", "a")),
            MakeAllocationCandidate("b", MakeSourceProvenance("two", "b"))
        });
        MPP_CHECK(WithFirstProvenance.has_value());
        MPP_CHECK(WithDifferentProvenance.has_value());
        MPP_CHECK(*WithFirstProvenance == *WithDifferentProvenance);

        const auto FirstIndependentCall = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("first")
        });
        const auto SecondIndependentCall = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("second")
        });
        MPP_CHECK(FirstIndependentCall.has_value());
        MPP_CHECK(SecondIndependentCall.has_value());
        MPP_CHECK((*FirstIndependentCall)[0U].GetDescriptorIdentifier() ==
            NodeDescriptorId(1U));
        MPP_CHECK((*SecondIndependentCall)[0U].GetDescriptorIdentifier() ==
            NodeDescriptorId(1U));

        const std::vector<DescriptorIdentifierAllocationCandidate> Original = Candidates;
        const auto CopyInputResult = DescriptorIdentifierAllocator::Allocate(Original);
        MPP_CHECK(CopyInputResult.has_value());
        MPP_CHECK(Original == Candidates);

        MPP_CHECK(DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(0U));
        MPP_CHECK(DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(1U));
        MPP_CHECK(DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
            std::numeric_limits<std::uint32_t>::max()));
        if constexpr (sizeof(std::size_t) > sizeof(std::uint32_t))
        {
            MPP_CHECK(!DescriptorCatalogueDetail::IsDescriptorIdentifierCountWithinDomain(
                static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()) + 1U));
        }
    }

    void TestInvalidAndDuplicateAllocation()
    {
        const SourceProvenance InvalidIdentityProvenance =
            MakeSourceProvenance("identity-document", "identity-record");
        const auto InvalidIdentity = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("", InvalidIdentityProvenance)
        });
        MPP_CHECK(!InvalidIdentity.has_value());
        MPP_CHECK(HasDiagnosticCode(
            InvalidIdentity.error(), DiagnosticCode::InvalidExternalNodeIdentity));
        const Diagnostic* InvalidIdentityDiagnostic = FindDiagnostic(
            InvalidIdentity.error(), DiagnosticCode::InvalidExternalNodeIdentity);
        MPP_CHECK(InvalidIdentityDiagnostic != nullptr);
        MPP_CHECK(InvalidIdentityDiagnostic->PrimarySourceProvenance ==
            InvalidIdentityProvenance);

        const auto InvalidProvenance = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("node", SourceProvenance())
        });
        MPP_CHECK(!InvalidProvenance.has_value());
        MPP_CHECK(InvalidProvenance.error().size() == 1U);
        MPP_CHECK(HasDiagnosticCode(
            InvalidProvenance.error(), DiagnosticCode::InvalidSourceProvenance));
        MPP_CHECK(!InvalidProvenance.error()[0U].PrimarySourceProvenance.has_value());

        const SourceProvenance FirstProvenance = MakeSourceProvenance("document", "first");
        const SourceProvenance SecondProvenance = MakeSourceProvenance("document", "second");
        const auto Duplicate = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("duplicate", SecondProvenance),
            MakeAllocationCandidate("duplicate", FirstProvenance),
            MakeAllocationCandidate("other")
        });
        MPP_CHECK(!Duplicate.has_value());
        MPP_CHECK(Duplicate.error().size() == 1U);
        const Diagnostic* DuplicateDiagnostic = FindDiagnostic(
            Duplicate.error(), DiagnosticCode::DuplicateExternalNodeIdentity);
        MPP_CHECK(DuplicateDiagnostic != nullptr);
        MPP_CHECK(DuplicateDiagnostic->PrimarySourceProvenance == FirstProvenance);
        MPP_CHECK(DuplicateDiagnostic->RelatedSourceProvenance == SecondProvenance);

        const auto DuplicateWithoutProvenance = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("duplicate"),
            MakeAllocationCandidate("duplicate")
        });
        MPP_CHECK(!DuplicateWithoutProvenance.has_value());
        const Diagnostic* DuplicateWithoutProvenanceDiagnostic = FindDiagnostic(
            DuplicateWithoutProvenance.error(), DiagnosticCode::DuplicateExternalNodeIdentity);
        MPP_CHECK(DuplicateWithoutProvenanceDiagnostic != nullptr);
        MPP_CHECK(!DuplicateWithoutProvenanceDiagnostic->PrimarySourceProvenance.has_value());
        MPP_CHECK(!DuplicateWithoutProvenanceDiagnostic->RelatedSourceProvenance.has_value());

        const std::vector<DescriptorIdentifierAllocationCandidate> DuplicatePermutationA = {
            MakeAllocationCandidate("z", MakeSourceProvenance("document", "z-2")),
            MakeAllocationCandidate("a", MakeSourceProvenance("document", "a-2")),
            MakeAllocationCandidate("z", MakeSourceProvenance("document", "z-1")),
            MakeAllocationCandidate("a", MakeSourceProvenance("document", "a-1"))
        };
        const std::vector<DescriptorIdentifierAllocationCandidate> DuplicatePermutationB = {
            DuplicatePermutationA[3U],
            DuplicatePermutationA[2U],
            DuplicatePermutationA[1U],
            DuplicatePermutationA[0U]
        };
        const auto DuplicateDiagnosticsA = DescriptorIdentifierAllocator::Allocate(
            DuplicatePermutationA);
        const auto DuplicateDiagnosticsB = DescriptorIdentifierAllocator::Allocate(
            DuplicatePermutationB);
        MPP_CHECK(!DuplicateDiagnosticsA.has_value());
        MPP_CHECK(!DuplicateDiagnosticsB.has_value());
        MPP_CHECK(SameDiagnostics(
            DuplicateDiagnosticsA.error(), DuplicateDiagnosticsB.error()));
        MPP_CHECK(CountDiagnosticCode(
            DuplicateDiagnosticsA.error(), DiagnosticCode::DuplicateExternalNodeIdentity) == 2U);

        const auto ThreeDuplicates = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("three", MakeSourceProvenance("document", "three-3")),
            MakeAllocationCandidate("three", MakeSourceProvenance("document", "three-1")),
            MakeAllocationCandidate("three", MakeSourceProvenance("document", "three-2"))
        });
        MPP_CHECK(!ThreeDuplicates.has_value());
        MPP_CHECK(CountDiagnosticCode(
            ThreeDuplicates.error(), DiagnosticCode::DuplicateExternalNodeIdentity) == 2U);
        for (const Diagnostic& Diagnostic : ThreeDuplicates.error())
        {
            MPP_CHECK(Diagnostic.PrimarySourceProvenance.has_value());
            MPP_CHECK(Diagnostic.RelatedSourceProvenance.has_value());
            MPP_CHECK(Diagnostic.PrimarySourceProvenance->GetSourceRecordIdentifier() ==
                "three-1");
        }
    }

    void TestDiagnosticCompatibility()
    {
        const Diagnostic LegacyDiagnostic{
            .Severity = DiagnosticSeverity::Error,
            .Code = DiagnosticCode::MissingDescriptor,
            .Message = "legacy diagnostic"
        };
        MPP_CHECK(!LegacyDiagnostic.PrimarySourceProvenance.has_value());
        MPP_CHECK(!LegacyDiagnostic.RelatedSourceProvenance.has_value());

        const Diagnostic GraphDiagnostic{
            .Severity = DiagnosticSeverity::Error,
            .Code = DiagnosticCode::InvalidGraphIRPinReference,
            .Message = "graph diagnostic",
            .SourceNodeIdentifier = NodeIdentifier(1U),
            .DestinationNodeIdentifier = NodeIdentifier(2U),
            .SourcePinReference = PinReference{
                NodeIdentifier(1U), PinIdentifier(3U)
            },
            .DestinationPinReference = PinReference{
                NodeIdentifier(2U), PinIdentifier(4U)
            }
        };
        MPP_CHECK(GraphDiagnostic.SourceNodeIdentifier == NodeIdentifier(1U));
        MPP_CHECK(GraphDiagnostic.DestinationNodeIdentifier == NodeIdentifier(2U));
        MPP_CHECK(GraphDiagnostic.SourcePinReference->LocalPinIdentifier == PinIdentifier(3U));
        MPP_CHECK(GraphDiagnostic.DestinationPinReference->LocalPinIdentifier ==
            PinIdentifier(4U));
        MPP_CHECK(!GraphDiagnostic.PrimarySourceProvenance.has_value());
        MPP_CHECK(!GraphDiagnostic.RelatedSourceProvenance.has_value());
    }

    void TestNodeDescriptorIdCatalogueScope()
    {
        const auto FirstCatalogueAssignment = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("catalogue-a-node")
        });
        const auto SecondCatalogueAssignment = DescriptorIdentifierAllocator::Allocate({
            MakeAllocationCandidate("catalogue-b-node")
        });
        MPP_CHECK(FirstCatalogueAssignment.has_value());
        MPP_CHECK(SecondCatalogueAssignment.has_value());
        MPP_CHECK((*FirstCatalogueAssignment)[0U].GetDescriptorIdentifier() ==
            (*SecondCatalogueAssignment)[0U].GetDescriptorIdentifier());
        MPP_CHECK((*FirstCatalogueAssignment)[0U].GetExternalIdentity() !=
            (*SecondCatalogueAssignment)[0U].GetExternalIdentity());

        const DescriptorCatalogueIdentity FirstIdentity = MakeCatalogueIdentity(
            "source", "build-1", 1U, "content-a");
        const DescriptorCatalogueIdentity SecondIdentity = MakeCatalogueIdentity(
            "source", "build-2", 1U, "content-b");
        const auto Compatibility = ValidateDescriptorCatalogueCompatibility(
            DescriptorCatalogueBinding(FirstIdentity),
            SecondIdentity
        );
        MPP_CHECK(!Compatibility.has_value());
        MPP_CHECK(HasDiagnosticCode(
            Compatibility.error(), DiagnosticCode::DescriptorCatalogueMismatch));
    }
}

int main()
{
    TestDescriptorCatalogueSemanticSchemaVersion();
    TestDescriptorCatalogueContentIdentifier();
    TestDescriptorCatalogueIdentity();
    TestExternalNodeIdentity();
    TestSourceProvenance();
    TestDescriptorCatalogueBinding();
    TestCatalogueCompatibility();
    TestDescriptorIdentifierAllocationCandidate();
    TestDescriptorIdentifierAssignment();
    TestDeterministicDescriptorIdentifierAllocation();
    TestInvalidAndDuplicateAllocation();
    TestDiagnosticCompatibility();
    TestNodeDescriptorIdCatalogueScope();
    return EXIT_SUCCESS;
}
