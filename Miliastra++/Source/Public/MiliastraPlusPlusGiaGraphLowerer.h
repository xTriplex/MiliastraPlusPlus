#pragma once

#include <algorithm>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaBackendGraph.h"
#include "MiliastraPlusPlusGiaExportContext.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

namespace MiliastraPlusPlus
{
    namespace GiaGraphLowererDetail
    {
        enum class ValidationStage
        {
            ContextValidation,
            GraphIRValidation,
            DescriptorResolution,
            MappingResolution,
            TargetPreflight,
            ValueResolution,
            ConnectionResolution,
            ModelValidation
        };

        struct PendingDiagnostic final
        {
            ValidationStage Stage;
            DiagnosticCode Code;
            std::optional<NodeInstanceId> Node;
            std::optional<PinIndex> Pin;
            std::string ExternalIdentityKey;
            std::string Message;
            std::optional<SourceProvenance> PrimarySourceProvenance;
            std::optional<SourceProvenance> RelatedSourceProvenance;
        };

        inline void Add(
            std::vector<PendingDiagnostic>& Diagnostics,
            ValidationStage Stage,
            DiagnosticCode Code,
            std::string Message,
            std::optional<NodeInstanceId> Node = std::nullopt,
            std::optional<PinIndex> Pin = std::nullopt,
            std::string ExternalIdentityKey = {},
            std::optional<SourceProvenance> PrimarySourceProvenance = std::nullopt,
            std::optional<SourceProvenance> RelatedSourceProvenance = std::nullopt
        )
        {
            Diagnostics.push_back(PendingDiagnostic{
                Stage,
                Code,
                Node,
                Pin,
                std::move(ExternalIdentityKey),
                std::move(Message),
                std::move(PrimarySourceProvenance),
                std::move(RelatedSourceProvenance)
            });
        }

        [[nodiscard]] inline auto DiagnosticSortKey(const PendingDiagnostic& Diagnostic)
        {
            return std::tuple{
                static_cast<int>(Diagnostic.Stage),
                static_cast<int>(Diagnostic.Code),
                Diagnostic.Node,
                Diagnostic.Pin,
                Diagnostic.ExternalIdentityKey,
                Diagnostic.Message
            };
        }

        [[nodiscard]] inline DiagnosticCollection Materialize(std::vector<PendingDiagnostic> Diagnostics)
        {
            std::stable_sort(
                Diagnostics.begin(),
                Diagnostics.end(),
                [](const PendingDiagnostic& Left, const PendingDiagnostic& Right)
                {
                    return DiagnosticSortKey(Left) < DiagnosticSortKey(Right);
                }
            );

            DiagnosticCollection Result;
            Result.reserve(Diagnostics.size());
            for (const PendingDiagnostic& Pending : Diagnostics)
            {
                Result.push_back(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = Pending.Code,
                    .Message = Pending.Message,
                    .PrimarySourceProvenance = Pending.PrimarySourceProvenance,
                    .RelatedSourceProvenance = Pending.RelatedSourceProvenance,
                    .ExternalIdentityKey = Pending.ExternalIdentityKey.empty()
                        ? std::nullopt
                        : std::optional<std::string>(Pending.ExternalIdentityKey)
                });
            }
            return Result;
        }

        [[nodiscard]] inline bool IsSupportedType(const TypeDesc& Type)
        {
            if (!Type.IsValid())
            {
                return false;
            }

            if (Type.GetKind() == TypeDesc::Kind::Boolean)
            {
                return true;
            }
            return Type.GetKind() == TypeDesc::Kind::Enum &&
                Type.GetEnumTypeIdentity() == EnumTypeIdentity("filter_return_type");
        }

        [[nodiscard]] inline bool IsLiteralCompatible(const LiteralValue& Literal, const TypeDesc& Type)
        {
            if (!IsSupportedType(Type) || !Literal.IsValid())
            {
                return false;
            }
            if (Type.GetKind() == TypeDesc::Kind::Boolean)
            {
                return Literal.Is<bool>();
            }
            return Literal.Is<EnumLiteralValue>() &&
                Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                Type.GetEnumTypeIdentity();
        }

