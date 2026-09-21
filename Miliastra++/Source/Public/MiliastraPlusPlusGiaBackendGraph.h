#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaBackendMapping.h"

namespace MiliastraPlusPlus
{
    enum class GiaBackendInputValueSourceKind
    {
        DescriptorDefault,
        ExplicitLiteral,
        DataConnection
    };

    struct GiaBackendNodeTrace
    {
        NodeInstanceId GraphNode;
        NodeDescriptorId Descriptor;
        ExternalNodeIdentity ExternalIdentity;
        std::optional<SourceProvenance> DescriptorProvenance;
        std::optional<SourceProvenance> MappingProvenance;
    };

    struct GiaBackendGraphHeader
    {
        DescriptorCatalogueIdentity CatalogueIdentity;
        GiaBackendMappingSchemaVersion MappingSchemaVersion;
        GiaExportTargetProfile TargetProfile = GiaExportTargetProfile::ClientBooleanFilter;
        GiaExportMode Mode = GiaExportMode::Beyond;
        GiaGraphIdentifier GraphIdentifier;
        std::string GraphName;
        GiaUniqueIdentifier UniqueIdentifier;
        double EvaluationInterval = 0.0;
        std::int32_t GraphType = 0;
        std::int32_t GraphWhich = 0;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return CatalogueIdentity.IsValid() &&
                MappingSchemaVersion.GetValue() == 1U &&
                TargetProfile == GiaExportTargetProfile::ClientBooleanFilter &&
                Mode == GiaExportMode::Beyond &&
                GraphIdentifier.IsValid() &&
                !GraphName.empty() &&
                UniqueIdentifier.IsValid() &&
                std::isfinite(EvaluationInterval) &&
                EvaluationInterval >= 0.0 &&
                GraphType == 20001 &&
                GraphWhich == 10;
        }

