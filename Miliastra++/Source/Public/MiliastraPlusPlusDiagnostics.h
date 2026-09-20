#pragma once

#include <compare>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusIdentifiers.h"

namespace MiliastraPlusPlus
{
    /// Identifies a logical source record; parser-specific coordinates are added later if needed.
    class SourceProvenance final
    {
    public:
        SourceProvenance() = default;

        SourceProvenance(
            std::string SourceDocumentIdentifier,
            std::string SourceRecordIdentifier
        )
            : m_SourceDocumentIdentifier(std::move(SourceDocumentIdentifier))
            , m_SourceRecordIdentifier(std::move(SourceRecordIdentifier))
        {
        }

        [[nodiscard]] bool IsValid() const
        {
            return !m_SourceDocumentIdentifier.empty() &&
                !m_SourceRecordIdentifier.empty();
        }

        [[nodiscard]] const std::string& GetSourceDocumentIdentifier() const
        {
            return m_SourceDocumentIdentifier;
        }

        [[nodiscard]] const std::string& GetSourceRecordIdentifier() const
        {
            return m_SourceRecordIdentifier;
        }

        auto operator<=>(const SourceProvenance&) const = default;

    private:
        std::string m_SourceDocumentIdentifier;
        std::string m_SourceRecordIdentifier;
    };

    enum class DiagnosticSeverity
    {
        Warning,
        Error
    };

    enum class DiagnosticCode
    {
        NullNode,
        NullPin,
        DuplicateNodeIdentifier,
        DuplicatePinIdentifier,
        DuplicateLinkIdentifier,
        MissingSourcePin,
        MissingDestinationPin,
        SelfLink,
        SourcePinMustBeOutput,
        DestinationPinMustBeInput,
        IncompatiblePinCategories,
        ExecutionPinsMustUseFlowType,
        IncompatibleDataPinTypes,
        MultipleDataInputProducers,
        DuplicateLink,
        UnresolvedGenericPinType,
        TypePropagationConflict,
        DuplicateDescriptor,
        MissingDescriptor,
        FileOpenFailure,
        FileWriteFailure,
        InvalidGraphIRNode,
        DuplicateGraphIRNodeIdentifier,
        InvalidGraphVariable,
        DuplicateGraphVariableIdentifier,
        MissingGraphVariable,
        InvalidGraphIRPinReference,
        InvalidInputBinding,
        DuplicateInputBinding,
        InvalidNodeDescriptor,
        InvalidGraphBuilderState,
        UnsupportedCppType,
        DuplicateGraphVariableName,
        GenericConstraintConflict,
        UnresolvedGenericType,
        IncompatibleGraphIRTypes,
        InvalidControlEdge,
        InvalidExecutionModel,
        InvalidExecutionEntry,
        DuplicateExecutionEntryIdentifier,
        InvalidExecutionRegion,
        DuplicateExecutionRegionIdentifier,
        InvalidExecutionOwnership,
        InvalidExecutionHandle,
        ForeignBuilderContext,
        ExecutionEndpointAlreadyConsumed,
        InvalidExecutionScope,
        InvalidExecutionControlRole,
        InvalidBranchArmState,
        InvalidBranchOutcome,
        MissingExplicitJoin,
        OpenExecutionScope,
        InvalidExecutionReachability,
        ExecutionDataNotDominated,
        InvalidLoopTransfer,
        MalformedGraphIRJson,
        MissingAdapterNodeMapping,
        InvalidGraphIRAdapterLink,
        InvalidGraphIRAdapterMapping,
        InvalidDescriptorCatalogueIdentity,
        InvalidExternalNodeIdentity,
        InvalidSourceProvenance,
        DuplicateExternalNodeIdentity,
        DescriptorIdentifierExhausted,
        DescriptorCatalogueMismatch,
        InvalidNormalizedDescriptorRecord,
        UnsupportedDescriptorCatalogueSemanticSchemaVersion,
        MalformedGenshinClientBooleanFilterDescriptorSource,
        MissingGenshinClientBooleanFilterDescriptorSourceField,
        UnsupportedGenshinClientBooleanFilterDescriptorSourceForm,
        MalformedGenshinClientBooleanFilterReflectedDescriptorFamilySource,
        MissingGenshinClientBooleanFilterReflectedDescriptorFamilySourceField,
        UnsupportedGenshinClientBooleanFilterReflectedDescriptorFamilySourceForm,
        InvalidDescriptorSpecializationFamily,
        InvalidDescriptorSpecializationVariantBinding,
        DuplicateDescriptorSpecializationVariant,
        MalformedDescriptorCatalogueSnapshot,
        UnsupportedDescriptorCatalogueSnapshotVersion,
        DescriptorCatalogueSnapshotContentMismatch,
        DescriptorCatalogueSnapshotIdentifierMismatch,
        DescriptorCatalogueSnapshotSpecializationMismatch,
        InvalidGiaExportConfiguration,
        UnsupportedGiaExportTarget,
        InvalidGiaBackendMappingPackage,
        UnsupportedGiaBackendMappingSchemaVersion,
        IncompatibleGiaBackendMappingPackage,
        InvalidGiaBackendNodeMapping,
        InvalidGiaBackendPinMapping,
        DuplicateGiaBackendPinMapping,
        InvalidGiaExportContext
    };

    /// Callers can branch on Code; Message carries human-readable context.
    struct Diagnostic
    {
        DiagnosticSeverity Severity;
        DiagnosticCode Code;
        std::string Message;
        std::optional<NodeIdentifier> SourceNodeIdentifier;
        std::optional<NodeIdentifier> DestinationNodeIdentifier;
        std::optional<PinReference> SourcePinReference;
        std::optional<PinReference> DestinationPinReference;
        std::optional<SourceProvenance> PrimarySourceProvenance;
        std::optional<SourceProvenance> RelatedSourceProvenance;
        std::optional<std::string> ExternalIdentityKey;
    };

    using DiagnosticCollection = std::vector<Diagnostic>;

    [[nodiscard]] inline bool ContainsError(const DiagnosticCollection& Diagnostics)
    {
        for (const Diagnostic& CurrentDiagnostic : Diagnostics)
        {
            if (CurrentDiagnostic.Severity == DiagnosticSeverity::Error)
            {
                return true;
            }
        }

        return false;
    }
}