        [[nodiscard]] inline const GiaBackendPinMapping* FindMapping(const GiaBackendNodeMapping& Mapping, PinIndex SemanticPin)
        {
            for (const GiaBackendPinMapping& PinMapping : Mapping.GetPinMappings())
            {
                if (PinMapping.GetSemanticPinIndex() == SemanticPin)
                {
                    return &PinMapping;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline const InputBindingRecord* FindBinding(const GraphIR& Graph, NodeInstanceId Node, PinIndex Pin)
        {
            for (const InputBindingRecord& Record : Graph.GetInputBindings())
            {
                if (Record.DestinationNode == Node &&
                    Record.DestinationInputPin == Pin)
                {
                    return &Record;
                }
            }
            return nullptr;
        }

        [[nodiscard]] inline const NodeInstance* FindNode(const GraphIR& Graph, NodeInstanceId Identifier)
        {
            return Graph.FindNode(Identifier);
        }

        [[nodiscard]] inline const PinSchema* FindPin(const NodeDescriptor& Descriptor, PinIndex Index)
        {
            if (!Index.IsValid() || Index.GetValue() >= Descriptor.GetPins().size())
            {
                return nullptr;
            }
            return &Descriptor.GetPins()[Index.GetValue()];
        }

        [[nodiscard]] inline bool IsDataInput(const PinSchema& Pin)
        {
            return Pin.GetCategory() == PinCategory::Data &&
                Pin.GetDirection() == PinDirection::Input;
        }

        [[nodiscard]] inline bool IsDataOutput(const PinSchema& Pin)
        {
            return Pin.GetCategory() == PinCategory::Data &&
                Pin.GetDirection() == PinDirection::Output;
        }

        [[nodiscard]] inline bool IsExecutionInput(const PinSchema& Pin)
        {
            return Pin.GetCategory() == PinCategory::Execution &&
                Pin.GetDirection() == PinDirection::Input;
        }

        [[nodiscard]] inline bool IsExecutionOutput(const PinSchema& Pin)
        {
            return Pin.GetCategory() == PinCategory::Execution &&
                Pin.GetDirection() == PinDirection::Output;
        }

        [[nodiscard]] inline bool IsBooleanTuple(const PinSchema& Pin, const GiaBackendPinMapping& Mapping)
        {
            return Pin.GetType().GetKind() == TypeDesc::Kind::Boolean &&
                Mapping.GetPinKind() == GiaPinKind::InputParameter &&
                Mapping.GetBackendTypeCode().GetValue() == 5 &&
                Mapping.GetLiteralEncoding() == GiaLiteralEncodingKind::Boolean;
        }

        [[nodiscard]] inline bool IsEnumTuple(const PinSchema& Pin, const GiaBackendPinMapping& Mapping)
        {
            return Pin.GetType().GetKind() == TypeDesc::Kind::Enum &&
                Pin.GetType().GetEnumTypeIdentity() ==
                    EnumTypeIdentity("filter_return_type") &&
                Mapping.GetPinKind() == GiaPinKind::InputParameter &&
                Mapping.GetBackendTypeCode().GetValue() == 13 &&
                Mapping.GetLiteralEncoding() == GiaLiteralEncodingKind::Enum;
        }

        [[nodiscard]] inline bool IsSupportedDataTypeTuple(const PinSchema& Pin, const GiaBackendPinMapping& Mapping)
        {
            const bool DirectionMatches =
                (Pin.GetDirection() == PinDirection::Input &&
                    Mapping.GetPinKind() == GiaPinKind::InputParameter) ||
                (Pin.GetDirection() == PinDirection::Output &&
                    Mapping.GetPinKind() == GiaPinKind::OutputParameter);
            if (!DirectionMatches)
            {
                return false;
            }
            if (Pin.GetType().GetKind() == TypeDesc::Kind::Boolean)
            {
                return Mapping.GetBackendTypeCode().GetValue() == 5;
            }
            return Pin.GetType().GetKind() == TypeDesc::Kind::Enum &&
                Pin.GetType().GetEnumTypeIdentity() ==
                    EnumTypeIdentity("filter_return_type") &&
                Mapping.GetBackendTypeCode().GetValue() == 13;
        }

        [[nodiscard]] inline bool IsSupportedLiteralEncoding(const PinSchema& Pin, const GiaBackendPinMapping& Mapping)
        {
            if (Pin.GetType().GetKind() == TypeDesc::Kind::Boolean)
            {
                return Mapping.GetLiteralEncoding() ==
                    GiaLiteralEncodingKind::Boolean;
            }
            return Pin.GetType().GetKind() == TypeDesc::Kind::Enum &&
                Mapping.GetLiteralEncoding() == GiaLiteralEncodingKind::Enum;
        }

        [[nodiscard]] inline bool IsSupportedLiteralEncoding(const LiteralValue& Literal, const GiaBackendPinMapping& Mapping)
        {
            if (Literal.Is<bool>())
            {
                return Mapping.GetLiteralEncoding() == GiaLiteralEncodingKind::Boolean &&
                    Mapping.GetBackendTypeCode().GetValue() == 5;
            }
            if (Literal.Is<EnumLiteralValue>())
            {
                return Mapping.GetLiteralEncoding() == GiaLiteralEncodingKind::Enum &&
                    Mapping.GetBackendTypeCode().GetValue() == 13;
            }
            return false;
        }

        [[nodiscard]] inline bool IsCanonicalDataConnectionOrder(const GiaBackendDataConnection& Left, const GiaBackendDataConnection& Right)
        {
            return Left < Right;
        }

        [[nodiscard]] inline bool IsCanonicalControlConnectionOrder(const GiaBackendControlConnection& Left, const GiaBackendControlConnection& Right)
        {
            return Left < Right;
        }

        struct ResolvedNode final
        {
            const NodeInstance* GraphNode = nullptr;
            const NodeDescriptor* Descriptor = nullptr;
            const DescriptorCatalogueEntry* CatalogueEntry = nullptr;
            const GiaBackendNodeMapping* Mapping = nullptr;
        };

        [[nodiscard]] inline std::string NodeMessage(NodeInstanceId Node, const std::string& Message)
        {
            return "Graph node " + std::to_string(Node.GetValue()) + ": " + Message;
        }

        [[nodiscard]] inline std::string PinMessage(NodeInstanceId Node, PinIndex Pin, const std::string& Message)
        {
            return NodeMessage(
                Node,
                "semantic pin " + std::to_string(Pin.GetValue()) + ": " + Message
            );
        }

        [[nodiscard]] inline bool HasPendingErrors(const std::vector<PendingDiagnostic>& Diagnostics)
        {
            return !Diagnostics.empty();
        }

        [[nodiscard]] inline bool IsSupportedTarget(const GiaExportConfiguration& Configuration)
        {
            return Configuration.GetTargetProfile() ==
                    GiaExportTargetProfile::ClientBooleanFilter &&
                Configuration.GetMode() == GiaExportMode::Beyond;
        }

        [[nodiscard]] inline GiaBackendGraphHeader MakeHeader(const GiaExportContext& Context)
        {
            const GiaExportConfiguration& Configuration = Context.GetConfiguration();
            return GiaBackendGraphHeader{
                .CatalogueIdentity = Context.GetRegistryContext().GetCatalogueIdentity(),
                .MappingSchemaVersion =
                    Context.GetMappingPackage().GetIdentity().GetSchemaVersion(),
                .TargetProfile = Configuration.GetTargetProfile(),
                .Mode = Configuration.GetMode(),
                .GraphIdentifier = Configuration.GetGraphIdentifier(),
                .GraphName = Configuration.GetGraphName(),
                .UniqueIdentifier = Configuration.GetUniqueIdentifier(),
                .EvaluationInterval = Configuration.GetEvaluationInterval(),
                .GraphType = 20001,
                .GraphWhich = 10
            };
        }
    }

    class GiaGraphLowerer final
    {
    public:
        [[nodiscard]] static std::expected<GiaBackendGraph, DiagnosticCollection> Lower(const GraphIR& Graph, const GiaExportContext& Context)
        {
            using namespace GiaGraphLowererDetail;

            if (!Context.IsValid())
            {
                std::vector<PendingDiagnostic> Diagnostics;
                Add(
                    Diagnostics,
                    ValidationStage::ContextValidation,
                    DiagnosticCode::InvalidGiaLoweringInput,
                    "The GIA lowering context is invalid."
                );
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            DiagnosticCollection GraphDiagnostics = GraphIRValidator::Validate(
                Graph,
                Context.GetRegistryContext().GetRegistry()
            );
            if (!GraphDiagnostics.empty())
            {
                return std::unexpected(std::move(GraphDiagnostics));
            }

            std::vector<const NodeInstance*> SortedGraphNodes;
            SortedGraphNodes.reserve(Graph.GetNodes().size());
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                SortedGraphNodes.push_back(&Node);
            }
            std::sort(
                SortedGraphNodes.begin(),
                SortedGraphNodes.end(),
                [](const NodeInstance* Left, const NodeInstance* Right)
                {
                    return Left->Identifier < Right->Identifier;
                }
            );

            std::vector<PendingDiagnostic> Diagnostics;
            std::vector<ResolvedNode> ResolvedNodes;
            ResolvedNodes.reserve(SortedGraphNodes.size());

            const NodeDescriptorRegistry& Registry =
                Context.GetRegistryContext().GetRegistry();
            const DescriptorCatalogue& Catalogue =
                Context.GetRegistryContext().GetCatalogue();
            const GiaBackendMappingPackage& MappingPackage =
                Context.GetMappingPackage();

            for (const NodeInstance* GraphNode : SortedGraphNodes)
            {
                ResolvedNode Resolved;
                Resolved.GraphNode = GraphNode;
                Resolved.Descriptor = Registry.Find(GraphNode->Descriptor);
                Resolved.CatalogueEntry = Catalogue.FindByDescriptorIdentifier(
                    GraphNode->Descriptor);

                if (Resolved.Descriptor == nullptr ||
                    Resolved.CatalogueEntry == nullptr ||
                    Resolved.CatalogueEntry->GetDescriptorIdentifier() !=
                        GraphNode->Descriptor)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::DescriptorResolution,
                        DiagnosticCode::InvalidGiaDescriptorCatalogueAssociation,
                        NodeMessage(
                            GraphNode->Identifier,
                            "the trusted descriptor and catalogue entry cannot be reconciled."
                        ),
                        GraphNode->Identifier
                    );
                    ResolvedNodes.push_back(Resolved);
                    continue;
                }

                Resolved.Mapping = MappingPackage.FindByExternalIdentity(
                    Resolved.CatalogueEntry->GetExternalIdentity());
                if (Resolved.Mapping == nullptr)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::MappingResolution,
                        DiagnosticCode::MissingGiaBackendNodeMapping,
                        NodeMessage(
                            GraphNode->Identifier,
                            "the exact opaque external identity has no backend mapping."
                        ),
                        GraphNode->Identifier,
                        std::nullopt,
                        Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                        Resolved.CatalogueEntry->GetSourceProvenance()
                    );
                }
                else
                {
                    if (!Resolved.Mapping->GetConcreteNodeIdentifier().has_value())
                    {
                        Add(
                            Diagnostics,
                            ValidationStage::MappingResolution,
                            DiagnosticCode::UnresolvedGiaBackendConcreteIdentity,
                            NodeMessage(
                                GraphNode->Identifier,
                                "the selected target requires a concrete backend identity."
                            ),
                            GraphNode->Identifier,
                            std::nullopt,
                            Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                            Resolved.Mapping->GetSourceProvenance()
                        );
                    }

                    for (std::size_t PinOffset = 0U;
                        PinOffset < Resolved.Descriptor->GetPins().size();
                        ++PinOffset)
                    {
                        const PinIndex SemanticPin(static_cast<std::uint32_t>(PinOffset));
                        if (FindMapping(*Resolved.Mapping, SemanticPin) == nullptr)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::MappingResolution,
                                DiagnosticCode::MissingGiaBackendPinMapping,
                                PinMessage(
                                    GraphNode->Identifier,
                                    SemanticPin,
                                    "the descriptor pin has no backend mapping."
                                ),
                                GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                    }
                }
                ResolvedNodes.push_back(Resolved);
            }

