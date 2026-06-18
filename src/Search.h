//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

#include "Parameters.h"
#include "nnue/Accumulator.h"

class Position;
class Accumulator;

inline int MAX_DEPTH = 128;

inline uint64_t MAX_NODES = std::numeric_limits<uint64_t>::max();

inline bool DisableStatistics = false;

// Search Stack to pass search-related information
struct SearchStack
{
    ContinuationHistory* contHistory = nullptr;
    Accumulator accumulator;
    int16_t historyScore{};
    int staticEval{};
    bool inCheck = false;
    int previousExtensions{};
    Move move = NO_MOVE;
    Piece movedPiece = NO_PIECE;
    Piece capturedPiece = NO_PIECE;
    Move excludedMove = NO_MOVE;
};


class MoveSearcher
{
private:
    int nodes = 0;

    Move pvTable[MAX_PLY][MAX_PLY]{};
    int pvLength[MAX_PLY]{};

    int generation = 0;

    bool nodesLimitReached = false;

    // Prints search-related information such as the current depth, the evaluation and the PV table of the position
    void printInfo(int depth, int score);

    // Quiescence search for horizon effect
    template<NodeType nodeType>
    int quiescence(Position& pos, int alpha, int beta, Move ttBestMove, int ply, SearchStack* ss);

    // Negamax with alpha beta pruning
    template<NodeType nodeType>
    int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove, SearchStack* ss, bool cutNode);

public:
    bool uciStop = false;

    ContinuationHistory continuationHistoryScores[12][64]{};
    int captureHistory[12][64][12]{}; // Victim | ToSquare | Attacker
    int historyScores[2][64][64]{};
    Move killerMoves[MAX_PLY][2]{};

    int getNodes() { return nodes; }

    Move findBestMove(Position pos, int& posEval);

    int scoreMove(Move move, const Position& pos, Move hashMove, SearchStack *ss, int ply = -1);

    void updateHistory(Color sideToMove, int fromSquare, int toSquare, int value);

    void updateCaptureHistory(Piece captured, int toSquare, Piece attacker, int bonus);

    void newGame()
    {
        memset(continuationHistoryScores, 0, sizeof(continuationHistoryScores));
        memset(captureHistory, 0, sizeof(captureHistory));
        memset(historyScores, 0, sizeof(historyScores));
        memset(killerMoves, 0, sizeof(killerMoves));
        generation = 0;
    }

    void applyAge()
    {
        // Increment generation for transposition table aging replacement scheme
        generation++;

        // Butterfly history aging, it is expensive but only called once before the search starts
        //for (int i = 0; i < 2; i++)
        //{
        //    for (int j = 0; j < 64; j++)
        //    {
        //        for (int k = 0; k < 64; k++)
        //        {
        //            historyScores[i][j][k] /= 2;
        //        }
        //    }
        //}
    }
};


extern int precomputedLMR[MAX_PLY][256];

// the LMR formula is computed once at the beginning of the program to avoid calling std::log millions of times in the search
inline void initLMR() {
    float LmrBase = Aspen::Parameters::LMR_Base / 100.0f;
    float LmrDivisor = Aspen::Parameters::LMR_Divisor / 100.0f;
    // for every depth up to the maximum ply
    for (int d = 0; d < MAX_PLY; d++) {
        // for every move up to 256
        for (int i = 0; i < 256; i++) {
            float reduction = LmrBase + std::log(d) * std::log(i) / LmrDivisor;
            precomputedLMR[d][i] = static_cast<int>(std::round(reduction));
        }
    }
}