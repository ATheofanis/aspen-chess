//
// Created by theoa on 07/03/2026.
//

#pragma once
#include "Position.h"

struct legalityInformation
{
    Bitboard pinners{};
    Bitboard pinned{};
    Bitboard checkers{};
    Bitboard legalSquaresMask{};
    Bitboard pinnedPieceLegalSquares[64] = {};
    int numOfChecks{};
};

legalityInformation getLegalityInfo(int kingSquare, Color allyColor, const Position& pos);

void generateWhitePawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves);
void generateBlackPawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves);

void generateKnightMoves(const legalityInformation& info, Bitboard knightPos, Bitboard allyPieces, Bitboard enemyPieces, Move moves[], int& numOfMoves);
void generateBishopMoves(const legalityInformation& info, Bitboard bishopPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateRookMoves(const legalityInformation& info, Bitboard rookPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateQueenMoves(const legalityInformation& info, Bitboard queenPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves);
void generateKingMoves(const legalityInformation& info, Bitboard kingPos, Bitboard allyPieces, Bitboard enemyPieces, const Position& pos, Move moves[], int& numOfMoves);


void generateWhiteKingMoves(const Position& pos, Move moves[], int& numOfMoves);
void generateBlackKingMoves(const Position& pos, Move moves[], int& numOfMoves);

// move generators. i split move generation into captures and non captures for staged move generation inside the search
void generateLegalMoves(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves); // every legal move
void generateCaptures(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves); // every legal capture
void generateQuietMoves(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves); // every legal non-capture move


// converts a move variable to readable format (chess board squares and flag display)
inline void printMoveInfo(const Move move)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    int flag = (move >> 12) & 0xF;
    std::cout << "\nMove: Starting Square : " << squareName(static_cast<Square>(fromSquare)) << "\nEnding square: " << squareName(static_cast<Square>(toSquare)) << "Flag: " << flag << "\n";
}

inline void printMove(const Move move)
{
    int fromSquare = move & 0x3F;
    int toSquare = (move >> 6) & 0x3F;
    std::cout << squareName(static_cast<Square>(fromSquare)) << squareName(static_cast<Square>(toSquare));
}


void generateQuietWhitePawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int& numOfMoves);
void generateQuietBlackPawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int& numOfMoves);

void generateQuietKnightMoves(const legalityInformation& info, Bitboard knightPos, Bitboard occupied, Move moves[], int& numOfMoves);
void generateQuietBishopMoves(const legalityInformation& info, Bitboard bishopPos, Bitboard occupied, Move moves[], int& numOfMoves);
