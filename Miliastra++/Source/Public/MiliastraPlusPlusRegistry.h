#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

#include "MiliastraPlusPlusNode.h"

namespace MiliastraPlusPlus
{
    class Registry
    {
    public:
        using NodeFactory = std::function<std::shared_ptr<Node>(uint32_t Id)>;

        static Registry& GetInstance()
        {
            static Registry Instance;
            return Instance;
        }

        void RegisterNodeType(const std::string& TypeName, NodeFactory Factory)
        {
            m_Factories[TypeName] = Factory;
        }

        std::shared_ptr<Node> CreateNode(const std::string& TypeName, uint32_t Id)
        {
            auto It = m_Factories.find(TypeName);
            if (It != m_Factories.end())
            {
                return It->second(Id);
            }
            return nullptr;
        }

    private:
        Registry() = default;
        ~Registry() = default;

        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        std::unordered_map<std::string, NodeFactory> m_Factories;
    };
}
