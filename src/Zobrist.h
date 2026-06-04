//
// Created by theoa on 18/03/2026.
//
#pragma once
#include "Types.h"

// Zobrist hashing starts by randomly generating bitstrings for each possible element of a board game
// - (https://en.wikipedia.org/wiki/Zobrist_hashing)


class Position;

// We need an array to hold randomly generated values for each combination of a piece and a position (6 pieces × 2 colors × 64 board positions)
extern ZobristHash zobristPieces[64][12];

// An array of randomly generated values for each type of castling right (White Kingside, White Queenside, Black Kingside, Black Queenside)
extern ZobristHash zobristCastleRights[4];

// An array of randomly generated values - one for every file where en-passant can be played
extern ZobristHash zobristEnpassantFile[8];

// A number to indicate that it is the black player's turn to move
extern ZobristHash zobristBlackToMove;

// This function will initialize the zobrist hash variables to randomly generated 64-bit values using the xoshiro256** algorithm
// It must be called once at startup
void initZobrist();

ZobristHash getKeyAfterMove(Move move, ZobristHash zobrist, const Position& pos);