//
// Created by theoa on 07/03/2026.
//

#pragma once
#include <cassert>
#include <cstring>

#include "Attacks.h"
#include "Bitboard.h"
#include "Score.h"
#include "Zobrist.h"


struct positionInfo
{
    Bitboard occupiedSquaresBitboard{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};

    ZobristHash zobrist{};
    ZobristHash pawnZobrist{};

    //int eval{};
    //int pstScore{};
    int enPassantSquare{};
    int mgScore;
    int egScore;
    int gamePhase;

    int irreversiblePositionTop{};

    Piece pieceCaptured;
    Move prevMove{};
    CastlingRights castleRights{};
};


class Position
{
private:
    // Bitboards for board representation
    positionInfo prevPositionsLog[1024];


    Bitboard pieceBitboard[12]{};
    Bitboard blackPiecesBitboard{};
    Bitboard whitePiecesBitboard{};
    Bitboard occupiedSquaresBitboard{};

    ZobristHash zobristHash{};
    ZobristHash pawnZobristHash{};
    ZobristHash previousPositions[1024]{}; // store all previous positions' zobrist

    Piece Board[64]{};


    int numOfPositions{}; // index used for the previousPositions array
    int irreversiblePositionTop{}; // index for last irreversible move (for 3-fold repetition check)

    int mgPstAndMaterialScore{};
    int egPstAndMaterialScore{};
    int gamePhase{};

    int enPassantSquare{};
    int positionLogTop;

    CastlingRights castleRights{};

    bool whiteToMoveFlag{};
    bool isCheckmate{};
    bool isStalemate{};

public:
    void clearPosition();

    // sets the game state to the starting position
    void setStartPos();

    void loadFen(const std::string& fenString);

    Position();

    // make (move) functions
    void makeMove(Move move); // any legal move
    void makeCapture(Move capture); // any legal capture
    // for now pass epSq to update zobrist ep file
    void makeNullMove(int epSq) // null move for null move pruning
    {
        whiteToMoveFlag = !whiteToMoveFlag;
        zobristHash ^= zobristBlackToMove;
        enPassantSquare = NO_SQUARE;

        if (epSq != NO_SQUARE) zobristHash ^= zobristEnpassantFile[epSq % 8];

        // ASSERT
        //assert(zobristHash == computeZobristHash());
    }


    // unmake (move) functions
    void unmakeMove();
    void unmakeCapture();
    void unmakeNullMove(int epSq) // undo null move for null move pruning
    {
        whiteToMoveFlag = !whiteToMoveFlag;
        zobristHash ^= zobristBlackToMove;
        enPassantSquare = epSq;

        if (epSq != NO_SQUARE) zobristHash ^= zobristEnpassantFile[epSq % 8];

        // ASSERT
        //assert(zobristHash == computeZobristHash());
    }

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

    [[nodiscard]] constexpr ZobristHash getZobristHash() const
    {
        return zobristHash;
    }

    [[nodiscard]] constexpr ZobristHash getPawnZobristHash() const
    {
        return pawnZobristHash;
    }

    [[nodiscard]] constexpr int getNumOfMoves() const
    {
        return numOfPositions;
    }

    // returns game phase for testing
    [[nodiscard]] int getGamePhase() const { return gamePhase; }

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

    // return total PST and static eval using phases and mg and eg evaluations
    int getTotalPSTAndMaterialScore() const
    {
        int mgPhase = gamePhase;
        if (mgPhase > 24) mgPhase = 24; // in case of early promotion
        int egPhase = 24 - mgPhase;

        //std::cout << "mgPstAndMaterialScore:" << mgPstAndMaterialScore * mgPhase / 24 << std::endl;
        //std::cout << "egPstAndMaterialScore:" << egPstAndMaterialScore * egPhase / 24 << std::endl;
        //std::cout << "Total eval: " << (mgPstAndMaterialScore * mgPhase + egPstAndMaterialScore * egPhase) / 24 << std::endl;
        return (mgPstAndMaterialScore * mgPhase + egPstAndMaterialScore * egPhase) / 24;
    }

    // manual computation of zobrist hash for loading fen and for debugging incremental zobrist
    ZobristHash computeZobristHash();

    // manual computation of pawns zobrist hash for pawn hash table
    ZobristHash computePawnZobristHash();


    // check if current position has been repeated twice before (3-fold repetition rule)
    bool checkRepetition(ZobristHash currentPositionZobrist)
    {
        int repetitionsCount = 0;

        for (int i = numOfPositions - 1; i >= irreversiblePositionTop; i--)
        {
            if (currentPositionZobrist == previousPositions[i])
            {
                repetitionsCount++;
                if (repetitionsCount == 2)
                {
                    //std::cout << "info string repetition detected" << std::endl;
                    return true;
                }
            }
        }
        return false;
    }

    Bitboard getAllAttackersToSquare(int targetSquare);

    Bitboard getLeastValuablePiece(Bitboard pieces, int bySide, int &piece); // bySide is 0 for white 6 for black

    Bitboard xRayAttackersToSquare(int targetSquare, Bitboard occupancy) const;


    int SEE( int toSquare, int targetPiece, int fromSquare, int attackerPiece);

};


