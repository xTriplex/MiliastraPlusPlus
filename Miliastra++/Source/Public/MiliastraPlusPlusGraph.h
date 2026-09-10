#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusLink.h"
#include "MiliastraPlusPlusNode.h"

namespace MiliastraPlusPlus
{
    class Graph
    {
    public:
        Graph(GraphIdentifier Identifier, std::string Name)
            : m_Identifier(Identifier)
            , m_Name(std::move(Name))
        {
        }

        Graph(const Graph& OtherGraph)
            : m_Identifier(OtherGraph.m_Identifier)
            , m_Name(OtherGraph.m_Name)
            , m_Links(OtherGraph.m_Links)
        {
            m_Nodes.reserve(OtherGraph.m_Nodes.size());
            for (const std::unique_ptr<Node>& OtherNode : OtherGraph.m_Nodes)
            {
                m_Nodes.push_back(OtherNode->Clone());
            }
        }

        Graph& operator=(const Graph&) = delete;
        Graph(Graph&&) noexcept = default;
        Graph& operator=(Graph&&) noexcept = default;
        virtual ~Graph() = default;

        [[nodiscard]] GraphIdentifier GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] std::expected<void, Diagnostic> AddNode(std::unique_ptr<Node> GraphNode)
        {
            if (!GraphNode)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::NullNode,
                    .Message = "A graph cannot own a null node."
                });
            }

            if (FindNodeByIdentifier(GraphNode->GetIdentifier()) != nullptr)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::DuplicateNodeIdentifier,
                    .Message = "A graph cannot contain two nodes with the same node identifier.",
                    .SourceNodeIdentifier = GraphNode->GetIdentifier()
                });
            }

            m_Nodes.push_back(std::move(GraphNode));
            return {};
        }

        [[nodiscard]] std::expected<void, Diagnostic> AddLink(const Link& GraphLink)
        {
            if (FindLinkByIdentifier(GraphLink.GetIdentifier()) != nullptr)
            {
                return std::unexpected(CreateLinkDiagnostic(
                    DiagnosticCode::DuplicateLinkIdentifier,
                    "A graph cannot contain two links with the same link identifier.",
                    GraphLink
                ));
            }

            if (const std::optional<Diagnostic> LinkDiagnostic = ValidateLink(GraphLink))
            {
                return std::unexpected(LinkDiagnostic.value());
            }

            m_Links.push_back(GraphLink);
            return {};
        }

        [[nodiscard]] const std::vector<std::unique_ptr<Node>>& GetNodes() const
        {
            return m_Nodes;
        }

        [[nodiscard]] const std::vector<Link>& GetLinks() const
        {
            return m_Links;
        }

        [[nodiscard]] Node* FindNodeByIdentifier(NodeIdentifier Identifier)
        {
            for (const std::unique_ptr<Node>& GraphNode : m_Nodes)
            {
                if (GraphNode->GetIdentifier() == Identifier)
                {
                    return GraphNode.get();
                }
            }

            return nullptr;
        }

        [[nodiscard]] const Node* FindNodeByIdentifier(NodeIdentifier Identifier) const
        {
            for (const std::unique_ptr<Node>& GraphNode : m_Nodes)
            {
                if (GraphNode->GetIdentifier() == Identifier)
                {
                    return GraphNode.get();
                }
            }

            return nullptr;
        }

        [[nodiscard]] Pin* FindPinByReference(const PinReference& Reference)
        {
            Node* ParentNode = FindNodeByIdentifier(Reference.OwningNodeIdentifier);
            return ParentNode == nullptr
                ? nullptr
                : ParentNode->GetPinByIdentifier(Reference.LocalPinIdentifier);
        }

        [[nodiscard]] const Pin* FindPinByReference(const PinReference& Reference) const
        {
            const Node* ParentNode = FindNodeByIdentifier(Reference.OwningNodeIdentifier);
            return ParentNode == nullptr
                ? nullptr
                : ParentNode->GetPinByIdentifier(Reference.LocalPinIdentifier);
        }

        [[nodiscard]] DiagnosticCollection PropagateTypes()
        {
            ClearResolvedGenericTypes();

            DiagnosticCollection Diagnostics;
            std::vector<GenericPinRecord> GenericPins = CollectGenericPins();
            DisjointSet GenericPinSets(GenericPins.size());

            for (std::size_t FirstIndex = 0; FirstIndex < GenericPins.size(); ++FirstIndex)
            {
                const GenericPinRecord& FirstGenericPin = GenericPins[FirstIndex];
                const std::int32_t TypeGroupIdentifier =
                    FirstGenericPin.GraphPin->GetTypeGroupIdentifier();
                if (TypeGroupIdentifier < 0)
                {
                    continue;
                }

                for (std::size_t SecondIndex = FirstIndex + 1;
                     SecondIndex < GenericPins.size();
                     ++SecondIndex)
                {
                    const GenericPinRecord& SecondGenericPin = GenericPins[SecondIndex];
                    if (
                        FirstGenericPin.Reference.OwningNodeIdentifier ==
                            SecondGenericPin.Reference.OwningNodeIdentifier &&
                        TypeGroupIdentifier == SecondGenericPin.GraphPin->GetTypeGroupIdentifier()
                    )
                    {
                        GenericPinSets.Union(FirstIndex, SecondIndex);
                    }
                }
            }

            for (const Link& GraphLink : m_Links)
            {
                Pin* SourcePin = FindPinByReference(GraphLink.GetSourcePinReference());
                Pin* DestinationPin = FindPinByReference(GraphLink.GetDestinationPinReference());
                if (SourcePin == nullptr || DestinationPin == nullptr)
                {
                    if (const std::optional<Diagnostic> LinkDiagnostic = ValidateLink(GraphLink))
                    {
                        Diagnostics.push_back(LinkDiagnostic.value());
                    }
                    continue;
                }

                if (
                    SourcePin->GetPinCategory() != EPinCategory::Data ||
                    DestinationPin->GetPinCategory() != EPinCategory::Data
                )
                {
                    continue;
                }

                const std::optional<std::size_t> SourceGenericIndex = FindGenericPinIndex(
                    GenericPins,
                    GraphLink.GetSourcePinReference()
                );
                const std::optional<std::size_t> DestinationGenericIndex = FindGenericPinIndex(
                    GenericPins,
                    GraphLink.GetDestinationPinReference()
                );

                if (SourceGenericIndex.has_value() && DestinationGenericIndex.has_value())
                {
                    GenericPinSets.Union(SourceGenericIndex.value(), DestinationGenericIndex.value());
                }
            }

            std::vector<std::vector<TypeConstraint>> TypeConstraints(GenericPins.size());

            for (const Link& GraphLink : m_Links)
            {
                Pin* SourcePin = FindPinByReference(GraphLink.GetSourcePinReference());
                Pin* DestinationPin = FindPinByReference(GraphLink.GetDestinationPinReference());
                if (SourcePin == nullptr || DestinationPin == nullptr)
                {
                    continue;
                }

                if (
                    SourcePin->GetPinCategory() != EPinCategory::Data ||
                    DestinationPin->GetPinCategory() != EPinCategory::Data
                )
                {
                    continue;
                }

                const std::optional<std::size_t> SourceGenericIndex = FindGenericPinIndex(
                    GenericPins,
                    GraphLink.GetSourcePinReference()
                );
                const std::optional<std::size_t> DestinationGenericIndex = FindGenericPinIndex(
                    GenericPins,
                    GraphLink.GetDestinationPinReference()
                );

                if (!SourceGenericIndex.has_value() && !DestinationGenericIndex.has_value())
                {
                    if (!SourcePin->IsCompatibleWith(*DestinationPin))
                    {
                        Diagnostics.push_back(CreateLinkDiagnostic(
                            DiagnosticCode::IncompatibleDataPinTypes,
                            "A data link connects incompatible concrete pin types.",
                            GraphLink
                        ));
                    }
                    continue;
                }

                if (SourceGenericIndex.has_value() && !DestinationGenericIndex.has_value())
                {
                    TypeConstraints[GenericPinSets.Find(SourceGenericIndex.value())].push_back({
                        DestinationPin->GetDeclaredTypeSignature(),
                        GraphLink
                    });
                }
                else if (!SourceGenericIndex.has_value() && DestinationGenericIndex.has_value())
                {
                    TypeConstraints[GenericPinSets.Find(DestinationGenericIndex.value())].push_back({
                        SourcePin->GetDeclaredTypeSignature(),
                        GraphLink
                    });
                }
            }

            for (std::size_t GenericPinIndex = 0; GenericPinIndex < GenericPins.size(); ++GenericPinIndex)
            {
                if (GenericPinSets.Find(GenericPinIndex) != GenericPinIndex)
                {
                    continue;
                }

                const std::vector<TypeConstraint>& Constraints = TypeConstraints[GenericPinIndex];
                if (Constraints.empty())
                {
                    Diagnostics.push_back(Diagnostic{
                        .Severity = DiagnosticSeverity::Warning,
                        .Code = DiagnosticCode::UnresolvedGenericPinType,
                        .Message = "A generic pin group has no concrete type constraint.",
                        .SourceNodeIdentifier =
                            GenericPins[GenericPinIndex].Reference.OwningNodeIdentifier,
                        .SourcePinReference = GenericPins[GenericPinIndex].Reference
                    });
                    continue;
                }

                const PinTypeSignature ResolvedTypeSignature = Constraints.front().TypeSignature;
                bool HasConflict = false;
                for (const TypeConstraint& Constraint : Constraints)
                {
                    if (Constraint.TypeSignature != ResolvedTypeSignature)
                    {
                        Diagnostics.push_back(CreateLinkDiagnostic(
                            DiagnosticCode::TypePropagationConflict,
                            "A generic pin group is constrained by incompatible concrete types.",
                            Constraint.GraphLink
                        ));
                        HasConflict = true;
                    }
                }

                if (HasConflict)
                {
                    continue;
                }

                for (std::size_t CandidateIndex = 0;
                     CandidateIndex < GenericPins.size();
                     ++CandidateIndex)
                {
                    if (GenericPinSets.Find(CandidateIndex) == GenericPinIndex)
                    {
                        GenericPins[CandidateIndex].GraphPin->ResolveType(ResolvedTypeSignature);
                    }
                }
            }

            return Diagnostics;
        }

        [[nodiscard]] nlohmann::json Serialize() const
        {
            nlohmann::json SerializedNodes = nlohmann::json::array();
            for (const std::unique_ptr<Node>& GraphNode : m_Nodes)
            {
                SerializedNodes.push_back(GraphNode->Serialize());
            }

            nlohmann::json SerializedLinks = nlohmann::json::array();
            for (const Link& GraphLink : m_Links)
            {
                SerializedLinks.push_back(GraphLink.Serialize());
            }

            return {
                { "Id", m_Identifier.GetValue() },
                { "Name", m_Name },
                { "Nodes", SerializedNodes },
                { "Links", SerializedLinks }
            };
        }

    private:
        struct GenericPinRecord
        {
            PinReference Reference;
            Pin* GraphPin;
        };

        struct TypeConstraint
        {
            PinTypeSignature TypeSignature;
            const Link& GraphLink;
        };

        class DisjointSet
        {
        public:
            explicit DisjointSet(std::size_t ElementCount)
                : m_ParentIndices(ElementCount)
                , m_Ranks(ElementCount, 0U)
            {
                for (std::size_t ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
                {
                    m_ParentIndices[ElementIndex] = ElementIndex;
                }
            }

            [[nodiscard]] std::size_t Find(std::size_t ElementIndex)
            {
                if (m_ParentIndices[ElementIndex] != ElementIndex)
                {
                    m_ParentIndices[ElementIndex] = Find(m_ParentIndices[ElementIndex]);
                }

                return m_ParentIndices[ElementIndex];
            }

            void Union(std::size_t FirstElementIndex, std::size_t SecondElementIndex)
            {
                std::size_t FirstRootIndex = Find(FirstElementIndex);
                std::size_t SecondRootIndex = Find(SecondElementIndex);
                if (FirstRootIndex == SecondRootIndex)
                {
                    return;
                }

                if (m_Ranks[FirstRootIndex] < m_Ranks[SecondRootIndex])
                {
                    std::swap(FirstRootIndex, SecondRootIndex);
                }

                m_ParentIndices[SecondRootIndex] = FirstRootIndex;
                if (m_Ranks[FirstRootIndex] == m_Ranks[SecondRootIndex])
                {
                    ++m_Ranks[FirstRootIndex];
                }
            }

        private:
            std::vector<std::size_t> m_ParentIndices;
            std::vector<std::size_t> m_Ranks;
        };

        [[nodiscard]] const Link* FindLinkByIdentifier(LinkIdentifier Identifier) const
        {
            for (const Link& GraphLink : m_Links)
            {
                if (GraphLink.GetIdentifier() == Identifier)
                {
                    return &GraphLink;
                }
            }

            return nullptr;
        }

        [[nodiscard]] std::optional<Diagnostic> ValidateLink(const Link& GraphLink) const
        {
            const PinReference& SourcePinReference = GraphLink.GetSourcePinReference();
            const PinReference& DestinationPinReference = GraphLink.GetDestinationPinReference();

            if (SourcePinReference == DestinationPinReference)
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::SelfLink,
                    "A link cannot connect a pin to itself.",
                    GraphLink
                );
            }

            const Pin* SourcePin = FindPinByReference(SourcePinReference);
            if (SourcePin == nullptr)
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::MissingSourcePin,
                    "The source pin does not belong to this graph.",
                    GraphLink
                );
            }

            const Pin* DestinationPin = FindPinByReference(DestinationPinReference);
            if (DestinationPin == nullptr)
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::MissingDestinationPin,
                    "The destination pin does not belong to this graph.",
                    GraphLink
                );
            }

            if (SourcePin->GetPinKind() != EPinKind::Output)
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::SourcePinMustBeOutput,
                    "The source pin of a link must be an output pin.",
                    GraphLink
                );
            }

            if (DestinationPin->GetPinKind() != EPinKind::Input)
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::DestinationPinMustBeInput,
                    "The destination pin of a link must be an input pin.",
                    GraphLink
                );
            }

            if (SourcePin->GetPinCategory() != DestinationPin->GetPinCategory())
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::IncompatiblePinCategories,
                    "Execution pins and data pins cannot be connected to each other.",
                    GraphLink
                );
            }

            if (SourcePin->GetPinCategory() == EPinCategory::Execution)
            {
                if (
                    SourcePin->GetEffectiveType() != EPinType::Flow ||
                    DestinationPin->GetEffectiveType() != EPinType::Flow
                )
                {
                    return CreateLinkDiagnostic(
                        DiagnosticCode::ExecutionPinsMustUseFlowType,
                        "Execution links require Flow pins at both endpoints.",
                        GraphLink
                    );
                }
            }
            else if (!SourcePin->IsCompatibleWith(*DestinationPin))
            {
                return CreateLinkDiagnostic(
                    DiagnosticCode::IncompatibleDataPinTypes,
                    "A data link connects incompatible pin types.",
                    GraphLink
                );
            }

            for (const Link& ExistingLink : m_Links)
            {
                if (
                    ExistingLink.GetSourcePinReference() == SourcePinReference &&
                    ExistingLink.GetDestinationPinReference() == DestinationPinReference
                )
                {
                    return CreateLinkDiagnostic(
                        DiagnosticCode::DuplicateLink,
                        "The graph already contains this link.",
                        GraphLink
                    );
                }

                if (
                    DestinationPin->GetPinCategory() == EPinCategory::Data &&
                    ExistingLink.GetDestinationPinReference() == DestinationPinReference
                )
                {
                    return CreateLinkDiagnostic(
                        DiagnosticCode::MultipleDataInputProducers,
                        "A data input pin can have only one producing link.",
                        GraphLink
                    );
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] static Diagnostic CreateLinkDiagnostic(
            DiagnosticCode Code,
            std::string Message,
            const Link& GraphLink
        )
        {
            return {
                .Severity = DiagnosticSeverity::Error,
                .Code = Code,
                .Message = std::move(Message),
                .SourceNodeIdentifier = GraphLink.GetSourcePinReference().OwningNodeIdentifier,
                .DestinationNodeIdentifier =
                    GraphLink.GetDestinationPinReference().OwningNodeIdentifier,
                .SourcePinReference = GraphLink.GetSourcePinReference(),
                .DestinationPinReference = GraphLink.GetDestinationPinReference()
            };
        }

        void ClearResolvedGenericTypes()
        {
            for (const std::unique_ptr<Node>& GraphNode : m_Nodes)
            {
                for (const std::unique_ptr<Pin>& NodePin : GraphNode->GetPins())
                {
                    if (NodePin->IsGeneric())
                    {
                        NodePin->ClearResolvedType();
                    }
                }
            }
        }

        [[nodiscard]] std::vector<GenericPinRecord> CollectGenericPins()
        {
            std::vector<GenericPinRecord> GenericPins;
            for (const std::unique_ptr<Node>& GraphNode : m_Nodes)
            {
                for (const std::unique_ptr<Pin>& NodePin : GraphNode->GetPins())
                {
                    if (NodePin->IsGeneric())
                    {
                        GenericPins.push_back({
                            .Reference = {
                                GraphNode->GetIdentifier(),
                                NodePin->GetIdentifier()
                            },
                            .GraphPin = NodePin.get()
                        });
                    }
                }
            }

            return GenericPins;
        }

        [[nodiscard]] static std::optional<std::size_t> FindGenericPinIndex(
            const std::vector<GenericPinRecord>& GenericPins,
            const PinReference& Reference
        )
        {
            for (std::size_t GenericPinIndex = 0;
                 GenericPinIndex < GenericPins.size();
                 ++GenericPinIndex)
            {
                if (GenericPins[GenericPinIndex].Reference == Reference)
                {
                    return GenericPinIndex;
                }
            }

            return std::nullopt;
        }

        GraphIdentifier m_Identifier;
        std::string m_Name;
        std::vector<std::unique_ptr<Node>> m_Nodes;
        std::vector<Link> m_Links;
    };
}
