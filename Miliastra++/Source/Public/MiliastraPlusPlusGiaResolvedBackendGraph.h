#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaBackendGraph.h"

namespace MiliastraPlusPlus
{
    enum class GiaResolvedGraphUnitClass : std::int32_t
    {
        Node = 1
    };

    enum class GiaResolvedGraphUnitType : std::int32_t
    {
        ClientGraph = 3
    };

    enum class GiaResolvedNodeGraphClass : std::int32_t
    {
        UserDefined = 10000
    };

    enum class GiaResolvedNodeGraphKind : std::int32_t
    {
        NodeGraph = 21001
    };

    class GiaResolvedNodeIndex final
    {
    public:
        constexpr GiaResolvedNodeIndex() = default;

        explicit constexpr GiaResolvedNodeIndex(std::int32_t Value)
            : m_Value(Value)
        {
        }

        [[nodiscard]] constexpr bool IsValid() const noexcept
        {
            return m_Value > 0;
        }

        [[nodiscard]] constexpr std::int32_t GetValue() const noexcept
        {
            return m_Value;
        }

        auto operator<=>(const GiaResolvedNodeIndex&) const = default;

    private:
        std::int32_t m_Value = 0;
    };

    struct GiaResolvedNodePosition
    {
        float X = 0.0F;
        float Y = 0.0F;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return std::isfinite(X) && std::isfinite(Y);
        }

