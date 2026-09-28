#include "mpppch.h"

#include "MiliastraPlusPlusGiaProtobufEncoder.h"

#include <algorithm>
#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
#include <atomic>
#endif
#include <cstdint>
#include <cstring>
#include <expected>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

#include "gia.pb.h"

namespace MiliastraPlusPlus
{
    namespace
    {
        enum class EncodingStage
        {
            InputModelValidation,
            ProtobufConstruction,
            Serialization,
            DecodeValidation,
            ModelComparison
        };

        struct PendingDiagnostic
        {
            EncodingStage Stage;
            DiagnosticCode Code;
            std::string Message;
            std::optional<std::uint64_t> Node;
            std::optional<std::uint32_t> Pin;
            std::string ExternalIdentity;
            std::optional<SourceProvenance> Provenance;
        };

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
        std::atomic<GiaProtobufEncoderTestFailure> g_TestFailure = GiaProtobufEncoderTestFailure::None;
#endif

        [[nodiscard]] bool OptionalNumberLess(const std::optional<std::uint64_t>& Left, const std::optional<std::uint64_t>& Right) noexcept
        {
            if (!Left.has_value() && Right.has_value())
            {
                return true;
            }
            if (Left.has_value() && !Right.has_value())
            {
                return false;
            }
            return !Left.has_value() || *Left < *Right;
        }

        [[nodiscard]] bool OptionalPinLess(const std::optional<std::uint32_t>& Left, const std::optional<std::uint32_t>& Right) noexcept
        {
            if (!Left.has_value() && Right.has_value())
            {
                return true;
            }
            if (Left.has_value() && !Right.has_value())
            {
                return false;
            }
            return !Left.has_value() || *Left < *Right;
        }

        [[nodiscard]] DiagnosticCollection Materialize(std::vector<PendingDiagnostic> Diagnostics)
        {
            std::stable_sort(Diagnostics.begin(), Diagnostics.end(),
                [](const PendingDiagnostic& Left, const PendingDiagnostic& Right)
                {
                    if (Left.Stage != Right.Stage)
                    {
                        return static_cast<int>(Left.Stage) < static_cast<int>(Right.Stage);
                    }
                    if (Left.Code != Right.Code)
                    {
                        return static_cast<int>(Left.Code) < static_cast<int>(Right.Code);
                    }
                    if (Left.Node != Right.Node)
                    {
                        return OptionalNumberLess(Left.Node, Right.Node);
                    }
                    if (Left.Pin != Right.Pin)
                    {
                        return OptionalPinLess(Left.Pin, Right.Pin);
                    }
                    if (Left.ExternalIdentity != Right.ExternalIdentity)
                    {
                        return Left.ExternalIdentity < Right.ExternalIdentity;
                    }
                    if (Left.Message != Right.Message)
                    {
                        return Left.Message < Right.Message;
                    }
                    if (Left.Provenance.has_value() != Right.Provenance.has_value())
                    {
                        return !Left.Provenance.has_value();
                    }
                    if (Left.Provenance.has_value() && Left.Provenance != Right.Provenance)
                    {
                        return *Left.Provenance < *Right.Provenance;
                    }
                    return false;
                }
            );

            DiagnosticCollection Result;
            Result.reserve(Diagnostics.size());
            for (const PendingDiagnostic& Pending : Diagnostics)
            {
                Diagnostic DiagnosticValue{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = Pending.Code,
                    .Message = Pending.Message
                };
                if (Pending.Node.has_value() && *Pending.Node <= std::numeric_limits<std::uint32_t>::max())
                {
                    DiagnosticValue.SourceNodeIdentifier = NodeIdentifier(static_cast<std::uint32_t>(*Pending.Node));
                }
                if (Pending.Pin.has_value() && DiagnosticValue.SourceNodeIdentifier.has_value())
                {
                    DiagnosticValue.SourcePinReference = PinReference{*DiagnosticValue.SourceNodeIdentifier, PinIdentifier(*Pending.Pin)};
                }
                if (!Pending.ExternalIdentity.empty())
                {
                    DiagnosticValue.ExternalIdentityKey = Pending.ExternalIdentity;
                }
                DiagnosticValue.PrimarySourceProvenance = Pending.Provenance;
                Result.push_back(std::move(DiagnosticValue));
            }
            return Result;
        }

        [[nodiscard]] DiagnosticCollection Failure(
            EncodingStage Stage,
            DiagnosticCode Code,
            std::string Message,
            const GiaResolvedNode* Node = nullptr,
            const GiaResolvedPin* Pin = nullptr)
        {
            return Materialize({
                PendingDiagnostic{
                    Stage,
                    Code,
                    std::move(Message),
                    Node == nullptr ? std::nullopt : std::optional<std::uint64_t>(Node->GraphNode.GetValue()),
                    Pin == nullptr ? std::nullopt : std::optional<std::uint32_t>(Pin->SemanticPin.GetValue()),
                    Node == nullptr ? std::string{} : Node->ExternalIdentity.GetKey(),
                    Node == nullptr ? std::nullopt : Node->MappingProvenance
                }
            });
        }

        [[nodiscard]] bool FitsInt32(std::int64_t Value) noexcept
        {
            return Value >= std::numeric_limits<std::int32_t>::min() && Value <= std::numeric_limits<std::int32_t>::max();
        }

