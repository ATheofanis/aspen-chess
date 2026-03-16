//
// Created by theoa on 07/03/2026.
//

#pragma once
#include "Bitboard.h"
#include "Score.h"


struct positionInfo
{
    Bitboard occupiedSquaresBitboard{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};
    //int eval{};
    //int pstScore{};
    int enPassantSquare{};
    int mgScore;
    int egScore;
    int gamePhase;

    Piece pieceCaptured;
    Move prevMove{};
    CastlingRights castleRights{};
};


class Position
{
private:
    // Bitboards for board representation
    positionInfo prevPositionsLog[256];


    Bitboard pieceBitboard[12]{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};
    Bitboard occupiedSquaresBitboard{};

    Piece Board[64]{};

    int mgPstAndMaterialScore{};
    int egPstAndMaterialScore{};
    int gamePhase{};

    int enPassantSquare{};
    int positionLogTop;

    CastlingRights castleRights{};

    bool whiteToMoveFlag{};
    bool isCheckmate{};
    bool isStalemate{};;

public:
    void clearPosition();

    // sets the game state to the starting position
    void setStartPos();

    void loadFen(const std::string& fenString);

    Position();

    void makeMove(Move move);

    void unmakeMove();

    // return the bitboard of a given piece
    [[nodiscard]] constexpr Bitboard getPieceBitboard(int pieceIndex) const
    {
        return pieceBitboard[pieceIndex];
    }

    // returns a bitboard with the position of every white piece
    [[nodiscard]] constexpr Bitboard getWhiteBitboard() const
    {
        return whitePiecesBitboard;
    }

    // returns a bitboard with the position of every black piece
    [[nodiscard]] constexpr Bitboard getBlackBitboard() const
    {
        return blackPiecesBitboard;
    }

    // returns a bitboard of all occupied squares
    [[nodiscard]] constexpr Bitboard getOccupiedBitboard() const
    {
        return occupiedSquaresBitboard;
    }

   //Piece* getBoard()
   //{
   //    return Board;
   //}

    // returns the piece that is in square using the Board[64] mailbox
    [[nodiscard]] Piece getPieceFromBoard(int square) const { return Board[square]; }

    // true if it is whites turn to move
    [[nodiscard]] constexpr bool isWhiteToMove() const { return whiteToMoveFlag; }

    // returns castle rights
    [[nodiscard]] CastlingRights getCastlingRights() const { return castleRights; }

    // returns the square where enpassant can be played (otherwise returns NO_SQUARE)
    [[nodiscard]] int getEnpassantSquare() const { return enPassantSquare; }

    inline void calculatePstAndMaterialScore();

    constexpr int getMGPstAndMaterialScore() const { return mgPstAndMaterialScore; }
    constexpr int getEGPstAndMaterialScore() const { return egPstAndMaterialScore; }

    int getTotalPSTAndMaterialScore() const
    {
        int mgPhase = gamePhase;
        if (mgPhase > 24) mgPhase = 24; // in case of early promotion
        int egPhase = 24 - mgPhase;

        return (mgPstAndMaterialScore * mgPhase + egPstAndMaterialScore * egPhase) / 24;
    }


};
