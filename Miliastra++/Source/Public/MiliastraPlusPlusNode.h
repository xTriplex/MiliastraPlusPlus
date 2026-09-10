#pragma once

#include <expected>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusDiagnostics.h"
#include "MiliastraPlusPlusPin.h"

namespace MiliastraPlusPlus
{
    class Node
    {
    public:
        Node(NodeIdentifier Identifier, std::string Name)
            : m_Identifier(Identifier)
            , m_Name(std::move(Name))
        {
        }

        Node(const Node& OtherNode)
            : m_Identifier(OtherNode.m_Identifier)
            , m_Name(OtherNode.m_Name)
        {
            m_Pins.reserve(OtherNode.m_Pins.size());
            for (const std::unique_ptr<Pin>& OtherPin : OtherNode.m_Pins)
            {
                m_Pins.push_back(std::make_unique<Pin>(*OtherPin));
            }
        }

        Node& operator=(const Node&) = delete;
        Node(Node&&) noexcept = default;
        Node& operator=(Node&&) noexcept = default;
        virtual ~Node() = default;

        [[nodiscard]] virtual std::unique_ptr<Node> Clone() const
        {
            return std::make_unique<Node>(*this);
        }

        [[nodiscard]] NodeIdentifier GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] const std::string& GetName() const
        {
            return m_Name;
        }

        [[nodiscard]] std::expected<void, Diagnostic> AddPin(std::unique_ptr<Pin> NodePin)
        {
            if (!NodePin)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::NullPin,
                    .Message = "A node cannot own a null pin.",
                    .SourceNodeIdentifier = m_Identifier
                });
            }

            if (GetPinByIdentifier(NodePin->GetIdentifier()) != nullptr)
            {
                return std::unexpected(Diagnostic{
                    .Severity = DiagnosticSeverity::Error,
                    .Code = DiagnosticCode::DuplicatePinIdentifier,
                    .Message = "A node cannot contain two pins with the same local pin identifier.",
                    .SourceNodeIdentifier = m_Identifier,
                    .SourcePinReference = PinReference{ m_Identifier, NodePin->GetIdentifier() }
                });
            }

            m_Pins.push_back(std::move(NodePin));
            return {};
        }

        [[nodiscard]] const std::vector<std::unique_ptr<Pin>>& GetPins() const
        {
            return m_Pins;
        }

        [[nodiscard]] Pin* GetPinByIdentifier(PinIdentifier Identifier)
        {
            for (const std::unique_ptr<Pin>& NodePin : m_Pins)
            {
                if (NodePin->GetIdentifier() == Identifier)
                {
                    return NodePin.get();
                }
            }

            return nullptr;
        }

        [[nodiscard]] const Pin* GetPinByIdentifier(PinIdentifier Identifier) const
        {
            for (const std::unique_ptr<Pin>& NodePin : m_Pins)
            {
                if (NodePin->GetIdentifier() == Identifier)
                {
                    return NodePin.get();
                }
            }

            return nullptr;
        }

        [[nodiscard]] virtual nlohmann::json Serialize() const
        {
            nlohmann::json SerializedPins = nlohmann::json::array();
            for (const std::unique_ptr<Pin>& NodePin : m_Pins)
            {
                SerializedPins.push_back(NodePin->Serialize());
            }

            return {
                { "Id", m_Identifier.GetValue() },
                { "Name", m_Name },
                { "Pins", SerializedPins }
            };
        }

    private:
        NodeIdentifier m_Identifier;
        std::string m_Name;
        std::vector<std::unique_ptr<Pin>> m_Pins;
    };
}
