#include "mpppch.h"

#include "MiliastraPlusPlusGiaExporter.h"

#include <algorithm>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusGiaGraphLowerer.h"
#include "MiliastraPlusPlusGiaGraphResolver.h"
#include "MiliastraPlusPlusGiaProtobufEncoder.h"

namespace MiliastraPlusPlus
{
    namespace
    {
        constexpr std::size_t GiaHeaderSize = 20U;
        constexpr std::size_t GiaTailSize = 4U;
        constexpr std::size_t GiaOverheadSize = GiaHeaderSize + GiaTailSize;
        constexpr std::uint32_t GiaSchemaVersion = 1U;
        constexpr std::uint32_t GiaHeadTag = 0x0326U;
        constexpr std::uint32_t GiaFileType = 3U;
        constexpr std::uint32_t GiaTailTag = 0x0679U;

        [[nodiscard]] Diagnostic MakeFramingInvariantDiagnostic()
        {
            return Diagnostic{
                .Severity = DiagnosticSeverity::Error,
                .Code = DiagnosticCode::InvalidGiaEncodingInput,
                .Message = "The successful P6.4 protobuf payload violates the bounded GIA framing size invariant."
            };
        }

        void WriteBigEndianUint32(std::span<std::byte> Destination, std::size_t Offset, std::uint32_t Value)
        {
            Destination[Offset] = static_cast<std::byte>((Value >> 24U) & 0xFFU);
            Destination[Offset + 1U] = static_cast<std::byte>((Value >> 16U) & 0xFFU);
            Destination[Offset + 2U] = static_cast<std::byte>((Value >> 8U) & 0xFFU);
            Destination[Offset + 3U] = static_cast<std::byte>(Value & 0xFFU);
        }

        [[nodiscard]] std::expected<std::vector<std::byte>, DiagnosticCollection> FrameGiaBytes(std::span<const std::byte> Payload)
        {
            constexpr std::uint64_t MaximumUint32 = std::numeric_limits<std::uint32_t>::max();
            constexpr std::uint64_t MaximumPayloadSize = MaximumUint32 - GiaOverheadSize;

            const std::uint64_t PayloadSize = static_cast<std::uint64_t>(Payload.size());
            if (PayloadSize > static_cast<std::uint64_t>(std::numeric_limits<int>::max()) || PayloadSize > MaximumPayloadSize)
            {
                return std::unexpected(DiagnosticCollection{MakeFramingInvariantDiagnostic()});
            }

            const std::uint64_t TotalSize = PayloadSize + GiaOverheadSize;
            const std::uint32_t ProtoSize = static_cast<std::uint32_t>(PayloadSize);
            const std::uint32_t LeftSize = static_cast<std::uint32_t>(PayloadSize + GiaHeaderSize);
            std::vector<std::byte> Bytes(static_cast<std::size_t>(TotalSize));
            const std::span<std::byte> WritableBytes(Bytes);

            WriteBigEndianUint32(WritableBytes, 0U, LeftSize);
            WriteBigEndianUint32(WritableBytes, 4U, GiaSchemaVersion);
            WriteBigEndianUint32(WritableBytes, 8U, GiaHeadTag);
            WriteBigEndianUint32(WritableBytes, 12U, GiaFileType);
            WriteBigEndianUint32(WritableBytes, 16U, ProtoSize);
            std::copy(Payload.begin(), Payload.end(), Bytes.begin() + static_cast<std::ptrdiff_t>(GiaHeaderSize));
            WriteBigEndianUint32(WritableBytes, GiaHeaderSize + static_cast<std::size_t>(PayloadSize), GiaTailTag);

            return Bytes;
        }
    }

    std::expected<GiaEncodedGia, DiagnosticCollection> GiaExporter::Export(const GraphIR& Graph, const GiaExportContext& Context)
    {
        const auto BackendGraph = GiaGraphLowerer::Lower(Graph, Context);
        if (!BackendGraph.has_value())
        {
            return std::unexpected(BackendGraph.error());
        }

        const auto ResolvedGraph = GiaGraphResolver::Resolve(*BackendGraph);
        if (!ResolvedGraph.has_value())
        {
            return std::unexpected(ResolvedGraph.error());
        }

        const auto EncodedProtobuf = GiaProtobufEncoder::Encode(*ResolvedGraph);
        if (!EncodedProtobuf.has_value())
        {
            return std::unexpected(EncodedProtobuf.error());
        }

        const auto FramedBytes = FrameGiaBytes(EncodedProtobuf->GetBytes());
        if (!FramedBytes.has_value())
        {
            return std::unexpected(FramedBytes.error());
        }

        return GiaEncodedGia(std::move(*FramedBytes));
    }
}
