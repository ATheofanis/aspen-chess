//
// Created by theoa on 14/03/2026.
//

#pragma once
#include "Position.h"

class Position;

inline int MAX_DEPTH = 12;


class MovePicker
{
public:
    Move nextMove();
    MovePicker() = default;
private:
    Move moves[256];
    Move ttMove;
};


Move findBestMove(Position pos);