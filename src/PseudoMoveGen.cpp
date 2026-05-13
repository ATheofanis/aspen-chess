//
// Created by theoa on 13/05/2026.
//

#include "PseudoMoveGen.h"
#include "Position.h"

// Store every pseudo-legal single square pawn advancing moves for white pawns
void pseudoPawnAdvances(Bitboard oneSqAdvanceQuiet, Bitboard oneSqAdvancePromotion, Color pawnColor, Move moves[], int &numOfMoves)
{
    int colorMultiplier = pawnColor == White ? 1 : -1;

    // Store every quiet one square advance
    while (oneSqAdvanceQuiet)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(oneSqAdvanceQuiet));

        // Use the color multiplier to determine the starting square of the pawn
        Move startSquare  = endSquare - (8 * colorMultiplier);

        // Store the move with flag set to 0
        Move move = startSquare | (endSquare << 6);

        moves[numOfMoves++] = move;
    }

    // Store every one square advance move that leads to a promotion
    while (oneSqAdvancePromotion)
    {
        // Calculate the end square and the starting square like we did with the quiet one square advance
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(oneSqAdvancePromotion));
        Move startSquare  = endSquare - (8 * colorMultiplier);

        /* Store all 4 types of pawn promotion
         Flag = 8  : Knight
         Flag = 9  : Bishop
         Flag = 10 : Rook
         Flag = 11 : Queen
        */
        for (Move flag = 8; flag < 12; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);

            moves[numOfMoves++] = move;
        }
    }
}


// Calculates and stores every pseudo-legal white pawn move
void generateWhitePawnPseudoMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard whitePawns = pos.getPieceBitboard(wp);
    Bitboard blackPieces = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;


    // Bitboard to map all the single square white pawn advances
    Bitboard oneSqAdvance = ((whitePawns << 8) & empty);

    // Bitboard to map all the double square white pawn advances
    Bitboard twoSqAdvance = ((oneSqAdvance & rank3) << 8) & empty;

    // Differentiate between quiet single square advances and promotion advances based on the rank of the destination square
    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank8;
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank8;

    // Store the moves we have found so far with the appropriate 16 bit move encoding
    pseudoPawnAdvances(oneSqAdvanceQuiet, oneSqAdvancePromotion, White, moves, numOfMoves);

    // Store double square advances
    while (twoSqAdvance)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(twoSqAdvance));
        Move startSquare  = endSquare - 16;

        // The flag for double square push is 1
        Move move = startSquare | (endSquare << 6) | (1 << 12);
        moves[numOfMoves++] = move;
    }

    // WHITE PAWN CAPTURES:

    // Leftside captures
    Bitboard whitePawnLeftCaptures = ((whitePawns & ~fileA) << 7) & blackPieces;

    // Differentiate between normal lefside captures and captures that lead to promotion
    Bitboard whitePawnLeftNormalCaptures = whitePawnLeftCaptures & ~rank8;
    Bitboard whitePawnLeftPromoCaptures = whitePawnLeftCaptures & rank8;

    // Store normal leftside captures
    while (whitePawnLeftNormalCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnLeftNormalCaptures));
        Move startSquare = endSquare - 7;

        // Flag for normal captures is set to 4
        Move move = startSquare | (endSquare << 6) | (4 << 12);

        // Store the move
        moves[numOfMoves++] = move;
    }

    // Store every leftside capture that leads to promotion
    while (whitePawnLeftPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnLeftPromoCaptures));
        Move startSquare = endSquare - 7;

        /* Include all 4 types of promotion capture moves
         Flag = 12 : Knight Promotion & Capture
         Flag = 13 : Bishop Promotion & Capture
         Flag = 14 : Rook Promotion & Capture
         Flag = 15 : Queen Promotion & Capture
        */

        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);
            moves[numOfMoves++] = move;
        }
    }


    // Rightside captures
    Bitboard whitePawnRightCaptures = ((whitePawns & ~fileH) << 9) & blackPieces;

    // Differentiate between normal rightside captures and captures that lead to promotion
    Bitboard whitePawnRightQuietCaptures = whitePawnRightCaptures & ~rank8;
    Bitboard whitePawnRightPromoCaptures = whitePawnRightCaptures & rank8;

    // Store normal rightside captures
    while (whitePawnRightQuietCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnRightQuietCaptures));
        Move startSquare = endSquare - 9;

        // Flag for normal captures is set to 4
        Move move = startSquare | (endSquare << 6) | (4 << 12);

        // Store the move
        moves[numOfMoves++] = move;
    }


    // Store every rightside capture that leads to promotion
    while (whitePawnRightPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnRightPromoCaptures));
        Move startSquare = endSquare - 9;

        /* Include all 4 types of promotion capture moves
         Flag = 12 : Knight Promotion & Capture
         Flag = 13 : Bishop Promotion & Capture
         Flag = 14 : Rook Promotion & Capture
         Flag = 15 : Queen Promotion & Capture
        */

        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);
            moves[numOfMoves++] = move;
        }
    }


    // EN-PASSANT SECTION
    if (pos.getEnpassantSquare() != NO_SQUARE)
    {
        int epEndSquare = pos.getEnpassantSquare();

        // Bitboard to mask the square where en-passant can be played
        Bitboard epMask = 1ULL << epEndSquare;

        // Bitboard to mark the square from which en-passant can be played from the left side
        Bitboard leftEpCap = ((whitePawns & ~fileA) << 7) & epMask;

        // If leftside en-passant can be played
        if (leftEpCap)
        {
            int epStartSquare = epEndSquare - 7;

            // The flag for en-passant is set to 5
            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            moves[numOfMoves++] = move;
        }

        // Same for rightside en-passant
        Bitboard rightEpCap = ((whitePawns & ~fileH) << 9) & epMask;

        // Check if there is any right-side en-passant candidate square
        if (rightEpCap)
        {
            int epStartSquare = epEndSquare - 9;

            // Store the move setting its flag to 5 to signal en-passant
            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);
            moves[numOfMoves++] = move;
        }
    }
}