        [[nodiscard]] std::optional<NodePin_Index_Kind> ToProtoPinKind(GiaPinKind Kind) noexcept
        {
            switch (Kind)
            {
            case GiaPinKind::InputFlow:
                return NodePin::Index::InFlow;
            case GiaPinKind::OutputFlow:
                return NodePin::Index::OutFlow;
            case GiaPinKind::InputParameter:
                return NodePin::Index::InParam;
            case GiaPinKind::OutputParameter:
                return NodePin::Index::OutParam;
            case GiaPinKind::Unknown:
            case GiaPinKind::ClientExecution:
            case GiaPinKind::ClientSignal:
                return std::nullopt;
            }
            return std::nullopt;
        }

        [[nodiscard]] GraphNode* FindProtoNode(NodeGraph& Graph, GiaResolvedNodeIndex NodeIndex) noexcept
        {
            for (int Index = 0; Index < Graph.nodes_size(); ++Index)
            {
                if (Graph.nodes(Index).nodeindex() == NodeIndex.GetValue())
                {
                    return Graph.mutable_nodes(Index);
                }
            }
            return nullptr;
        }

        [[nodiscard]] const NodePin* FindProtoPin(const GraphNode& Node, const GiaResolvedPinEndpoint& Endpoint) noexcept
        {
            const auto ProtoKind = ToProtoPinKind(Endpoint.Kind);
            if (!ProtoKind.has_value())
            {
                return nullptr;
            }
            for (int Index = 0; Index < Node.pins_size(); ++Index)
            {
                const NodePin& Pin = Node.pins(Index);
                if (Pin.has_i1() &&
                    Pin.has_i2() &&
                    Pin.i1().kind() == *ProtoKind &&
                    Pin.i1().index() == Endpoint.PrimaryIndex.GetValue() &&
                    Pin.i2().kind() == *ProtoKind &&
                    Pin.i2().index() == Endpoint.SecondaryIndex.GetValue())
                {
                    return &Pin;
                }
            }
            return nullptr;
        }

        void SetNodeProperty(NodeProperty& Property, NodeGraph_Id_Class Class, NodeProperty_Type Type, NodeGraph_Id_Kind Kind, std::int32_t NodeId)
        {
            Property.set_class_(Class);
            Property.set_type(Type);
            Property.set_kind(Kind);
            Property.set_nodeid(NodeId);
        }

        [[nodiscard]] bool SetPinIndex(NodePin_Index& Index, GiaPinKind Kind, GiaPinIndex PinIndexValue)
        {
            const auto ProtoKind = ToProtoPinKind(Kind);
            if (!ProtoKind.has_value())
            {
                return false;
            }
            Index.set_kind(*ProtoKind);
            Index.set_index(PinIndexValue.GetValue());
            return true;
        }

        [[nodiscard]] bool SetTypedValue(VarBase& Value, const GiaResolvedPin& Pin, const std::optional<LiteralValue>& Literal)
        {
            if (Pin.BackendTypeCode.GetValue() != 5 && Pin.BackendTypeCode.GetValue() != 13)
            {
                return false;
            }

            Value.set_class_(VarBase::EnumBase);
            Value.set_alreadysetval(Literal.has_value());
            VarBase::ItemType* ItemType = Value.mutable_itemtype();
            ItemType->set_classbase(VarBase::ItemType::Client);
            ItemType->mutable_type_client()->set_type(static_cast<ClientVarType>(Pin.BackendTypeCode.GetValue()));

            if (!Literal.has_value())
            {
                return true;
            }
            if (Literal->Is<bool>())
            {
                if (Pin.BackendTypeCode.GetValue() != 5)
                {
                    return false;
                }
                Value.mutable_benum()->set_val(*Literal->TryGet<bool>() ? 1 : 0);
                return true;
            }
            if (Literal->Is<EnumLiteralValue>())
            {
                const EnumLiteralValue& EnumValue = *Literal->TryGet<EnumLiteralValue>();
                if (Pin.BackendTypeCode.GetValue() != 13 || !FitsInt32(EnumValue.GetValue()))
                {
                    return false;
                }
                Value.mutable_benum()->set_val(static_cast<std::int32_t>(EnumValue.GetValue()));
                return true;
            }
            return false;
        }

        [[nodiscard]] bool SetPin(NodePin& ProtoPin, const GiaResolvedPin& Pin)
        {
            if (Pin.EmissionPolicy != GiaPinEmissionPolicy::Emit ||
                !SetPinIndex(*ProtoPin.mutable_i1(), Pin.Kind, Pin.PrimaryIndex) ||
                !SetPinIndex(*ProtoPin.mutable_i2(), Pin.Kind, Pin.SecondaryIndex))
            {
                return false;
            }

            if (Pin.Kind == GiaPinKind::InputParameter || Pin.Kind == GiaPinKind::OutputParameter)
            {
                ProtoPin.set_type(Pin.BackendTypeCode.GetValue());
                std::optional<LiteralValue> Literal;
                if (Pin.InputValue.has_value())
                {
                    Literal = Pin.InputValue->Literal;
                }
                return SetTypedValue(*ProtoPin.mutable_value(), Pin, Literal);
            }

            return Pin.LiteralEncoding == GiaLiteralEncodingKind::None;
        }

