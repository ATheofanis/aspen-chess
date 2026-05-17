//
// Created by theoa on 13/05/2026.
//

#include "PseudoMoveGen.h"
#include "Position.h"

//    |======================================|
//    |  Pseudo-Legal Quiet Moves Generator  |
//    |======================================|


// Store every pseudo-legal quiet pawn move
void pseudoPawnAdvances(Bitboard oneSqAdvance, Bitboard twoSqAdvance, Color pawnColor, Move moves[], int &numOfMoves)
{
    // Store every quiet one-square advance
    while (oneSqAdvance)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(oneSqAdvance));

        // Get the start square based on the pawn's color
        Move startSquare  = endSquare - (pawnColor == White ? 8 : -8);

        // Store the move with flag set to 0
        Move move = startSquare | (endSquare << 6);

        moves[numOfMoves++] = move;
    }

    // Store every quiet two-square advance
    while (twoSqAdvance)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(twoSqAdvance));

        // Get the start square based on the pawn's color
        Move startSquare  = endSquare - (pawnColor == White ? 16 : -16);

        // The flag for double square push is 1
        Move move = startSquare | (endSquare << 6) | (1 << 12);
        moves[numOfMoves++] = move;
    }
}


// Calculates and stores every pseudo-legal white pawn move
void generateWhitePawnPseudoQuietMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard whitePawns = pos.getPieceBitboard(wp);
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;


    // Bitboard to map all the single square white pawn advances
    Bitboard oneSqAdvance = ((whitePawns << 8) & empty);

    // Bitboard to map all the double square white pawn advances
    Bitboard twoSqAdvance = ((oneSqAdvance & rank3) << 8) & empty;

    // Keep only the quiet moves - those that do not lead to promotions
    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank8;

    // Store the moves we have found so far with the appropriate 16 bit move encoding
    pseudoPawnAdvances(oneSqAdvanceQuiet, twoSqAdvance, White, moves, numOfMoves);
}

// Calculates and stores every pseudo-legal black pawn move
void generateBlackPawnPseudoQuietMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard blackPawns = pos.getPieceBitboard(bp);
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;

    // Bitboard to map all the single square black pawn advances
    Bitboard oneSqAdvance = (blackPawns >> 8) & empty;

    // Bitboard to map all the double square black pawn advances
    Bitboard twoSqAdvance = ((oneSqAdvance & rank6) >> 8) & empty;

    // Keep only the quiet moves - those that do not lead to promotions
    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank1;

    // Bitboard to map all the double square white pawn advances
    pseudoPawnAdvances(oneSqAdvanceQuiet, twoSqAdvance, Black, moves, numOfMoves);
}



// Generate all pseudo-legal quiet knight moves for a given side
void generateKnightPseudoQuietMoves(Bitboard knightPos, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop for every knight in the board
    while (knightPos)
    {
        // Extract the number of the square the current knight is on
        int startSquare = popLsbAndReturnIndex(knightPos);

        // Use the precomputed knight attacks table to quickly get every pseudo-legal knight move
        // Keep only quiet moves, those with empty destination squares
        Bitboard quietKnightMoves = knightAttacks[startSquare] & ~occupied;

        // Store every quiet knight move with flag set to 0
        while (quietKnightMoves)
        {
            int endSquare = popLsbAndReturnIndex(quietKnightMoves);
            Move move = (startSquare) | (endSquare << 6);
            moves[numOfMoves++] = move;
        }
    }
}


// Generate all pseudo-legal quiet bishop moves for a given side
void generateBishopPseudoQuietMoves(Bitboard bishopPos, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every bishop in the board
    while (bishopPos)
    {
        int startSquare = popLsbAndReturnIndex(bishopPos);

        // Use magic bitboards to quickly get all the pseudo-legal bishop moves
        // Exclude every square that is occupied by a friendly or an enemy piece
        Bitboard quietBishopMoves = getBishopAttacks(startSquare, occupied) & ~occupied;

        // Store quiet moves
        while (quietBishopMoves)
        {
            int endSquare = popLsbAndReturnIndex(quietBishopMoves);
            Move move = (startSquare) | (endSquare << 6);

            moves[numOfMoves++] = move;
        }
    }
}

