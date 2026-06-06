//
// Created by theoa on 28/05/2026.
//
#pragma once
#include <string>
#include <vector>

namespace Aspen::Parameters
{
    struct TunableEntry
    {
        std::string name; // Name to be printed after 'uci'
        int *value; // Points to one of the below parameters (current value of the parameter)
        // [min, max] range for the parameter
        int minValue;
        int maxValue;
    };



    // Below are all the tunable parameters initialized with their default value

    // Q-SEARCH PARAMETERS
    inline int QS_DeltaMargin = 980;
    inline int QS_DeltaCaptureMargin = 200;
    inline int QS_SEE_Threshold = 0;

    // NEGAMAX
    inline int CheckExtensionsLimit = 5;

    // RFP PARAMETERS
    inline int RFP_MaxDepth = 7;
    inline int RFP_MarginMultiplier = 70;     // NOT SCALED
    inline int RFP_MarginBase = 0;           // BASE VALUE FOR RFP
    inline int RFP_ImprovingMarginReduction = 15;


    // RAZORING PARAM
    inline int RazoringMaxDepth = 3;
    inline int RazoringMarginMultiplier = 60;   // 60 * DEPTH
    inline int RazoringMarginBase = 300;   // + 300

    // NMP PARAM
    inline int NMP_MinDepth = 4;
    inline int NMP_Base = 4;
    inline int NMP_Divisor = 3;
    inline int NMP_ImprovingReduction = 1;     // reduce more when improving


    // FUTILITY PRUNING AND EXTENDED
    inline int FP_Margin1 = 200;   // MARGIN FOR DEPTH 1 FUTILITY
    inline int FP_Margin2 = 500;   // MARGIN FOR EXTENDED FUTILITY (DEPTH 2)


    // SINGULAR EXTENSION
    inline int SE_MinDepth = 7;
    inline int SE_MarginMultiplier = 1;      // MARGIN MULTIPLIER FOR SE
    inline int SE_MarginDivisor = 1;                 // MARGIN DIVISON FOR SE
    inline int SE_DepthSubtractor = 2;        // HOW MUCH WE SUBTRACT FROM THE DEPTH
    inline int SE_DepthDivisor = 2;
    // THRESHOLDS FOR EXTENSIONS/REDUCTIONS
    inline int SE_TripleMaxDepth = 15;      // MAX DEPTH FOR +3 EXT SEARCH
    inline int SE_TripleThreshold = 60;      // MARGIN FOR +3

    inline int SE_DoubleMaxDepth = 15;         // MAX DEPTH FOR +2 EXT SEARCH
    inline int SE_DoubleThreshold = 20;      // MARGIN FOR +2


    // SEE & CAPHISTORY PRUNING
    inline int SEE_MinDepth = 3;
    inline int CapHistPruningThreshold = 8192;
    inline int SEE_Threshold = 0;


    // LMP PARAMETERS
    inline int LMP_Multiplier = 6;
    inline int LMP_Base = 0;
    inline int LMP_HistoryReductionThreshold = 8192;
    inline int LMP_HistoryReductionMultiplier = 2;

    // LMR PARAMETERS
    inline int LMR_MinDepth = 2;
    inline int LMR_MinMoveIndex = 2;
    inline int LMR_Base = 88;                   // 0.88 , DIVIDE BY 100
    inline int LMR_Divisor = 200;               // 2.00 , DIVIDE BY 100
    inline int LMR_HashIsCapPromoPenalty = 1;  // SCALED BY 100 PENALTY FOR WHEN HASH IS TOO GOOD
    inline int LMR_NotImprovingPenalty = 1;   // -1.00 , DIVIDE BY 100 AND -
    inline int LMR_KillerBonus = 1;           // 1.00 , DIVIDE BY 100 AND
    inline int LMR_CutNodeReduction = 1;
    inline int LMR_LowerThanTTDepthPenalty = 1;
    inline int LMR_TTNotPvPenalty = 1;



    // HISTORY BONUS -> (300 * depth - 250)
    inline int HistoryBonusMultiplier = 300;
    inline int HistoryBonusSubtractor = 250;
    inline int HistoryBonusMax = 1200;

    // HISTORY PENALTY
    inline int HistoryPenaltyMultiplier = 300;
    inline int HistoryPenaltySubtractor = 250;
    inline int HistoryPenaltyMax = 1200;


    // ASPIRATION WINDOW
    inline int AspirationWindowBase = 15;

    // TIME EXTENSION AFTER SUDDEN SCORE DROP
    inline int TM_DropMargin = 25;


    // Print all the tunable parameters
    void printTunableParameters();

    // Set the value of a parameter
    void setParameter(const std::string& name, int value);
}
