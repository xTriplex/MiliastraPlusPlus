#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusGiaResolvedBackendGraph.h"

namespace MiliastraPlusPlus
{
    class GiaEncodedProtobuf final
    {
    public:
        GiaEncodedProtobuf(const GiaEncodedProtobuf&) = default;
        GiaEncodedProtobuf(GiaEncodedProtobuf&&) noexcept = default;
        GiaEncodedProtobuf& operator=(const GiaEncodedProtobuf&) = default;
        GiaEncodedProtobuf& operator=(GiaEncodedProtobuf&&) noexcept = default;

        [[nodiscard]] std::span<const std::byte> GetBytes() const noexcept
        {
            return m_Bytes;
        }

    private:
        friend class GiaProtobufEncoder;

        explicit GiaEncodedProtobuf(std::vector<std::byte> Bytes)
            : m_Bytes(std::move(Bytes))
        {
        }

        std::vector<std::byte> m_Bytes;
    };

#if defined(MILIASTRA_PHASE6_TEST_ACCESS)
    enum class GiaProtobufEncoderTestFailure
    {
        None,
        Serialization,
        Decode,
        MalformedDecodedStructure,
        ModelMismatch
    };

    namespace GiaProtobufEncoderDetail
    {
        struct GiaProtobufTestConnection final
        {
            std::int32_t PeerNodeIndex = 0;
            std::int32_t ConnectKind = 0;
            std::int32_t ConnectIndex = 0;
            std::int32_t Connect2Kind = 0;
            std::int32_t Connect2Index = 0;

            bool operator==(const GiaProtobufTestConnection&) const = default;
        };

        struct GiaProtobufTestPin final
        {
            std::int32_t Kind = 0;
            std::int32_t PrimaryIndex = 0;
            std::int32_t SecondaryKind = 0;
            std::int32_t SecondaryIndex = 0;
            std::int32_t TypeCode = 0;
            bool HasValue = false;
            bool AlreadySetValue = false;
            bool HasLiteral = false;
            std::int64_t LiteralValue = 0;
            std::int32_t ValueType = 0;
            std::vector<GiaProtobufTestConnection> Connections;

            bool operator==(const GiaProtobufTestPin&) const = default;
        };

        struct GiaProtobufTestNode final
        {
            std::int32_t NodeIndex = 0;
            std::int32_t GenericClass = 0;
            std::int32_t GenericType = 0;
            std::int32_t GenericKind = 0;
            std::int64_t GenericNodeId = 0;
            bool HasConcrete = false;
            std::int32_t ConcreteClass = 0;
            std::int32_t ConcreteType = 0;
            std::int32_t ConcreteKind = 0;
            std::int64_t ConcreteNodeId = 0;
            float X = 0.0F;
            float Y = 0.0F;
            std::vector<GiaProtobufTestPin> Pins;

            bool operator==(const GiaProtobufTestNode&) const = default;
        };

        struct GiaProtobufTestSnapshot final
        {
            bool HasGraph = false;
            std::int32_t GraphClass = 0;
            std::int32_t GraphType = 0;
            std::int32_t GraphUnitIdentifier = 0;
            std::string GraphName;
            std::int32_t GraphWhich = 0;
            bool HasModeFlag = false;
            bool HasInner = false;
            bool HasNodeGraph = false;
            std::int32_t NodeGraphClass = 0;
            std::int32_t NodeGraphType = 0;
            std::int32_t NodeGraphKind = 0;
            std::int64_t NodeGraphIdentifier = 0;
            std::string NodeGraphName;
            bool HasEntrySlot = false;
            std::int32_t EntrySlot = 0;
            bool HasEvaluationInterval = false;
            float EvaluationInterval = 0.0F;
            std::string FilePath;
            std::string GameVersion;
            std::size_t Accessories = 0;
            std::size_t CompositePins = 0;
            std::size_t Comments = 0;
            std::size_t GraphValues = 0;
            std::size_t Affiliations = 0;
            std::vector<GiaProtobufTestNode> Nodes;

            bool operator==(const GiaProtobufTestSnapshot&) const = default;
        };

        void SetTestFailure(GiaProtobufEncoderTestFailure Failure) noexcept;

        [[nodiscard]] std::expected<GiaProtobufTestSnapshot, DiagnosticCollection> InspectForTesting(std::span<const std::byte> Bytes);
    }
#endif

    class GiaProtobufEncoder final
    {
    public:
        [[nodiscard]] static std::expected<GiaEncodedProtobuf, DiagnosticCollection> Encode(const GiaResolvedBackendGraph& Graph);
    };
}