// Generate all pseudo-legal quiet rook moves for a given side
void generateRookPseudoQuietMoves(Bitboard rookPos, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every rook in the board
    while (rookPos)
    {
        int startSquare = popLsbAndReturnIndex(rookPos);

        // Use magic bitboards to quickly get all the pseudo-legal rook moves
        // Exclude every square that is occupied by either a friendly or an enemy piece
        Bitboard quietRookMoves = getRookAttacks(startSquare, occupied) & ~occupied;

        // Store quiet moves
        while (quietRookMoves)
        {
            int endSquare = popLsbAndReturnIndex(quietRookMoves);
            Move move = (startSquare) | (endSquare << 6);

            moves[numOfMoves++] = move;
        }
    }
}

// Generate all pseudo-legal queen moves for a given side
void generateQueenPseudoQuietMoves(Bitboard queenPos, Bitboard occupied, Move moves[], int& numOfMoves)
{
    generateRookPseudoQuietMoves(queenPos, occupied, moves, numOfMoves);
    generateBishopPseudoQuietMoves(queenPos, occupied, moves, numOfMoves);
}


// Generate every pseudo-legal quiet king move excluding castling
void generateKingPseudoQuietMoves(Bitboard kingPos, Bitboard occupied, Move moves[], int& numOfMoves)
{
    int startSquare = popLsbAndReturnIndex(kingPos);

    // Use the precomputed king attacks table to quickly generate quiet king moves
    Bitboard quietKingMoves = kingAttacks[startSquare] & ~occupied;

    // Store every quiet king move
    while (quietKingMoves)
    {
        int endSquare = popLsbAndReturnIndex(quietKingMoves);
        Move move = (startSquare) | (endSquare << 6);
        moves[numOfMoves++] = move;
    }
}


// Generates every quiet pseudo-legal black king move apart from castling
void generateWhiteKingPseudoQuietMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard whiteKingBB = pos.getPieceBitboard(wK);
    Bitboard occupied = pos.getOccupiedBitboard();

    // Generate every quiet non-castling move
    generateKingPseudoQuietMoves(whiteKingBB, occupied, moves, numOfMoves);

    // CASTLING MOVES SECTION - Legality will be checked afterwards:
    CastlingRights castleRights = pos.getCastlingRights();

    // Store white kingside castles
    if (castleRights & WK) // If white kingside castling is allowed
    {
        if ((occupied & 96) == 0) // 96 is the king side castling squares mask, the squares that need to be empty, between king and rook
        {
            Move move = 4 | (6 << 6) | (2 << 12);
            moves[numOfMoves++] = move;
        }
    }

    // Store white queenside castles
    if (castleRights & WQ) // if white queenside castling is allowed
    {
        if ((occupied & 14) == 0) // 14 is the queen side castling squares mask
        {
            Move move = 4 | (2 << 6) | (3 << 12);
            moves[numOfMoves++] = move;
        }
    }
}



// Generates every pseudo-legal white king move apart from castling
// Only stored castling moves that pass the legality check
void generateBlackKingPseudoQuietMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard blackKingBB = pos.getPieceBitboard(bK);
    Bitboard occupied = pos.getOccupiedBitboard();

    // Generate every quiet non-castling move
    generateKingPseudoQuietMoves(blackKingBB, occupied, moves, numOfMoves);

    // CASTLING MOVES SECTION :
    CastlingRights castleRights = pos.getCastlingRights();

    // Store black kingside castles
    if (castleRights & BK) // If black kingside castling is allowed
    {
        if ((occupied & 0x6000000000000000) == 0) // 0x6000000000000000 is the black king side castling mask of inbetween squares
        {
            Move move = 60 | (62 << 6) | (2 << 12);
            moves[numOfMoves++] = move;
        }
    }

    // Store black queenside castles
    if (castleRights & BQ) // If black queenside castling is allowed
    {
        if ((occupied & 0x0E00000000000000) == 0) // 0x0E00000000000000 is the queen side castling squares mask
        {
            Move move = 60 | (58 << 6) | (3 << 12);
            moves[numOfMoves++] = move;
        }
    }
}



