#pragma once

#include <string>
#include <vector>
#include <memory>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusNode.h"
#include "MiliastraPlusPlusLink.h"


namespace MiliastraPlusPlus
{
    class Graph
    {
    public:
        Graph(uint32_t Id, const std::string& Name)
            : m_Id(Id)
            , m_Name(Name)
        {
        }

        virtual ~Graph() = default;

        uint32_t GetId() const
        {
            return m_Id;
        }

        const std::string& GetName() const
        {
            return m_Name;
        }

        void AddNode(std::shared_ptr<Node> GraphNode)
        {
            m_Nodes.push_back(GraphNode);
        }

        void AddLink(std::shared_ptr<Link> GraphLink)
        {
            m_Links.push_back(GraphLink);
        }

        const std::vector<std::shared_ptr<Node>>& GetNodes() const
        {
            return m_Nodes;
        }

        const std::vector<std::shared_ptr<Link>>& GetLinks() const
        {
            return m_Links;
        }

        nlohmann::json Serialize() const
        {
            nlohmann::json NodesArray = nlohmann::json::array();
            for (const auto& GraphNode : m_Nodes)
            {
                NodesArray.push_back(GraphNode->Serialize());
            }

            nlohmann::json LinksArray = nlohmann::json::array();
            for (const auto& GraphLink : m_Links)
            {
                NodesArray.push_back(GraphLink->Serialize());
            }

            return
            {
                {"Id", m_Id},
                {"Name", m_Name},
                {"Nodes", NodesArray},
                {"Links", LinksArray}
            };
        }

    private:
        uint32_t m_Id;
        std::string m_Name;
        std::vector<std::shared_ptr<Node>> m_Nodes;
        std::vector<std::shared_ptr<Link>> m_Links;
    };
}
