//
// Created by theoa on 18/03/2026.
//
#pragma once
#include "Types.h"

extern ZobristHash zobristPieces[64][12];
extern ZobristHash zobristCastleRights[4];
extern ZobristHash zobristEnpassantFile[8];
extern ZobristHash zobristBlackToMove;
void initZobrist();