// Generates every single pseudo-legal quiet move
void generatePseudoLegalQuietMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard occupied = pos.getOccupiedBitboard();

    if (pos.isWhiteToMove())
    {
        generateWhitePawnPseudoQuietMoves(pos, moves, numOfMoves);

        generateKnightPseudoQuietMoves(pos.getPieceBitboard(wN), occupied, moves, numOfMoves);

        generateBishopPseudoQuietMoves(pos.getPieceBitboard(wB), occupied, moves, numOfMoves);

        generateRookPseudoQuietMoves(pos.getPieceBitboard(wR), occupied, moves, numOfMoves);

        generateQueenPseudoQuietMoves(pos.getPieceBitboard(wQ), occupied, moves, numOfMoves);

        generateWhiteKingPseudoQuietMoves(pos, moves, numOfMoves);
    }
    else
    {
        generateBlackPawnPseudoQuietMoves(pos, moves, numOfMoves);

        generateKnightPseudoQuietMoves(pos.getPieceBitboard(bN), occupied, moves, numOfMoves);

        generateBishopPseudoQuietMoves(pos.getPieceBitboard(bB), occupied, moves, numOfMoves);

        generateRookPseudoQuietMoves(pos.getPieceBitboard(bR), occupied, moves, numOfMoves);

        generateQueenPseudoQuietMoves(pos.getPieceBitboard(bQ), occupied, moves, numOfMoves);

        generateBlackKingPseudoQuietMoves(pos, moves, numOfMoves);
    }
}




//    |==================================================|
//    |  Pseudo-Legal Captures And Promotions Generator  |
//    |==================================================|


