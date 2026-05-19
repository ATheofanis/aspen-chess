//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

#include "nnue/Accumulator.h"

class Position;
class Accumulator;

constexpr int historyThreshold = 8000;
constexpr int firstKillerScore = 9000;
constexpr int secondKillerScore = 8500;
constexpr int hashMoveScore = 320000;

inline int MAX_DEPTH = 128;

inline uint64_t MAX_NODES = std::numeric_limits<uint64_t>::max();

inline bool DataGenFlag = false;


// Search Stack to pass search-related information - work in progress
struct SearchStack
{
    Accumulator accumulator;
};



class MoveSearcher
{
private:
    int nodes = 0;
    int quieNodes = 0;

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
    int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove, SearchStack* ss);

public:
    int historyMoves[64][64]{};
    Move killerMoves[MAX_PLY][2]{};

    Move findBestMove(Position pos, int& posEval);

    int scoreQuiescenceMove(const Move& move, Position& pos, const Move& bestMove);

    int scoreMove(const Move& move, const Position& pos, const Move& hashMove, const int& ply);

    void newGame()
    {
        memset(historyMoves, 0, sizeof(historyMoves));
        memset(killerMoves, 0, sizeof(killerMoves));
        generation = 0;
    }
};


extern int precomputedLMR[128][256];


// the LMR formula is computed once at the beginning of the program to avoid calling std::log millions of times in the search
inline void initLMR() {
    // for every depth up to 128
    for (int d = 0; d < 128; d++) {
        // for every move up to 256
        for (int i = 0; i < 256; i++) {
            precomputedLMR[d][i] = 1 + std::log(d) * std::log(i) / 2.0;
        }
    }
}