        [[nodiscard]] bool SetGraphRoot(Root& RootValue, const GiaResolvedBackendGraph& Graph)
        {
            const GiaResolvedGraphHeader& Header = Graph.GetHeader();
            if (!FitsInt32(Header.GraphIdentifier.GetValue()))
            {
                return false;
            }

            GraphUnit* GraphUnitValue = RootValue.mutable_graph();
            GraphUnit_Id* GraphUnitId = GraphUnitValue->mutable_id();
            GraphUnitId->set_class_(GraphUnit::Id::Node);
            GraphUnitId->set_type(GraphUnit::Id::ClientGraph);
            GraphUnitId->set_id(static_cast<std::int32_t>(Header.GraphIdentifier.GetValue()));
            GraphUnitValue->set_name(Header.GraphName);
            GraphUnitValue->set_which(GraphUnit::BooleanFilter);

            NodeGraph* NodeGraphValue = GraphUnitValue->mutable_graph()->mutable_inner()->mutable_graph();
            NodeGraph_Id* NodeGraphId = NodeGraphValue->mutable_id();
            NodeGraphId->set_class_(NodeGraph::Id::UserDefined);
            NodeGraphId->set_type(NodeGraph::Id::BooleanFilter);
            NodeGraphId->set_kind(NodeGraph::Id::NodeGraph);
            NodeGraphId->set_id(Header.GraphIdentifier.GetValue());
            NodeGraphValue->set_name(Header.GraphName);
            NodeGraphValue->set_entryslotindex(Header.EntrySlotIndex);
            NodeGraphValue->set_evaluationinterval(Header.EvaluationInterval);

            for (const GiaResolvedNode& ResolvedNode : Graph.GetNodes())
            {
                GraphNode* ProtoNode = NodeGraphValue->add_nodes();
                ProtoNode->set_nodeindex(ResolvedNode.NodeIndex.GetValue());
                SetNodeProperty(*ProtoNode->mutable_genericid(), NodeGraph::Id::SystemDefined, NodeProperty::Filter, NodeGraph::Id::SysCall, ResolvedNode.GenericNodeIdentifier.GetValue());
                if (!ResolvedNode.ConcreteNodeIdentifier.has_value())
                {
                    return false;
                }
                SetNodeProperty(*ProtoNode->mutable_concreteid(), NodeGraph::Id::SystemDefined, NodeProperty::Filter, NodeGraph::Id::SysCall, ResolvedNode.ConcreteNodeIdentifier->GetValue());
                ProtoNode->set_x(ResolvedNode.Position.X);
                ProtoNode->set_y(ResolvedNode.Position.Y);
                for (const GiaResolvedPin& Pin : ResolvedNode.Pins)
                {
                    if (Pin.EmissionPolicy == GiaPinEmissionPolicy::Emit && !SetPin(*ProtoNode->add_pins(), Pin))
                    {
                        return false;
                    }
                }
            }

            for (const GiaResolvedDataConnection& Connection : Graph.GetDataConnections())
            {
                GraphNode* DestinationNode = FindProtoNode(*NodeGraphValue, Connection.Destination.Node);
                if (DestinationNode == nullptr)
                {
                    return false;
                }
                const GiaResolvedPinEndpoint& Destination = Connection.Destination;
                NodePin* DestinationPin = nullptr;
                for (int Index = 0; Index < DestinationNode->pins_size(); ++Index)
                {
                    NodePin* Candidate = DestinationNode->mutable_pins(Index);
                    if (Candidate->i1().kind() == NodePin::Index::InParam &&
                        Candidate->i1().index() == Destination.PrimaryIndex.GetValue() &&
                        Candidate->i2().index() == Destination.SecondaryIndex.GetValue())
                    {
                        DestinationPin = Candidate;
                        break;
                    }
                }
                if (DestinationPin == nullptr)
                {
                    return false;
                }
                NodeConnection* ProtoConnection = DestinationPin->add_connects();
                ProtoConnection->set_id(Connection.Source.Node.GetValue());
                if (!SetPinIndex(*ProtoConnection->mutable_connect(), Connection.Source.Kind, Connection.Source.PrimaryIndex) ||
                    !SetPinIndex(*ProtoConnection->mutable_connect2(), Connection.Source.Kind, Connection.Source.SecondaryIndex))
                {
                    return false;
                }
            }

            for (const GiaResolvedControlConnection& Connection : Graph.GetControlConnections())
            {
                GraphNode* SourceNode = FindProtoNode(*NodeGraphValue, Connection.Source.Node);
                if (SourceNode == nullptr)
                {
                    return false;
                }
                NodePin* SourcePin = nullptr;
                for (int Index = 0; Index < SourceNode->pins_size(); ++Index)
                {
                    NodePin* Candidate = SourceNode->mutable_pins(Index);
                    if (Candidate->i1().kind() == NodePin::Index::OutFlow &&
                        Candidate->i1().index() == Connection.Source.PrimaryIndex.GetValue() &&
                        Candidate->i2().index() == Connection.Source.SecondaryIndex.GetValue())
                    {
                        SourcePin = Candidate;
                        break;
                    }
                }
                if (SourcePin == nullptr)
                {
                    return false;
                }
                NodeConnection* ProtoConnection = SourcePin->add_connects();
                ProtoConnection->set_id(Connection.Destination.Node.GetValue());
                if (!SetPinIndex(*ProtoConnection->mutable_connect(), Connection.Destination.Kind, Connection.Destination.PrimaryIndex) ||
                    !SetPinIndex(*ProtoConnection->mutable_connect2(), Connection.Destination.Kind, Connection.Destination.SecondaryIndex))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool SamePinIndex(const NodePin_Index& Actual, GiaPinKind ExpectedKind, GiaPinIndex ExpectedIndex) noexcept
        {
            const auto ProtoKind = ToProtoPinKind(ExpectedKind);
            return ProtoKind.has_value() &&
                Actual.kind() == *ProtoKind &&
                Actual.index() == ExpectedIndex.GetValue() &&
                !Actual.has_nodeid();
        }

        [[nodiscard]] bool ValidateVarBase(const VarBase& Value, const GiaResolvedPin& Pin)
        {
            if (Value.class_() != VarBase::EnumBase ||
                !Value.has_itemtype() ||
                Value.itemtype().classbase() != VarBase::ItemType::Client ||
                !Value.itemtype().has_type_client() ||
                Value.itemtype().type_client().type() != static_cast<ClientVarType>(Pin.BackendTypeCode.GetValue()))
            {
                return false;
            }

            if (Pin.InputValue.has_value() && Pin.InputValue->Literal.has_value())
            {
                if (!Value.alreadysetval() || Value.baseValues_case() != VarBase::kBEnum)
                {
                    return false;
                }
                const LiteralValue& Literal = *Pin.InputValue->Literal;
                const std::int32_t Expected = Literal.Is<bool>()
                    ? (*Literal.TryGet<bool>() ? 1 : 0)
                    : static_cast<std::int32_t>(Literal.TryGet<EnumLiteralValue>()->GetValue());
                return Value.benum().val() == Expected;
            }

            return !Value.alreadysetval() && Value.baseValues_case() == VarBase::BASEVALUES_NOT_SET;
        }

        [[nodiscard]] bool ValidateDecodedStructure(const Root& RootValue)
        {
            if (!RootValue.has_graph() ||
                RootValue.accessories_size() != 0 ||
                !RootValue.filepath().empty() ||
                RootValue.has_modeflag() ||
                !RootValue.gameversion().empty())
            {
                return false;
            }

            const GraphUnit& GraphUnitValue = RootValue.graph();
            if (!GraphUnitValue.has_id() ||
                GraphUnitValue.relatedids_size() != 0 ||
                GraphUnitValue.id().class_() != GraphUnit::Id::Node ||
                GraphUnitValue.id().type() != GraphUnit::Id::ClientGraph ||
                GraphUnitValue.which() != GraphUnit::BooleanFilter ||
                GraphUnitValue.name().empty() ||
                GraphUnitValue.graphType_case() != GraphUnit::kGraph ||
                !GraphUnitValue.has_graph() ||
                !GraphUnitValue.graph().has_inner() ||
                !GraphUnitValue.graph().inner().has_graph())
            {
                return false;
            }

            const NodeGraph& NodeGraphValue = GraphUnitValue.graph().inner().graph();
            if (!NodeGraphValue.has_id() ||
                NodeGraphValue.id().class_() != NodeGraph::Id::UserDefined ||
                NodeGraphValue.id().type() != NodeGraph::Id::BooleanFilter ||
                NodeGraphValue.id().kind() != NodeGraph::Id::NodeGraph ||
                NodeGraphValue.name().empty() ||
                NodeGraphValue.nodes_size() <= 0 ||
                !NodeGraphValue.has_entryslotindex() ||
                !NodeGraphValue.has_evaluationinterval() ||
                NodeGraphValue.compositepins_size() != 0 ||
                NodeGraphValue.comments_size() != 0 ||
                NodeGraphValue.graphvalues_size() != 0 ||
                NodeGraphValue.affiliations_size() != 0)
            {
                return false;
            }

            for (int Index = 0; Index < NodeGraphValue.nodes_size(); ++Index)
            {
                const GraphNode& Node = NodeGraphValue.nodes(Index);
                if (!Node.has_genericid() ||
                    !Node.has_concreteid() ||
                    Node.genericid().class_() != NodeGraph::Id::SystemDefined ||
                    Node.genericid().type() != NodeProperty::Filter ||
                    Node.genericid().kind() != NodeGraph::Id::SysCall ||
                    Node.concreteid().class_() != NodeGraph::Id::SystemDefined ||
                    Node.concreteid().type() != NodeProperty::Filter ||
                    Node.concreteid().kind() != NodeGraph::Id::SysCall ||
                    Node.nodeindex() <= 0 ||
                    Node.has_comments() ||
                    Node.has_contextdeclaration() ||
                    Node.has_signalversion() ||
                    Node.usingstruct_size() != 0 ||
                    Node.has_statusnodeextension())
                {
                    return false;
                }
                for (int PinIndexValue = 0; PinIndexValue < Node.pins_size(); ++PinIndexValue)
                {
                    const NodePin& Pin = Node.pins(PinIndexValue);
                    if (!Pin.has_i1() ||
                        !Pin.has_i2() ||
                        Pin.i1().kind() != Pin.i2().kind() ||
                        !ToProtoPinKind(
                            Pin.i1().kind() == NodePin::Index::InFlow
                                ? GiaPinKind::InputFlow
                                : Pin.i1().kind() == NodePin::Index::OutFlow
                                    ? GiaPinKind::OutputFlow
                                    : Pin.i1().kind() == NodePin::Index::InParam
                                        ? GiaPinKind::InputParameter
                                        : Pin.i1().kind() == NodePin::Index::OutParam
                                            ? GiaPinKind::OutputParameter
                                            : GiaPinKind::Unknown).has_value() ||
                        Pin.has_clientexecnode() ||
                        Pin.has_compositepinindex())
                    {
                        return false;
                    }
                    const bool IsFlow = Pin.i1().kind() == NodePin::Index::InFlow || Pin.i1().kind() == NodePin::Index::OutFlow;
                    if ((IsFlow && Pin.has_value()) || (!IsFlow && !Pin.has_value()))
                    {
                        return false;
                    }
                    for (int ConnectionIndex = 0; ConnectionIndex < Pin.connects_size(); ++ConnectionIndex)
                    {
                        const NodeConnection& Connection = Pin.connects(ConnectionIndex);
                        if (!Connection.has_connect() ||
                            !Connection.has_connect2() ||
                            Connection.connect().kind() != Connection.connect2().kind())
                        {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        [[nodiscard]] bool CompareDecodedModel(const Root& RootValue, const GiaResolvedBackendGraph& Graph)
        {
            const GiaResolvedGraphHeader& Header = Graph.GetHeader();
            const GraphUnit& GraphUnitValue = RootValue.graph();
            const NodeGraph& NodeGraphValue = GraphUnitValue.graph().inner().graph();
            if (GraphUnitValue.id().id() != Header.GraphIdentifier.GetValue() ||
                GraphUnitValue.name() != Header.GraphName ||
                NodeGraphValue.id().id() != Header.GraphIdentifier.GetValue() ||
                NodeGraphValue.name() != Header.GraphName ||
                NodeGraphValue.entryslotindex() != Header.EntrySlotIndex ||
                NodeGraphValue.evaluationinterval() != Header.EvaluationInterval ||
                NodeGraphValue.nodes_size() != static_cast<int>(Graph.GetNodes().size()))
            {
                return false;
            }

            for (std::size_t NodeIndex = 0U; NodeIndex < Graph.GetNodes().size(); ++NodeIndex)
            {
                const GiaResolvedNode& ExpectedNode = Graph.GetNodes()[NodeIndex];
                const GraphNode& ActualNode = NodeGraphValue.nodes(static_cast<int>(NodeIndex));
                if (ActualNode.nodeindex() != ExpectedNode.NodeIndex.GetValue() ||
                    ActualNode.genericid().class_() != NodeGraph::Id::SystemDefined ||
                    ActualNode.genericid().type() != NodeProperty::Filter ||
                    ActualNode.genericid().kind() != NodeGraph::Id::SysCall ||
                    ActualNode.genericid().nodeid() != ExpectedNode.GenericNodeIdentifier.GetValue() ||
                    ActualNode.concreteid().class_() != NodeGraph::Id::SystemDefined ||
                    ActualNode.concreteid().type() != NodeProperty::Filter ||
                    ActualNode.concreteid().kind() != NodeGraph::Id::SysCall ||
                    ActualNode.concreteid().nodeid() != ExpectedNode.ConcreteNodeIdentifier->GetValue() ||
                    ActualNode.x() != ExpectedNode.Position.X ||
                    ActualNode.y() != ExpectedNode.Position.Y)
                {
                    return false;
                }

                std::size_t EmittedIndex = 0U;
                for (const GiaResolvedPin& ExpectedPin : ExpectedNode.Pins)
                {
                    if (ExpectedPin.EmissionPolicy == GiaPinEmissionPolicy::Omit)
                    {
                        continue;
                    }
                    if (EmittedIndex >= static_cast<std::size_t>(ActualNode.pins_size()))
                    {
                        return false;
                    }
                    const NodePin& ActualPin = ActualNode.pins(static_cast<int>(EmittedIndex++));
                    const auto ProtoKind = ToProtoPinKind(ExpectedPin.Kind);
                    const std::int32_t ExpectedType =
                        ExpectedPin.Kind == GiaPinKind::InputParameter || ExpectedPin.Kind == GiaPinKind::OutputParameter
                            ? ExpectedPin.BackendTypeCode.GetValue()
                            : 0;
                    if (!ProtoKind.has_value() ||
                        ActualPin.i1().kind() != *ProtoKind ||
                        ActualPin.i1().index() != ExpectedPin.PrimaryIndex.GetValue() ||
                        ActualPin.i2().kind() != *ProtoKind ||
                        ActualPin.i2().index() != ExpectedPin.SecondaryIndex.GetValue() ||
                        ActualPin.type() != ExpectedType)
                    {
                        return false;
                    }
                    if (ExpectedPin.Kind == GiaPinKind::InputFlow || ExpectedPin.Kind == GiaPinKind::OutputFlow)
                    {
                        if (ActualPin.has_value())
                        {
                            return false;
                        }
                    }
                    else if (!ActualPin.has_value() || !ValidateVarBase(ActualPin.value(), ExpectedPin))
                    {
                        return false;
                    }
                }
                if (EmittedIndex != static_cast<std::size_t>(ActualNode.pins_size()))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] bool CompareConnection(const NodeConnection& Actual, const GiaResolvedPinEndpoint& Peer, bool Control) noexcept
        {
            return Actual.id() == Peer.Node.GetValue() &&
                Actual.has_connect() &&
                Actual.has_connect2() &&
                SamePinIndex(Actual.connect(), Peer.Kind, Peer.PrimaryIndex) &&
                SamePinIndex(Actual.connect2(), Peer.Kind, Peer.SecondaryIndex) &&
                ((Control && Peer.Kind == GiaPinKind::InputFlow) ||
                    (!Control && Peer.Kind == GiaPinKind::OutputParameter));
        }

        [[nodiscard]] bool CompareConnections(const Root& RootValue, const GiaResolvedBackendGraph& Graph)
        {
            const NodeGraph& NodeGraphValue = RootValue.graph().graph().inner().graph();
            std::size_t ActualConnectionCount = 0U;
            for (int NodeIndex = 0; NodeIndex < NodeGraphValue.nodes_size(); ++NodeIndex)
            {
                const GraphNode& Node = NodeGraphValue.nodes(NodeIndex);
                for (int PinIndexValue = 0; PinIndexValue < Node.pins_size(); ++PinIndexValue)
                {
                    ActualConnectionCount += static_cast<std::size_t>(Node.pins(PinIndexValue).connects_size());
                }
            }
            if (ActualConnectionCount != Graph.GetDataConnections().size() + Graph.GetControlConnections().size())
            {
                return false;
            }

            for (const GiaResolvedDataConnection& Expected : Graph.GetDataConnections())
            {
                const GraphNode* DestinationNode = nullptr;
                for (int Index = 0; Index < NodeGraphValue.nodes_size(); ++Index)
                {
                    if (NodeGraphValue.nodes(Index).nodeindex() == Expected.Destination.Node.GetValue())
                    {
                        DestinationNode = &NodeGraphValue.nodes(Index);
                        break;
                    }
                }
                if (DestinationNode == nullptr)
                {
                    return false;
                }
                const NodePin* DestinationPin = FindProtoPin(*DestinationNode, Expected.Destination);
                if (DestinationPin == nullptr)
                {
                    return false;
                }
                std::vector<const GiaResolvedDataConnection*> ExpectedForPin;
                for (const GiaResolvedDataConnection& Candidate : Graph.GetDataConnections())
                {
                    if (Candidate.Destination == Expected.Destination)
                    {
                        ExpectedForPin.push_back(&Candidate);
                    }
                }
                if (DestinationPin->connects_size() != static_cast<int>(ExpectedForPin.size()))
                {
                    return false;
                }
                const auto ExpectedIterator = std::find(ExpectedForPin.begin(), ExpectedForPin.end(), &Expected);
                if (ExpectedIterator == ExpectedForPin.end())
                {
                    return false;
                }
                const std::size_t ExpectedIndex = static_cast<std::size_t>(ExpectedIterator - ExpectedForPin.begin());
                if (!CompareConnection(DestinationPin->connects(static_cast<int>(ExpectedIndex)), Expected.Source, false))
                {
                    return false;
                }
            }

            for (const GiaResolvedControlConnection& Expected : Graph.GetControlConnections())
            {
                const GraphNode* SourceNode = nullptr;
                for (int Index = 0; Index < NodeGraphValue.nodes_size(); ++Index)
                {
                    if (NodeGraphValue.nodes(Index).nodeindex() == Expected.Source.Node.GetValue())
                    {
                        SourceNode = &NodeGraphValue.nodes(Index);
                        break;
                    }
                }
                if (SourceNode == nullptr)
                {
                    return false;
                }
                const NodePin* SourcePin = FindProtoPin(*SourceNode, Expected.Source);
                if (SourcePin == nullptr)
                {
                    return false;
                }
                std::vector<const GiaResolvedControlConnection*> ExpectedForPin;
                for (const GiaResolvedControlConnection& Candidate : Graph.GetControlConnections())
                {
                    if (Candidate.Source == Expected.Source)
                    {
                        ExpectedForPin.push_back(&Candidate);
                    }
                }
                if (SourcePin->connects_size() != static_cast<int>(ExpectedForPin.size()))
                {
                    return false;
                }
                const auto ExpectedIterator = std::find(ExpectedForPin.begin(), ExpectedForPin.end(), &Expected);
                if (ExpectedIterator == ExpectedForPin.end())
                {
                    return false;
                }
                const std::size_t ExpectedIndex = static_cast<std::size_t>(ExpectedIterator - ExpectedForPin.begin());
                if (!CompareConnection(SourcePin->connects(static_cast<int>(ExpectedIndex)), Expected.Destination, true))
                {
                    return false;
                }
            }
            return true;
        }
    }

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
    void GiaProtobufEncoderDetail::SetTestFailure(GiaProtobufEncoderTestFailure Failure) noexcept
    {
        g_TestFailure.store(Failure);
    }

    std::expected<GiaProtobufEncoderDetail::GiaProtobufTestSnapshot, DiagnosticCollection> GiaProtobufEncoderDetail::InspectForTesting(std::span<const std::byte> Bytes)
    {
        if (Bytes.size() > std::numeric_limits<int>::max())
        {
            return std::unexpected(Failure(EncodingStage::DecodeValidation, DiagnosticCode::GiaProtobufDecodeFailure, "The test protobuf inspection input is too large."));
        }

        Root RootValue;
        if (!RootValue.ParseFromArray(reinterpret_cast<const char*>(Bytes.data()), static_cast<int>(Bytes.size())))
        {
            return std::unexpected(Failure(EncodingStage::DecodeValidation, DiagnosticCode::GiaProtobufDecodeFailure, "The test protobuf inspection input could not be decoded."));
        }

        GiaProtobufTestSnapshot Result;
        Result.HasGraph = RootValue.has_graph();
        Result.FilePath = RootValue.filepath();
        Result.GameVersion = RootValue.gameversion();
        Result.HasModeFlag = RootValue.has_modeflag();
        Result.Accessories = static_cast<std::size_t>(RootValue.accessories_size());

        if (!Result.HasGraph)
        {
            return Result;
        }

        const GraphUnit& GraphUnitValue = RootValue.graph();
        if (GraphUnitValue.has_id())
        {
            Result.GraphClass = static_cast<std::int32_t>(GraphUnitValue.id().class_());
            Result.GraphType = static_cast<std::int32_t>(GraphUnitValue.id().type());
            Result.GraphUnitIdentifier = GraphUnitValue.id().id();
        }
        Result.GraphName = GraphUnitValue.name();
        Result.GraphWhich = static_cast<std::int32_t>(GraphUnitValue.which());
        Result.HasInner = GraphUnitValue.has_graph() && GraphUnitValue.graph().has_inner();

        if (!Result.HasInner)
        {
            return Result;
        }

        const NodeGraphWrapper& Wrapper = GraphUnitValue.graph();
        Result.HasInner = Wrapper.has_inner();
        if (!Result.HasInner)
        {
            return Result;
        }

        const NodeGraphWrapper_InnerWrapper& Inner = Wrapper.inner();
        Result.HasNodeGraph = Inner.has_graph();
        if (!Result.HasNodeGraph)
        {
            return Result;
        }

        const NodeGraph& NodeGraphValue = Inner.graph();
        if (NodeGraphValue.has_id())
        {
            Result.NodeGraphClass = static_cast<std::int32_t>(NodeGraphValue.id().class_());
            Result.NodeGraphType = static_cast<std::int32_t>(NodeGraphValue.id().type());
            Result.NodeGraphKind = static_cast<std::int32_t>(NodeGraphValue.id().kind());
            Result.NodeGraphIdentifier = NodeGraphValue.id().id();
        }
        Result.NodeGraphName = NodeGraphValue.name();
        Result.HasEntrySlot = NodeGraphValue.has_entryslotindex();
        Result.EntrySlot = NodeGraphValue.entryslotindex();
        Result.HasEvaluationInterval = NodeGraphValue.has_evaluationinterval();
        Result.EvaluationInterval = NodeGraphValue.evaluationinterval();
        Result.CompositePins = static_cast<std::size_t>(NodeGraphValue.compositepins_size());
        Result.Comments = static_cast<std::size_t>(NodeGraphValue.comments_size());
        Result.GraphValues = static_cast<std::size_t>(NodeGraphValue.graphvalues_size());
        Result.Affiliations = static_cast<std::size_t>(NodeGraphValue.affiliations_size());

        Result.Nodes.reserve(static_cast<std::size_t>(NodeGraphValue.nodes_size()));
        for (int NodeIndex = 0; NodeIndex < NodeGraphValue.nodes_size(); ++NodeIndex)
        {
            const GraphNode& ProtoNode = NodeGraphValue.nodes(NodeIndex);
            GiaProtobufTestNode& Node = Result.Nodes.emplace_back();
            Node.NodeIndex = ProtoNode.nodeindex();
            if (ProtoNode.has_genericid())
            {
                Node.GenericClass = static_cast<std::int32_t>(ProtoNode.genericid().class_());
                Node.GenericType = static_cast<std::int32_t>(ProtoNode.genericid().type());
                Node.GenericKind = static_cast<std::int32_t>(ProtoNode.genericid().kind());
                Node.GenericNodeId = ProtoNode.genericid().nodeid();
            }
            Node.HasConcrete = ProtoNode.has_concreteid();
            if (Node.HasConcrete)
            {
                Node.ConcreteClass = static_cast<std::int32_t>(ProtoNode.concreteid().class_());
                Node.ConcreteType = static_cast<std::int32_t>(ProtoNode.concreteid().type());
                Node.ConcreteKind = static_cast<std::int32_t>(ProtoNode.concreteid().kind());
                Node.ConcreteNodeId = ProtoNode.concreteid().nodeid();
            }
            Node.X = ProtoNode.x();
            Node.Y = ProtoNode.y();
            Node.Pins.reserve(static_cast<std::size_t>(ProtoNode.pins_size()));
            for (int PinIndex = 0; PinIndex < ProtoNode.pins_size(); ++PinIndex)
            {
                const NodePin& ProtoPin = ProtoNode.pins(PinIndex);
                GiaProtobufTestPin& Pin = Node.Pins.emplace_back();
                if (ProtoPin.has_i1())
                {
                    Pin.Kind = static_cast<std::int32_t>(ProtoPin.i1().kind());
                    Pin.PrimaryIndex = ProtoPin.i1().index();
                }
                if (ProtoPin.has_i2())
                {
                    Pin.SecondaryKind = static_cast<std::int32_t>(ProtoPin.i2().kind());
                    Pin.SecondaryIndex = ProtoPin.i2().index();
                }
                Pin.TypeCode = ProtoPin.type();
                Pin.HasValue = ProtoPin.has_value();
                if (Pin.HasValue)
                {
                    const VarBase& Value = ProtoPin.value();
                    Pin.AlreadySetValue = Value.alreadysetval();
                    if (Value.has_itemtype() && Value.itemtype().has_type_client())
                    {
                        Pin.ValueType = static_cast<std::int32_t>(Value.itemtype().type_client().type());
                    }
                    Pin.HasLiteral = Value.baseValues_case() == VarBase::kBEnum;
                    if (Pin.HasLiteral)
                    {
                        Pin.LiteralValue = Value.benum().val();
                    }
                }
                Pin.Connections.reserve(static_cast<std::size_t>(ProtoPin.connects_size()));
                for (int ConnectionIndex = 0; ConnectionIndex < ProtoPin.connects_size(); ++ConnectionIndex)
                {
                    const NodeConnection& ProtoConnection = ProtoPin.connects(ConnectionIndex);
                    GiaProtobufTestConnection& Connection = Pin.Connections.emplace_back();
                    Connection.PeerNodeIndex = ProtoConnection.id();
                    if (ProtoConnection.has_connect())
                    {
                        Connection.ConnectKind = static_cast<std::int32_t>(ProtoConnection.connect().kind());
                        Connection.ConnectIndex = ProtoConnection.connect().index();
                    }
                    if (ProtoConnection.has_connect2())
                    {
                        Connection.Connect2Kind = static_cast<std::int32_t>(ProtoConnection.connect2().kind());
                        Connection.Connect2Index = ProtoConnection.connect2().index();
                    }
                }
            }
        }
        return Result;
    }
#endif

    std::expected<GiaEncodedProtobuf, DiagnosticCollection> GiaProtobufEncoder::Encode(const GiaResolvedBackendGraph& Graph)
    {
#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
        const GiaProtobufEncoderTestFailure TestFailure = g_TestFailure.exchange(GiaProtobufEncoderTestFailure::None);
#endif

        if (!Graph.IsValid())
        {
            return std::unexpected(Failure(EncodingStage::InputModelValidation, DiagnosticCode::InvalidGiaEncodingInput, "The resolved GIA backend graph is invalid."));
        }
        if (!FitsInt32(Graph.GetHeader().GraphIdentifier.GetValue()))
        {
            return std::unexpected(Failure(EncodingStage::InputModelValidation, DiagnosticCode::InvalidGiaEncodingInput, "The graph identifier cannot be represented by the GIA int32 field."));
        }

        Root RootValue;
        if (!SetGraphRoot(RootValue, Graph))
        {
            return std::unexpected(Failure(EncodingStage::ProtobufConstruction, DiagnosticCode::InvalidGiaProtobufConstruction, "The resolved graph could not be materialized as the bounded GIA Root structure."));
        }

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
        if (TestFailure == GiaProtobufEncoderTestFailure::Serialization)
        {
            return std::unexpected(Failure(EncodingStage::Serialization, DiagnosticCode::GiaProtobufSerializationFailure, "Injected serialization failure."));
        }
#endif

        std::string Serialized;
        bool SerializationSucceeded = false;
        {
            google::protobuf::io::StringOutputStream StringStream(&Serialized);
            google::protobuf::io::CodedOutputStream CodedStream(&StringStream);
            CodedStream.SetSerializationDeterministic(true);
            SerializationSucceeded = RootValue.SerializeToCodedStream(&CodedStream) && !CodedStream.HadError();
        }
        if (!SerializationSucceeded)
        {
            return std::unexpected(Failure(EncodingStage::Serialization, DiagnosticCode::GiaProtobufSerializationFailure, "The GIA Root protobuf could not be serialized."));
        }

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
        if (TestFailure == GiaProtobufEncoderTestFailure::Decode)
        {
            return std::unexpected(Failure(EncodingStage::DecodeValidation, DiagnosticCode::GiaProtobufDecodeFailure, "Injected decode failure."));
        }
#endif

        Root Decoded;
        if (Serialized.size() > std::numeric_limits<int>::max() ||
            !Decoded.ParseFromArray(Serialized.data(), static_cast<int>(Serialized.size())))
        {
            return std::unexpected(Failure(EncodingStage::DecodeValidation, DiagnosticCode::GiaProtobufDecodeFailure, "The serialized GIA Root protobuf could not be decoded."));
        }

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
        if (TestFailure == GiaProtobufEncoderTestFailure::MalformedDecodedStructure)
        {
            Decoded.mutable_graph()->clear_graph();
        }
        else if (TestFailure == GiaProtobufEncoderTestFailure::ModelMismatch)
        {
            Decoded.mutable_graph()->set_name("injected-mismatch");
        }
#endif

        if (!ValidateDecodedStructure(Decoded))
        {
            return std::unexpected(Failure(EncodingStage::DecodeValidation, DiagnosticCode::InvalidGiaDecodedProtobuf, "The decoded GIA protobuf violates the bounded structural contract."));
        }
        if (!CompareDecodedModel(Decoded, Graph) || !CompareConnections(Decoded, Graph))
        {
            return std::unexpected(Failure(EncodingStage::ModelComparison, DiagnosticCode::GiaProtobufModelMismatch, "The decoded GIA protobuf does not match the resolved backend model."));
        }

        std::vector<std::byte> Bytes(Serialized.size());
        if (!Serialized.empty())
        {
            std::memcpy(Bytes.data(), Serialized.data(), Serialized.size());
        }
        return GiaEncodedProtobuf(std::move(Bytes));
    }
}