        bool operator==(const GiaBackendGraphHeader&) const = default;
    };

    struct GiaBackendInputValue
    {
        PinIndex SemanticPin;
        TypeDesc SemanticType;
        GiaBackendInputValueSourceKind SourceKind =
            GiaBackendInputValueSourceKind::DescriptorDefault;
        std::optional<LiteralValue> Literal;

        [[nodiscard]] bool IsValid() const
        {
            if (!SemanticPin.IsValid() || !SemanticType.IsValid())
            {
                return false;
            }

            if (SourceKind == GiaBackendInputValueSourceKind::DataConnection)
            {
                return !Literal.has_value();
            }

            if (SourceKind != GiaBackendInputValueSourceKind::DescriptorDefault &&
                SourceKind != GiaBackendInputValueSourceKind::ExplicitLiteral)
            {
                return false;
            }

            return Literal.has_value() && Literal->IsValid();
        }

        bool operator==(const GiaBackendInputValue&) const = default;
    };

    struct GiaBackendNode
    {
        GiaBackendNodeTrace Trace;
        GiaBackendNodeMapping Mapping;
        std::vector<GiaBackendInputValue> Inputs;

        [[nodiscard]] bool IsValid() const
        {
            if (!Trace.GraphNode.IsValid() ||
                !Trace.Descriptor.IsValid() ||
                !Trace.ExternalIdentity.IsValid() ||
                !Mapping.IsValid() ||
                Trace.ExternalIdentity != Mapping.GetExternalIdentity() ||
                !Mapping.GetGenericNodeIdentifier().IsValid() ||
                !Mapping.GetConcreteNodeIdentifier().has_value() ||
                !Mapping.GetConcreteNodeIdentifier()->IsValid())
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < Mapping.GetPinMappings().size(); ++Index)
            {
                const GiaBackendPinMapping& MappingValue =
                    Mapping.GetPinMappings()[Index];
                if (Index > 0U &&
                    Mapping.GetPinMappings()[Index - 1U].GetSemanticPinIndex() >=
                        MappingValue.GetSemanticPinIndex())
                {
                    return false;
                }
            }

            for (std::size_t Index = 0U; Index < Inputs.size(); ++Index)
            {
                if (!Inputs[Index].IsValid())
                {
                    return false;
                }
                if (Index > 0U &&
                    Inputs[Index - 1U].SemanticPin >= Inputs[Index].SemanticPin)
                {
                    return false;
                }

                const GiaBackendPinMapping* PinMapping = FindPinMapping(
                    Inputs[Index].SemanticPin);
                if (PinMapping == nullptr ||
                    !IsSupportedInputMappingTuple(Inputs[Index], *PinMapping))
                {
                    return false;
                }
            }

            return true;
        }

        bool operator==(const GiaBackendNode& Other) const
        {
            return Trace.GraphNode == Other.Trace.GraphNode &&
                Trace.Descriptor == Other.Trace.Descriptor &&
                Trace.ExternalIdentity == Other.Trace.ExternalIdentity &&
                Mapping == Other.Mapping &&
                Inputs == Other.Inputs;
        }

    private:
        [[nodiscard]] const GiaBackendPinMapping* FindPinMapping(
            PinIndex SemanticPin
        ) const
        {
            for (const GiaBackendPinMapping& MappingValue : Mapping.GetPinMappings())
            {
                if (MappingValue.GetSemanticPinIndex() == SemanticPin)
                {
                    return &MappingValue;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static bool IsLiteralCompatible(
            const LiteralValue& Literal,
            const TypeDesc& Type
        )
        {
            if (!Literal.IsValid() || !Type.IsValid())
            {
                return false;
            }

            switch (Type.GetKind())
            {
            case TypeDesc::Kind::Boolean:
                return Literal.Is<bool>();
            case TypeDesc::Kind::Enum:
                return Literal.Is<EnumLiteralValue>() &&
                    Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                    Type.GetEnumTypeIdentity();
            default:
                return false;
            }
        }

        [[nodiscard]] static bool IsSupportedInputMappingTuple(
            const GiaBackendInputValue& Input,
            const GiaBackendPinMapping& PinMapping
        )
        {
            if (PinMapping.GetPinKind() != GiaPinKind::InputParameter ||
                PinMapping.GetEmissionPolicy() != GiaPinEmissionPolicy::Emit)
            {
                return false;
            }

            if (Input.SemanticType.GetKind() == TypeDesc::Kind::Boolean)
            {
                if (PinMapping.GetBackendTypeCode().GetValue() != 5 ||
                    PinMapping.GetLiteralEncoding() != GiaLiteralEncodingKind::Boolean)
                {
                    return false;
                }
            }
            else if (Input.SemanticType.GetKind() == TypeDesc::Kind::Enum)
            {
                if (Input.SemanticType.GetEnumTypeIdentity() !=
                        EnumTypeIdentity("filter_return_type") ||
                    PinMapping.GetBackendTypeCode().GetValue() != 13 ||
                    PinMapping.GetLiteralEncoding() != GiaLiteralEncodingKind::Enum)
                {
                    return false;
                }
            }
            else
            {
                return false;
            }

            if (Input.SourceKind == GiaBackendInputValueSourceKind::DataConnection)
            {
                return !Input.Literal.has_value();
            }

            return Input.Literal.has_value() &&
                IsLiteralCompatible(*Input.Literal, Input.SemanticType);
        }
    };

    struct GiaBackendDataConnection
    {
        NodeInstanceId SourceNode;
        PinIndex SourcePin;
        NodeInstanceId DestinationNode;
        PinIndex DestinationPin;
        TypeDesc ValueType;

        [[nodiscard]] bool IsValid() const
        {
            return SourceNode.IsValid() && SourcePin.IsValid() &&
                DestinationNode.IsValid() && DestinationPin.IsValid() &&
                ValueType.IsValid();
        }

        auto operator<=>(const GiaBackendDataConnection&) const = default;
    };

    struct GiaBackendControlConnection
    {
        NodeInstanceId SourceNode;
        PinIndex SourcePin;
        NodeInstanceId DestinationNode;
        PinIndex DestinationPin;

        [[nodiscard]] bool IsValid() const
        {
            return SourceNode.IsValid() && SourcePin.IsValid() &&
                DestinationNode.IsValid() && DestinationPin.IsValid();
        }

        auto operator<=>(const GiaBackendControlConnection&) const = default;
    };

    class GiaBackendGraph;

    namespace GiaBackendGraphDetail
    {
        [[nodiscard]] GiaBackendGraph CreateForTesting(
            GiaBackendGraphHeader Header,
            std::vector<GiaBackendNode> Nodes,
            std::vector<GiaBackendDataConnection> DataConnections,
            std::vector<GiaBackendControlConnection> ControlConnections
        );
    }

    class GiaBackendGraph final
    {
    public:
        GiaBackendGraph(const GiaBackendGraph&) = default;
        GiaBackendGraph(GiaBackendGraph&&) = default;
        GiaBackendGraph& operator=(const GiaBackendGraph&) = default;
        GiaBackendGraph& operator=(GiaBackendGraph&&) = default;

        [[nodiscard]] const GiaBackendGraphHeader& GetHeader() const noexcept
        {
            return m_Header;
        }

        [[nodiscard]] const std::vector<GiaBackendNode>& GetNodes() const noexcept
        {
            return m_Nodes;
        }

        [[nodiscard]] const std::vector<GiaBackendDataConnection>&
            GetDataConnections() const noexcept
        {
            return m_DataConnections;
        }

        [[nodiscard]] const std::vector<GiaBackendControlConnection>&
            GetControlConnections() const noexcept
        {
            return m_ControlConnections;
        }

        [[nodiscard]] bool IsValid() const
        {
            if (!m_Header.IsValid() || m_Nodes.empty())
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < m_Nodes.size(); ++Index)
            {
                if (!m_Nodes[Index].IsValid())
                {
                    return false;
                }
                if (Index > 0U &&
                    m_Nodes[Index - 1U].Trace.GraphNode >=
                        m_Nodes[Index].Trace.GraphNode)
                {
                    return false;
                }
            }

            for (std::size_t Index = 0U; Index < m_DataConnections.size(); ++Index)
            {
                const GiaBackendDataConnection& Connection =
                    m_DataConnections[Index];
                if (!Connection.IsValid() ||
                    !IsCanonicalConnectionOrder(m_DataConnections, Index))
                {
                    return false;
                }
                if (!ValidateDataConnection(Connection))
                {
                    return false;
                }
            }

            for (std::size_t Index = 0U; Index < m_ControlConnections.size(); ++Index)
            {
                const GiaBackendControlConnection& Connection =
                    m_ControlConnections[Index];
                if (!Connection.IsValid() ||
                    !IsCanonicalConnectionOrder(m_ControlConnections, Index) ||
                    !ValidateControlConnection(Connection))
                {
                    return false;
                }
            }

            for (const GiaBackendNode& Node : m_Nodes)
            {
                for (const GiaBackendInputValue& Input : Node.Inputs)
                {
                    if (Input.SourceKind ==
                        GiaBackendInputValueSourceKind::DataConnection)
                    {
                        const std::size_t ConnectionCount = std::count_if(
                            m_DataConnections.begin(),
                            m_DataConnections.end(),
                            [&Node, &Input](const GiaBackendDataConnection& Connection)
                            {
                                return Connection.DestinationNode ==
                                            Node.Trace.GraphNode &&
                                        Connection.DestinationPin == Input.SemanticPin;
                            }
                        );
                        if (ConnectionCount != 1U)
                        {
                            return false;
                        }
                    }
                }
            }

            return true;
        }

        bool operator==(const GiaBackendGraph& Other) const
        {
            return m_Header == Other.m_Header &&
                m_Nodes == Other.m_Nodes &&
                m_DataConnections == Other.m_DataConnections &&
                m_ControlConnections == Other.m_ControlConnections;
        }

    private:
        friend class GiaGraphLowerer;
        friend GiaBackendGraph GiaBackendGraphDetail::CreateForTesting(
            GiaBackendGraphHeader Header,
            std::vector<GiaBackendNode> Nodes,
            std::vector<GiaBackendDataConnection> DataConnections,
            std::vector<GiaBackendControlConnection> ControlConnections
        );

        GiaBackendGraph(
            GiaBackendGraphHeader Header,
            std::vector<GiaBackendNode> Nodes,
            std::vector<GiaBackendDataConnection> DataConnections,
            std::vector<GiaBackendControlConnection> ControlConnections
        )
            : m_Header(std::move(Header))
            , m_Nodes(std::move(Nodes))
            , m_DataConnections(std::move(DataConnections))
            , m_ControlConnections(std::move(ControlConnections))
        {
        }

        [[nodiscard]] const GiaBackendNode* FindNode(NodeInstanceId Identifier) const
        {
            for (const GiaBackendNode& Node : m_Nodes)
            {
                if (Node.Trace.GraphNode == Identifier)
                {
                    return &Node;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static bool IsCanonicalConnectionOrder(
            const std::vector<GiaBackendDataConnection>& Connections,
            std::size_t Index
        )
        {
            return Index == 0U ||
                Connections[Index - 1U] < Connections[Index];
        }

        [[nodiscard]] static bool IsCanonicalConnectionOrder(
            const std::vector<GiaBackendControlConnection>& Connections,
            std::size_t Index
        )
        {
            return Index == 0U ||
                Connections[Index - 1U] < Connections[Index];
        }

        [[nodiscard]] bool ValidateDataConnection(
            const GiaBackendDataConnection& Connection
        ) const
        {
            const GiaBackendNode* Source = FindNode(Connection.SourceNode);
            const GiaBackendNode* Destination = FindNode(Connection.DestinationNode);
            if (Source == nullptr || Destination == nullptr)
            {
                return false;
            }

            const GiaBackendPinMapping* SourceMapping = FindMapping(
                *Source,
                Connection.SourcePin);
            const GiaBackendPinMapping* DestinationMapping = FindMapping(
                *Destination,
                Connection.DestinationPin);
            const GiaBackendInputValue* DestinationInput = FindInput(
                *Destination,
                Connection.DestinationPin);
            return SourceMapping != nullptr && DestinationMapping != nullptr &&
                DestinationInput != nullptr &&
                SourceMapping->GetPinKind() == GiaPinKind::OutputParameter &&
                DestinationMapping->GetPinKind() == GiaPinKind::InputParameter &&
                DestinationMapping->IsConnectable() &&
                DestinationMapping->GetEmissionPolicy() == GiaPinEmissionPolicy::Emit &&
                DestinationInput->SourceKind ==
                    GiaBackendInputValueSourceKind::DataConnection &&
                DestinationInput->SemanticType.IsCompatibleWith(Connection.ValueType);
        }

        [[nodiscard]] bool ValidateControlConnection(
            const GiaBackendControlConnection& Connection
        ) const
        {
            const GiaBackendNode* Source = FindNode(Connection.SourceNode);
            const GiaBackendNode* Destination = FindNode(Connection.DestinationNode);
            if (Source == nullptr || Destination == nullptr)
            {
                return false;
            }

            const GiaBackendPinMapping* SourceMapping = FindMapping(
                *Source,
                Connection.SourcePin);
            const GiaBackendPinMapping* DestinationMapping = FindMapping(
                *Destination,
                Connection.DestinationPin);
            return SourceMapping != nullptr && DestinationMapping != nullptr &&
                SourceMapping->GetPinKind() == GiaPinKind::OutputFlow &&
                DestinationMapping->GetPinKind() == GiaPinKind::InputFlow;
        }

        [[nodiscard]] static const GiaBackendPinMapping* FindMapping(
            const GiaBackendNode& Node,
            PinIndex Pin
        )
        {
            for (const GiaBackendPinMapping& Mapping : Node.Mapping.GetPinMappings())
            {
                if (Mapping.GetSemanticPinIndex() == Pin)
                {
                    return &Mapping;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static const GiaBackendInputValue* FindInput(
            const GiaBackendNode& Node,
            PinIndex Pin
        )
        {
            for (const GiaBackendInputValue& Input : Node.Inputs)
            {
                if (Input.SemanticPin == Pin)
                {
                    return &Input;
                }
            }
            return nullptr;
        }

        GiaBackendGraphHeader m_Header;
        std::vector<GiaBackendNode> m_Nodes;
        std::vector<GiaBackendDataConnection> m_DataConnections;
        std::vector<GiaBackendControlConnection> m_ControlConnections;
    };

    namespace GiaBackendGraphDetail
    {
        inline GiaBackendGraph CreateForTesting(
            GiaBackendGraphHeader Header,
            std::vector<GiaBackendNode> Nodes,
            std::vector<GiaBackendDataConnection> DataConnections,
            std::vector<GiaBackendControlConnection> ControlConnections
        )
        {
            return GiaBackendGraph(
                std::move(Header),
                std::move(Nodes),
                std::move(DataConnections),
                std::move(ControlConnections)
            );
        }
    }
}