// Calculates and stores every pseudo-legal black pawn move
void generateBlackPawnPseudoMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard blackPawns = pos.getPieceBitboard(bp);
    Bitboard whitePieces = pos.getWhiteBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;

    // Bitboard to map all the single square black pawn advances
    Bitboard oneSqAdvance = (blackPawns >> 8) & empty;

    // Bitboard to map all the double square black pawn advances
    Bitboard twoSqAdvance = ((oneSqAdvance & rank6) >> 8) & empty;

    // Bitboard to map all the double square black pawn advances
    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank1;
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank1;

    // Bitboard to map all the double square white pawn advances
    pseudoPawnAdvances(oneSqAdvanceQuiet, oneSqAdvancePromotion, Black, moves, numOfMoves);

    // Store double square advances
    while (twoSqAdvance)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(twoSqAdvance));
        Move startSquare  = endSquare + 16;

        // The flag for double square push is 1
        Move move = startSquare | (endSquare << 6) | (1 << 12);
        moves[numOfMoves++] = move;
    }

    // BLACK PAWN CAPTURES:

    // Leftside captures
    Bitboard blackPawnLeftCaptures = ((blackPawns & ~fileA) >> 9) & whitePieces;

    // Differentiate between normal lefside captures and captures that lead to promotion
    Bitboard blackPawnLeftNormalCaptures = blackPawnLeftCaptures & ~rank1;
    Bitboard blackPawnLeftPromoCaptures = blackPawnLeftCaptures & rank1;

    // Store normal leftside captures
    while (blackPawnLeftNormalCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnLeftNormalCaptures));
        Move startSquare = endSquare + 9;

        Move move = startSquare | (endSquare << 6) | (4 << 12);

        // Store the move
        moves[numOfMoves++] = move;
    }

    // Store every leftside capture that leads to promotion
    while (blackPawnLeftPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnLeftPromoCaptures));
        Move startSquare = endSquare + 9;

        /* Include all 4 types of promotion-capture moves
         Flag = 12 : Knight Promotion & Capture
         Flag = 13 : Bishop Promotion & Capture
         Flag = 14 : Rook Promotion & Capture
         Flag = 15 : Queen Promotion & Capture
        */
        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);
            moves[numOfMoves++] = move;
        }
    }


    // Rightside captures
    Bitboard blackPawnRightCaptures = ((blackPawns & ~fileH) >> 7) & whitePieces;

    // Differentiate between normal rightside captures and captures that lead to promotion
    Bitboard blackPawnRightQuietCaptures = blackPawnRightCaptures & ~rank1;
    Bitboard blackPawnRightPromoCaptures = blackPawnRightCaptures & rank1;


    // Store normal rightside captures
    while (blackPawnRightQuietCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnRightQuietCaptures));
        Move startSquare = endSquare + 7;

        // Flag for normal captures is set to 4
        Move move = startSquare | (endSquare << 6) | (4 << 12);

        // Store the move
        moves[numOfMoves++] = move;
    }

    // Store every rightside capture that leads to promotion
    while (blackPawnRightPromoCaptures)
    {

        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnRightPromoCaptures));
        Move startSquare = endSquare + 7;

        /* Include all 4 types of promotion capture moves
         Flag = 12 : Knight Promotion & Capture
         Flag = 13 : Bishop Promotion & Capture
         Flag = 14 : Rook Promotion & Capture
         Flag = 15 : Queen Promotion & Capture
        */
        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);
            moves[numOfMoves++] = move;
        }
    }

    // EN-PASSANT SECTION
    if (pos.getEnpassantSquare() != NO_SQUARE)
    {
        int epEndSquare = pos.getEnpassantSquare();

        // Bitboard to mask the square where en-passant can be played
        Bitboard epMask = 1ULL << epEndSquare;

        // Bitboard to mark the square from which en-passant can be played from the left side
        Bitboard leftEpCap = ((blackPawns & ~fileA) >> 9) & epMask;


        // If leftside en-passant can be played
        if (leftEpCap)
        {
            int epStartSquare = epEndSquare + 9;

            // The flag for en-passant is set to 5
            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            moves[numOfMoves++] = move;
        }


        // Rightside en-passant
        Bitboard rightEpCap = ((blackPawns & ~fileH) >> 7) & epMask;

        // Check if there is any right-side en-passant candidate square
        if (rightEpCap)
        {
            int epStartSquare = epEndSquare + 7;

            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            moves[numOfMoves++] = move;
        }
    }
}