// Store every single pseudo-legal promotion move
void pseudoPawnPromotions(Bitboard promotions, Color pawnColor, Move moves[], int &numOfMoves)
{
    // Loop through all the available promotion squares
    while (promotions)
    {
        // Get the end square and the starting square
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(promotions));
        Move startSquare  = endSquare - (pawnColor == White ? 8 : -8);

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


// Calculates and stores every pseudo-legal white pawn capture and promotion move
void generateWhitePawnPseudoCapturesAndPromo(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard whitePawns = pos.getPieceBitboard(wp);
    Bitboard blackPieces = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;


    // Bitboard to map all the single square white pawn advances
    Bitboard oneSqAdvance = ((whitePawns << 8) & empty);

    // Only keep the one square advances that lead to promotion
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank8;

    // Store all non-capture promotions
    pseudoPawnPromotions(oneSqAdvancePromotion, White, moves, numOfMoves);


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

// Calculates and stores every pseudo-legal black pawn promotion and capture move
void generateBlackPawnPseudoCapturesAndPromo(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard blackPawns = pos.getPieceBitboard(bp);
    Bitboard whitePieces = pos.getWhiteBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;

    // Bitboard to map all the single square black pawn advances
    Bitboard oneSqAdvance = (blackPawns >> 8) & empty;

    // Only keep promotions
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank1;

    // Store every non-capture promotion
    pseudoPawnPromotions(oneSqAdvancePromotion, Black, moves, numOfMoves);

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



// Generate all pseudo-legal knight captures for a given side
void generateKnightPseudoCaptures(Bitboard knightPos, Bitboard enemyPieces, Move moves[], int& numOfMoves)
{
    // Loop for every knight in the board
    while (knightPos)
    {
        // Extract the number of the square the current knight is on
        int startSquare = popLsbAndReturnIndex(knightPos);

        // Use the precomputed knight attacks table to quickly get every pseudo-legal knight move
        // Keep only the moves where the destination square is occupied by an enemy piece
        Bitboard knightCaptures = knightAttacks[startSquare] & enemyPieces;


        // Store every knight capture move with the flag set to 4
        while (knightCaptures)
        {
            int endSquare = popLsbAndReturnIndex(knightCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);
            moves[numOfMoves++] = move;
        }
    }
}


// Generate all pseudo-legal bishop captures for a given side
void generateBishopPseudoCaptures(Bitboard bishopPos, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every bishop in the board
    while (bishopPos)
    {
        int startSquare = popLsbAndReturnIndex(bishopPos);

        // Use magic bitboards to quickly get all the pseudo-legal bishop moves
        // Exclude every square that is not occupied by an enemy piece
        Bitboard bishopCaptures = getBishopAttacks(startSquare, occupied) & enemyPieces;

        // Store captures
        while (bishopCaptures)
        {
            int endSquare = popLsbAndReturnIndex(bishopCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            moves[numOfMoves++] = move;
        }

    }
}

// Generate all pseudo-legal rook captures for a given side
void generateRookPseudoCaptures(Bitboard rookPos, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    // Loop through every rook in the board
    while (rookPos)
    {
        int startSquare = popLsbAndReturnIndex(rookPos);

        // Use magic bitboards to quickly get all the pseudo-legal rook moves
        // Exclude every square that is not occupied by an enemy piece
        Bitboard rookCaptures = getRookAttacks(startSquare, occupied) & enemyPieces;

        // Store captures
        while (rookCaptures)
        {
            int endSquare = popLsbAndReturnIndex(rookCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            moves[numOfMoves++] = move;
        }
    }
}

// Generate all pseudo-legal queen captures for a given side
void generateQueenPseudoCaptures(Bitboard queenPos, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    generateRookPseudoCaptures(queenPos, enemyPieces, occupied, moves, numOfMoves);
    generateBishopPseudoCaptures(queenPos, enemyPieces, occupied, moves, numOfMoves);
}


// Generate every pseudo-legal king capture move
void generateKingPseudoCaptures(Bitboard kingPos, Bitboard enemyPieces, Move moves[], int& numOfMoves)
{
    int startSquare = popLsbAndReturnIndex(kingPos);

    // Use the precomputed king attacks table to quickly generate king captures
    Bitboard kingCaptures = kingAttacks[startSquare] & enemyPieces;

    // Store every king capture move
    while (kingCaptures)
    {
        int endSquare = popLsbAndReturnIndex(kingCaptures);
        Move move = (startSquare) | (endSquare << 6) | (4 << 12);
        moves[numOfMoves++] = move;
    }
}


// Generate every speudo legal capture move for the white king
void generateWhiteKingPseudoCaptures(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard whiteKingBB = pos.getPieceBitboard(wK);
    Bitboard blackPiecesBB = pos.getBlackBitboard();

    // Generate every non-castling move
    generateKingPseudoCaptures(whiteKingBB, blackPiecesBB, moves, numOfMoves);
}



// Generate every speudo legal capture move for the black king
void generateBlackKingPseudoCaptures(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard blackKingBB = pos.getPieceBitboard(bK);
    Bitboard whitePiecesBB = pos.getWhiteBitboard();

    // Generate every non-castling move
    generateKingPseudoCaptures(blackKingBB, whitePiecesBB, moves, numOfMoves);
}

// Generates every pseudo-legal capture and/or promotion
void generatePseudoLegalCapAndPromoMoves(const Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard occupied = pos.getOccupiedBitboard();

    if (pos.isWhiteToMove())
    {
        Bitboard enemyPieces = pos.getBlackBitboard();

        // White pawns
        generateWhitePawnPseudoCapturesAndPromo(pos, moves, numOfMoves);

        // White knights
        generateKnightPseudoCaptures(pos.getPieceBitboard(wN), enemyPieces, moves, numOfMoves);

        // White bishops
        generateBishopPseudoCaptures(pos.getPieceBitboard(wB), enemyPieces, occupied, moves, numOfMoves);

        // White rooks
        generateRookPseudoCaptures(pos.getPieceBitboard(wR), enemyPieces, occupied, moves, numOfMoves);

        // White queens
        generateQueenPseudoCaptures(pos.getPieceBitboard(wQ), enemyPieces, occupied, moves, numOfMoves);

        // White king
        generateWhiteKingPseudoCaptures(pos, moves, numOfMoves);
    }
    else
    {
        Bitboard enemyPieces = pos.getWhiteBitboard();

        // Black pawns
        generateBlackPawnPseudoCapturesAndPromo(pos, moves, numOfMoves);

        // Black knights
        generateKnightPseudoCaptures(pos.getPieceBitboard(bN), enemyPieces, moves, numOfMoves);

        // Black bishops
        generateBishopPseudoCaptures(pos.getPieceBitboard(bB), enemyPieces, occupied, moves, numOfMoves);

        // Black rooks
        generateRookPseudoCaptures(pos.getPieceBitboard(bR), enemyPieces, occupied, moves, numOfMoves);

        // Black queens
        generateQueenPseudoCaptures(pos.getPieceBitboard(bQ), enemyPieces, occupied, moves, numOfMoves);

        // Black king
        generateBlackKingPseudoCaptures(pos, moves, numOfMoves);
    }
}


