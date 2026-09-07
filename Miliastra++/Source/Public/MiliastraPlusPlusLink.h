#pragma once

#include <string>

#include <nlohmann/json.hpp>

namespace MiliastraPlusPlus
{
    class Link
    {
    public:
        Link(uint32_t Id, uint32_t StartPinId, uint32_t EndPinId)
            : m_Id(Id)
            , m_StartPinId(StartPinId)
            , m_EndPinId(EndPinId)
        {
        }

        virtual ~Link() = default;

        uint32_t GetId() const
        {
            return m_Id;
        }

        uint32_t GetStartPinId() const
        {
            return m_StartPinId;
        }

        uint32_t GetEndPinId() const
        {
            return m_EndPinId;
        }

        nlohmann::json Serialize() const
        {
            return
            {
                {"Id", m_Id},
                {"StartPinId", m_StartPinId},
                {"EndPinId", m_EndPinId}
            };
        }

    private:
        uint32_t m_Id;
        uint32_t m_StartPinId;
        uint32_t m_EndPinId;
    };
}