            if (!IsSupportedTarget(Context.GetConfiguration()))
            {
                Add(
                    Diagnostics,
                    ValidationStage::TargetPreflight,
                    DiagnosticCode::UnsupportedGiaTargetGraphFeature,
                    "The export configuration is outside the supported P6.2 target."
                );
            }

            if (!Graph.GetVariables().empty())
            {
                Add(
                    Diagnostics,
                    ValidationStage::TargetPreflight,
                    DiagnosticCode::UnsupportedGiaTargetGraphFeature,
                    "Graph variables are unsupported for the ClientBooleanFilter target."
                );
            }

            for (const ResolvedNode& Resolved : ResolvedNodes)
            {
                if (Resolved.Descriptor == nullptr || Resolved.Mapping == nullptr)
                {
                    continue;
                }

                for (std::size_t PinOffset = 0U;
                    PinOffset < Resolved.Descriptor->GetPins().size();
                    ++PinOffset)
                {
                    const PinIndex SemanticPin(static_cast<std::uint32_t>(PinOffset));
                    const PinSchema& Pin = Resolved.Descriptor->GetPins()[PinOffset];
                    const GiaBackendPinMapping* PinMapping = FindMapping(
                        *Resolved.Mapping,
                        SemanticPin
                    );
                    if (PinMapping == nullptr)
                    {
                        continue;
                    }

                    if (Pin.GetCategory() == PinCategory::Data)
                    {
                        const bool DirectionMatches =
                            (Pin.GetDirection() == PinDirection::Input &&
                                PinMapping->GetPinKind() == GiaPinKind::InputParameter) ||
                            (Pin.GetDirection() == PinDirection::Output &&
                                PinMapping->GetPinKind() == GiaPinKind::OutputParameter);
                        if (!DirectionMatches ||
                            !IsSupportedType(Pin.GetType()))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::TargetPreflight,
                                DiagnosticCode::UnsupportedGiaBackendType,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the descriptor type and backend pin tuple are unsupported."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        else if (!IsSupportedDataTypeTuple(Pin, *PinMapping))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::TargetPreflight,
                                DiagnosticCode::UnsupportedGiaBackendType,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the input does not have the required target type tuple."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        else if (Pin.GetDirection() == PinDirection::Input &&
                            PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Emit &&
                            !IsSupportedLiteralEncoding(Pin, *PinMapping))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::TargetPreflight,
                                DiagnosticCode::UnsupportedGiaBackendValue,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the input does not have the required target literal encoding."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                    }
                    else if (PinMapping->GetPinKind() == GiaPinKind::ClientExecution ||
                        PinMapping->GetPinKind() == GiaPinKind::ClientSignal ||
                        ((Pin.GetDirection() == PinDirection::Input &&
                            PinMapping->GetPinKind() != GiaPinKind::InputFlow) ||
                            (Pin.GetDirection() == PinDirection::Output &&
                                PinMapping->GetPinKind() != GiaPinKind::OutputFlow)))
                    {
                        Add(
                            Diagnostics,
                            ValidationStage::TargetPreflight,
                            DiagnosticCode::UnsupportedGiaTargetGraphFeature,
                            PinMessage(
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                "the execution pin has no supported ordinary-flow mapping."
                            ),
                            Resolved.GraphNode->Identifier,
                            SemanticPin,
                            Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                            Resolved.Mapping->GetSourceProvenance()
                        );
                    }
                }
            }

