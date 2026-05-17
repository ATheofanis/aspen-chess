//
// Created by theoa on 16/05/2026.
//
#pragma once
#include "Types.h"
#include "Position.h"

enum class MovePickerPhase : int {
    HashMove,                   // If there is a valid hash move it is always searched first
    GenAndScoreCapsAndPromos,   // Generate and sort every capture and promotion
    WinningCapturesAndPromos,   // Winning captures and queen promotions
    FirstKillerMove,             // The first killer move is selected
    SecondKillerMove,           // The second killer move is selected
    GenAndScoreQuietMoves,      // Generate and sort all quiet moves
    QuietMoves,                 // Every quiet move
    LosingCapturesAndPromos,    // Losing captures and under-promotions
    End                         // The end - all moves have been searched
};


// Move picker class - to be continued
class MovePicker
{
private:
    // Position and legality information for move legality checks (hash move and killer moves)
    const Position& position;
    const legalityInformation& legalityInfo;

    MovePickerPhase phase;
    Move moves[256]{};

    // Hash move and killers
    Move hashMove;
    Move firstKillerMove;
    Move secondKillerMove;

    // History moves passed by the move searcher
    const history& historyMoves;

    int moveScores[256]{};
    int numOfMoves{};
    int currentMoveIndex{};
    int losingCapAndPromoStartIndex{};
    int losingCapAndPromoEndIndex{};
    int ply;

public:
    Move nextMove();

    MovePicker(const Position& pos, const legalityInformation& info, const history& history, Move ttMove, Move killer1, Move killer2, int pl) :
        position(pos), legalityInfo(info), historyMoves(history), hashMove(ttMove), firstKillerMove(killer1), secondKillerMove(killer2),
        phase(MovePickerPhase::HashMove), ply(pl) {}

    int scoreQuietMove(Move move);
    int scoreCaptureAndPromoMove(Move move);

    void scoreCapAndPromoMoves();
    void scoreQuietMoves();
};