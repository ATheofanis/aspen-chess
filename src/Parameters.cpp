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
        {"QS_SEE_Threshold", &QS_SEE_Threshold, -100, 100},

        // NEGAMAX
        {"CheckExtensionsLimit", &CheckExtensionsLimit, 1, 10},

        // RFP
        {"RFP_MaxDepth", &RFP_MaxDepth, 3, 10},
        {"RFP_MarginMultiplier", &RFP_MarginMultiplier, 20, 150},
        {"RFP_MarginBase", &RFP_MarginBase, -100, 200},
        {"RFP_ImprovingMarginReduction", &RFP_ImprovingMarginReduction, 0, 50},

        // RAZORING
        {"RazoringMaxDepth", &RazoringMaxDepth, 1, 6},
        {"RazoringMarginMultiplier", &RazoringMarginMultiplier, 10, 150},
        {"RazoringMarginBase", &RazoringMarginBase, 100, 600},

        // NMP
        {"NMP_MinDepth", &NMP_MinDepth, 1, 6},
        {"NMP_Base", &NMP_Base, 1, 10},
        {"NMP_Divisor", &NMP_Divisor, 1, 10},
        {"NMP_ImprovingReduction", &NMP_ImprovingReduction, 0, 5},

        // FUTILITY PRUNING AND EXTENDED
        {"FP_Margin1", &FP_Margin1, 50, 400},
        {"FP_Margin2", &FP_Margin2, 200, 800},

        // SINGULAR EXTENSIONS
        {"SE_MinDepth", &SE_MinDepth, 3, 10},
        {"SE_MarginMultiplier", &SE_MarginMultiplier, 0, 5},
        {"SE_MarginDivisor", &SE_MarginDivisor, 1, 5},
        {"SE_DepthSubtractor", &SE_DepthSubtractor, 0, 5},
        {"SE_DepthDivisor", &SE_DepthDivisor, 1, 5},

        {"SE_TripleMaxDepth", &SE_TripleMaxDepth, 5, 20},
        {"SE_TripleThreshold", &SE_TripleThreshold, 20, 90},
        {"SE_DoubleMaxDepth", &SE_DoubleMaxDepth, 5, 20},
        {"SE_DoubleThreshold", &SE_DoubleThreshold, 0, 40},


        // SEE & CAPHISTORY PRUNING
        {"SEE_MinDepth", &SEE_MinDepth, 1, 10},
        {"CapHistPruningThreshold", &CapHistPruningThreshold, 0, 16384},
        {"SEE_Threshold", &SEE_Threshold, -200, 200},

        // LMP
        {"LMP_Multiplier", &LMP_Multiplier, 1, 12},
        {"LMP_Base", &LMP_Base, 0, 10},
        {"LMP_HistoryReductionThreshold", &LMP_HistoryReductionThreshold, 0, 16384},
        {"LMP_HistoryReductionMultiplier", &LMP_HistoryReductionMultiplier, 1, 5},

        // LMR
        {"LMR_MinDepth", &LMR_MinDepth, 1, 5},
        {"LMR_MinMoveIndex", &LMR_MinMoveIndex, 1, 5},
        {"LMR_Base", &LMR_Base, 30, 150},
        {"LMR_Divisor", &LMR_Divisor, 100, 400},
        {"LMR_HashIsCapPromoPenalty", &LMR_HashIsCapPromoPenalty, 0, 3},
        {"LMR_NotImprovingPenalty", &LMR_NotImprovingPenalty, 0, 3},
        {"LMR_KillerBonus", &LMR_KillerBonus, 0, 3},
        {"LMR_CutNodeReduction", &LMR_CutNodeReduction, 0, 3},
        {"LMR_LowerThanTTDepthPenalty", &LMR_LowerThanTTDepthPenalty, 0, 3},
        {"LMR_TTNotPvPenalty", &LMR_TTNotPvPenalty, 0, 3},

        // History Bonus
        {"HistoryBonusMultiplier", &HistoryBonusMultiplier, 100, 600},
        {"HistoryBonusSubtractor", &HistoryBonusSubtractor, 0, 600},
        {"HistoryBonusMax", &HistoryBonusMax, 500, 4000},

        // History Penalty
        {"HistoryPenaltyMultiplier", &HistoryPenaltyMultiplier, 100, 600},
        {"HistoryPenaltySubtractor", &HistoryPenaltySubtractor, 0, 600},
        {"HistoryPenaltyMax", &HistoryPenaltyMax, 500, 4000},

        // Aspiration Window
        {"AspirationMinimum", &AspirationMinimum, 1, 10},
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
