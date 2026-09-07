#pragma once

#include <string>

#include <nlohmann/json.hpp>


namespace MiliastraPlusPlus
{
    enum class EPinCategory
    {
        Execution,
        Data
    };

    enum class EPinType
    {
        Flow,
        Bool,
        Integer,
        Float,
        String,
        Entity,
        GUID,
        Vector3,
        PrefabID,
        ConfigID,
        Structure,
        List,
        Object
    };

    enum class EPinKind
    {
        Input,
        Output
    };

    class Pin
    {
    public:
        Pin(uint32_t Id, const std::string& Name, EPinCategory PinCategory, EPinType PinType, EPinKind PinKind)
            : m_Id(Id)
            , m_Name(Name)
            , m_PinCategory(PinCategory)
            , m_PinType(PinType)
            , m_PinKind(PinKind)
        { 
        }

        virtual ~Pin() = default;

        uint32_t GetId() const
        {
            return m_Id;
        }

        const std::string& GetName() const
        {
            return m_Name;
        }

        EPinCategory GetPinCategory() const
        {
            return m_PinCategory;
        }

        EPinType GetPinType() const
        {
            return m_PinType;
        }

        EPinKind GetPinKind() const
        {
            return m_PinKind;
        }

        bool IsExecution() const
        {
            return m_PinCategory == EPinCategory::Execution;
        }

        nlohmann::json Serialize() const
        {
            return
            {
                {"Id", m_Id},
                {"Name", m_Name},
                {"Pin Category", m_PinCategory},
                {"Pin Type", m_PinType},
                {"Pin Kind", m_PinKind}
            };
        }

    private:
        uint32_t m_Id;
        std::string m_Name;
        EPinCategory m_PinCategory;
        EPinType m_PinType;
        EPinKind m_PinKind;
    };
}
