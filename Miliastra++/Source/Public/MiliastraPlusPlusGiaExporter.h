#pragma once

#include <cstddef>
#include <expected>
#include <span>
#include <utility>
#include <vector>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusGiaExportContext.h"
#include "MiliastraPlusPlusGraphIR.h"

namespace MiliastraPlusPlus
{
    /// Owns a complete deterministic in-memory GIA byte sequence.
    class GiaEncodedGia final
    {
    public:
        GiaEncodedGia(const GiaEncodedGia&) = default;
        GiaEncodedGia(GiaEncodedGia&&) noexcept = default;
        GiaEncodedGia& operator=(const GiaEncodedGia&) = default;
        GiaEncodedGia& operator=(GiaEncodedGia&&) noexcept = default;
        ~GiaEncodedGia() = default;

        [[nodiscard]] std::span<const std::byte> GetBytes() const noexcept
        {
            return m_Bytes;
        }

    private:
        friend class GiaExporter;

        explicit GiaEncodedGia(std::vector<std::byte> Bytes)
            : m_Bytes(std::move(Bytes))
        {
        }

        std::vector<std::byte> m_Bytes;
    };

    class GiaExporter final
    {
    public:
        [[nodiscard]] static std::expected<GiaEncodedGia, DiagnosticCollection> Export(const GraphIR& Graph, const GiaExportContext& Context);
    };
}