            std::vector<GiaBackendDataConnection> DataConnections;
            for (const ResolvedNode& Resolved : ResolvedNodes)
            {
                if (Resolved.Descriptor == nullptr || Resolved.Mapping == nullptr)
                {
                    continue;
                }

                for (std::size_t PinOffset = 0U;
                    PinOffset < Resolved.Descriptor->GetPins().size();
                    ++PinOffset)
                {
                    const PinIndex SemanticPin(static_cast<std::uint32_t>(PinOffset));
                    const PinSchema& Pin = Resolved.Descriptor->GetPins()[PinOffset];
                    if (!IsDataInput(Pin))
                    {
                        continue;
                    }

                    const GiaBackendPinMapping* PinMapping = FindMapping(
                        *Resolved.Mapping,
                        SemanticPin
                    );
                    if (PinMapping == nullptr)
                    {
                        continue;
                    }

                    const InputBindingRecord* BindingRecord = FindBinding(
                        Graph,
                        Resolved.GraphNode->Identifier,
                        SemanticPin
                    );
                    if (BindingRecord == nullptr)
                    {
                        if (Pin.GetDefaultValue().has_value())
                        {
                            if (PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Omit)
                            {
                                Add(
                                    Diagnostics,
                                    ValidationStage::ValueResolution,
                                    DiagnosticCode::InvalidGiaBackendInputResolution,
                                    PinMessage(
                                        Resolved.GraphNode->Identifier,
                                        SemanticPin,
                                        "a descriptor default cannot target an omitted backend pin."
                                    ),
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                    Resolved.Mapping->GetSourceProvenance()
                                );
                            }
                            else if (!IsLiteralCompatible(
                                *Pin.GetDefaultValue(),
                                Pin.GetType()) ||
                                !IsSupportedLiteralEncoding(
                                    *Pin.GetDefaultValue(),
                                    *PinMapping))
                            {
                                Add(
                                    Diagnostics,
                                    ValidationStage::ValueResolution,
                                    DiagnosticCode::UnsupportedGiaBackendValue,
                                    PinMessage(
                                        Resolved.GraphNode->Identifier,
                                        SemanticPin,
                                        "the descriptor default is unsupported by the backend tuple."
                                    ),
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                    Resolved.Mapping->GetSourceProvenance()
                                );
                            }
                        }
                        else if (PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Emit)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ValueResolution,
                                DiagnosticCode::InvalidGiaBackendInputResolution,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "an emitted input has neither a binding nor a descriptor default."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        continue;
                    }

                    const InputBinding& Binding = BindingRecord->Binding;
                    if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Binding))
                    {
                        if (PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Omit)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ValueResolution,
                                DiagnosticCode::InvalidGiaBackendInputResolution,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "a bound literal cannot target an omitted backend pin."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        else if (!IsLiteralCompatible(*Literal, Pin.GetType()))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ValueResolution,
                                DiagnosticCode::UnsupportedGiaBackendValue,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the explicit literal is not supported by the target type."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        else if (!IsSupportedLiteralEncoding(*Literal, *PinMapping))
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ValueResolution,
                                DiagnosticCode::UnsupportedGiaBackendValue,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the explicit literal does not match the backend literal encoding."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                    }
                    else if (const GraphVariableReference* Variable =
                        std::get_if<GraphVariableReference>(&Binding))
                    {
                        (void)Variable;
                        Add(
                            Diagnostics,
                            ValidationStage::TargetPreflight,
                            DiagnosticCode::UnsupportedGiaTargetGraphFeature,
                            PinMessage(
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                "graph-variable references are unsupported by the target."
                            ),
                            Resolved.GraphNode->Identifier,
                            SemanticPin,
                            Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                            Resolved.Mapping->GetSourceProvenance()
                        );
                    }
                    else if (const OutputReference* Output =
                        std::get_if<OutputReference>(&Binding))
                    {
                        const NodeInstance* SourceNode = FindNode(Graph, Output->SourceNode);
                        const NodeDescriptor* SourceDescriptor = nullptr;
                        const ResolvedNode* SourceResolved = nullptr;
                        for (const ResolvedNode& Candidate : ResolvedNodes)
                        {
                            if (Candidate.GraphNode == SourceNode)
                            {
                                SourceResolved = &Candidate;
                                SourceDescriptor = Candidate.Descriptor;
                                break;
                            }
                        }

                        const PinSchema* SourcePin = SourceDescriptor == nullptr
                            ? nullptr
                            : FindPin(*SourceDescriptor, Output->SourceOutputPin);
                        const GiaBackendPinMapping* SourceMapping =
                            SourceResolved == nullptr || SourceResolved->Mapping == nullptr
                            ? nullptr
                            : FindMapping(
                                *SourceResolved->Mapping,
                                Output->SourceOutputPin
                            );
                        const bool ValidConnection =
                            SourceNode != nullptr && SourcePin != nullptr &&
                            SourceMapping != nullptr &&
                            IsDataOutput(*SourcePin) &&
                            SourceMapping->GetPinKind() == GiaPinKind::OutputParameter &&
                            IsSupportedType(SourcePin->GetType()) &&
                            PinMapping->GetPinKind() == GiaPinKind::InputParameter &&
                            PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Emit &&
                            PinMapping->IsConnectable() &&
                            SourcePin->GetType().IsCompatibleWith(Pin.GetType());
                        if (!ValidConnection)
                        {
                            Add(
                                Diagnostics,
                                ValidationStage::ConnectionResolution,
                                DiagnosticCode::InvalidGiaBackendConnection,
                                PinMessage(
                                    Resolved.GraphNode->Identifier,
                                    SemanticPin,
                                    "the output reference cannot become a supported semantic data connection."
                                ),
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Resolved.CatalogueEntry->GetExternalIdentity().GetKey(),
                                Resolved.Mapping->GetSourceProvenance()
                            );
                        }
                        else
                        {
                            DataConnections.push_back(GiaBackendDataConnection{
                                Output->SourceNode,
                                Output->SourceOutputPin,
                                Resolved.GraphNode->Identifier,
                                SemanticPin,
                                Pin.GetType()
                            });
                        }
                    }
                }
            }

            std::vector<GiaBackendControlConnection> ControlConnections;
            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                const NodeInstance* SourceNode = FindNode(Graph, Edge.SourceNode);
                const NodeInstance* DestinationNode = FindNode(Graph, Edge.DestinationNode);
                const ResolvedNode* SourceResolved = nullptr;
                const ResolvedNode* DestinationResolved = nullptr;
                for (const ResolvedNode& Candidate : ResolvedNodes)
                {
                    if (Candidate.GraphNode == SourceNode)
                    {
                        SourceResolved = &Candidate;
                    }
                    if (Candidate.GraphNode == DestinationNode)
                    {
                        DestinationResolved = &Candidate;
                    }
                }

                const PinSchema* SourcePin = SourceResolved == nullptr ||
                    SourceResolved->Descriptor == nullptr
                    ? nullptr
                    : FindPin(*SourceResolved->Descriptor, Edge.SourceOutputPin);
                const PinSchema* DestinationPin = DestinationResolved == nullptr ||
                    DestinationResolved->Descriptor == nullptr
                    ? nullptr
                    : FindPin(*DestinationResolved->Descriptor, Edge.DestinationInputPin);
                const GiaBackendPinMapping* SourceMapping = SourceResolved == nullptr ||
                    SourceResolved->Mapping == nullptr
                    ? nullptr
                    : FindMapping(*SourceResolved->Mapping, Edge.SourceOutputPin);
                const GiaBackendPinMapping* DestinationMapping = DestinationResolved == nullptr ||
                    DestinationResolved->Mapping == nullptr
                    ? nullptr
                    : FindMapping(*DestinationResolved->Mapping, Edge.DestinationInputPin);

                const bool ValidConnection =
                    SourceNode != nullptr && DestinationNode != nullptr &&
                    SourcePin != nullptr && DestinationPin != nullptr &&
                    SourceMapping != nullptr && DestinationMapping != nullptr &&
                    IsExecutionOutput(*SourcePin) && IsExecutionInput(*DestinationPin) &&
                    SourceMapping->GetPinKind() == GiaPinKind::OutputFlow &&
                    DestinationMapping->GetPinKind() == GiaPinKind::InputFlow;
                if (!ValidConnection)
                {
                    Add(
                        Diagnostics,
                        ValidationStage::ConnectionResolution,
                        DiagnosticCode::InvalidGiaBackendConnection,
                        "A control edge cannot become a supported semantic flow connection.",
                        SourceNode == nullptr
                            ? std::nullopt
                            : std::optional<NodeInstanceId>(SourceNode->Identifier),
                        SourceNode == nullptr
                            ? std::nullopt
                            : std::optional<PinIndex>(Edge.SourceOutputPin)
                    );
                }
                else
                {
                    ControlConnections.push_back(GiaBackendControlConnection{
                        Edge.SourceNode,
                        Edge.SourceOutputPin,
                        Edge.DestinationNode,
                        Edge.DestinationInputPin
                    });
                }
            }

            std::sort(
                DataConnections.begin(),
                DataConnections.end(),
                IsCanonicalDataConnectionOrder
            );
            std::sort(
                ControlConnections.begin(),
                ControlConnections.end(),
                IsCanonicalControlConnectionOrder
            );

            if (HasPendingErrors(Diagnostics))
            {
                return std::unexpected(Materialize(std::move(Diagnostics)));
            }

            std::vector<GiaBackendNode> Nodes;
            Nodes.reserve(ResolvedNodes.size());
            for (const ResolvedNode& Resolved : ResolvedNodes)
            {
                GiaBackendNode Node{
                    .Trace = GiaBackendNodeTrace{
                        Resolved.GraphNode->Identifier,
                        Resolved.GraphNode->Descriptor,
                        Resolved.CatalogueEntry->GetExternalIdentity(),
                        Resolved.CatalogueEntry->GetSourceProvenance(),
                        Resolved.Mapping->GetSourceProvenance()
                    },
                    .Mapping = *Resolved.Mapping,
                    .Inputs = {}
                };

                for (std::size_t PinOffset = 0U;
                    PinOffset < Resolved.Descriptor->GetPins().size();
                    ++PinOffset)
                {
                    const PinIndex SemanticPin(static_cast<std::uint32_t>(PinOffset));
                    const PinSchema& Pin = Resolved.Descriptor->GetPins()[PinOffset];
                    if (!IsDataInput(Pin))
                    {
                        continue;
                    }

                    const GiaBackendPinMapping* PinMapping = FindMapping(
                        *Resolved.Mapping,
                        SemanticPin
                    );
                    if (PinMapping == nullptr)
                    {
                        continue;
                    }

                    const InputBindingRecord* BindingRecord = FindBinding(
                        Graph,
                        Resolved.GraphNode->Identifier,
                        SemanticPin
                    );
                    if (BindingRecord == nullptr)
                    {
                        if (Pin.GetDefaultValue().has_value() &&
                            PinMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Emit)
                        {
                            Node.Inputs.push_back(GiaBackendInputValue{
                                SemanticPin,
                                Pin.GetType(),
                                GiaBackendInputValueSourceKind::DescriptorDefault,
                                Pin.GetDefaultValue()
                            });
                        }
                        continue;
                    }

                    if (const LiteralValue* Literal =
                        std::get_if<LiteralValue>(&BindingRecord->Binding))
                    {
                        Node.Inputs.push_back(GiaBackendInputValue{
                            SemanticPin,
                            Pin.GetType(),
                            GiaBackendInputValueSourceKind::ExplicitLiteral,
                            *Literal
                        });
                    }
                    else if (std::holds_alternative<OutputReference>(BindingRecord->Binding))
                    {
                        Node.Inputs.push_back(GiaBackendInputValue{
                            SemanticPin,
                            Pin.GetType(),
                            GiaBackendInputValueSourceKind::DataConnection,
                            std::nullopt
                        });
                    }
                }
                Nodes.push_back(std::move(Node));
            }

            std::sort(
                Nodes.begin(),
                Nodes.end(),
                [](const GiaBackendNode& Left, const GiaBackendNode& Right)
                {
                    return Left.Trace.GraphNode < Right.Trace.GraphNode;
                }
            );

            GiaBackendGraph Result(
                MakeHeader(Context),
                std::move(Nodes),
                std::move(DataConnections),
                std::move(ControlConnections)
            );
            if (!Result.IsValid())
            {
                std::vector<PendingDiagnostic> ModelDiagnostics;
                Add(
                    ModelDiagnostics,
                    ValidationStage::ModelValidation,
                    DiagnosticCode::InvalidGiaBackendModel,
                    "The complete GIA semantic backend model violates its invariants."
                );
                return std::unexpected(Materialize(std::move(ModelDiagnostics)));
            }

            return Result;
        }
    };
}