// Generate all pseudo-legal knight moves for a given side
void generateKnightPseudoMoves(Bitboard knightPos, Bitboard allyPieces, Bitboard enemyPieces, Move moves[], int& numOfMoves)
{
    // Loop for every knight in the board
    while (knightPos)
    {
        // Extract the number of the square the current knight is on
        int startSquare = popLsbAndReturnIndex(knightPos);

        // Use the precomputed knight attacks table to quickly get every pseudo-legal knight move
        // Keep only the moves where the destination square is not occupied by a friendly piece
        Bitboard knightMoves = knightAttacks[startSquare] & ~allyPieces;

        // Now store every move by splitting them into quiet and capture moves
        Bitboard knightCaptures = knightMoves & enemyPieces;
        Bitboard knightQuietMoves = knightMoves & ~enemyPieces;

        // Store every knight capture move with the flag set to 4
        while (knightCaptures)
        {
            int endSquare = popLsbAndReturnIndex(knightCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);
            moves[numOfMoves++] = move;
        }

        // Store every quiet knight move with flag set to 0
        while (knightQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(knightQuietMoves);
            Move move = (startSquare) | (endSquare << 6);
            moves[numOfMoves++] = move;
        }
    }
}


// Generate all pseudo-legal bishop moves for a given side
void generateBishopPseudoMoves(Bitboard bishopPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every bishop in the board
    while (bishopPos)
    {
        int startSquare = popLsbAndReturnIndex(bishopPos);

        // Use magic bitboards to quickly get all the pseudo-legal bishop moves
        // Exclude every square that is occupied by a friendly piece
        Bitboard bishopMoves = getBishopAttacks(startSquare, occupied) & ~allyPieces;

        // Store every capture and quiet bishop move seperately
        Bitboard bishopCaptures = bishopMoves & enemyPieces;
        Bitboard bishopQuietMoves = bishopMoves & ~enemyPieces;

        // Store captures
        while (bishopCaptures)
        {
            int endSquare = popLsbAndReturnIndex(bishopCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            moves[numOfMoves++] = move;
        }

        // Store quiet moves
        while (bishopQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(bishopQuietMoves);
            Move move = (startSquare) | (endSquare << 6);

            moves[numOfMoves++] = move;
        }
    }
}

// Generate all pseudo-legal rook moves for a given side
void generateRookPseudoMoves(Bitboard rookPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every rook in the board
    while (rookPos)
    {
        int startSquare = popLsbAndReturnIndex(rookPos);

        // Use magic bitboards to quickly get all the pseudo-legal rook moves
        // Exclude every square that is occupied by a friendly piece
        Bitboard rookMoves = getRookAttacks(startSquare, occupied) & ~allyPieces;

        Bitboard rookCaptures = rookMoves & enemyPieces;
        Bitboard rookQuietMoves = rookMoves & ~enemyPieces;

        // Store captures
        while (rookCaptures)
        {
            int endSquare = popLsbAndReturnIndex(rookCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            moves[numOfMoves++] = move;
        }

        // Store quiet moves
        while (rookQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(rookQuietMoves);
            Move move = (startSquare) | (endSquare << 6);

            moves[numOfMoves++] = move;
        }
    }
}

// Generate all pseudo-legal queen moves for a given side
void generateQueenPseudoMoves(Bitboard queenPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    generateRookPseudoMoves(queenPos, allyPieces, enemyPieces, occupied, moves, numOfMoves);
    generateBishopPseudoMoves(queenPos, allyPieces, enemyPieces, occupied, moves, numOfMoves);
}


// Generate every pseudo-legal king move excluding castling
void generateKingMoves(Bitboard kingPos, Bitboard allyPieces, Bitboard enemyPieces, const Position& pos, Move moves[], int& numOfMoves)
{
    int startSquare = popLsbAndReturnIndex(kingPos);

    // Use the precomputed king attacks table to quickly generate king moves
    Bitboard kingMoves = kingAttacks[startSquare] & ~allyPieces;
    Bitboard kingCaptures = kingMoves & enemyPieces;
    Bitboard kingQuietMoves = kingMoves & ~enemyPieces;


    // Store every king capture move
    while (kingCaptures)
    {
        int endSquare = popLsbAndReturnIndex(kingCaptures);
        Move move = (startSquare) | (endSquare << 6) | (4 << 12);
        moves[numOfMoves++] = move;
    }

    // Store every quiet king move
    while (kingQuietMoves)
    {
        int endSquare = popLsbAndReturnIndex(kingQuietMoves);
        Move move = (startSquare) | (endSquare << 6);
        moves[numOfMoves++] = move;
    }
}


// Generates every pseudo-legal black king move apart from castling
// Only stored castling moves that pass the legality check
void generateWhiteKingPseudoMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard whiteKingBB = pos.getPieceBitboard(wK);
    Bitboard whitePiecesBB = pos.getWhiteBitboard();
    Bitboard blackPiecesBB = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();

    // Generate every non-castling move
    generateKingMoves(whiteKingBB, whitePiecesBB, blackPiecesBB, pos, moves, numOfMoves);

    // CASTLING MOVES SECTION :
    CastlingRights castleRights = pos.getCastlingRights();

    // Store white kingside castles if the squares that the king passes are not under attack by an enemy piece
    if (castleRights & WK) // If white kingside castling is allowed
    {
        if ((occupied & 96) == 0) // 96 is the king side castling squares mask, the squares that need to be empty, between king and rook
        {
            if (!squareUnderAttack(e1, Black, pos, occupied))
            {
                if (!squareUnderAttack(f1, Black, pos, occupied))
                {
                    if (!squareUnderAttack(g1, Black, pos, occupied))
                    {
                        Move move = 4 | (6 << 6) | (2 << 12);
                        moves[numOfMoves++] = move;
                    }
                }
            }
        }
    }

    // Store white queenside castles if the squares that the king passes are not under attack by an enemy piece
    if (castleRights & WQ) // if white queenside castling is allowed
    {
        if ((occupied & 14) == 0) // 14 is the queen side castling squares mask
        {
            if (!squareUnderAttack(e1, Black, pos, occupied))
            {
                if (!squareUnderAttack(d1, Black, pos, occupied))
                {
                    if (!squareUnderAttack(c1, Black, pos, occupied))
                    {
                        Move move = 4 | (2 << 6) | (3 << 12);
                        moves[numOfMoves++] = move;
                    }
                }
            }
        }
    }
}



// Generates every pseudo-legal white king move apart from castling
// Only stored castling moves that pass the legality check
void generateBlackKingPseudoMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard blackKingBB = pos.getPieceBitboard(bK);
    Bitboard whitePiecesBB = pos.getWhiteBitboard();
    Bitboard blackPiecesBB = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();

    // Generate every non-castling move
    generateKingMoves(blackKingBB, blackPiecesBB, whitePiecesBB, pos, moves, numOfMoves);

    // CASTLING MOVES SECTION :
    CastlingRights castleRights = pos.getCastlingRights();

    // Store black kingside castles if the squares that the king passes are not under attack by an enemy piece
    if (castleRights & BK) // If black kingside castling is allowed
    {
        if ((occupied & 0x6000000000000000) == 0) // 0x6000000000000000 is the black king side castling mask of inbetween squares
        {
            if (!squareUnderAttack(e8, White, pos, occupied))
            {
                if (!squareUnderAttack(f8, White, pos, occupied))
                {
                    if (!squareUnderAttack(g8, White, pos, occupied))
                    {
                        Move move = 60 | (62 << 6) | (2 << 12);
                        moves[numOfMoves++] = move;
                    }
                }
            }
        }
    }

    // Store black queenside castles if the squares that the king passes are not under attack by an enemy piece
    if (castleRights & BQ) // If black queenside castling is allowed
    {
        if ((occupied & 0x0E00000000000000) == 0) // 0x0E00000000000000 is the queen side castling squares mask
        {
            if (!squareUnderAttack(e8, White, pos, occupied))
            {
                if (!squareUnderAttack(d8, White, pos, occupied))
                {
                    if (!squareUnderAttack(c8, White, pos, occupied))
                    {
                        Move move = 60 | (58 << 6) | (3 << 12);
                        moves[numOfMoves++] = move;
                    }
                }
            }
        }
    }
}



