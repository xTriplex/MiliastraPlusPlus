#pragma once

#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraph.h"

namespace MiliastraPlusPlus
{
    class Compiler
    {
    public:
        static std::string CompileToJSON(const Graph& TargetGraph)
        {
            nlohmann::json Root = TargetGraph.Serialize();
            return Root.dump(4);
        }

        static bool CompileToFile(const Graph& TargetGraph, const std::string& FilePath)
        {
            try
            {
                std::ofstream OutFile(FilePath);
                if (!OutFile.is_open())
                {
                    return false;
                }

                OutFile << CompileToJSON(TargetGraph);
                return true;
            }
            catch (...)
            {
                return false;
            }
        }
    };
}
