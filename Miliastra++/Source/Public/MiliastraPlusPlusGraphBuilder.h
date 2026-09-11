#pragma once

#include <cstdint>
#include <expected>
#include <limits>
#include <memory>
#include <utility>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusDescriptors.h"
#include "MiliastraPlusPlusGraphIR.h"
#include "MiliastraPlusPlusGraphIRValidation.h"

namespace MiliastraPlusPlus
{
    class GraphBuilder;

    class NodeHandle
    {
    public:
        NodeHandle() = default;

        [[nodiscard]] bool IsValid() const
        {
            return m_Identifier.IsValid() && m_Descriptor.IsValid() && !m_Context.expired();
        }

        [[nodiscard]] NodeInstanceId GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] NodeDescriptorId GetDescriptor() const
        {
            return m_Descriptor;
        }

    private:
        struct Context;

        NodeHandle(
            NodeInstanceId Identifier,
            NodeDescriptorId Descriptor,
            const std::shared_ptr<const Context>& ContextToken
        )
            : m_Identifier(Identifier)
            , m_Descriptor(Descriptor)
            , m_Context(ContextToken)
        {
        }

        friend class GraphBuilder;

        NodeInstanceId m_Identifier;
        NodeDescriptorId m_Descriptor;
        std::weak_ptr<const Context> m_Context;
    };

    struct NodeHandle::Context
    {
    };

    class GraphBuilder
    {
    public:
        explicit GraphBuilder(const NodeDescriptorRegistry& Descriptors)
            : m_Descriptors(Descriptors)
            , m_Context(std::make_shared<const NodeHandle::Context>())
        {
        }

        GraphBuilder(const GraphBuilder&) = delete;
        GraphBuilder& operator=(const GraphBuilder&) = delete;

        GraphBuilder(GraphBuilder&& Other) noexcept
            : m_Descriptors(Other.m_Descriptors)
            , m_Graph(std::move(Other.m_Graph))
            , m_NextNodeIdentifier(Other.m_NextNodeIdentifier)
            , m_Context(std::move(Other.m_Context))
            , m_IsClosed(Other.m_IsClosed)
        {
            Other.m_IsClosed = true;
            Other.m_Context.reset();
        }

        GraphBuilder& operator=(GraphBuilder&&) = delete;

        [[nodiscard]] std::expected<NodeHandle, DiagnosticCollection> AddNode(
            NodeDescriptorId DescriptorIdentifier
        )
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot add a node.")
                });
            }

            if (!DescriptorIdentifier.IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("An invalid node descriptor identifier was supplied.")
                });
            }

            const NodeDescriptor* Descriptor = m_Descriptors.Find(DescriptorIdentifier);
            if (Descriptor == nullptr)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeMissingDescriptorDiagnostic("The requested node descriptor is not registered.")
                });
            }

            if (!Descriptor->IsValid())
            {
                return std::unexpected(DiagnosticCollection{
                    MakeDescriptorDiagnostic("The requested node descriptor is invalid.")
                });
            }

            if (m_NextNodeIdentifier == 0U)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("The graph builder exhausted its node identifier range.")
                });
            }

            const NodeInstanceId Identifier(m_NextNodeIdentifier);
            m_Graph.AddNode(NodeInstance{Identifier, DescriptorIdentifier});

            if (m_NextNodeIdentifier == std::numeric_limits<std::uint64_t>::max())
            {
                m_NextNodeIdentifier = 0U;
            }
            else
            {
                ++m_NextNodeIdentifier;
            }

            return NodeHandle(Identifier, DescriptorIdentifier, m_Context);
        }

        [[nodiscard]] bool IsHandleUsable(const NodeHandle& Handle) const
        {
            if (m_IsClosed || !Handle.IsValid() || !HasSameContext(Handle))
            {
                return false;
            }

            const NodeInstance* Node = m_Graph.FindNode(Handle.GetIdentifier());
            return Node != nullptr && Node->Descriptor == Handle.GetDescriptor();
        }

        [[nodiscard]] DiagnosticCollection Validate() const
        {
            if (m_IsClosed)
            {
                return DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be validated.")
                };
            }

            return GraphIRValidator::Validate(m_Graph, m_Descriptors);
        }

        [[nodiscard]] std::expected<GraphIR, DiagnosticCollection> Finalize() &&
        {
            if (m_IsClosed)
            {
                return std::unexpected(DiagnosticCollection{
                    MakeLifecycleDiagnostic("A closed graph builder cannot be finalized.")
                });
            }

            m_IsClosed = true;
            m_Context.reset();

            DiagnosticCollection Diagnostics = GraphIRValidator::Validate(m_Graph, m_Descriptors);
            if (ContainsError(Diagnostics))
            {
                return std::unexpected(std::move(Diagnostics));
            }

            return std::move(m_Graph);
        }

    private:
        [[nodiscard]] bool HasSameContext(const NodeHandle& Handle) const
        {
            if (m_Context == nullptr)
            {
                return false;
            }

            const std::weak_ptr<const NodeHandle::Context> BuilderContext = m_Context;
            return !BuilderContext.owner_before(Handle.m_Context) &&
                !Handle.m_Context.owner_before(BuilderContext);
        }

        [[nodiscard]] static Diagnostic MakeDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidNodeDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeMissingDescriptorDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::MissingDescriptor,
                .Message = Message
            };
        }

        [[nodiscard]] static Diagnostic MakeLifecycleDiagnostic(const char* Message)
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGraphBuilderState,
                .Message = Message
            };
        }

        const NodeDescriptorRegistry& m_Descriptors;
        GraphIR m_Graph;
        std::uint64_t m_NextNodeIdentifier = 1U;
        std::shared_ptr<const NodeHandle::Context> m_Context;
        bool m_IsClosed = false;
    };
}