// generates ONLY LEGAL moves, must receive a legalityInformation struct and a position argument
void generatePseudoLegalMoves(Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard occupied = pos.getOccupiedBitboard();

    if (pos.isWhiteToMove())
    {
        Bitboard allyPieces = pos.getWhiteBitboard();
        Bitboard enemyPieces = pos.getBlackBitboard();

        generateWhitePawnPseudoMoves(pos, moves, numOfMoves);

        generateKnightPseudoMoves(pos.getPieceBitboard(wN), allyPieces, enemyPieces, moves, numOfMoves);

        generateBishopPseudoMoves(pos.getPieceBitboard(wB), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateRookPseudoMoves(pos.getPieceBitboard(wR), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateQueenPseudoMoves(pos.getPieceBitboard(wQ), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateWhiteKingPseudoMoves(pos, moves, numOfMoves);
    }
    else
    {
        Bitboard allyPieces = pos.getBlackBitboard();
        Bitboard enemyPieces = pos.getWhiteBitboard();

        generateBlackPawnPseudoMoves(pos, moves, numOfMoves);

        generateKnightPseudoMoves(pos.getPieceBitboard(bN), allyPieces, enemyPieces, moves, numOfMoves);

        generateBishopPseudoMoves(pos.getPieceBitboard(bB), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateRookPseudoMoves(pos.getPieceBitboard(bR), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateQueenPseudoMoves(pos.getPieceBitboard(bQ), allyPieces, enemyPieces, occupied, moves, numOfMoves);

        generateBlackKingPseudoMoves(pos, moves, numOfMoves);
    }

}