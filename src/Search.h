//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"
#include <math.h>

#include "nnue/Accumulator.h"

class Position;
class Accumulator;

inline int MAX_DEPTH = 128;

inline uint64_t MAX_NODES = std::numeric_limits<uint64_t>::max();

inline bool DataGenFlag = false;

// Search Stack to pass search-related information
struct SearchStack
{
    Accumulator accumulator;
    int staticEval{};
    int16_t previousReductions{};
    int16_t previousExtensions{};
    Move previousMove = NO_MOVE;
    Piece pieceMoved = NO_PIECE;
    bool inCheck = false;
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
    int negaMaxAlphaBeta(Position& pos, int alpha, int beta, int depth, Move& bestMove, int ply, int rootDepth, bool allowNullMove, SearchStack* ss);

public:
    bool uciStop = false;

    int16_t historyScores[2][64][64]{}; // History heuristic

    int16_t counterMovesHistory[12][64][12][64]{};

    Move killerMoves[MAX_PLY][2]{}; // Killer Moves


    Move findBestMove(Position pos, int& posEval);

    int scoreMove(Move move, const Position& pos, Move hashMove, SearchStack* ss, int ply = -1);

    void updateHistory(Color sideToMove, int fromSquare, int toSquare, int value);

    void updateCounterHistory(Piece enemyPiece, int enemyToSq, Piece friendlyPiece, int friendlyToSq, int value);

    void newGame()
    {
        memset(counterMovesHistory, 0, sizeof(counterMovesHistory));
        memset(historyScores, 0, sizeof(historyScores));
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
            precomputedLMR[d][i] = 0.88 + std::log(d) * std::log(i) / 2.0;
        }
    }
}