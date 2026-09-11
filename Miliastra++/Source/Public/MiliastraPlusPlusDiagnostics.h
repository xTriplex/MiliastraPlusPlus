#pragma once

#include <optional>
#include <string>
#include <vector>

#include "MiliastraPlusPlusIdentifiers.h"

namespace MiliastraPlusPlus
{
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
        IncompatibleGraphIRTypes,
        InvalidControlEdge,
        MalformedGraphIRJson,
        MissingAdapterNodeMapping,
        InvalidGraphIRAdapterLink,
        InvalidGraphIRAdapterMapping
    };

    struct Diagnostic
    {
        DiagnosticSeverity Severity;
        DiagnosticCode Code;
        std::string Message;
        std::optional<NodeIdentifier> SourceNodeIdentifier;
        std::optional<NodeIdentifier> DestinationNodeIdentifier;
        std::optional<PinReference> SourcePinReference;
        std::optional<PinReference> DestinationPinReference;
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
