#pragma once

#include <string>
#include <fstream>
#include <utility>

#include <nlohmann/json.hpp>

#include "MiliastraPlusPlusGraph.h"

namespace MiliastraPlusPlus
{
    class Compiler
    {
    public:
        static std::string CompileToJSON(
            Graph TargetGraph,
            DiagnosticCollection* Diagnostics = nullptr
        )
        {
            DiagnosticCollection PropagationDiagnostics = TargetGraph.PropagateTypes();
            const bool HasErrors = ContainsError(PropagationDiagnostics);
            if (Diagnostics != nullptr)
            {
                *Diagnostics = std::move(PropagationDiagnostics);
            }

            if (HasErrors)
            {
                return {};
            }

            nlohmann::json Root = TargetGraph.Serialize();
            return Root.dump(4);
        }

        static bool CompileToFile(
            Graph TargetGraph,
            const std::string& FilePath,
            DiagnosticCollection* Diagnostics = nullptr
        )
        {
            try
            {
                std::ofstream OutFile(FilePath);
                if (!OutFile.is_open())
                {
                    if (Diagnostics != nullptr)
                    {
                        Diagnostics->push_back({
                            .Severity = DiagnosticSeverity::Error,
                            .Code = DiagnosticCode::FileOpenFailure,
                            .Message = "The compiler could not open the output file."
                        });
                    }
                    return false;
                }

                const std::string SerializedGraph = CompileToJSON(TargetGraph, Diagnostics);
                if (SerializedGraph.empty())
                {
                    return false;
                }

                OutFile << SerializedGraph;
                if (OutFile.good())
                {
                    return true;
                }

                if (Diagnostics != nullptr)
                {
                    Diagnostics->push_back({
                        .Severity = DiagnosticSeverity::Error,
                        .Code = DiagnosticCode::FileWriteFailure,
                        .Message = "The compiler could not write the complete output file."
                    });
                }
                return false;
            }
            catch (...)
            {
                if (Diagnostics != nullptr)
                {
                    Diagnostics->push_back({
                        .Severity = DiagnosticSeverity::Error,
                        .Code = DiagnosticCode::FileWriteFailure,
                        .Message = "The compiler encountered an exception while writing the output file."
                    });
                }
                return false;
            }
        }
    };
}
