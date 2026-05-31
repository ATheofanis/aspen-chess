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


    // LMP PARAMETERS FOR EACH DEPTH
    inline int lateMovePruningThreshold[5] = {999, 6, 12, 17, 25};

    // LMR PARAMETERS
    inline int LMR_Base = 88;                   // 0.88 , DIVIDE BY 100
    inline int LMR_Divisor = 200;               // 2.00 , DIVIDE BY 100
    inline int LMR_NoHashPenalty = 100;         // -1.00 , DIVIDE BY 100 AND -
    inline int LMR_HashIsCapPromoPenalty = 100;  // SCALED BY 100 PENALTY FOR WHEN HASH IS TOO GOOD
    inline int LMR_NotImprovingPenalty = 100;   // -1.00 , DIVIDE BY 100 AND -
    inline int LMR_KillerBonus = 100;           // 1.00 , DIVIDE BY 100 AND

    // RFP PARAMETERS
    inline int RFP_MaxDepth = 7;
    inline int RFP_MarginMultiplier = 70;     // NOT SCALED
    inline int RFP_MarginBase = 0;           // BASE VALUE FOR RFP
    // MARGIN REDUCTION WHEN IMPROVING
    inline int RFP_ImprovingMultiplier = 15;           // - DEPTH * 15
    inline int RFP_ImprovingBase = 0;              // Base reduction


    // RAZORING PARAM
    inline int RazoringMaxDepth = 3;
    inline int RazoringMarginMultiplier = 60;   // 60 * DEPTH
    inline int RazoringMarginBase = 300;   // + 300

    // NMP PARAM
    inline int NMP_MinDepth = 3;
    inline int NMP_Base = 400;                // SCALED BASE DEPTH REDUCTION BY 100
    inline int NMP_Divisor = 300;             // DEPTH / 3 , SCALED 100
    inline int NMP_ImprovingBonus = 100;     // reduce more when improving, SCALED 100

    // FUTILITY PRUNING AND EXTENDED
    inline int FP_Margin1 = 200;   // MARGIN FOR DEPTH 1 FUTILITY
    inline int FP_Margin2 = 500;   // MARGIN FOR EXTENDED FUTILITY (DEPTH 2)


    // SINGULAR EXTENSION
    inline int SE_MinDepth = 6;
    inline int SE_MarginMultiplier = 6;      // MARGIN MULTIPLIER FOR SE
    inline int SE_MarginDivisor = 8;                 // MARGIN DIVISON FOR SE
    inline int SE_DepthSubtractor = 1;        // HOW MUCH WE SUBTRACT FROM THE DEPTH
    inline int SE_DepthDivisor = 2;
    // THRESHOLDS FOR EXTENSIONS/REDUCTIONS
    inline int SE_TripleMaxDepth = 7;      // MAX DEPTH FOR +3 EXT SEARCH
    inline int SE_TripleThreshold = 43;      // MARGIN FOR +3

    inline int SE_DoubleMaxDepth = 6;         // MAX DEPTH FOR +2 EXT SEARCH
    inline int SE_DoubleThreshold = 12;      // MARGIN FOR +2


    // HISTORY BONUS -> (300 * depth - 250)
    inline int HistoryBonusMultiplier = 300;
    inline int HistoryBonusSubtractor = 250;


    // ASPIRATION WINDOW
    inline int AspirationWindowBase = 15;

    // TIME EXTENSION AFTER SUDDEN SCORE DROP
    inline int TM_DropMargin = 25;


    // Print all the tunable parameters
    void printTunableParameters();

    // Set the value of a parameter
    void setParameter(const std::string& name, int value);
}
