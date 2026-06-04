//
// Created by theoa on 28/05/2026.
//

#include "Parameters.h"

#include <iostream>

#include "Search.h"

namespace Aspen::Parameters
{
    std::vector<TunableEntry> tunableParameters =
    {
        // QSEARCH
        {"QS_DeltaMargin", &QS_DeltaMargin, 500, 1500},
        {"QS_DeltaCaptureMargin", &QS_DeltaCaptureMargin, 0, 500},
        {"QS_SEE_Threshold", &QS_SEE_Threshold, -200, 100},

        // LMP
        {"LMP_D1", &lateMovePruningThreshold[1], 1, 15},
        {"LMP_D2", &lateMovePruningThreshold[2], 2, 30},
        {"LMP_D3", &lateMovePruningThreshold[3], 5, 45},
        {"LMP_D4", &lateMovePruningThreshold[4], 10, 60},

        // LMR
        {"LMR_Base", &LMR_Base, 30, 150},
        {"LMR_Divisor", &LMR_Divisor, 100, 400},
        {"LMR_NoHashPenalty", &LMR_NoHashPenalty, 0, 300},
        {"LMR_HashIsCapPromoPenalty", &LMR_HashIsCapPromoPenalty, 0, 300},
        {"LMR_NotImprovingPenalty", &LMR_NotImprovingPenalty, 0, 300},
        {"LMR_KillerBonus", &LMR_KillerBonus, 0, 300},

        // RFP
        {"RFP_MaxDepth", &RFP_MaxDepth, 3, 10},
        {"RFP_MarginMultiplier", &RFP_MarginMultiplier, 20, 150},
        {"RFP_MarginBase", &RFP_MarginBase, -100, 200},
        {"RFP_ImprovingMultiplier", &RFP_ImprovingMultiplier, 0, 50},
        {"RFP_ImprovingBase", &RFP_ImprovingBase, -100, 100},

        // Razoring
        {"RazoringMaxDepth", &RazoringMaxDepth, 1, 6},
        {"RazoringMarginMultiplier", &RazoringMarginMultiplier, 10, 150},
        {"RazoringMarginBase", &RazoringMarginBase, 100, 600},

        // NMP
        {"NMP_MinDepth", &NMP_MinDepth, 1, 6},
        {"NMP_Base", &NMP_Base, 200, 700},
        {"NMP_Divisor", &NMP_Divisor, 150, 600},
        {"NMP_ImprovingBonus", &NMP_ImprovingBonus, 0, 300},

        // Futility & Extended
        {"FP_Margin1", &FP_Margin1, 50, 400},
        {"FP_Margin2", &FP_Margin2, 200, 800},

        // Singular Extension
        {"SE_MinDepth", &SE_MinDepth, 3, 10},
        {"SE_MarginMultiplier", &SE_MarginMultiplier, 2, 14},
        {"SE_MarginDivisor", &SE_MarginMultiplier, 2, 14},
        {"SE_DepthSubtractor", &SE_DepthSubtractor, 2, 14},
        {"SE_Divisor", &SE_DepthDivisor, 100, 400},
        {"SE_TripleMaxDepth", &SE_TripleMaxDepth, 4, 12},
        {"SE_TripleThreshold", &SE_TripleThreshold, 20, 80},
        {"SE_DoubleMaxDepth", &SE_DoubleMaxDepth, 3, 10},
        {"SE_DoubleThreshold", &SE_DoubleThreshold, 0, 30},

        // Capture History Pruning
        {"CapHistPruningThreshold", &CapHistPruningThreshold, -MaxHistoryScore, MaxHistoryScore},

        // History Bonus
        {"HistoryBonusMultiplier", &HistoryBonusMultiplier, 100, 600},
        {"HistoryBonusSubtractor", &HistoryBonusSubtractor, 0, 600},

        // Aspiration Window
        {"AspirationWindowBase", &AspirationWindowBase, 5, 50},

        // Time Controls
        {"TM_DropMargin", &TM_DropMargin, 5, 60}
    };


    // Print all the tunable parameters
    void printTunableParameters()
    {
        for (const auto& parameter : tunableParameters)
        {
            std::cout << "option name " << parameter.name  << " type spin default " << *(parameter.value)
                      << " min " << parameter.minValue << " max " << parameter.maxValue << "\n";
        }
    }

    // Set the value of a parameter
    void setParameter(const std::string& name, int value)
    {
        for (const auto& parameter : tunableParameters)
        {
            if (parameter.name == name)
            {
                *(parameter.value) = value;

                // If we changed one of the two values used in the LMR formula,
                // we must update the LMR table with the new values
                if (name == "LMR_Base" || name == "LMR_Divisor")
                {
                    initLMR();
                }
                break;
            }
        }
    }
}
