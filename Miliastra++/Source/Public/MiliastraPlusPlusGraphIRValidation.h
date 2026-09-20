#pragma once

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "MiliastraPlusPlusGraphIR.h"
#include "MiliastraPlusPlusGraphIRGenericValidation.h"

namespace MiliastraPlusPlus
{
    /// Canonical whole-graph validator for builder output and raw or deserialized
    /// GraphIR.
    class GraphIRValidator
    {
    public:
        [[nodiscard]] static DiagnosticCollection Validate(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            DiagnosticCollection Diagnostics;

            for (std::size_t Index = 0U; Index < Graph.GetNodes().size(); ++Index)
            {
                const NodeInstance& Node = Graph.GetNodes()[Index];
                if (!Node.Identifier.IsValid() || !Node.Descriptor.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphIRNode,
                        "GraphIR contains a node with an invalid instance or descriptor identifier.");
                }
                if (FindPriorNode(Graph, Node.Identifier, Index) != nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateGraphIRNodeIdentifier,
                        "GraphIR contains duplicate node instance identifiers.");
                }
                if (Node.Descriptor.IsValid() && Descriptors.Find(Node.Descriptor) == nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::MissingDescriptor,
                        "GraphIR references a node descriptor that is not registered.");
                }
                else if (Node.Descriptor.IsValid())
                {
                    const NodeDescriptor* Descriptor = Descriptors.Find(Node.Descriptor);
                    if (Descriptor != nullptr && !Descriptor->IsValid())
                    {
                        Add(
                            Diagnostics,
                            DiagnosticCode::InvalidNodeDescriptor,
                            "GraphIR references a registered descriptor with an invalid trusted schema.");
                    }
                }
            }

            for (std::size_t Index = 0U; Index < Graph.GetVariables().size(); ++Index)
            {
                const GraphVariable& Variable = Graph.GetVariables()[Index];
                if (!Variable.IsValid() || ContainsFlowType(Variable.Type))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphVariable,
                        "GraphIR contains an invalid graph variable or a control Flow type used as data.");
                }
                if (FindPriorVariable(Graph, Variable.Identifier, Index) != nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateGraphVariableIdentifier,
                        "GraphIR contains duplicate graph variable identifiers.");
                }
                if (Variable.DefaultValue.has_value() &&
                    !IsLiteralCompatible(*Variable.DefaultValue, Variable.Type))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::IncompatibleGraphIRTypes,
                        "A graph variable default literal is incompatible with its declared type.");
                }
            }

            for (const InputBindingRecord& Record : Graph.GetInputBindings())
            {
                const NodeInstance* DestinationNode = Graph.FindNode(Record.DestinationNode);
                const PinSchema* DestinationPin = FindPin(
                    DestinationNode, Record.DestinationInputPin, Descriptors);
                if (DestinationNode == nullptr)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidInputBinding,
                        "An input binding references a missing destination node.");
                    continue;
                }
                if (DestinationPin == nullptr || !Record.DestinationInputPin.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidGraphIRPinReference,
                        "An input binding references an invalid destination pin.");
                    continue;
                }
                if (DestinationPin->GetDirection() != PinDirection::Input ||
                    DestinationPin->GetCategory() != PinCategory::Data)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidInputBinding,
                        "An input binding destination must be a data input pin.");
                    continue;
                }
                if (DestinationPin->GetCardinality() != PinCardinality::Multiple &&
                    CountBindings(Graph, Record.DestinationNode, Record.DestinationInputPin) > 1U)
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::DuplicateInputBinding,
                        "A Single or Optional input pin has multiple bindings.");
                }
                ValidateBinding(Graph, Descriptors, Record, *DestinationPin, Diagnostics);
            }

            for (std::size_t Index = 0U; Index < Graph.GetControlEdges().size(); ++Index)
            {
                const ControlEdge& Edge = Graph.GetControlEdges()[Index];
                const NodeInstance* SourceNode = Graph.FindNode(Edge.SourceNode);
                const NodeInstance* DestinationNode = Graph.FindNode(Edge.DestinationNode);
                const PinSchema* SourcePin = FindPin(SourceNode, Edge.SourceOutputPin, Descriptors);
                const PinSchema* DestinationPin = FindPin(
                    DestinationNode, Edge.DestinationInputPin, Descriptors);
                if (SourceNode == nullptr || DestinationNode == nullptr ||
                    SourcePin == nullptr || DestinationPin == nullptr ||
                    !Edge.SourceOutputPin.IsValid() || !Edge.DestinationInputPin.IsValid())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "A control edge contains a missing or invalid node or pin reference.");
                    continue;
                }
                if (SourcePin->GetDirection() != PinDirection::Output ||
                    DestinationPin->GetDirection() != PinDirection::Input ||
                    SourcePin->GetCategory() != PinCategory::Execution ||
                    DestinationPin->GetCategory() != PinCategory::Execution ||
                    SourcePin->GetType() != TypeDesc::Flow() ||
                    DestinationPin->GetType() != TypeDesc::Flow())
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "A control edge must connect Flow-typed execution output and input pins.");
                }
                if (HasPriorControlEdge(Graph, Edge, Index))
                {
                    Add(
                        Diagnostics,
                        DiagnosticCode::InvalidControlEdge,
                        "GraphIR contains a duplicate control edge.");
                }
            }

            ValidateExecutionMetadata(Graph, Descriptors, Diagnostics);
            ValidateStructuredExecution(Graph, Descriptors, Diagnostics);

            DiagnosticCollection GenericDiagnostics =
                GraphIRGenericValidationDetail::ValidateGenericTypes(Graph, Descriptors);
            Diagnostics.insert(
                Diagnostics.end(),
                std::make_move_iterator(GenericDiagnostics.begin()),
                std::make_move_iterator(GenericDiagnostics.end())
            );

            return Diagnostics;
        }

    private:
        static bool IsValidExecutionModel(ExecutionModel Model)
        {
            switch (Model)
            {
            case ExecutionModel::Unstructured:
            case ExecutionModel::Structured:
                return true;
            }
            return false;
        }

        static bool IsValidRegionKind(ExecutionRegionKind Kind)
        {
            switch (Kind)
            {
            case ExecutionRegionKind::Entry:
            case ExecutionRegionKind::BranchArm:
            case ExecutionRegionKind::LoopBody:
                return true;
            }
            return false;
        }

        static bool HasFlowPins(const NodeDescriptor& Descriptor)
        {
            for (const PinSchema& Pin : Descriptor.GetPins())
            {
                if (Pin.GetCategory() == PinCategory::Execution ||
                    Pin.GetType() == TypeDesc::Flow())
                {
                    return true;
                }
            }
            return false;
        }

        static std::size_t CountEntries(
            const GraphIR& Graph,
            ExecutionEntryId Identifier
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                if (Entry.Identifier == Identifier)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountRegions(
            const GraphIR& Graph,
            ExecutionRegionId Identifier
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.Identifier == Identifier)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountRegionsForEntry(
            const GraphIR& Graph,
            ExecutionEntryId Entry,
            ExecutionRegionKind Kind
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.Entry == Entry && Region.Kind == Kind)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountOwnerRegions(
            const GraphIR& Graph,
            NodeInstanceId Owner,
            ExecutionRegionKind Kind
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.OwnerNode == Owner && Region.Kind == Kind)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountOwnerRegionsAtPin(
            const GraphIR& Graph,
            NodeInstanceId Owner,
            ExecutionRegionKind Kind,
            PinIndex Pin
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.OwnerNode == Owner && Region.Kind == Kind &&
                    Region.OwnerOutputPin == Pin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountRootReferences(
            const GraphIR& Graph,
            NodeInstanceId Root
        )
        {
            std::size_t Count = 0U;
            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                if (Entry.RootNode == Root)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static bool RegionOwnerPinMatches(
            const NodeDescriptor& Descriptor,
            ExecutionRegionKind Kind,
            PinIndex OwnerPin
        )
        {
            if (!Descriptor.GetExecutionControlSchema().has_value())
            {
                return false;
            }
            return std::visit([Kind, OwnerPin](const auto& Schema)
            {
                using SchemaType = std::decay_t<decltype(Schema)>;
                if constexpr (std::is_same_v<SchemaType, BranchControlSchema>)
                {
                    return Kind == ExecutionRegionKind::BranchArm &&
                        (Schema.TrueOutput == OwnerPin || Schema.FalseOutput == OwnerPin);
                }
                else if constexpr (std::is_same_v<SchemaType, LoopControlSchema>)
                {
                    return Kind == ExecutionRegionKind::LoopBody &&
                        Schema.BodyOutput == OwnerPin;
                }
                else
                {
                    return false;
                }
            }, *Descriptor.GetExecutionControlSchema());
        }

        static void ValidateExecutionMetadata(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics
        )
        {
            if (!IsValidExecutionModel(Graph.GetExecutionModel()))
            {
                Add(Diagnostics, DiagnosticCode::InvalidExecutionModel,
                    "GraphIR contains an invalid execution-model discriminant.");
                return;
            }

            if (Graph.GetExecutionModel() == ExecutionModel::Unstructured)
            {
                if (!Graph.GetExecutionEntries().empty() ||
                    !Graph.GetExecutionRegions().empty())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionModel,
                        "An Unstructured graph cannot contain execution entries or regions.");
                }
                for (const NodeInstance& Node : Graph.GetNodes())
                {
                    if (Node.ExecutionRegion.has_value())
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                            "An Unstructured node cannot carry execution-region membership.");
                    }
                }
                return;
            }

            if (Graph.GetExecutionEntries().empty())
            {
                Add(Diagnostics, DiagnosticCode::InvalidExecutionModel,
                    "A Structured graph must contain at least one execution entry.");
            }

            for (std::size_t Index = 0U; Index < Graph.GetExecutionEntries().size(); ++Index)
            {
                const ExecutionEntry& Entry = Graph.GetExecutionEntries()[Index];
                if (!Entry.Identifier.IsValid() || !Entry.RootNode.IsValid())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                        "A structured execution entry has an invalid identifier or root node.");
                }
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (Graph.GetExecutionEntries()[Prior].Identifier == Entry.Identifier)
                    {
                        Add(Diagnostics, DiagnosticCode::DuplicateExecutionEntryIdentifier,
                            "Structured execution entry identifiers must be unique.");
                        break;
                    }
                }
            }

            for (std::size_t Index = 0U; Index < Graph.GetExecutionRegions().size(); ++Index)
            {
                const ExecutionRegion& Region = Graph.GetExecutionRegions()[Index];
                if (!Region.Identifier.IsValid() || !Region.Entry.IsValid() ||
                    !IsValidRegionKind(Region.Kind))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A structured execution region has an invalid identifier, entry, or kind.");
                }
                for (std::size_t Prior = 0U; Prior < Index; ++Prior)
                {
                    if (Graph.GetExecutionRegions()[Prior].Identifier == Region.Identifier)
                    {
                        Add(Diagnostics, DiagnosticCode::DuplicateExecutionRegionIdentifier,
                            "Structured execution region identifiers must be unique.");
                        break;
                    }
                }
            }

            // Entry roots and their unique parentless Entry regions are persisted independently.
            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                if (!Entry.Identifier.IsValid() || CountEntries(Graph, Entry.Identifier) != 1U)
                {
                    continue;
                }
                if (!Entry.RootNode.IsValid() ||
                    std::count_if(Graph.GetNodes().begin(), Graph.GetNodes().end(),
                        [&Entry](const NodeInstance& Node)
                        {
                            return Node.Identifier == Entry.RootNode;
                        }) != 1)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                        "An execution entry root must reference exactly one graph node.");
                    continue;
                }
                const NodeInstance* RootNode = Graph.FindNode(Entry.RootNode);
                const NodeDescriptor* RootDescriptor = RootNode == nullptr
                    ? nullptr : Descriptors.Find(RootNode->Descriptor);
                if (RootDescriptor == nullptr ||
                    !RootDescriptor->GetExecutionControlSchema().has_value() ||
                    !std::holds_alternative<EntryControlSchema>(
                        *RootDescriptor->GetExecutionControlSchema()))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                        "An execution entry root must use a descriptor with the Entry control role.");
                }
                if (CountRootReferences(Graph, Entry.RootNode) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                        "An Entry-role node can be the root of exactly one execution entry.");
                }
                const ExecutionRegion* RootRegion = nullptr;
                std::size_t RootRegionCount = 0U;
                for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
                {
                    if (Region.Entry == Entry.Identifier &&
                        Region.Kind == ExecutionRegionKind::Entry)
                    {
                        RootRegion = &Region;
                        ++RootRegionCount;
                    }
                }
                if (RootRegionCount != 1U || RootRegion == nullptr)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                        "Each execution entry must own exactly one Entry region.");
                }
                else if (RootNode != nullptr &&
                    RootNode->ExecutionRegion != RootRegion->Identifier)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "An execution entry root must belong to its entry region.");
                }
            }

            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (!Region.Identifier.IsValid() || CountRegions(Graph, Region.Identifier) != 1U ||
                    !IsValidRegionKind(Region.Kind))
                {
                    continue;
                }
                if (!Region.Entry.IsValid() || CountEntries(Graph, Region.Entry) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "An execution region must reference exactly one declared entry.");
                    continue;
                }

                if (Region.Kind == ExecutionRegionKind::Entry)
                {
                    if (Region.Parent.has_value() || Region.OwnerNode.has_value() ||
                        Region.OwnerOutputPin.has_value())
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                            "An Entry region cannot have a parent or child-owner metadata.");
                    }
                    continue;
                }

                if (!Region.Parent.has_value() || !Region.Parent->IsValid() ||
                    !Region.OwnerNode.has_value() || !Region.OwnerNode->IsValid() ||
                    !Region.OwnerOutputPin.has_value() || !Region.OwnerOutputPin->IsValid())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A child execution region requires a parent, owner node, and owner output pin.");
                    continue;
                }
                if (CountRegions(Graph, *Region.Parent) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A child execution region references a missing or ambiguous parent region.");
                    continue;
                }
                const ExecutionRegion* Parent = Graph.FindExecutionRegion(*Region.Parent);
                if (Parent == nullptr || Parent->Entry != Region.Entry)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A child execution region and its parent must belong to the same entry.");
                }
                if (std::count_if(Graph.GetNodes().begin(), Graph.GetNodes().end(),
                    [&Region](const NodeInstance& Node)
                    {
                        return Node.Identifier == *Region.OwnerNode;
                    }) != 1)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A child execution region owner must reference exactly one graph node.");
                    continue;
                }
                const NodeInstance* OwnerNode = Graph.FindNode(*Region.OwnerNode);
                if (OwnerNode == nullptr || OwnerNode->ExecutionRegion != Region.Parent)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "A child-region owner node must belong to the declared parent region.");
                }
                const NodeDescriptor* OwnerDescriptor = OwnerNode == nullptr
                    ? nullptr : Descriptors.Find(OwnerNode->Descriptor);
                if (OwnerDescriptor == nullptr ||
                    !RegionOwnerPinMatches(*OwnerDescriptor, Region.Kind,
                        *Region.OwnerOutputPin))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A child-region owner pin must match the trusted Branch or Loop role schema.");
                }
                if (CountOwnerRegionsAtPin(Graph, *Region.OwnerNode, Region.Kind,
                    *Region.OwnerOutputPin) > 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "A construct cannot own duplicate child regions for the same output role.");
                }
            }

            // Every parent chain must be a finite path ending at one Entry region.
            for (const ExecutionRegion& Start : Graph.GetExecutionRegions())
            {
                if (!Start.Identifier.IsValid() || CountRegions(Graph, Start.Identifier) != 1U)
                {
                    continue;
                }
                std::vector<ExecutionRegionId> Visited;
                const ExecutionRegion* Current = &Start;
                bool Broken = false;
                bool HasCycle = false;
                while (Current != nullptr)
                {
                    bool Repeated = false;
                    for (const ExecutionRegionId Prior : Visited)
                    {
                        if (Prior == Current->Identifier)
                        {
                            Repeated = true;
                            break;
                        }
                    }
                    if (Repeated)
                    {
                        HasCycle = true;
                        break;
                    }
                    Visited.push_back(Current->Identifier);
                    if (Current->Kind == ExecutionRegionKind::Entry)
                    {
                        break;
                    }
                    if (!Current->Parent.has_value() ||
                        CountRegions(Graph, *Current->Parent) != 1U)
                    {
                        Broken = true;
                        break;
                    }
                    Current = Graph.FindExecutionRegion(*Current->Parent);
                }
                if (HasCycle)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "Execution-region parent relationships must be acyclic.");
                }
                else if (Broken || Current == nullptr ||
                    Current->Kind != ExecutionRegionKind::Entry ||
                    Current->Entry != Start.Entry)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                        "Every execution-region hierarchy must terminate at its owning Entry region.");
                }
            }

            // Verify region counts required by each descriptor-backed construct and every node's owner.
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                    ? Descriptors.Find(Node.Descriptor) : nullptr;
                if (Descriptor == nullptr)
                {
                    continue;
                }
                const bool HasSchema = Descriptor->GetExecutionControlSchema().has_value();
                const bool HasFlow = HasFlowPins(*Descriptor);
                if (!HasSchema)
                {
                    if (HasFlow)
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                            "A Flow-bearing descriptor without a trusted control schema is Unstructured-only.");
                    }
                    if (Node.ExecutionRegion.has_value())
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                            "A data-only node cannot belong to an execution region.");
                    }
                    continue;
                }

                if (!Node.ExecutionRegion.has_value() || !Node.ExecutionRegion->IsValid())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "Every structured execution-capable node must belong to exactly one region.");
                }
                else if (CountRegions(Graph, *Node.ExecutionRegion) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "A node execution-region membership must reference exactly one region.");
                }

                if (Descriptor->GetExecutionControlSchema().has_value())
                {
                    const ExecutionControlSchema& Schema =
                        *Descriptor->GetExecutionControlSchema();
                    if (std::holds_alternative<EntryControlSchema>(Schema))
                    {
                        if (CountRootReferences(Graph, Node.Identifier) != 1U)
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidExecutionEntry,
                                "Every Entry-role node must be the root of exactly one execution entry.");
                        }
                    }
                    else if (std::holds_alternative<BranchControlSchema>(Schema))
                    {
                        const auto& Branch = std::get<BranchControlSchema>(Schema);
                        if (CountOwnerRegions(Graph, Node.Identifier,
                                ExecutionRegionKind::BranchArm) != 2U ||
                            CountOwnerRegionsAtPin(Graph, Node.Identifier,
                                ExecutionRegionKind::BranchArm, Branch.TrueOutput) != 1U ||
                            CountOwnerRegionsAtPin(Graph, Node.Identifier,
                                ExecutionRegionKind::BranchArm, Branch.FalseOutput) != 1U)
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                                "A Branch-role node must own one region for each declared binary arm output.");
                        }
                    }
                    else if (std::holds_alternative<LoopControlSchema>(Schema))
                    {
                        const auto& Loop = std::get<LoopControlSchema>(Schema);
                        if (CountOwnerRegions(Graph, Node.Identifier,
                                ExecutionRegionKind::LoopBody) != 1U ||
                            CountOwnerRegionsAtPin(Graph, Node.Identifier,
                                ExecutionRegionKind::LoopBody, Loop.BodyOutput) != 1U)
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidExecutionRegion,
                                "A Loop-role node must own exactly one LoopBody region at its Body output.");
                        }
                    }
                }
            }

            // Control edges may connect only execution nodes in the same declared entry.
            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
                const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                if (Source == nullptr || Destination == nullptr ||
                    std::count_if(Graph.GetNodes().begin(), Graph.GetNodes().end(),
                        [&Edge](const NodeInstance& Node)
                        {
                            return Node.Identifier == Edge.SourceNode;
                        }) != 1 ||
                    std::count_if(Graph.GetNodes().begin(), Graph.GetNodes().end(),
                        [&Edge](const NodeInstance& Node)
                        {
                            return Node.Identifier == Edge.DestinationNode;
                        }) != 1 ||
                    !Source->ExecutionRegion.has_value() ||
                    !Destination->ExecutionRegion.has_value() ||
                    CountRegions(Graph, *Source->ExecutionRegion) != 1U ||
                    CountRegions(Graph, *Destination->ExecutionRegion) != 1U)
                {
                    continue;
                }
                const ExecutionRegion* SourceRegion =
                    Graph.FindExecutionRegion(*Source->ExecutionRegion);
                const ExecutionRegion* DestinationRegion =
                    Graph.FindExecutionRegion(*Destination->ExecutionRegion);
                if (SourceRegion != nullptr && DestinationRegion != nullptr &&
                    SourceRegion->Entry != DestinationRegion->Entry)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "A structured control edge cannot cross execution-entry ownership.");
                }
            }
        }

        template<typename Schema>
        static const Schema* GetControlSchema(const NodeDescriptor* Descriptor)
        {
            if (Descriptor == nullptr || !Descriptor->GetExecutionControlSchema().has_value())
            {
                return nullptr;
            }
            return std::get_if<Schema>(&*Descriptor->GetExecutionControlSchema());
        }

        static const ExecutionRegion* FindBranchArmRegion(
            const GraphIR& Graph,
            NodeInstanceId BranchNode,
            PinIndex OutputPin
        )
        {
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.Kind == ExecutionRegionKind::BranchArm &&
                    Region.OwnerNode == BranchNode && Region.OwnerOutputPin == OutputPin)
                {
                    return &Region;
                }
            }
            return nullptr;
        }

        static const ExecutionRegion* FindLoopBodyRegion(
            const GraphIR& Graph,
            NodeInstanceId LoopNode,
            PinIndex BodyOutput
        )
        {
            for (const ExecutionRegion& Region : Graph.GetExecutionRegions())
            {
                if (Region.Kind == ExecutionRegionKind::LoopBody &&
                    Region.OwnerNode == LoopNode && Region.OwnerOutputPin == BodyOutput)
                {
                    return &Region;
                }
            }
            return nullptr;
        }

        static const ExecutionRegion* EffectiveSourceRegion(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ControlEdge& Edge
        )
        {
            const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
            const NodeDescriptor* Descriptor = Source == nullptr
                ? nullptr : Descriptors.Find(Source->Descriptor);
            const BranchControlSchema* Branch = GetControlSchema<BranchControlSchema>(Descriptor);
            if (Branch != nullptr && (Edge.SourceOutputPin == Branch->TrueOutput ||
                Edge.SourceOutputPin == Branch->FalseOutput))
            {
                return FindBranchArmRegion(Graph, Edge.SourceNode, Edge.SourceOutputPin);
            }
            const LoopControlSchema* Loop = GetControlSchema<LoopControlSchema>(Descriptor);
            if (Loop != nullptr && Edge.SourceOutputPin == Loop->BodyOutput)
            {
                return FindLoopBodyRegion(Graph, Edge.SourceNode, Loop->BodyOutput);
            }
            return Source != nullptr && Source->ExecutionRegion.has_value()
                ? Graph.FindExecutionRegion(*Source->ExecutionRegion) : nullptr;
        }

        static bool IsInsideLoopBody(
            const GraphIR& Graph,
            const NodeInstance& Node
        )
        {
            if (!Node.ExecutionRegion.has_value())
            {
                return false;
            }
            const ExecutionRegion* Region = Graph.FindExecutionRegion(*Node.ExecutionRegion);
            std::vector<ExecutionRegionId> VisitedRegions;
            while (Region != nullptr)
            {
                if (std::find(VisitedRegions.begin(), VisitedRegions.end(), Region->Identifier) !=
                    VisitedRegions.end())
                {
                    // The region-tree validation reports this malformed cycle. Stop ancestry
                    // queries here so later branch/loop checks cannot hang on the bad graph.
                    return false;
                }
                VisitedRegions.push_back(Region->Identifier);
                if (Region->Kind == ExecutionRegionKind::LoopBody)
                {
                    return true;
                }
                if (!Region->Parent.has_value())
                {
                    break;
                }
                Region = Graph.FindExecutionRegion(*Region->Parent);
            }
            return false;
        }

        static bool IsLoopControlNode(
            const NodeInstance& Node,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                ? Descriptors.Find(Node.Descriptor) : nullptr;
            return GetControlSchema<LoopControlSchema>(Descriptor) != nullptr;
        }

        static bool IsReturnNode(
            const NodeInstance& Node,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                ? Descriptors.Find(Node.Descriptor) : nullptr;
            return GetControlSchema<ReturnControlSchema>(Descriptor) != nullptr;
        }

        static void ValidateReturnTerminalTopology(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics
        )
        {
            if (Graph.GetExecutionModel() != ExecutionModel::Structured)
            {
                return;
            }
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                    ? Descriptors.Find(Node.Descriptor) : nullptr;
                const ReturnControlSchema* Return =
                    GetControlSchema<ReturnControlSchema>(Descriptor);
                if (Return == nullptr)
                {
                    continue;
                }

                if (CountIncomingEndpoint(Graph, Node.Identifier, Return->ExecutionInput) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "A structured Return must have exactly one explicit execution predecessor.");
                }
                const bool HasSuccessor = std::any_of(
                    Graph.GetControlEdges().begin(), Graph.GetControlEdges().end(),
                    [&Node](const ControlEdge& Edge)
                    {
                        return Edge.SourceNode == Node.Identifier;
                    });
                if (HasSuccessor)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "A Return path is terminal and cannot have an execution successor.");
                }
            }
        }

        static bool IsLoopTransferEdge(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ControlEdge& Edge
        )
        {
            const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
            if (Destination == nullptr)
            {
                return false;
            }
            const NodeDescriptor* DestinationDescriptor = Descriptors.Find(Destination->Descriptor);
            const LoopControlSchema* DestinationLoop =
                GetControlSchema<LoopControlSchema>(DestinationDescriptor);
            return DestinationLoop != nullptr &&
                (Edge.DestinationInputPin == DestinationLoop->RepeatInput ||
                    Edge.DestinationInputPin == DestinationLoop->BreakInput);
        }

        static std::size_t CountOutgoingEndpoint(
            const GraphIR& Graph,
            NodeInstanceId Node,
            PinIndex Pin
        )
        {
            std::size_t Count = 0U;
            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                if (Edge.SourceNode == Node && Edge.SourceOutputPin == Pin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static std::size_t CountIncomingEndpoint(
            const GraphIR& Graph,
            NodeInstanceId Node,
            PinIndex Pin
        )
        {
            std::size_t Count = 0U;
            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                if (Edge.DestinationNode == Node && Edge.DestinationInputPin == Pin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static void ValidateStructuredExecution(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics
        )
        {
            if (Graph.GetExecutionModel() != ExecutionModel::Structured)
            {
                return;
            }
            const std::size_t InitialDiagnosticCount = Diagnostics.size();

            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
                const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                if (Source == nullptr || Destination == nullptr)
                {
                    continue;
                }
                const PinSchema* SourcePin = FindPin(Source, Edge.SourceOutputPin, Descriptors);
                const PinSchema* DestinationPin = FindPin(
                    Destination, Edge.DestinationInputPin, Descriptors);
                if (SourcePin == nullptr || DestinationPin == nullptr ||
                    SourcePin->GetCategory() != PinCategory::Execution ||
                    DestinationPin->GetCategory() != PinCategory::Execution)
                {
                    continue;
                }
                if (IsReturnNode(*Source, Descriptors))
                {
                    continue;
                }
                if (CountOutgoingEndpoint(Graph, Edge.SourceNode, Edge.SourceOutputPin) > 1U)
                {
                    Add(Diagnostics, DiagnosticCode::ExecutionEndpointAlreadyConsumed,
                        "A structured Flow output endpoint may have at most one successor.");
                }
                if (DestinationPin->GetCardinality() != PinCardinality::Multiple &&
                    CountIncomingEndpoint(Graph, Edge.DestinationNode,
                        Edge.DestinationInputPin) > 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidControlEdge,
                        "A structured Flow input with Single or Optional cardinality has multiple predecessors.");
                }
            }

            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
                const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                if (Source == nullptr || Destination == nullptr ||
                    !Source->ExecutionRegion.has_value() ||
                    !Destination->ExecutionRegion.has_value() ||
                    IsReturnNode(*Source, Descriptors) ||
                    IsLoopTransferEdge(Graph, Descriptors, Edge))
                {
                    continue;
                }
                const ExecutionRegion* SourceRegion =
                    EffectiveSourceRegion(Graph, Descriptors, Edge);
                const ExecutionRegion* DestinationRegion =
                    Graph.FindExecutionRegion(*Destination->ExecutionRegion);
                if (SourceRegion == nullptr || DestinationRegion == nullptr)
                {
                    continue;
                }
                if (SourceRegion->Entry != DestinationRegion->Entry)
                {
                    continue;
                }
                if (SourceRegion->Identifier == DestinationRegion->Identifier)
                {
                    continue;
                }
                const bool IsParentExit = SourceRegion->Kind == ExecutionRegionKind::BranchArm &&
                    SourceRegion->Parent.has_value() &&
                    *SourceRegion->Parent == DestinationRegion->Identifier;
                const NodeDescriptor* DestinationDescriptor = Descriptors.Find(Destination->Descriptor);
                const bool ParentContinuation =
                    GetControlSchema<JoinControlSchema>(DestinationDescriptor) != nullptr ||
                    GetControlSchema<SequenceControlSchema>(DestinationDescriptor) != nullptr;
                if (IsParentExit && ParentContinuation)
                {
                    continue;
                }
                const NodeDescriptor* SourceDescriptor = Descriptors.Find(Source->Descriptor);
                const BranchControlSchema* Branch =
                    GetControlSchema<BranchControlSchema>(SourceDescriptor);
                const ExecutionRegion* DestinationParent = DestinationRegion->Parent.has_value()
                    ? Graph.FindExecutionRegion(*DestinationRegion->Parent) : nullptr;
                const bool IsOwnedArmEntry = Branch != nullptr &&
                    (Edge.SourceOutputPin == Branch->TrueOutput ||
                        Edge.SourceOutputPin == Branch->FalseOutput) &&
                    DestinationRegion->Kind == ExecutionRegionKind::BranchArm &&
                    DestinationRegion->OwnerNode == Source->Identifier &&
                    DestinationRegion->OwnerOutputPin == Edge.SourceOutputPin &&
                    DestinationParent != nullptr &&
                    Source->ExecutionRegion == DestinationParent->Identifier;
                if (!IsOwnedArmEntry)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                        "A structured control edge crosses an execution region boundary without its owning Branch role.");
                }
            }

            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                if (!Entry.Identifier.IsValid() || !Entry.RootNode.IsValid() ||
                    CountEntries(Graph, Entry.Identifier) != 1U)
                {
                    continue;
                }
                const NodeInstance* Root = Graph.FindNode(Entry.RootNode);
                const NodeDescriptor* RootDescriptor = Root == nullptr
                    ? nullptr : Descriptors.Find(Root->Descriptor);
                const EntryControlSchema* EntrySchema =
                    GetControlSchema<EntryControlSchema>(RootDescriptor);
                if (Root == nullptr || EntrySchema == nullptr)
                {
                    continue;
                }
                if (CountOutgoingEndpoint(Graph, Root->Identifier,
                    EntrySchema->ExecutionOutput) == 0U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "A structured Entry root must have an explicit execution successor.");
                }

                std::vector<NodeInstanceId> Reachable;
                Reachable.push_back(Root->Identifier);
                for (std::size_t Cursor = 0U; Cursor < Reachable.size(); ++Cursor)
                {
                    const NodeInstanceId Current = Reachable[Cursor];
                    const NodeInstance* CurrentNode = Graph.FindNode(Current);
                    if (CurrentNode == nullptr || IsReturnNode(*CurrentNode, Descriptors))
                    {
                        continue;
                    }
                    for (const ControlEdge& Edge : Graph.GetControlEdges())
                    {
                        if (Edge.SourceNode != Current ||
                            IsLoopTransferEdge(Graph, Descriptors, Edge))
                        {
                            continue;
                        }
                        const PinSchema* SourcePin = FindPin(
                            CurrentNode, Edge.SourceOutputPin, Descriptors);
                        const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                        const PinSchema* DestinationPin = FindPin(
                            Destination, Edge.DestinationInputPin, Descriptors);
                        const ExecutionRegion* Region = Destination != nullptr &&
                            Destination->ExecutionRegion.has_value()
                            ? Graph.FindExecutionRegion(*Destination->ExecutionRegion) : nullptr;
                        if (SourcePin == nullptr || SourcePin->GetDirection() != PinDirection::Output ||
                            SourcePin->GetCategory() != PinCategory::Execution ||
                            SourcePin->GetType() != TypeDesc::Flow() ||
                            DestinationPin == nullptr ||
                            DestinationPin->GetDirection() != PinDirection::Input ||
                            DestinationPin->GetCategory() != PinCategory::Execution ||
                            DestinationPin->GetType() != TypeDesc::Flow() ||
                            Destination == nullptr || Region == nullptr || Region->Entry != Entry.Identifier ||
                            IsInsideLoopBody(Graph, *Destination))
                        {
                            continue;
                        }
                        bool Seen = false;
                        for (const NodeInstanceId Prior : Reachable)
                        {
                            if (Prior == Destination->Identifier)
                            {
                                Seen = true;
                                break;
                            }
                        }
                        if (!Seen)
                        {
                            Reachable.push_back(Destination->Identifier);
                        }
                    }
                }

                for (const NodeInstance& Node : Graph.GetNodes())
                {
                    const ExecutionRegion* Region = Node.ExecutionRegion.has_value()
                        ? Graph.FindExecutionRegion(*Node.ExecutionRegion) : nullptr;
                    const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                        ? Descriptors.Find(Node.Descriptor) : nullptr;
                    if (Region == nullptr || Region->Entry != Entry.Identifier ||
                        Descriptor == nullptr || !Descriptor->GetExecutionControlSchema().has_value() ||
                        IsInsideLoopBody(Graph, Node) || IsLoopControlNode(Node, Descriptors))
                    {
                        continue;
                    }
                    bool IsReachable = false;
                    for (const NodeInstanceId ReachableNode : Reachable)
                    {
                        IsReachable = IsReachable || ReachableNode == Node.Identifier;
                    }
                    if (!IsReachable)
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                            "A structured execution node is unreachable from its explicit Entry root.");
                    }
                }

                ValidateBranchOutcomes(Graph, Descriptors, Entry.Identifier, Diagnostics);
                ValidateStructuredCycles(Graph, Descriptors, Entry.Identifier, Reachable, Diagnostics);
            }

            ValidateReturnTerminalTopology(Graph, Descriptors, Diagnostics);
            if (Diagnostics.size() == InitialDiagnosticCount && !ContainsError(Diagnostics))
            {
                ValidateExecutionDataDominance(Graph, Descriptors, Diagnostics);
            }
            ValidateLoopExecution(Graph, Descriptors, Diagnostics);
        }

        static void ValidateBranchOutcomes(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            ExecutionEntryId Entry,
            DiagnosticCollection& Diagnostics
        )
        {
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                const ExecutionRegion* ParentRegion = Node.ExecutionRegion.has_value()
                    ? Graph.FindExecutionRegion(*Node.ExecutionRegion) : nullptr;
                const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                    ? Descriptors.Find(Node.Descriptor) : nullptr;
                const BranchControlSchema* Branch = GetControlSchema<BranchControlSchema>(Descriptor);
                if (ParentRegion == nullptr || ParentRegion->Entry != Entry || Branch == nullptr)
                {
                    continue;
                }
                const ExecutionRegion* TrueRegion = FindBranchArmRegion(
                    Graph, Node.Identifier, Branch->TrueOutput);
                const ExecutionRegion* FalseRegion = FindBranchArmRegion(
                    Graph, Node.Identifier, Branch->FalseOutput);
                if (TrueRegion == nullptr || FalseRegion == nullptr ||
                    !TrueRegion->Parent.has_value() || !FalseRegion->Parent.has_value())
                {
                    continue;
                }
                std::vector<const ControlEdge*> TrueExits;
                std::vector<const ControlEdge*> FalseExits;
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    if (IsLoopTransferEdge(Graph, Descriptors, Edge))
                    {
                        continue;
                    }
                    const NodeInstance* EdgeSource = Graph.FindNode(Edge.SourceNode);
                    if (EdgeSource == nullptr || IsReturnNode(*EdgeSource, Descriptors))
                    {
                        continue;
                    }
                    const ExecutionRegion* SourceRegion = EffectiveSourceRegion(Graph, Descriptors, Edge);
                    const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                    if (SourceRegion == nullptr || Destination == nullptr ||
                        !Destination->ExecutionRegion.has_value())
                    {
                        continue;
                    }
                    const bool ExitsToParent = *Destination->ExecutionRegion == *TrueRegion->Parent;
                    if (!ExitsToParent)
                    {
                        continue;
                    }
                    if (SourceRegion->Identifier == TrueRegion->Identifier)
                    {
                        TrueExits.push_back(&Edge);
                    }
                    else if (SourceRegion->Identifier == FalseRegion->Identifier)
                    {
                        FalseExits.push_back(&Edge);
                    }
                }
                if (TrueExits.size() > 1U || FalseExits.size() > 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "Each BranchArm may have at most one live parent-region outcome.");
                    continue;
                }
                const std::size_t LiveCount = TrueExits.size() + FalseExits.size();
                if (LiveCount == 1U)
                {
                    const ControlEdge* Exit = TrueExits.empty()
                        ? FalseExits.front() : TrueExits.front();
                    const NodeInstance* Destination = Graph.FindNode(Exit->DestinationNode);
                    const NodeDescriptor* DestinationDescriptor = Destination == nullptr
                        ? nullptr : Descriptors.Find(Destination->Descriptor);
                    if (GetControlSchema<SequenceControlSchema>(DestinationDescriptor) == nullptr)
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                            "A single live BranchArm must continue through a parent-region Sequence.");
                    }
                }
                else if (LiveCount == 2U)
                {
                    const NodeInstance* TrueDestination = Graph.FindNode(TrueExits.front()->DestinationNode);
                    const NodeInstance* FalseDestination = Graph.FindNode(FalseExits.front()->DestinationNode);
                    const NodeDescriptor* JoinDescriptor = TrueDestination == nullptr
                        ? nullptr : Descriptors.Find(TrueDestination->Descriptor);
                    if (TrueDestination == nullptr || FalseDestination == nullptr ||
                        TrueDestination->Identifier != FalseDestination->Identifier ||
                        TrueDestination->ExecutionRegion != ParentRegion->Identifier ||
                        GetControlSchema<JoinControlSchema>(JoinDescriptor) == nullptr)
                    {
                        Add(Diagnostics, DiagnosticCode::MissingExplicitJoin,
                            "Two live BranchArms must reconverge at one explicit parent-region Join.");
                    }
                }
            }

            for (const NodeInstance& JoinNode : Graph.GetNodes())
            {
                const NodeDescriptor* Descriptor = JoinNode.Descriptor.IsValid()
                    ? Descriptors.Find(JoinNode.Descriptor) : nullptr;
                const JoinControlSchema* Join = GetControlSchema<JoinControlSchema>(Descriptor);
                if (Join == nullptr || !JoinNode.ExecutionRegion.has_value())
                {
                    continue;
                }
                const ExecutionRegion* JoinRegion = Graph.FindExecutionRegion(*JoinNode.ExecutionRegion);
                if (JoinRegion == nullptr || JoinRegion->Entry != Entry)
                {
                    continue;
                }
                std::vector<const ControlEdge*> Incoming;
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    if (Edge.DestinationNode == JoinNode.Identifier &&
                        Edge.DestinationInputPin == Join->ExecutionInput)
                    {
                        Incoming.push_back(&Edge);
                    }
                }
                bool ValidPair = Incoming.size() == 2U;
                const ExecutionRegion* FirstRegion = nullptr;
                const ExecutionRegion* SecondRegion = nullptr;
                if (ValidPair)
                {
                    FirstRegion = EffectiveSourceRegion(Graph, Descriptors, *Incoming[0U]);
                    SecondRegion = EffectiveSourceRegion(Graph, Descriptors, *Incoming[1U]);
                    ValidPair = FirstRegion != nullptr && SecondRegion != nullptr &&
                        FirstRegion->Kind == ExecutionRegionKind::BranchArm &&
                        SecondRegion->Kind == ExecutionRegionKind::BranchArm &&
                        FirstRegion->Identifier != SecondRegion->Identifier &&
                        FirstRegion->Parent == JoinNode.ExecutionRegion &&
                        SecondRegion->Parent == JoinNode.ExecutionRegion &&
                        FirstRegion->OwnerNode == SecondRegion->OwnerNode;
                    const NodeInstance* Owner = ValidPair && FirstRegion->OwnerNode.has_value()
                        ? Graph.FindNode(*FirstRegion->OwnerNode) : nullptr;
                    const NodeDescriptor* OwnerDescriptor = Owner == nullptr
                        ? nullptr : Descriptors.Find(Owner->Descriptor);
                    const BranchControlSchema* Branch =
                        GetControlSchema<BranchControlSchema>(OwnerDescriptor);
                    ValidPair = ValidPair && Branch != nullptr &&
                        ((FirstRegion->OwnerOutputPin == Branch->TrueOutput &&
                            SecondRegion->OwnerOutputPin == Branch->FalseOutput) ||
                         (FirstRegion->OwnerOutputPin == Branch->FalseOutput &&
                            SecondRegion->OwnerOutputPin == Branch->TrueOutput));
                }
                if (!ValidPair)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidControlEdge,
                        "A structured Join must have exactly one incoming tail from each arm of one binary Branch.");
                }
            }
        }

        static void ValidateStructuredCycles(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            ExecutionEntryId Entry,
            const std::vector<NodeInstanceId>& Reachable,
            DiagnosticCollection& Diagnostics
        )
        {
            std::vector<NodeInstanceId> Ordered;
            for (const NodeInstanceId NodeId : Reachable)
            {
                const NodeInstance* Node = Graph.FindNode(NodeId);
                if (Node != nullptr && !IsInsideLoopBody(Graph, *Node))
                {
                    Ordered.push_back(NodeId);
                }
            }
            std::vector<NodeInstanceId> Removed;
            bool Progress = true;
            while (Progress)
            {
                Progress = false;
                for (const NodeInstanceId NodeId : Ordered)
                {
                    bool AlreadyRemoved = false;
                    for (const NodeInstanceId RemovedId : Removed)
                    {
                        AlreadyRemoved = AlreadyRemoved || RemovedId == NodeId;
                    }
                    if (AlreadyRemoved)
                    {
                        continue;
                    }
                    bool HasRemainingSuccessor = false;
                    for (const ControlEdge& Edge : Graph.GetControlEdges())
                    {
                        if (Edge.SourceNode != NodeId || IsLoopTransferEdge(Graph, Descriptors, Edge))
                        {
                            continue;
                        }
                        const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
                        const PinSchema* SourcePin = FindPin(
                            Source, Edge.SourceOutputPin, Descriptors);
                        const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                        const PinSchema* DestinationPin = FindPin(
                            Destination, Edge.DestinationInputPin, Descriptors);
                        const ExecutionRegion* Region = Destination != nullptr &&
                            Destination->ExecutionRegion.has_value()
                            ? Graph.FindExecutionRegion(*Destination->ExecutionRegion) : nullptr;
                        if (SourcePin != nullptr &&
                            SourcePin->GetDirection() == PinDirection::Output &&
                            SourcePin->GetCategory() == PinCategory::Execution &&
                            SourcePin->GetType() == TypeDesc::Flow() &&
                            DestinationPin != nullptr &&
                            DestinationPin->GetDirection() == PinDirection::Input &&
                            DestinationPin->GetCategory() == PinCategory::Execution &&
                            DestinationPin->GetType() == TypeDesc::Flow() &&
                            Destination != nullptr && Region != nullptr && Region->Entry == Entry &&
                            !IsInsideLoopBody(Graph, *Destination))
                        {
                            bool DestinationRemoved = false;
                            for (const NodeInstanceId RemovedId : Removed)
                            {
                                DestinationRemoved = DestinationRemoved || RemovedId == Destination->Identifier;
                            }
                            HasRemainingSuccessor = HasRemainingSuccessor || !DestinationRemoved;
                        }
                    }
                    if (!HasRemainingSuccessor)
                    {
                        Removed.push_back(NodeId);
                        Progress = true;
                    }
                }
            }
            if (Removed.size() != Ordered.size())
            {
                Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                    "A structured execution cycle outside LoopBody is not authorized in M4.2.");
            }
        }

        static bool IsRegionWithin(
            const GraphIR& Graph,
            ExecutionRegionId RegionIdentifier,
            ExecutionRegionId AncestorIdentifier
        )
        {
            const ExecutionRegion* Region = Graph.FindExecutionRegion(RegionIdentifier);
            std::vector<ExecutionRegionId> Visited;
            while (Region != nullptr)
            {
                if (Region->Identifier == AncestorIdentifier)
                {
                    return true;
                }
                if (std::find(Visited.begin(), Visited.end(), Region->Identifier) != Visited.end())
                {
                    return false;
                }
                Visited.push_back(Region->Identifier);
                if (!Region->Parent.has_value())
                {
                    return false;
                }
                Region = Graph.FindExecutionRegion(*Region->Parent);
            }
            return false;
        }

        static const ExecutionRegion* FindNearestLoopBodyRegion(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ExecutionRegion* Start
        )
        {
            const ExecutionRegion* Region = Start;
            std::vector<ExecutionRegionId> Visited;
            while (Region != nullptr)
            {
                if (std::find(Visited.begin(), Visited.end(), Region->Identifier) != Visited.end())
                {
                    return nullptr;
                }
                Visited.push_back(Region->Identifier);
                if (Region->Kind == ExecutionRegionKind::LoopBody)
                {
                    if (!Region->OwnerNode.has_value() || !Region->OwnerOutputPin.has_value())
                    {
                        return nullptr;
                    }
                    const NodeInstance* Owner = Graph.FindNode(*Region->OwnerNode);
                    const NodeDescriptor* Descriptor = Owner == nullptr
                        ? nullptr : Descriptors.Find(Owner->Descriptor);
                    const LoopControlSchema* Loop = GetControlSchema<LoopControlSchema>(Descriptor);
                    if (Loop == nullptr || Loop->BodyOutput != *Region->OwnerOutputPin)
                    {
                        return nullptr;
                    }
                    return Region;
                }
                if (!Region->Parent.has_value())
                {
                    return nullptr;
                }
                Region = Graph.FindExecutionRegion(*Region->Parent);
            }
            return nullptr;
        }

        static const LoopControlSchema* FindLoopSchema(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            NodeInstanceId NodeIdentifier
        )
        {
            const NodeInstance* Node = Graph.FindNode(NodeIdentifier);
            const NodeDescriptor* Descriptor = Node == nullptr
                ? nullptr : Descriptors.Find(Node->Descriptor);
            return GetControlSchema<LoopControlSchema>(Descriptor);
        }

        static bool IsAuthorizedLoopTransfer(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ControlEdge& Edge
        )
        {
            const NodeInstance* Target = Graph.FindNode(Edge.DestinationNode);
            const NodeInstance* Source = Graph.FindNode(Edge.SourceNode);
            if (Target == nullptr || Source == nullptr ||
                !Source->ExecutionRegion.has_value() || !Target->ExecutionRegion.has_value())
            {
                return false;
            }
            const NodeDescriptor* SourceDescriptor = Descriptors.Find(Source->Descriptor);
            const PinSchema* SourcePin = FindPin(Source, Edge.SourceOutputPin, Descriptors);
            if (GetControlSchema<ReturnControlSchema>(SourceDescriptor) != nullptr ||
                SourcePin == nullptr || SourcePin->GetDirection() != PinDirection::Output ||
                SourcePin->GetCategory() != PinCategory::Execution ||
                SourcePin->GetType() != TypeDesc::Flow())
            {
                return false;
            }
            const LoopControlSchema* TargetSchema = FindLoopSchema(
                Graph, Descriptors, Target->Identifier);
            if (TargetSchema == nullptr ||
                (Edge.DestinationInputPin != TargetSchema->RepeatInput &&
                    Edge.DestinationInputPin != TargetSchema->BreakInput))
            {
                return false;
            }
            const ExecutionRegion* SourceRegion = EffectiveSourceRegion(Graph, Descriptors, Edge);
            const ExecutionRegion* NearestBody = FindNearestLoopBodyRegion(
                Graph, Descriptors, SourceRegion);
            const ExecutionRegion* TargetBody = FindLoopBodyRegion(
                Graph, Target->Identifier, TargetSchema->BodyOutput);
            return SourceRegion != nullptr && NearestBody != nullptr && TargetBody != nullptr &&
                NearestBody->Identifier == TargetBody->Identifier &&
                SourceRegion->Entry == TargetBody->Entry &&
                Target->ExecutionRegion == TargetBody->Parent;
        }

        static void AddReachableNode(
            std::vector<NodeInstanceId>& Reachable,
            NodeInstanceId Node
        )
        {
            if (std::find(Reachable.begin(), Reachable.end(), Node) == Reachable.end())
            {
                Reachable.push_back(Node);
            }
        }

        static bool ContainsNode(
            const std::vector<NodeInstanceId>& Nodes,
            NodeInstanceId Node
        )
        {
            return std::find(Nodes.begin(), Nodes.end(), Node) != Nodes.end();
        }

        static bool ContainsEntryLoop(
            const std::vector<NodeInstanceId>& Loops,
            NodeInstanceId Loop
        )
        {
            return ContainsNode(Loops, Loop);
        }

        static std::vector<NodeInstanceId> FindStructuredReachability(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ExecutionEntry& Entry,
            std::vector<NodeInstanceId>& ReachableBreakLoops
        )
        {
            std::vector<NodeInstanceId> Reachable;
            Reachable.push_back(Entry.RootNode);
            for (std::size_t Cursor = 0U; Cursor < Reachable.size(); ++Cursor)
            {
                const NodeInstanceId Current = Reachable[Cursor];
                const NodeInstance* CurrentNode = Graph.FindNode(Current);
                if (CurrentNode == nullptr || IsReturnNode(*CurrentNode, Descriptors))
                {
                    continue;
                }
                const LoopControlSchema* CurrentLoop = FindLoopSchema(
                    Graph, Descriptors, Current);
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    if (Edge.SourceNode != Current)
                    {
                        continue;
                    }
                    const PinSchema* SourcePin = FindPin(
                        CurrentNode, Edge.SourceOutputPin, Descriptors);
                    const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                    const PinSchema* DestinationPin = FindPin(
                        Destination, Edge.DestinationInputPin, Descriptors);
                    if (SourcePin == nullptr || SourcePin->GetDirection() != PinDirection::Output ||
                        SourcePin->GetCategory() != PinCategory::Execution ||
                        SourcePin->GetType() != TypeDesc::Flow() ||
                        DestinationPin == nullptr ||
                        DestinationPin->GetDirection() != PinDirection::Input ||
                        DestinationPin->GetCategory() != PinCategory::Execution ||
                        DestinationPin->GetType() != TypeDesc::Flow())
                    {
                        continue;
                    }
                    const LoopControlSchema* DestinationLoop = Destination == nullptr
                        ? nullptr : FindLoopSchema(Graph, Descriptors, Destination->Identifier);
                    if (DestinationLoop != nullptr &&
                        Edge.DestinationInputPin == DestinationLoop->RepeatInput)
                    {
                        // The loop controller is already reachable through its execution input.
                        // Repeat is the sole authorized cycle-closing transfer.
                        continue;
                    }
                    if (DestinationLoop != nullptr &&
                        Edge.DestinationInputPin == DestinationLoop->BreakInput)
                    {
                        if (IsAuthorizedLoopTransfer(Graph, Descriptors, Edge))
                        {
                            AddReachableNode(ReachableBreakLoops, Destination->Identifier);
                            for (const ControlEdge& ExitEdge : Graph.GetControlEdges())
                            {
                                if (ExitEdge.SourceNode == Destination->Identifier &&
                                    ExitEdge.SourceOutputPin == DestinationLoop->ExitOutput)
                                {
                                    AddReachableNode(Reachable, ExitEdge.DestinationNode);
                                }
                            }
                        }
                        continue;
                    }
                    if (CurrentLoop != nullptr)
                    {
                        const NodeInstance* LoopNode = Graph.FindNode(Current);
                        const NodeDescriptor* LoopDescriptor = LoopNode == nullptr
                            ? nullptr : Descriptors.Find(LoopNode->Descriptor);
                        const LoopControlSchema* Loop = GetControlSchema<LoopControlSchema>(LoopDescriptor);
                        if (Loop != nullptr && Edge.SourceOutputPin == Loop->ExitOutput &&
                            Loop->ExitPolicy == LoopExitPolicy::Unconditional &&
                            !ContainsEntryLoop(ReachableBreakLoops, Current))
                        {
                            continue;
                        }
                        if (Loop != nullptr && Edge.SourceOutputPin != Loop->BodyOutput &&
                            Edge.SourceOutputPin != Loop->ExitOutput)
                        {
                            continue;
                        }
                    }
                    if (Destination != nullptr)
                    {
                        AddReachableNode(Reachable, Destination->Identifier);
                    }
                }
            }
            return Reachable;
        }

        static bool IsExecutionNode(
            const NodeInstance& Node,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                ? Descriptors.Find(Node.Descriptor) : nullptr;
            return Descriptor != nullptr && Descriptor->GetExecutionControlSchema().has_value();
        }

        static void ValidateLoopExecution(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics
        )
        {
            if (Graph.GetExecutionModel() != ExecutionModel::Structured)
            {
                return;
            }
            const std::size_t InitialDiagnosticCount = Diagnostics.size();
            for (const NodeInstance& LoopNode : Graph.GetNodes())
            {
                const LoopControlSchema* Loop = FindLoopSchema(
                    Graph, Descriptors, LoopNode.Identifier);
                if (Loop == nullptr)
                {
                    continue;
                }
                const ExecutionRegion* Body = FindLoopBodyRegion(
                    Graph, LoopNode.Identifier, Loop->BodyOutput);
                const ExecutionRegion* Parent = LoopNode.ExecutionRegion.has_value()
                    ? Graph.FindExecutionRegion(*LoopNode.ExecutionRegion) : nullptr;
                if (Body == nullptr || Parent == nullptr || Body->Parent != Parent->Identifier ||
                    Body->Entry != Parent->Entry)
                {
                    continue;
                }
                if (CountIncomingEndpoint(Graph, LoopNode.Identifier, Loop->ExecutionInput) != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "A Loop controller must have exactly one explicit execution predecessor.");
                }
                std::vector<const ControlEdge*> BodyEdges;
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    if (Edge.SourceNode == LoopNode.Identifier &&
                        Edge.SourceOutputPin == Loop->BodyOutput)
                    {
                        BodyEdges.push_back(&Edge);
                    }
                    if (Edge.SourceNode == LoopNode.Identifier &&
                        Edge.SourceOutputPin == Loop->ExitOutput)
                    {
                        const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                        if (Destination == nullptr || !Destination->ExecutionRegion.has_value() ||
                            *Destination->ExecutionRegion != Parent->Identifier)
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                                "A Loop Exit output may continue only in its parent execution region.");
                        }
                    }
                    if (Edge.DestinationNode == LoopNode.Identifier &&
                        Edge.DestinationInputPin == Loop->ExecutionInput)
                    {
                        const ExecutionRegion* SourceRegion = EffectiveSourceRegion(
                            Graph, Descriptors, Edge);
                        if (SourceRegion == nullptr || SourceRegion->Identifier != Parent->Identifier ||
                            SourceRegion->Entry != Parent->Entry)
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                                "A Loop ExecutionInput must be reached from its parent region.");
                        }
                    }
                }
                if (BodyEdges.size() != 1U)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                        "A LoopBody must begin with exactly one explicit Body output successor.");
                }
                else
                {
                    const ControlEdge& BodyAction = *BodyEdges.front();
                    const bool IsAuthorizedDirectTransfer =
                        BodyAction.DestinationNode == LoopNode.Identifier &&
                        (BodyAction.DestinationInputPin == Loop->BreakInput ||
                            BodyAction.DestinationInputPin == Loop->RepeatInput) &&
                        IsAuthorizedLoopTransfer(Graph, Descriptors, BodyAction);
                    const NodeInstance* BodyRoot = IsAuthorizedDirectTransfer
                        ? nullptr : Graph.FindNode(BodyAction.DestinationNode);
                    if (!IsAuthorizedDirectTransfer &&
                        (BodyRoot == nullptr || BodyRoot->ExecutionRegion != Body->Identifier ||
                            !IsExecutionNode(*BodyRoot, Descriptors)))
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionOwnership,
                            "A Loop Body output must enter an execution node in its exactly owned LoopBody or directly transfer to its own authorized Break or Repeat input.");
                    }
                }
                if (Loop->ExitPolicy == LoopExitPolicy::Conditional)
                {
                    if (!Loop->ConditionInput.has_value())
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionControlRole,
                            "A Conditional Loop requires its declared Boolean condition input.");
                    }
                    else
                    {
                        const PinSchema& ConditionPin =
                            Descriptors.Find(LoopNode.Descriptor)->GetPins()[Loop->ConditionInput->GetValue()];
                        if (Graph.GetInputBinding(LoopNode.Identifier, *Loop->ConditionInput) == nullptr &&
                            !ConditionPin.GetDefaultValue().has_value())
                        {
                            Add(Diagnostics, DiagnosticCode::InvalidInputBinding,
                                "A Conditional Loop requires a condition binding or descriptor default.");
                        }
                    }
                }
                else if (Loop->ConditionInput.has_value())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidExecutionControlRole,
                        "An Unconditional Loop cannot declare a condition input.");
                }
            }

            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                if (Destination == nullptr)
                {
                    continue;
                }
                const LoopControlSchema* DestinationLoop = FindLoopSchema(
                    Graph, Descriptors, Destination->Identifier);
                if (DestinationLoop != nullptr &&
                    (Edge.DestinationInputPin == DestinationLoop->RepeatInput ||
                        Edge.DestinationInputPin == DestinationLoop->BreakInput) &&
                    !IsAuthorizedLoopTransfer(Graph, Descriptors, Edge))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidLoopTransfer,
                        "Repeat and Break transfers must target the nearest owning LoopBody controller.");
                }
            }

            std::vector<NodeInstanceId> ReachableBreakLoops;
            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                if (!Entry.Identifier.IsValid() || !Entry.RootNode.IsValid())
                {
                    continue;
                }
                const NodeInstance* Root = Graph.FindNode(Entry.RootNode);
                if (Root == nullptr || CountEntries(Graph, Entry.Identifier) != 1U)
                {
                    continue;
                }
                std::vector<NodeInstanceId> EntryBreakLoops;
                const std::vector<NodeInstanceId> Reachable = FindStructuredReachability(
                    Graph, Descriptors, Entry, EntryBreakLoops);
                for (const NodeInstanceId BreakLoop : EntryBreakLoops)
                {
                    AddReachableNode(ReachableBreakLoops, BreakLoop);
                }
                for (const NodeInstance& Node : Graph.GetNodes())
                {
                    if (!IsExecutionNode(Node, Descriptors) || !Node.ExecutionRegion.has_value())
                    {
                        continue;
                    }
                    const ExecutionRegion* Region = Graph.FindExecutionRegion(*Node.ExecutionRegion);
                    if (Region == nullptr || Region->Entry != Entry.Identifier)
                    {
                        continue;
                    }
                    if (!ContainsNode(Reachable, Node.Identifier))
                    {
                        Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                            "A structured execution node, including LoopBody nodes, must be reachable from its Entry root.");
                    }
                }
                ValidateAuthorizedLoopCycles(Graph, Descriptors, Entry, Reachable,
                    ReachableBreakLoops, Diagnostics);
            }

            for (const NodeInstance& LoopNode : Graph.GetNodes())
            {
                const LoopControlSchema* Loop = FindLoopSchema(
                    Graph, Descriptors, LoopNode.Identifier);
                if (Loop == nullptr || Loop->ExitPolicy != LoopExitPolicy::Unconditional)
                {
                    continue;
                }
                bool HasExitEdge = false;
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    HasExitEdge = HasExitEdge || (Edge.SourceNode == LoopNode.Identifier &&
                        Edge.SourceOutputPin == Loop->ExitOutput);
                }
                if (HasExitEdge && !ContainsEntryLoop(ReachableBreakLoops, LoopNode.Identifier))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidLoopTransfer,
                        "An Unconditional Loop Exit output is reachable only when a valid Break path exists.");
                }
            }

            if (Diagnostics.size() == InitialDiagnosticCount && !ContainsError(Diagnostics))
            {
                ValidateLoopAwareDataDominance(Graph, Descriptors, Diagnostics,
                    ReachableBreakLoops);
            }
        }

        static void ValidateAuthorizedLoopCycles(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const ExecutionEntry& Entry,
            const std::vector<NodeInstanceId>& Reachable,
            const std::vector<NodeInstanceId>& ReachableBreakLoops,
            DiagnosticCollection& Diagnostics
        )
        {
            std::vector<NodeInstanceId> Nodes;
            for (const NodeInstanceId Node : Reachable)
            {
                const NodeInstance* Instance = Graph.FindNode(Node);
                if (Instance != nullptr && IsExecutionNode(*Instance, Descriptors) &&
                    Instance->ExecutionRegion.has_value())
                {
                    const ExecutionRegion* Region = Graph.FindExecutionRegion(*Instance->ExecutionRegion);
                    if (Region != nullptr && Region->Entry == Entry.Identifier)
                    {
                        Nodes.push_back(Node);
                    }
                }
            }
            std::vector<std::size_t> InDegree(Nodes.size(), 0U);
            std::vector<std::vector<std::size_t>> Successors(Nodes.size());
            for (const ControlEdge& Edge : Graph.GetControlEdges())
            {
                std::size_t SourceIndex = Nodes.size();
                std::size_t DestinationIndex = Nodes.size();
                for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                {
                    if (Nodes[Index] == Edge.SourceNode)
                    {
                        SourceIndex = Index;
                    }
                    if (Nodes[Index] == Edge.DestinationNode)
                    {
                        DestinationIndex = Index;
                    }
                }
                if (SourceIndex == Nodes.size())
                {
                    continue;
                }
                const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                const LoopControlSchema* DestinationLoop = Destination == nullptr
                    ? nullptr : FindLoopSchema(Graph, Descriptors, Destination->Identifier);
                if (DestinationLoop != nullptr &&
                    Edge.DestinationInputPin == DestinationLoop->RepeatInput)
                {
                    // Repeat is the only removed cycle-closing persisted edge.
                    continue;
                }
                if (DestinationLoop != nullptr &&
                    Edge.DestinationInputPin == DestinationLoop->BreakInput)
                {
                    if (!IsAuthorizedLoopTransfer(Graph, Descriptors, Edge))
                    {
                        continue;
                    }
                    for (const ControlEdge& ExitEdge : Graph.GetControlEdges())
                    {
                        if (ExitEdge.SourceNode != Destination->Identifier ||
                            ExitEdge.SourceOutputPin != DestinationLoop->ExitOutput)
                        {
                            continue;
                        }
                        for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                        {
                            if (Nodes[Index] == ExitEdge.DestinationNode)
                            {
                                Successors[SourceIndex].push_back(Index);
                                ++InDegree[Index];
                            }
                        }
                    }
                    continue;
                }
                if (DestinationIndex == Nodes.size())
                {
                    continue;
                }
                const LoopControlSchema* SourceLoop = FindLoopSchema(
                    Graph, Descriptors, Edge.SourceNode);
                if (SourceLoop != nullptr && Edge.SourceOutputPin == SourceLoop->ExitOutput &&
                    SourceLoop->ExitPolicy == LoopExitPolicy::Unconditional &&
                    !ContainsEntryLoop(ReachableBreakLoops, Edge.SourceNode))
                {
                    continue;
                }
                Successors[SourceIndex].push_back(DestinationIndex);
                ++InDegree[DestinationIndex];
            }
            std::vector<std::size_t> Ready;
            for (std::size_t Index = 0U; Index < InDegree.size(); ++Index)
            {
                if (InDegree[Index] == 0U)
                {
                    Ready.push_back(Index);
                }
            }
            std::size_t RemovedCount = 0U;
            for (std::size_t Cursor = 0U; Cursor < Ready.size(); ++Cursor)
            {
                const std::size_t Current = Ready[Cursor];
                ++RemovedCount;
                for (const std::size_t Successor : Successors[Current])
                {
                    if (--InDegree[Successor] == 0U)
                    {
                        Ready.push_back(Successor);
                    }
                }
            }
            if (RemovedCount != Nodes.size())
            {
                Add(Diagnostics, DiagnosticCode::InvalidExecutionReachability,
                    "After authorized Repeat transfers are removed, structured execution must be acyclic.");
            }
        }

        static void ValidateLoopAwareDataDominance(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics,
            const std::vector<NodeInstanceId>& ReachableBreakLoops
        )
        {
            struct ProvenanceRecord
            {
                NodeInstanceId Node;
                std::vector<NodeInstanceId> Origins;
                bool Unknown = false;
            };
            std::vector<ProvenanceRecord> Provenance;
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                if (IsExecutionNode(Node, Descriptors))
                {
                    continue;
                }
                Provenance.push_back(ProvenanceRecord{Node.Identifier, {}, false});
            }
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                if (Node.ExecutionRegion.has_value() && IsExecutionNode(Node, Descriptors))
                {
                    Provenance.push_back(ProvenanceRecord{Node.Identifier, {Node.Identifier}, false});
                }
            }
            bool ProvenanceChanged = true;
            while (ProvenanceChanged)
            {
                ProvenanceChanged = false;
                for (const InputBindingRecord& Binding : Graph.GetInputBindings())
                {
                    const OutputReference* Output = std::get_if<OutputReference>(&Binding.Binding);
                    if (Output == nullptr)
                    {
                        continue;
                    }
                    ProvenanceRecord* DestinationRecord = nullptr;
                    const ProvenanceRecord* SourceRecord = nullptr;
                    for (ProvenanceRecord& Record : Provenance)
                    {
                        if (Record.Node == Binding.DestinationNode)
                        {
                            DestinationRecord = &Record;
                        }
                        if (Record.Node == Output->SourceNode)
                        {
                            SourceRecord = &Record;
                        }
                    }
                    if (DestinationRecord == nullptr || SourceRecord == nullptr)
                    {
                        continue;
                    }
                    if (SourceRecord->Unknown && !DestinationRecord->Unknown)
                    {
                        DestinationRecord->Unknown = true;
                        ProvenanceChanged = true;
                    }
                    for (const NodeInstanceId Origin : SourceRecord->Origins)
                    {
                        if (std::find(DestinationRecord->Origins.begin(),
                                DestinationRecord->Origins.end(), Origin) ==
                            DestinationRecord->Origins.end())
                        {
                            DestinationRecord->Origins.push_back(Origin);
                            ProvenanceChanged = true;
                        }
                    }
                }
            }

            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                const NodeInstance* Root = Graph.FindNode(Entry.RootNode);
                if (Root == nullptr || CountEntries(Graph, Entry.Identifier) != 1U)
                {
                    continue;
                }
                std::vector<NodeInstanceId> BreakLoops;
                const std::vector<NodeInstanceId> Reachable = FindStructuredReachability(
                    Graph, Descriptors, Entry, BreakLoops);
                for (const NodeInstanceId LoopNode : ReachableBreakLoops)
                {
                    if (FindLoopSchema(Graph, Descriptors, LoopNode) != nullptr)
                    {
                        AddReachableNode(BreakLoops, LoopNode);
                    }
                }
                std::vector<NodeInstanceId> Nodes;
                for (const NodeInstanceId Node : Reachable)
                {
                    const NodeInstance* Instance = Graph.FindNode(Node);
                    if (Instance != nullptr && IsExecutionNode(*Instance, Descriptors) &&
                        Instance->ExecutionRegion.has_value())
                    {
                        const ExecutionRegion* Region = Graph.FindExecutionRegion(*Instance->ExecutionRegion);
                        if (Region != nullptr && Region->Entry == Entry.Identifier)
                        {
                            Nodes.push_back(Node);
                        }
                    }
                }
                std::vector<std::pair<std::size_t, std::size_t>> Edges;
                for (const ControlEdge& Edge : Graph.GetControlEdges())
                {
                    std::size_t SourceIndex = Nodes.size();
                    std::size_t DestinationIndex = Nodes.size();
                    for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                    {
                        if (Nodes[Index] == Edge.SourceNode)
                        {
                            SourceIndex = Index;
                        }
                        if (Nodes[Index] == Edge.DestinationNode)
                        {
                            DestinationIndex = Index;
                        }
                    }
                    if (SourceIndex == Nodes.size())
                    {
                        continue;
                    }
                    const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                    const LoopControlSchema* DestinationLoop = Destination == nullptr
                        ? nullptr : FindLoopSchema(Graph, Descriptors, Destination->Identifier);
                    if (DestinationLoop != nullptr &&
                        Edge.DestinationInputPin == DestinationLoop->RepeatInput)
                    {
                        continue;
                    }
                    if (DestinationLoop != nullptr &&
                        Edge.DestinationInputPin == DestinationLoop->BreakInput)
                    {
                        if (!IsAuthorizedLoopTransfer(Graph, Descriptors, Edge))
                        {
                            continue;
                        }
                        for (const ControlEdge& ExitEdge : Graph.GetControlEdges())
                        {
                            if (ExitEdge.SourceNode != Destination->Identifier ||
                                ExitEdge.SourceOutputPin != DestinationLoop->ExitOutput)
                            {
                                continue;
                            }
                            for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                            {
                                if (Nodes[Index] == ExitEdge.DestinationNode)
                                {
                                    Edges.emplace_back(SourceIndex, Index);
                                }
                            }
                        }
                        continue;
                    }
                    if (DestinationIndex == Nodes.size())
                    {
                        continue;
                    }
                    const LoopControlSchema* SourceLoop = FindLoopSchema(
                        Graph, Descriptors, Edge.SourceNode);
                    if (SourceLoop != nullptr && Edge.SourceOutputPin == SourceLoop->ExitOutput &&
                        SourceLoop->ExitPolicy == LoopExitPolicy::Unconditional &&
                        !ContainsEntryLoop(BreakLoops, Edge.SourceNode))
                    {
                        continue;
                    }
                    Edges.emplace_back(SourceIndex, DestinationIndex);
                }
                if (Nodes.empty())
                {
                    continue;
                }
                std::size_t RootIndex = Nodes.size();
                for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                {
                    if (Nodes[Index] == Entry.RootNode)
                    {
                        RootIndex = Index;
                        break;
                    }
                }
                if (RootIndex == Nodes.size())
                {
                    continue;
                }
                std::vector<std::vector<bool>> Dominators(
                    Nodes.size(), std::vector<bool>(Nodes.size(), true));
                for (std::size_t Candidate = 0U; Candidate < Nodes.size(); ++Candidate)
                {
                    Dominators[RootIndex][Candidate] = Candidate == RootIndex;
                }
                bool Changed = true;
                while (Changed)
                {
                    Changed = false;
                    for (std::size_t NodeIndex = 0U; NodeIndex < Nodes.size(); ++NodeIndex)
                    {
                        if (NodeIndex == RootIndex)
                        {
                            continue;
                        }
                        std::vector<std::size_t> Predecessors;
                        for (const auto& [Source, Destination] : Edges)
                        {
                            if (Destination == NodeIndex)
                            {
                                Predecessors.push_back(Source);
                            }
                        }
                        std::vector<bool> Next(Nodes.size(), false);
                        Next[NodeIndex] = true;
                        if (!Predecessors.empty())
                        {
                            for (std::size_t Candidate = 0U; Candidate < Nodes.size(); ++Candidate)
                            {
                                bool InAll = true;
                                for (const std::size_t Predecessor : Predecessors)
                                {
                                    InAll = InAll && Dominators[Predecessor][Candidate];
                                }
                                Next[Candidate] = Next[Candidate] || InAll;
                            }
                        }
                        if (Next != Dominators[NodeIndex])
                        {
                            Dominators[NodeIndex] = std::move(Next);
                            Changed = true;
                        }
                    }
                }

                for (const InputBindingRecord& Binding : Graph.GetInputBindings())
                {
                    const OutputReference* Output = std::get_if<OutputReference>(&Binding.Binding);
                    const NodeInstance* Destination = Graph.FindNode(Binding.DestinationNode);
                    if (Output == nullptr || Destination == nullptr ||
                        !IsExecutionNode(*Destination, Descriptors) ||
                        !Destination->ExecutionRegion.has_value())
                    {
                        continue;
                    }
                    const ExecutionRegion* DestinationRegion =
                        Graph.FindExecutionRegion(*Destination->ExecutionRegion);
                    if (DestinationRegion == nullptr || DestinationRegion->Entry != Entry.Identifier)
                    {
                        continue;
                    }
                    const ProvenanceRecord* SourceRecord = nullptr;
                    for (const ProvenanceRecord& Record : Provenance)
                    {
                        if (Record.Node == Output->SourceNode)
                        {
                            SourceRecord = &Record;
                            break;
                        }
                    }
                    if (SourceRecord == nullptr || SourceRecord->Unknown)
                    {
                        continue;
                    }
                    std::size_t ConsumerIndex = Nodes.size();
                    for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                    {
                        if (Nodes[Index] == Destination->Identifier)
                        {
                            ConsumerIndex = Index;
                            break;
                        }
                    }
                    if (ConsumerIndex == Nodes.size())
                    {
                        continue;
                    }
                    for (const NodeInstanceId Origin : SourceRecord->Origins)
                    {
                        const NodeInstance* Producer = Graph.FindNode(Origin);
                        if (Producer == nullptr || !Producer->ExecutionRegion.has_value())
                        {
                            continue;
                        }
                        const ExecutionRegion* ProducerRegion =
                            Graph.FindExecutionRegion(*Producer->ExecutionRegion);
                        std::size_t ProducerIndex = Nodes.size();
                        for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                        {
                            if (Nodes[Index] == Origin)
                            {
                                ProducerIndex = Index;
                                break;
                            }
                        }
                        bool EscapesLoopBody = false;
                        const ExecutionRegion* Region = ProducerRegion;
                        std::vector<ExecutionRegionId> VisitedRegions;
                        while (Region != nullptr &&
                            std::find(VisitedRegions.begin(), VisitedRegions.end(), Region->Identifier) ==
                                VisitedRegions.end())
                        {
                            VisitedRegions.push_back(Region->Identifier);
                            if (Region->Kind == ExecutionRegionKind::LoopBody &&
                                !IsRegionWithin(Graph, *Destination->ExecutionRegion, Region->Identifier))
                            {
                                EscapesLoopBody = true;
                                break;
                            }
                            if (!Region->Parent.has_value())
                            {
                                break;
                            }
                            Region = Graph.FindExecutionRegion(*Region->Parent);
                        }
                        if (ProducerRegion == nullptr || ProducerRegion->Entry != Entry.Identifier ||
                            ProducerIndex == Nodes.size() ||
                            !Dominators[ConsumerIndex][ProducerIndex] || EscapesLoopBody)
                        {
                            Add(Diagnostics, DiagnosticCode::ExecutionDataNotDominated,
                                "A structured data origin must dominate its use in the same entry and may not escape its LoopBody.");
                            break;
                        }
                    }
                }
            }
        }

        static void ValidateExecutionDataDominance(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            DiagnosticCollection& Diagnostics
        )
        {
            struct ProvenanceRecord
            {
                NodeInstanceId Node;
                bool Unknown = false;
                std::vector<NodeInstanceId> Origins;
            };

            std::vector<ProvenanceRecord> Provenance;
            Provenance.reserve(Graph.GetNodes().size());
            for (const NodeInstance& Node : Graph.GetNodes())
            {
                const NodeDescriptor* Descriptor = Node.Descriptor.IsValid()
                    ? Descriptors.Find(Node.Descriptor) : nullptr;
                const bool IsExecutionOwned = Descriptor != nullptr &&
                    Descriptor->GetExecutionControlSchema().has_value() &&
                    Node.ExecutionRegion.has_value();
                ProvenanceRecord Record{Node.Identifier};
                if (IsExecutionOwned)
                {
                    Record.Origins.push_back(Node.Identifier);
                }
                Provenance.push_back(std::move(Record));
            }

            bool Changed = true;
            std::size_t Iterations = 0U;
            const std::size_t IterationLimit = Graph.GetNodes().size() + 1U;
            while (Changed && Iterations <= IterationLimit)
            {
                Changed = false;
                ++Iterations;
                for (const InputBindingRecord& Binding : Graph.GetInputBindings())
                {
                    const NodeInstance* Destination = Graph.FindNode(Binding.DestinationNode);
                    const NodeDescriptor* DestinationDescriptor = Destination == nullptr
                        ? nullptr : Descriptors.Find(Destination->Descriptor);
                    if (Destination == nullptr || Destination->ExecutionRegion.has_value() ||
                        (DestinationDescriptor != nullptr &&
                            DestinationDescriptor->GetExecutionControlSchema().has_value()))
                    {
                        continue;
                    }
                    const OutputReference* Output = std::get_if<OutputReference>(&Binding.Binding);
                    if (Output == nullptr)
                    {
                        continue;
                    }
                    const NodeInstance* Source = Graph.FindNode(Output->SourceNode);
                    if (Source == nullptr)
                    {
                        continue;
                    }
                    ProvenanceRecord* DestinationRecord = nullptr;
                    ProvenanceRecord* SourceRecord = nullptr;
                    for (ProvenanceRecord& Record : Provenance)
                    {
                        if (Record.Node == Destination->Identifier)
                        {
                            DestinationRecord = &Record;
                        }
                        if (Record.Node == Source->Identifier)
                        {
                            SourceRecord = &Record;
                        }
                    }
                    if (DestinationRecord == nullptr || SourceRecord == nullptr)
                    {
                        continue;
                    }
                    if (SourceRecord->Unknown && !DestinationRecord->Unknown)
                    {
                        DestinationRecord->Unknown = true;
                        Changed = true;
                    }
                    for (const NodeInstanceId Origin : SourceRecord->Origins)
                    {
                        bool Exists = false;
                        for (const NodeInstanceId Prior : DestinationRecord->Origins)
                        {
                            Exists = Exists || Prior == Origin;
                        }
                        if (!Exists)
                        {
                            DestinationRecord->Origins.push_back(Origin);
                            Changed = true;
                        }
                    }
                }
            }

            for (const ExecutionEntry& Entry : Graph.GetExecutionEntries())
            {
                const NodeInstance* Root = Graph.FindNode(Entry.RootNode);
                if (Root == nullptr || !Root->ExecutionRegion.has_value())
                {
                    continue;
                }
                std::vector<NodeInstanceId> Nodes;
                Nodes.push_back(Root->Identifier);
                for (std::size_t Cursor = 0U; Cursor < Nodes.size(); ++Cursor)
                {
                    const NodeInstanceId Current = Nodes[Cursor];
                    const NodeInstance* CurrentNode = Graph.FindNode(Current);
                    if (CurrentNode == nullptr || IsReturnNode(*CurrentNode, Descriptors))
                    {
                        continue;
                    }
                    for (const ControlEdge& Edge : Graph.GetControlEdges())
                    {
                        if (Edge.SourceNode != Current ||
                            IsLoopTransferEdge(Graph, Descriptors, Edge))
                        {
                            continue;
                        }
                        const NodeInstance* Destination = Graph.FindNode(Edge.DestinationNode);
                        const ExecutionRegion* DestinationRegion = Destination != nullptr &&
                            Destination->ExecutionRegion.has_value()
                            ? Graph.FindExecutionRegion(*Destination->ExecutionRegion) : nullptr;
                        if (Destination == nullptr || DestinationRegion == nullptr ||
                            DestinationRegion->Entry != Entry.Identifier ||
                            IsInsideLoopBody(Graph, *Destination))
                        {
                            continue;
                        }
                        bool Seen = false;
                        for (const NodeInstanceId Prior : Nodes)
                        {
                            Seen = Seen || Prior == Destination->Identifier;
                        }
                        if (!Seen)
                        {
                            Nodes.push_back(Destination->Identifier);
                        }
                    }
                }

                if (Nodes.empty())
                {
                    continue;
                }
                const std::size_t RootIndex = 0U;
                std::vector<std::vector<bool>> Dominators(
                    Nodes.size(), std::vector<bool>(Nodes.size(), true));
                for (std::size_t Candidate = 0U; Candidate < Nodes.size(); ++Candidate)
                {
                    Dominators[RootIndex][Candidate] = Candidate == RootIndex;
                }
                bool DominatorsChanged = true;
                while (DominatorsChanged)
                {
                    DominatorsChanged = false;
                    for (std::size_t NodeIndex = 1U; NodeIndex < Nodes.size(); ++NodeIndex)
                    {
                        std::vector<std::size_t> Predecessors;
                        for (const ControlEdge& Edge : Graph.GetControlEdges())
                        {
                            if (Edge.DestinationNode != Nodes[NodeIndex] ||
                                IsLoopTransferEdge(Graph, Descriptors, Edge))
                            {
                                continue;
                            }
                            for (std::size_t SourceIndex = 0U; SourceIndex < Nodes.size(); ++SourceIndex)
                            {
                                if (Nodes[SourceIndex] == Edge.SourceNode)
                                {
                                    Predecessors.push_back(SourceIndex);
                                    break;
                                }
                            }
                        }
                        std::vector<bool> NewDominators(Nodes.size(), false);
                        NewDominators[NodeIndex] = true;
                        if (!Predecessors.empty())
                        {
                            for (std::size_t Candidate = 0U; Candidate < Nodes.size(); ++Candidate)
                            {
                                bool InEveryPredecessor = true;
                                for (const std::size_t Predecessor : Predecessors)
                                {
                                    InEveryPredecessor = InEveryPredecessor &&
                                        Dominators[Predecessor][Candidate];
                                }
                                NewDominators[Candidate] = NewDominators[Candidate] ||
                                    InEveryPredecessor;
                            }
                        }
                        if (NewDominators != Dominators[NodeIndex])
                        {
                            Dominators[NodeIndex] = std::move(NewDominators);
                            DominatorsChanged = true;
                        }
                    }
                }

                for (const InputBindingRecord& Binding : Graph.GetInputBindings())
                {
                    const OutputReference* Output = std::get_if<OutputReference>(&Binding.Binding);
                    if (Output == nullptr)
                    {
                        continue;
                    }
                    const NodeInstance* Destination = Graph.FindNode(Binding.DestinationNode);
                    const ExecutionRegion* DestinationRegion = Destination != nullptr &&
                        Destination->ExecutionRegion.has_value()
                        ? Graph.FindExecutionRegion(*Destination->ExecutionRegion) : nullptr;
                    const NodeDescriptor* DestinationDescriptor = Destination == nullptr
                        ? nullptr : Descriptors.Find(Destination->Descriptor);
                    if (Destination == nullptr || DestinationRegion == nullptr ||
                        DestinationRegion->Entry != Entry.Identifier || DestinationDescriptor == nullptr ||
                        !DestinationDescriptor->GetExecutionControlSchema().has_value() ||
                        IsInsideLoopBody(Graph, *Destination))
                    {
                        continue;
                    }
                    const ProvenanceRecord* SourceRecord = nullptr;
                    for (const ProvenanceRecord& Record : Provenance)
                    {
                        if (Record.Node == Output->SourceNode)
                        {
                            SourceRecord = &Record;
                            break;
                        }
                    }
                    if (SourceRecord == nullptr || SourceRecord->Unknown)
                    {
                        continue;
                    }
                    std::size_t ConsumerIndex = Nodes.size();
                    for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                    {
                        if (Nodes[Index] == Destination->Identifier)
                        {
                            ConsumerIndex = Index;
                            break;
                        }
                    }
                    if (ConsumerIndex == Nodes.size())
                    {
                        continue;
                    }
                    for (const NodeInstanceId Origin : SourceRecord->Origins)
                    {
                        const NodeInstance* Producer = Graph.FindNode(Origin);
                        const ExecutionRegion* ProducerRegion = Producer != nullptr &&
                            Producer->ExecutionRegion.has_value()
                            ? Graph.FindExecutionRegion(*Producer->ExecutionRegion) : nullptr;
                        if (ProducerRegion == nullptr)
                        {
                            continue;
                        }
                        std::size_t ProducerIndex = Nodes.size();
                        for (std::size_t Index = 0U; Index < Nodes.size(); ++Index)
                        {
                            if (Nodes[Index] == Origin)
                            {
                                ProducerIndex = Index;
                                break;
                            }
                        }
                        if (ProducerRegion->Entry != Entry.Identifier ||
                            ProducerIndex == Nodes.size() ||
                            !Dominators[ConsumerIndex][ProducerIndex])
                        {
                            Add(Diagnostics, DiagnosticCode::ExecutionDataNotDominated,
                                "A structured data use depends on an execution producer that does not dominate it in the same entry.");
                            break;
                        }
                    }
                }
            }
        }

        static void Add(
            DiagnosticCollection& Diagnostics,
            DiagnosticCode Code,
            const char* Message
        )
        {
            Diagnostics.push_back(Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = Message
            });
        }

        static const NodeInstance* FindPriorNode(
            const GraphIR& Graph,
            NodeInstanceId Identifier,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                if (Graph.GetNodes()[Index].Identifier == Identifier)
                {
                    return &Graph.GetNodes()[Index];
                }
            }
            return nullptr;
        }

        static const GraphVariable* FindPriorVariable(
            const GraphIR& Graph,
            GraphVariableId Identifier,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                if (Graph.GetVariables()[Index].Identifier == Identifier)
                {
                    return &Graph.GetVariables()[Index];
                }
            }
            return nullptr;
        }

        static bool HasPriorControlEdge(
            const GraphIR& Graph,
            const ControlEdge& Edge,
            std::size_t EndIndex
        )
        {
            for (std::size_t Index = 0U; Index < EndIndex; ++Index)
            {
                const ControlEdge& Prior = Graph.GetControlEdges()[Index];
                if (Prior.SourceNode == Edge.SourceNode &&
                    Prior.SourceOutputPin == Edge.SourceOutputPin &&
                    Prior.DestinationNode == Edge.DestinationNode &&
                    Prior.DestinationInputPin == Edge.DestinationInputPin)
                {
                    return true;
                }
            }
            return false;
        }

        static const PinSchema* FindPin(
            const NodeInstance* Node,
            PinIndex Index,
            const NodeDescriptorRegistry& Descriptors
        )
        {
            if (Node == nullptr || !Node->Descriptor.IsValid() || !Index.IsValid())
            {
                return nullptr;
            }
            const NodeDescriptor* Descriptor = Descriptors.Find(Node->Descriptor);
            if (Descriptor == nullptr || Index.GetValue() >= Descriptor->GetPins().size())
            {
                return nullptr;
            }
            return &Descriptor->GetPins()[Index.GetValue()];
        }

        static std::size_t CountBindings(
            const GraphIR& Graph,
            NodeInstanceId Node,
            PinIndex Pin
        )
        {
            std::size_t Count = 0U;
            for (const InputBindingRecord& Record : Graph.GetInputBindings())
            {
                if (Record.DestinationNode == Node && Record.DestinationInputPin == Pin)
                {
                    ++Count;
                }
            }
            return Count;
        }

        static void ValidateBinding(
            const GraphIR& Graph,
            const NodeDescriptorRegistry& Descriptors,
            const InputBindingRecord& Record,
            const PinSchema& DestinationPin,
            DiagnosticCollection& Diagnostics)
        {
            const InputBinding& Binding = Record.Binding;
            const bool HasValidOutputTypeConstraint =
                Record.OutputTypeConstraint.has_value() &&
                Record.OutputTypeConstraint->IsValid() &&
                !ContainsGenericType(*Record.OutputTypeConstraint) &&
                !ContainsFlowType(*Record.OutputTypeConstraint);
            if (Record.OutputTypeConstraint.has_value())
            {
                if (!std::holds_alternative<OutputReference>(Binding))
                {
                    Add(Diagnostics, DiagnosticCode::InvalidInputBinding,
                        "An output type constraint can only be attached to an output binding.");
                }
                if (!HasValidOutputTypeConstraint)
                {
                    Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                        "An output type constraint must be a valid concrete data type.");
                }
            }

            if (const LiteralValue* Literal = std::get_if<LiteralValue>(&Binding))
            {
                if (!DestinationPin.AllowsLiteral())
                {
                    Add(Diagnostics, DiagnosticCode::InvalidInputBinding,
                        "A literal is not allowed by the destination pin schema.");
                }
                else if (!IsLiteralCompatible(*Literal, DestinationPin.GetType()))
                {
                    Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                        "A literal binding is incompatible with the destination pin type.");
                }
                return;
            }
            if (const OutputReference* Output = std::get_if<OutputReference>(&Binding))
            {
                const NodeInstance* SourceNode = Graph.FindNode(Output->SourceNode);
                const PinSchema* SourcePin = FindPin(
                    SourceNode, Output->SourceOutputPin, Descriptors);
                if (!Output->IsValid() || SourcePin == nullptr)
                {
                    Add(Diagnostics, DiagnosticCode::InvalidGraphIRPinReference,
                        "An output binding references a missing or invalid source pin.");
                }
                else
                {
                    if (SourcePin->GetDirection() != PinDirection::Output ||
                        SourcePin->GetCategory() != PinCategory::Data ||
                        ContainsFlowType(SourcePin->GetType()) ||
                        ContainsFlowType(DestinationPin.GetType()) ||
                        !SourcePin->GetType().IsCompatibleWith(DestinationPin.GetType()))
                    {
                        Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                            "An output binding is incompatible with its destination pin.");
                    }
                    if (HasValidOutputTypeConstraint)
                    {
                        if (!SourcePin->GetType().IsCompatibleWith(
                            *Record.OutputTypeConstraint))
                        {
                            Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                                "An output type constraint is structurally incompatible with its source pin.");
                        }
                        else if (!ContainsGenericType(SourcePin->GetType()) &&
                            SourcePin->GetType() != *Record.OutputTypeConstraint)
                        {
                            Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                                "A concrete output type must equal its typed-use constraint.");
                        }
                    }
                }
                return;
            }
            const GraphVariableReference& Variable = std::get<GraphVariableReference>(Binding);
            const GraphVariable* SourceVariable = Graph.FindVariable(Variable.Variable);
            if (!Variable.IsValid() || SourceVariable == nullptr)
            {
                Add(Diagnostics, DiagnosticCode::MissingGraphVariable,
                    "An input binding references a missing or invalid graph variable.");
            }
            else if (!ContainsFlowType(SourceVariable->Type) &&
                !SourceVariable->Type.IsCompatibleWith(DestinationPin.GetType()))
            {
                Add(Diagnostics, DiagnosticCode::IncompatibleGraphIRTypes,
                    "A graph variable binding is incompatible with the destination pin type.");
            }
        }

        static bool IsLiteralCompatible(const LiteralValue& Literal, const TypeDesc& Type)
        {
            if (!Literal.IsValid() || !Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Boolean: return Literal.Is<bool>();
            case TypeDesc::Kind::Integer: return Literal.Is<std::int64_t>();
            case TypeDesc::Kind::Float: return Literal.Is<double>();
            case TypeDesc::Kind::String: return Literal.Is<std::string>();
            case TypeDesc::Kind::GUID: return Literal.Is<GuidValue>();
            case TypeDesc::Kind::Vector3: return Literal.Is<Vector3Value>();
            case TypeDesc::Kind::PrefabId: return Literal.Is<PrefabIdValue>();
            case TypeDesc::Kind::ConfigId: return Literal.Is<ConfigIdValue>();
            case TypeDesc::Kind::Faction: return Literal.Is<FactionValue>();
            case TypeDesc::Kind::Enum:
                return Literal.Is<EnumLiteralValue>() &&
                    Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                    Type.GetEnumTypeIdentity();
            default: return false;
            }
        }

        static bool ContainsGenericType(const TypeDesc& Type)
        {
            if (!Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Generic:
                return true;
            case TypeDesc::Kind::List:
                return ContainsGenericType(*Type.GetElementType());
            case TypeDesc::Kind::Dictionary:
                return ContainsGenericType(*Type.GetKeyType()) ||
                    ContainsGenericType(*Type.GetValueType());
            default:
                return false;
            }
        }

        static bool ContainsFlowType(const TypeDesc& Type)
        {
            if (!Type.IsValid())
            {
                return false;
            }
            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Flow:
                return true;
            case TypeDesc::Kind::List:
                return ContainsFlowType(*Type.GetElementType());
            case TypeDesc::Kind::Dictionary:
                return ContainsFlowType(*Type.GetKeyType()) ||
                    ContainsFlowType(*Type.GetValueType());
            default:
                return false;
            }
        }
    };
}
