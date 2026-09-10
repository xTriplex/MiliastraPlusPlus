#pragma once

#include <utility>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusIdentifiers.h"

namespace MiliastraPlusPlus
{
    class Link
    {
    public:
        Link(
            LinkIdentifier Identifier,
            PinReference SourcePinReference,
            PinReference DestinationPinReference
        )
            : m_Identifier(Identifier)
            , m_SourcePinReference(std::move(SourcePinReference))
            , m_DestinationPinReference(std::move(DestinationPinReference))
        {
        }

        [[nodiscard]] LinkIdentifier GetIdentifier() const
        {
            return m_Identifier;
        }

        [[nodiscard]] const PinReference& GetSourcePinReference() const
        {
            return m_SourcePinReference;
        }

        [[nodiscard]] const PinReference& GetDestinationPinReference() const
        {
            return m_DestinationPinReference;
        }

        [[nodiscard]] nlohmann::json Serialize() const
        {
            return {
                { "Id", m_Identifier.GetValue() },
                {
                    "Source Pin",
                    {
                        { "Node Id", m_SourcePinReference.OwningNodeIdentifier.GetValue() },
                        { "Pin Id", m_SourcePinReference.LocalPinIdentifier.GetValue() }
                    }
                },
                {
                    "Destination Pin",
                    {
                        { "Node Id", m_DestinationPinReference.OwningNodeIdentifier.GetValue() },
                        { "Pin Id", m_DestinationPinReference.LocalPinIdentifier.GetValue() }
                    }
                }
            };
        }

    private:
        LinkIdentifier m_Identifier;
        PinReference m_SourcePinReference;
        PinReference m_DestinationPinReference;
    };
}
