#pragma once

#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusPin.h"

namespace MiliastraPlusPlus
{
    class Node
    {
    public:
        Node(uint32_t Id, const std::string& Name)
            : m_Id(Id)
            , m_Name(Name)
        {
        }

        virtual ~Node() = default;

        uint32_t GetId() const
        {
            return m_Id;
        }

        const std::string& GetName() const
        {
            return m_Name;
        }

        void AddPin(std::shared_ptr<Pin> NodePin)
        {
            m_Pins.push_back(NodePin);
        }

        const std::vector<std::shared_ptr<Pin>>& GetPins() const
        {
            return m_Pins;
        }

        virtual nlohmann::json Serialize() const
        {
            nlohmann::json PinsArray = nlohmann::json::array();
            for (const auto& NodePin : m_Pins)
            {
                PinsArray.push_back(NodePin->Serialize());
            }

            return
            {
                {"Id", m_Id},
                {"Name", m_Name},
                {"Pins", PinsArray}
            };
        }

    private:
        uint32_t m_Id;
        std::string m_Name;
        std::vector<std::shared_ptr<Pin>> m_Pins;
    };
}