        bool operator==(const GiaResolvedNodePosition&) const = default;
    };

    struct GiaResolvedGraphHeader
    {
        DescriptorCatalogueIdentity CatalogueIdentity;
        GiaBackendMappingSchemaVersion MappingSchemaVersion;
        GiaExportTargetProfile TargetProfile =
            GiaExportTargetProfile::ClientBooleanFilter;
        GiaExportMode Mode = GiaExportMode::Beyond;
        GiaGraphIdentifier GraphIdentifier;
        std::string GraphName;
        GiaUniqueIdentifier UniqueIdentifier;
        float EvaluationInterval = 0.0F;
        std::int32_t GraphType = 0;
        std::int32_t GraphWhich = 0;
        GiaResolvedGraphUnitClass RootClass =
            GiaResolvedGraphUnitClass::Node;
        GiaResolvedGraphUnitType RootType =
            GiaResolvedGraphUnitType::ClientGraph;
        GiaResolvedNodeGraphClass InnerClass =
            GiaResolvedNodeGraphClass::UserDefined;
        GiaResolvedNodeGraphKind InnerKind =
            GiaResolvedNodeGraphKind::NodeGraph;
        std::int32_t EntrySlotIndex = 0;
        std::optional<std::uint32_t> RootModeFlag;

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
                EvaluationInterval >= 0.0F &&
                GraphType == 20001 &&
                GraphWhich == 10 &&
                RootClass == GiaResolvedGraphUnitClass::Node &&
                RootType == GiaResolvedGraphUnitType::ClientGraph &&
                InnerClass == GiaResolvedNodeGraphClass::UserDefined &&
                InnerKind == GiaResolvedNodeGraphKind::NodeGraph &&
                EntrySlotIndex == 1 &&
                !RootModeFlag.has_value();
        }

        bool operator==(const GiaResolvedGraphHeader&) const = default;
    };

    struct GiaResolvedPin;
    struct GiaResolvedPinEndpoint;
    struct GiaResolvedDataConnection;
    struct GiaResolvedControlConnection;

    namespace GiaResolvedBackendGraphDetail
    {
        using PinOrderKeyType = std::tuple<
            int,
            std::int32_t,
            std::int32_t,
            std::uint32_t>;
        using EndpointOrderKeyType = std::tuple<
            std::int32_t,
            int,
            std::int32_t,
            std::int32_t>;
        using DataConnectionOrderKeyType = std::tuple<
            EndpointOrderKeyType,
            EndpointOrderKeyType>;
        using ControlConnectionOrderKeyType = DataConnectionOrderKeyType;

        [[nodiscard]] inline bool IsSupportedPinKind(GiaPinKind Kind) noexcept
        {
            return Kind == GiaPinKind::InputFlow ||
                Kind == GiaPinKind::OutputFlow ||
                Kind == GiaPinKind::InputParameter ||
                Kind == GiaPinKind::OutputParameter;
        }

        [[nodiscard]] inline bool IsValidLiteralEncoding(GiaLiteralEncodingKind LiteralEncoding) noexcept
        {
            return LiteralEncoding == GiaLiteralEncodingKind::None ||
                LiteralEncoding == GiaLiteralEncodingKind::Boolean ||
                LiteralEncoding == GiaLiteralEncodingKind::Enum;
        }

        [[nodiscard]] inline bool IsSupportedSemanticType(const TypeDesc& Type)
        {
            return Type.GetKind() == TypeDesc::Kind::Boolean ||
                (Type.GetKind() == TypeDesc::Kind::Enum &&
                    Type.GetEnumTypeIdentity() ==
                        EnumTypeIdentity("filter_return_type"));
        }

        [[nodiscard]] inline bool IsLiteralCompatible(const LiteralValue& Literal, const TypeDesc& Type)
        {
            if (!Literal.IsValid() || !Type.IsValid())
            {
                return false;
            }

            if (Type.GetKind() == TypeDesc::Kind::Boolean)
            {
                return Literal.Is<bool>();
            }

            if (Type.GetKind() == TypeDesc::Kind::Enum)
            {
                return Literal.Is<EnumLiteralValue>() &&
                    Literal.TryGet<EnumLiteralValue>()->GetEnumTypeIdentity() ==
                    Type.GetEnumTypeIdentity();
            }

            return false;
        }

        [[nodiscard]] inline bool IsInputTupleValid(
            const TypeDesc& SemanticType,
            GiaBackendTypeCode BackendTypeCode,
            GiaLiteralEncodingKind LiteralEncoding,
            GiaBackendInputValueSourceKind SourceKind,
            const std::optional<LiteralValue>& Literal)
        {
            if (!IsSupportedSemanticType(SemanticType) ||
                !BackendTypeCode.IsValid())
            {
                return false;
            }

            if (SemanticType.GetKind() == TypeDesc::Kind::Boolean)
            {
                if (BackendTypeCode.GetValue() != 5 ||
                    LiteralEncoding != GiaLiteralEncodingKind::Boolean)
                {
                    return false;
                }
            }
            else if (BackendTypeCode.GetValue() != 13 ||
                LiteralEncoding != GiaLiteralEncodingKind::Enum)
            {
                return false;
            }

            if (SourceKind == GiaBackendInputValueSourceKind::DataConnection)
            {
                return !Literal.has_value();
            }

            return (SourceKind == GiaBackendInputValueSourceKind::DescriptorDefault ||
                    SourceKind == GiaBackendInputValueSourceKind::ExplicitLiteral) &&
                Literal.has_value() &&
                IsLiteralCompatible(*Literal, SemanticType);
        }

        [[nodiscard]] inline PinOrderKeyType PinOrderKey(const GiaResolvedPin& Pin);

        [[nodiscard]] inline EndpointOrderKeyType EndpointOrderKey(const GiaResolvedPinEndpoint& Endpoint);

        [[nodiscard]] inline DataConnectionOrderKeyType DataConnectionOrderKey(const GiaResolvedDataConnection& Connection);

        [[nodiscard]] inline ControlConnectionOrderKeyType ControlConnectionOrderKey(const GiaResolvedControlConnection& Connection);
    }

    struct GiaResolvedInputValue
    {
        TypeDesc SemanticType;
        GiaBackendInputValueSourceKind SourceKind =
            GiaBackendInputValueSourceKind::DescriptorDefault;
        std::optional<LiteralValue> Literal;
        GiaBackendTypeCode BackendTypeCode;
        GiaLiteralEncodingKind LiteralEncoding = GiaLiteralEncodingKind::None;

        [[nodiscard]] bool IsValid() const
        {
            return GiaResolvedBackendGraphDetail::IsInputTupleValid(
                SemanticType,
                BackendTypeCode,
                LiteralEncoding,
                SourceKind,
                Literal
            );
        }

        bool operator==(const GiaResolvedInputValue&) const = default;
    };

    struct GiaResolvedPin
    {
        PinIndex SemanticPin;
        GiaPinKind Kind = GiaPinKind::Unknown;
        GiaPinIndex PrimaryIndex;
        GiaPinIndex SecondaryIndex;
        GiaBackendTypeCode BackendTypeCode;
        GiaLiteralEncodingKind LiteralEncoding = GiaLiteralEncodingKind::None;
        GiaPinEmissionPolicy EmissionPolicy = GiaPinEmissionPolicy::Omit;
        bool IsConnectable = false;
        std::optional<GiaResolvedInputValue> InputValue;

        [[nodiscard]] bool IsValid() const
        {
            if (!SemanticPin.IsValid() ||
                !GiaResolvedBackendGraphDetail::IsSupportedPinKind(Kind) ||
                !PrimaryIndex.IsValid() ||
                !SecondaryIndex.IsValid() ||
                !BackendTypeCode.IsValid() ||
                !GiaResolvedBackendGraphDetail::IsValidLiteralEncoding(
                    LiteralEncoding
                ) ||
                (EmissionPolicy != GiaPinEmissionPolicy::Emit &&
                    EmissionPolicy != GiaPinEmissionPolicy::Omit))
            {
                return false;
            }

            if ((Kind == GiaPinKind::InputFlow ||
                    Kind == GiaPinKind::OutputFlow) &&
                LiteralEncoding != GiaLiteralEncodingKind::None)
            {
                return false;
            }

            if (EmissionPolicy == GiaPinEmissionPolicy::Omit &&
                (IsConnectable || InputValue.has_value() ||
                    LiteralEncoding != GiaLiteralEncodingKind::None))
            {
                return false;
            }

            if (InputValue.has_value())
            {
                return Kind == GiaPinKind::InputParameter &&
                    EmissionPolicy == GiaPinEmissionPolicy::Emit &&
                    InputValue->BackendTypeCode == BackendTypeCode &&
                    InputValue->LiteralEncoding == LiteralEncoding &&
                    InputValue->IsValid();
            }

            return true;
        }

        bool operator==(const GiaResolvedPin&) const = default;
    };

    struct GiaResolvedPinEndpoint
    {
        GiaResolvedNodeIndex Node;
        GiaPinKind Kind = GiaPinKind::Unknown;
        GiaPinIndex PrimaryIndex;
        GiaPinIndex SecondaryIndex;

        [[nodiscard]] bool IsValid() const
        {
            return Node.IsValid() &&
                GiaResolvedBackendGraphDetail::IsSupportedPinKind(Kind) &&
                PrimaryIndex.IsValid() &&
                SecondaryIndex.IsValid();
        }

        bool operator==(const GiaResolvedPinEndpoint&) const = default;
    };

    struct GiaResolvedDataConnection
    {
        GiaResolvedPinEndpoint Source;
        GiaResolvedPinEndpoint Destination;
        TypeDesc ValueType;

        [[nodiscard]] bool IsValid() const
        {
            return Source.IsValid() && Destination.IsValid() &&
                ValueType.IsValid();
        }

        bool operator==(const GiaResolvedDataConnection&) const = default;
    };

    struct GiaResolvedControlConnection
    {
        GiaResolvedPinEndpoint Source;
        GiaResolvedPinEndpoint Destination;

        [[nodiscard]] bool IsValid() const
        {
            return Source.IsValid() && Destination.IsValid();
        }

        bool operator==(const GiaResolvedControlConnection&) const = default;
    };

    struct GiaResolvedNode
    {
        NodeInstanceId GraphNode;
        NodeDescriptorId Descriptor;
        ExternalNodeIdentity ExternalIdentity;
        std::optional<SourceProvenance> DescriptorProvenance;
        std::optional<SourceProvenance> MappingProvenance;
        GiaResolvedNodeIndex NodeIndex;
        GiaNodeGenericId GenericNodeIdentifier;
        std::optional<GiaNodeConcreteId> ConcreteNodeIdentifier;
        std::vector<GiaResolvedPin> Pins;
        GiaResolvedNodePosition Position;

        [[nodiscard]] bool IsValid() const
        {
            if (!GraphNode.IsValid() ||
                !Descriptor.IsValid() ||
                !ExternalIdentity.IsValid() ||
                !NodeIndex.IsValid() ||
                !GenericNodeIdentifier.IsValid() ||
                !ConcreteNodeIdentifier.has_value() ||
                !ConcreteNodeIdentifier->IsValid() ||
                (DescriptorProvenance.has_value() &&
                    !DescriptorProvenance->IsValid()) ||
                (MappingProvenance.has_value() &&
                    !MappingProvenance->IsValid()) ||
                !Position.IsValid())
            {
                return false;
            }

            for (std::size_t Index = 0U; Index < Pins.size(); ++Index)
            {
                if (!Pins[Index].IsValid())
                {
                    return false;
                }

                if (Index > 0U)
                {
                    if (!(GiaResolvedBackendGraphDetail::PinOrderKey(
                            Pins[Index - 1U]
                        ) < GiaResolvedBackendGraphDetail::PinOrderKey(
                            Pins[Index]
                        )))
                    {
                        return false;
                    }

                    if (Pins[Index - 1U].SemanticPin == Pins[Index].SemanticPin ||
                        (Pins[Index - 1U].Kind == Pins[Index].Kind &&
                            Pins[Index - 1U].PrimaryIndex == Pins[Index].PrimaryIndex &&
                            Pins[Index - 1U].SecondaryIndex == Pins[Index].SecondaryIndex))
                    {
                        return false;
                    }
                }

                for (std::size_t PriorIndex = 0U; PriorIndex < Index; ++PriorIndex)
                {
                    if (Pins[PriorIndex].SemanticPin == Pins[Index].SemanticPin)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool operator==(const GiaResolvedNode& Other) const
        {
            return GraphNode == Other.GraphNode &&
                Descriptor == Other.Descriptor &&
                ExternalIdentity == Other.ExternalIdentity &&
                NodeIndex == Other.NodeIndex &&
                GenericNodeIdentifier == Other.GenericNodeIdentifier &&
                ConcreteNodeIdentifier == Other.ConcreteNodeIdentifier &&
                Pins == Other.Pins &&
                Position == Other.Position;
        }
    };

    namespace GiaResolvedBackendGraphDetail
    {
        [[nodiscard]] inline PinOrderKeyType PinOrderKey(const GiaResolvedPin& Pin)
        {
            return std::tuple{
                static_cast<int>(Pin.Kind),
                Pin.PrimaryIndex.GetValue(),
                Pin.SecondaryIndex.GetValue(),
                Pin.SemanticPin.GetValue()
            };
        }

        [[nodiscard]] inline EndpointOrderKeyType EndpointOrderKey(const GiaResolvedPinEndpoint& Endpoint)
        {
            return std::tuple{
                Endpoint.Node.GetValue(),
                static_cast<int>(Endpoint.Kind),
                Endpoint.PrimaryIndex.GetValue(),
                Endpoint.SecondaryIndex.GetValue()
            };
        }

        [[nodiscard]] inline DataConnectionOrderKeyType DataConnectionOrderKey(const GiaResolvedDataConnection& Connection)
        {
            return std::tuple{
                EndpointOrderKey(Connection.Source),
                EndpointOrderKey(Connection.Destination)
            };
        }

        [[nodiscard]] inline ControlConnectionOrderKeyType ControlConnectionOrderKey(const GiaResolvedControlConnection& Connection)
        {
            return std::tuple{
                EndpointOrderKey(Connection.Source),
                EndpointOrderKey(Connection.Destination)
            };
        }

        [[nodiscard]] inline GiaResolvedPinEndpoint MakeEndpoint(const GiaResolvedNode& Node, const GiaResolvedPin& Pin)
        {
            return GiaResolvedPinEndpoint{
                Node.NodeIndex,
                Pin.Kind,
                Pin.PrimaryIndex,
                Pin.SecondaryIndex
            };
        }

        [[nodiscard]] inline bool SameEndpoint(const GiaResolvedPinEndpoint& Left, const GiaResolvedPinEndpoint& Right)
        {
            return Left == Right;
        }

        [[nodiscard]] inline bool IsStrictlyBefore(const GiaResolvedDataConnection& Left, const GiaResolvedDataConnection& Right)
        {
            return DataConnectionOrderKey(Left) < DataConnectionOrderKey(Right);
        }

        [[nodiscard]] inline bool IsStrictlyBefore(const GiaResolvedControlConnection& Left, const GiaResolvedControlConnection& Right)
        {
            return ControlConnectionOrderKey(Left) <
                ControlConnectionOrderKey(Right);
        }
    }

    class GiaGraphResolver;
    class GiaResolvedBackendGraph;

    namespace GiaResolvedBackendGraphDetail
    {
        [[nodiscard]] GiaResolvedBackendGraph CreateForTesting(
            GiaResolvedGraphHeader Header,
            std::vector<GiaResolvedNode> Nodes,
            std::vector<GiaResolvedDataConnection> DataConnections,
            std::vector<GiaResolvedControlConnection> ControlConnections);
    }

    class GiaResolvedBackendGraph final
    {
    public:
        GiaResolvedBackendGraph(const GiaResolvedBackendGraph&) = default;
        GiaResolvedBackendGraph(GiaResolvedBackendGraph&&) = default;
        GiaResolvedBackendGraph& operator=(const GiaResolvedBackendGraph&) = default;
        GiaResolvedBackendGraph& operator=(GiaResolvedBackendGraph&&) = default;

        [[nodiscard]] const GiaResolvedGraphHeader& GetHeader() const noexcept
        {
            return m_Header;
        }

        [[nodiscard]] const std::vector<GiaResolvedNode>& GetNodes() const noexcept
        {
            return m_Nodes;
        }

        [[nodiscard]] const std::vector<GiaResolvedDataConnection>& GetDataConnections() const noexcept
        {
            return m_DataConnections;
        }

        [[nodiscard]] const std::vector<GiaResolvedControlConnection>& GetControlConnections() const noexcept
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
                const GiaResolvedNode& Node = m_Nodes[Index];
                if (!Node.IsValid() ||
                    Node.GraphNode.GetValue() >
                        static_cast<std::uint64_t>(
                            std::numeric_limits<std::int32_t>::max()
                        ) ||
                    Node.GraphNode.GetValue() !=
                        static_cast<std::uint64_t>(Node.NodeIndex.GetValue()))
                {
                    return false;
                }

                if (Index > 0U &&
                    (m_Nodes[Index - 1U].GraphNode >= Node.GraphNode ||
                        m_Nodes[Index - 1U].NodeIndex >= Node.NodeIndex))
                {
                    return false;
                }
            }

            for (std::size_t Index = 0U;
                Index < m_DataConnections.size();
                ++Index)
            {
                const GiaResolvedDataConnection& Connection =
                    m_DataConnections[Index];
                if (!Connection.IsValid() ||
                    (Index > 0U &&
                        !GiaResolvedBackendGraphDetail::IsStrictlyBefore(
                            m_DataConnections[Index - 1U],
                            Connection
                        )) ||
                        !ValidateDataConnection(Connection))
                {
                    return false;
                }
            }

            for (std::size_t Index = 0U;
                Index < m_ControlConnections.size();
                ++Index)
            {
                const GiaResolvedControlConnection& Connection =
                    m_ControlConnections[Index];
                if (!Connection.IsValid() ||
                    (Index > 0U &&
                        !GiaResolvedBackendGraphDetail::IsStrictlyBefore(
                            m_ControlConnections[Index - 1U],
                            Connection
                        )) ||
                        !ValidateControlConnection(Connection))
                {
                    return false;
                }
            }

            for (const GiaResolvedNode& Node : m_Nodes)
            {
                for (const GiaResolvedPin& Pin : Node.Pins)
                {
                    const GiaResolvedPinEndpoint Endpoint =
                        GiaResolvedBackendGraphDetail::MakeEndpoint(Node, Pin);
                    const std::size_t DataCount = CountDataDestination(Endpoint);
                    const std::size_t ControlCount =
                        CountControlDestination(Endpoint);

                    if (Pin.EmissionPolicy == GiaPinEmissionPolicy::Omit)
                    {
                        if (Pin.InputValue.has_value() ||
                            DataCount != 0U ||
                            ControlCount != 0U)
                        {
                            return false;
                        }
                        continue;
                    }

                    if (Pin.Kind == GiaPinKind::InputParameter)
                    {
                        if (!Pin.InputValue.has_value())
                        {
                            return false;
                        }

                        if (Pin.InputValue->SourceKind ==
                            GiaBackendInputValueSourceKind::DataConnection)
                        {
                            if (DataCount != 1U || ControlCount != 0U)
                            {
                                return false;
                            }
                        }
                        else if (DataCount != 0U || ControlCount != 0U)
                        {
                            return false;
                        }
                    }
                    else if (Pin.Kind == GiaPinKind::InputFlow)
                    {
                        if (Pin.InputValue.has_value() || DataCount != 0U)
                        {
                            return false;
                        }
                    }
                    else if (Pin.InputValue.has_value() ||
                        DataCount != 0U || ControlCount != 0U)
                    {
                        return false;
                    }

                    if (ControlCount > 1U)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        bool operator==(const GiaResolvedBackendGraph& Other) const
        {
            return m_Header == Other.m_Header &&
                m_Nodes == Other.m_Nodes &&
                m_DataConnections == Other.m_DataConnections &&
                m_ControlConnections == Other.m_ControlConnections;
        }

    private:
        friend class GiaGraphResolver;

        friend GiaResolvedBackendGraph GiaResolvedBackendGraphDetail::CreateForTesting(
            GiaResolvedGraphHeader Header,
            std::vector<GiaResolvedNode> Nodes,
            std::vector<GiaResolvedDataConnection> DataConnections,
            std::vector<GiaResolvedControlConnection> ControlConnections
        );

        GiaResolvedBackendGraph(
            GiaResolvedGraphHeader Header,
            std::vector<GiaResolvedNode> Nodes,
            std::vector<GiaResolvedDataConnection> DataConnections,
            std::vector<GiaResolvedControlConnection> ControlConnections
        )
            : m_Header(std::move(Header))
            , m_Nodes(std::move(Nodes))
            , m_DataConnections(std::move(DataConnections))
            , m_ControlConnections(std::move(ControlConnections))
        {
        }

        [[nodiscard]] const GiaResolvedNode* FindNode(GiaResolvedNodeIndex NodeIndex) const
        {
            for (const GiaResolvedNode& Node : m_Nodes)
            {
                if (Node.NodeIndex == NodeIndex)
                {
                    return &Node;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static const GiaResolvedPin* FindPin(const GiaResolvedNode& Node, const GiaResolvedPinEndpoint& Endpoint)
        {
            for (const GiaResolvedPin& Pin : Node.Pins)
            {
                if (GiaResolvedBackendGraphDetail::MakeEndpoint(Node, Pin) ==
                    Endpoint)
                {
                    return &Pin;
                }
            }
            return nullptr;
        }

        [[nodiscard]] std::size_t CountDataDestination(const GiaResolvedPinEndpoint& Endpoint) const
        {
            return static_cast<std::size_t>(std::count_if(
                m_DataConnections.begin(),
                m_DataConnections.end(),
                [&Endpoint](const GiaResolvedDataConnection& Connection)
                {
                    return Connection.Destination == Endpoint;
                }
            ));
        }

        [[nodiscard]] std::size_t CountControlDestination(const GiaResolvedPinEndpoint& Endpoint) const
        {
            return static_cast<std::size_t>(std::count_if(
                m_ControlConnections.begin(),
                m_ControlConnections.end(),
                [&Endpoint](const GiaResolvedControlConnection& Connection)
                {
                    return Connection.Destination == Endpoint;
                }
            ));
        }

        [[nodiscard]] bool ValidateDataConnection(const GiaResolvedDataConnection& Connection) const
        {
            const GiaResolvedNode* Source = FindNode(Connection.Source.Node);
            const GiaResolvedNode* Destination =
                FindNode(Connection.Destination.Node);
            if (Source == nullptr || Destination == nullptr)
            {
                return false;
            }

            const GiaResolvedPin* SourcePin = FindPin(*Source, Connection.Source);
            const GiaResolvedPin* DestinationPin =
                FindPin(*Destination, Connection.Destination);
            if (SourcePin == nullptr || DestinationPin == nullptr ||
                SourcePin->Kind != GiaPinKind::OutputParameter ||
                DestinationPin->Kind != GiaPinKind::InputParameter ||
                SourcePin->EmissionPolicy != GiaPinEmissionPolicy::Emit ||
                DestinationPin->EmissionPolicy != GiaPinEmissionPolicy::Emit ||
                !DestinationPin->IsConnectable ||
                !DestinationPin->InputValue.has_value() ||
                DestinationPin->InputValue->SourceKind !=
                    GiaBackendInputValueSourceKind::DataConnection ||
                DestinationPin->InputValue->Literal.has_value() ||
                DestinationPin->InputValue->SemanticType != Connection.ValueType)
            {
                return false;
            }

            return true;
        }

        [[nodiscard]] bool ValidateControlConnection(const GiaResolvedControlConnection& Connection) const
        {
            const GiaResolvedNode* Source = FindNode(Connection.Source.Node);
            const GiaResolvedNode* Destination =
                FindNode(Connection.Destination.Node);
            if (Source == nullptr || Destination == nullptr)
            {
                return false;
            }

            const GiaResolvedPin* SourcePin = FindPin(*Source, Connection.Source);
            const GiaResolvedPin* DestinationPin =
                FindPin(*Destination, Connection.Destination);
            return SourcePin != nullptr && DestinationPin != nullptr &&
                SourcePin->Kind == GiaPinKind::OutputFlow &&
                DestinationPin->Kind == GiaPinKind::InputFlow &&
                SourcePin->EmissionPolicy == GiaPinEmissionPolicy::Emit &&
                DestinationPin->EmissionPolicy == GiaPinEmissionPolicy::Emit;
        }

        GiaResolvedGraphHeader m_Header;
        std::vector<GiaResolvedNode> m_Nodes;
        std::vector<GiaResolvedDataConnection> m_DataConnections;
        std::vector<GiaResolvedControlConnection> m_ControlConnections;
    };

    namespace GiaResolvedBackendGraphDetail
    {
        [[nodiscard]] inline GiaResolvedBackendGraph CreateForTesting(
            GiaResolvedGraphHeader Header,
            std::vector<GiaResolvedNode> Nodes,
            std::vector<GiaResolvedDataConnection> DataConnections,
            std::vector<GiaResolvedControlConnection> ControlConnections)
        {
            return GiaResolvedBackendGraph(
                std::move(Header),
                std::move(Nodes),
                std::move(DataConnections),
                std::move(ControlConnections)
            );
        }
    }
}
