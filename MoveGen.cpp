//
// Created by theoa on 07/03/2026.
//

#include "MoveGen.h"

#include "Attacks.h"


// LEGAL MOVES CHECKS -----------------------------------------------------------------------------------------------------------------------------

// in move gen we already have legal moves for king but we also need to check legality for the moves of other pieces
// for example if a rook is pinned its movement is limited to certain squares. this function will extract information to make the generation of legal moves possible and hopefully fast
// it returns information about the location of pinned pieces, pinners, checkers and number of checks.
legalityInformation getLegalityInfo(int kingSquare, Color allyColor, const Position& pos)
{
    legalityInformation info = {};

    // first we can extract info about knight attackers , and if we do find a checker(attacker) increase num of checks
    Bitboard attackingKnight = knightAttacks[kingSquare] & (allyColor ? pos.getPieceBitboard(wN) : pos.getPieceBitboard(bN)); // if ally color is 1 it means ally is black so enemy is white
    if (attackingKnight)
    {
        //std::cout << "UNDER ATTACK BY KNIGHT" << std::endl;
        info.checkers |= attackingKnight;
        //std::cout << "number of attackers: " << info.numOfChecks + 1<< std::endl;
        info.numOfChecks++;
    }

    // also detect pawn checks
    Bitboard attackingPawn = pawnAttacks[allyColor][kingSquare] & (allyColor ? pos.getPieceBitboard(wp) : pos.getPieceBitboard(bp)); // if ally color is 1 it means ally is black, so enemy is white
    if (attackingPawn)
    {
        //std::cout << "UNDER ATTACK BY PAWN" << std::endl;
        info.checkers |= attackingPawn;
        //std::cout << "number of attackers: " << info.numOfChecks + 1<< std::endl;
        info.numOfChecks++;
    }

    info.legalSquaresMask = info.checkers;

    // the legal squares will be the squares between the pinned piece and the attacker if piece is pinned and there is not a double check


    Bitboard enemyQueens;
    Bitboard enemyDiagonalAttackers;
    Bitboard enemyRookAttackers;
    Bitboard allyPieces;
    Bitboard enemyPieces;

    if (allyColor) // if ally color is black
    {
        // enemy : White
        allyPieces = pos.getBlackBitboard();
        enemyPieces = pos.getWhiteBitboard();
        enemyQueens = pos.getPieceBitboard(wQ);
        enemyDiagonalAttackers = pos.getPieceBitboard(wB) | enemyQueens;
        enemyRookAttackers = pos.getPieceBitboard(wR) | enemyQueens;
    } else
    {
        // enemy : Black
        allyPieces = pos.getWhiteBitboard();
        enemyPieces = pos.getBlackBitboard();
        enemyQueens = pos.getPieceBitboard(bQ);
        enemyDiagonalAttackers = pos.getPieceBitboard(bB) | enemyQueens;
        enemyRookAttackers = pos.getPieceBitboard(bR) | enemyQueens;
    }


    // now we extract the pinned and pinners information , first add a check which will hopefully reduce the load of this function by removing the pin info checks:
    // if there are no attackers in the same diagonal or line as the king then there are no pinners or checkers (no checkers because we are already checked for knights and pawns)

    // first for the diagonal attackers
    //Bitboard diagonalAttackers = getBishopAttacks(kingSquare, enemyDiagonalAttackers) & getBishopAttacks(kingSquare, 0ULL);
    Bitboard diagonalAttackers = getBishopAttacks(kingSquare, enemyPieces) & enemyDiagonalAttackers;
    //std::cout << "DIAGONAL ATTACKER BB:";


    while (diagonalAttackers)
    {
        int diagAttackerSquare = popLsbAndReturnIndex(diagonalAttackers);



        Bitboard lineB = lineBetween[kingSquare][diagAttackerSquare];




        Bitboard allyBlockers = lineB & allyPieces;
        int numOfBlockers = bitCount(allyBlockers);


        switch (numOfBlockers)
        {// if there are 0 blockers inbetween then it is a check
        case 0:

            info.numOfChecks++;

            info.checkers |= (1ULL << diagAttackerSquare);



            info.legalSquaresMask |= lineB;
            break;
        // 1 blocker means the blocker is a pinned piece
        case 1:

            info.pinners |= 1ULL << diagAttackerSquare;


            info.pinned |= allyBlockers;




            info.pinnedPieceLegalSquares[lsbIndex(allyBlockers)] = lineB;






            break;

        default:

            break;
        }
    }


    // now straight line attackers

    Bitboard rookAttackers = getRookAttacks(kingSquare, enemyPieces) & enemyRookAttackers;


    while (rookAttackers)
    {
        int rookAttackerSquare = popLsbAndReturnIndex(rookAttackers);

        Bitboard lineB = lineBetween[kingSquare][rookAttackerSquare];
        Bitboard allyBlockers = lineB & allyPieces;
        int numOfBlockers = bitCount(allyBlockers);

        switch (numOfBlockers)
        {
        // if there are 0 blockers inbetween then it is a check
        case 0:
            //std::cout << "0 BLOCKERS BETWEEN ROOK ATTACKER" << std::endl;
            info.numOfChecks++;
            info.checkers |= (1ULL << rookAttackerSquare);
            info.legalSquaresMask |= lineB;


            break;

            // 1 blocker means the blocker is a pinned piece
        case 1:
            //std::cout << "CASE 1 BLOCKER\n";
            info.pinners |= 1ULL << rookAttackerSquare;
            info.pinned |= allyBlockers;

            info.pinnedPieceLegalSquares[lsbIndex(allyBlockers)] = lineB;


            break;

        default:
            break;
        }
    }

    return info;
}

// LEGAL MOVES CHECKS -----------------------------------------------------------------------------------------------------------------------------





// PAWN MOVES ----------------------------------------------------------------------------------------------------------------------------

// store one square advances
void pawnAdvances(const legalityInformation& info, Bitboard oneSqAdvanceQuiet, Bitboard oneSqAdvancePromotion, Color pawnColor, Move moves[], int &numOfMoves)
{
    int colorMultiplier = pawnColor == White ? 1 : -1;
    while (oneSqAdvanceQuiet)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(oneSqAdvanceQuiet));
        Move startSquare  = endSquare - (8 * colorMultiplier);

        // flag for quiet moves is 0
        Move move = startSquare | (endSquare << 6);




        if (!(info.pinned & (1ULL << startSquare)))
        {
            moves[numOfMoves++] = move;

        } else
        {


            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }

    while (oneSqAdvancePromotion)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(oneSqAdvancePromotion));
        Move startSquare  = endSquare - (8 * colorMultiplier);
        // flags for normal promotions
        // checking if it is pinned
        // if the pawn is pinned one step before promotion it can make no legal advances, only captures, because it cant stop a bishop pin without capturing it, and if
        // the pinner is a rook then the only legal move would be to advance but in this case we know the promotion square is empty so that is not a possible case.
        //std::cout << "PROMOTION ADVANCE BEFORE CHECKING IF NOT PINNED\n";
        if (!(info.pinned & (1ULL << startSquare)))
        {
            //std::cout << "PROMOTION ADVANCE NOT PINNED\n";
            for (Move flag = 8; flag < 12; flag++)
            {
                Move move = startSquare | (endSquare << 6) | (flag << 12);

                moves[numOfMoves++] = move;
            }
        }
    }
}

// generate every legal white pawn move
void generateWhitePawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves)
{
    Bitboard whitePawns = pos.getPieceBitboard(wp);
    Bitboard blackPieces = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;


    // one square advance
    Bitboard oneSqAdvance = ((whitePawns << 8) & empty);
    // store two square advances of white pawns (using previously stored one square advances)
    Bitboard twoSqAdvance = ((oneSqAdvance & rank3) << 8) & empty;

    if (info.numOfChecks == 1)
    {
        // apply check masks if num of checks is 1
        oneSqAdvance &= info.legalSquaresMask;
        twoSqAdvance &= info.legalSquaresMask;
        blackPieces &= info.legalSquaresMask; // later for captures
    }

    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank8;
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank8;

    // store for later when we calculate double square push
    //Bitboard oneSqAdvQuiet = oneSqAdvanceQuiet;



    //std::cout << "CALLING PAWN ADVANCES\n";
    pawnAdvances(info, oneSqAdvanceQuiet, oneSqAdvancePromotion, White, moves, numOfMoves);



    //std::cout << "TWO SQUARE PAWN ADVANCES\n";
    while (twoSqAdvance)
    {

        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(twoSqAdvance));
        Move startSquare  = endSquare - 16;
        // flag for double push moves is 1
        Move move = startSquare | (endSquare << 6) | (1 << 12);

        //moves[numOfMoves++] = move;

        if (!(info.pinned & (1ULL << startSquare)))
        {
            //std::cout << "INSERTING MOVE: " << move << std::endl;

            moves[numOfMoves++] = move;
        } else
        {
            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }

    // captures

    // left captures

    Bitboard whitePawnLeftCaptures = ((whitePawns & ~fileA) << 7) & blackPieces;

    Bitboard whitePawnLeftQuietCaptures = whitePawnLeftCaptures & ~rank8;
    Bitboard whitePawnLeftPromoCaptures = whitePawnLeftCaptures & rank8;



    //std::cout << "CALLING PAWN LEFT CAPTURES\n";

    while (whitePawnLeftQuietCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnLeftQuietCaptures));
        Move startSquare = endSquare - 7;


        Move move = startSquare | (endSquare << 6) | (4 << 12);

        if (!(info.pinned & (1ULL << startSquare)))
        {
            moves[numOfMoves++] = move;
        } else
        {
            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }
    // capture with promotion
    while (whitePawnLeftPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnLeftPromoCaptures));
        Move startSquare = endSquare - 7;

        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }


    // right captures
    Bitboard whitePawnRightCaptures = ((whitePawns & ~fileH) << 9) & blackPieces;
    Bitboard whitePawnRightQuietCaptures = whitePawnRightCaptures & ~rank8;
    Bitboard whitePawnRightPromoCaptures = whitePawnRightCaptures & rank8;



    while (whitePawnRightQuietCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnRightQuietCaptures));
        Move startSquare = endSquare - 9;
        Move move = startSquare | (endSquare << 6) | (4 << 12);

        if (!(info.pinned & (1ULL << startSquare)))
        {
            moves[numOfMoves++] = move;
        } else
        {
            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }

    }

    // store right promo captures
    while (whitePawnRightPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(whitePawnRightPromoCaptures));
        Move startSquare = endSquare - 9;
        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);


            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }


    // EN-PASSANT WITH LEGALITY
    if (pos.getEnpassantSquare() != NO_SQUARE)
    {
        int epEndSquare = pos.getEnpassantSquare();
        Bitboard epMask = 1ULL << epEndSquare;

        Bitboard leftEpCap = ((whitePawns & ~fileA) << 7) & epMask;


        // left white ep
        if (leftEpCap)
        {
            int epStartSquare = epEndSquare - 7;

            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            // first check if pawn is pinned
            if (info.pinned & (1ULL << epStartSquare))
            {
                if ((info.pinnedPieceLegalSquares[epStartSquare] & (1ULL << epEndSquare)))
                {
                    moves[numOfMoves++] = move;
                }
            }
            // not pinned : we need to check if king is under attack then we can only do en passant if it is in the info.legal squares
            // the other case is the rook pin, we need to check if making ep gets king in check by rook type of piece
            else
            {
                // if king in check once
                if (info.numOfChecks)
                {
                    if ((info.legalSquaresMask & (1ULL << epEndSquare)) | (info.checkers & (1ULL << (epStartSquare - 1))  )  )
                    {
                        moves[numOfMoves++] = move;
                    }
                }
                // other case: rook type piece pin
                else
                {
                    if (!(squareUnderAttackByRookPiece(lsbIndex(pos.getPieceBitboard(wK)), Black, pos, (occupied & ~(1ULL << epStartSquare) & ~(1ULL << (epStartSquare - 1))   )    )  )   )
                    {
                        moves[numOfMoves++] = move;
                    }
                }

            }

        }

        // right white ep captures


        Bitboard rightEpCap = ((whitePawns & ~fileH) << 9) & epMask;


        // left white ep
        if (rightEpCap)
        {
            int epStartSquare = epEndSquare - 9;

            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            // first check if pawn is pinned
            if (info.pinned & (1ULL << epStartSquare))
            {
                //std::cout << "PINNED, START SQUARE: " << epStartSquare;
                if ((info.pinnedPieceLegalSquares[epStartSquare] & (1ULL << epEndSquare)))
                {
                    moves[numOfMoves++] = move;
                }
            }
            // not pinned : we need to check if king is under attack then we can only do en passant if it is in the info.legal squares
            // the other case is the rook pin, we need to check if making ep gets king in check by rook type of piece
            else
            {
                // if king in check once
                if (info.numOfChecks)
                {
                    if ((info.legalSquaresMask & (1ULL << epEndSquare)) | (info.checkers & (1ULL << (epStartSquare + 1))  )  )
                    {
                        moves[numOfMoves++] = move;
                    }
                }
                // other case: rook type piece pin
                else
                {
                    if (!(squareUnderAttackByRookPiece(lsbIndex(pos.getPieceBitboard(wK)), Black, pos, (occupied & ~(1ULL << epStartSquare) & ~(1ULL << (epStartSquare + 1))   )    )  )   )
                    {
                        moves[numOfMoves++] = move;
                    }
                }

            }

        }

    }

}


// generate every legal black pawn move
void generateBlackPawnMoves(const Position& pos, const legalityInformation& info, Move moves[], int &numOfMoves)
{
    Bitboard blackPawns = pos.getPieceBitboard(bp);
    Bitboard whitePieces = pos.getWhiteBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();
    Bitboard empty = ~occupied;

    // one square advance
    Bitboard oneSqAdvance = (blackPawns >> 8) & empty;
    // store two square advances of black pawns (using previously stored one square advances)
    Bitboard twoSqAdvance = ((oneSqAdvance & rank6) >> 8) & empty;
    if (info.numOfChecks == 1)
    {
        // apply check masks if num of checks is 1
        oneSqAdvance &= info.legalSquaresMask;
        twoSqAdvance &= info.legalSquaresMask;
        whitePieces &= info.legalSquaresMask; // store for captutes later
    }

    Bitboard oneSqAdvanceQuiet = oneSqAdvance & ~rank1;
    Bitboard oneSqAdvancePromotion = oneSqAdvance & rank1;

    // store 1 square push , promotion and quiet moves

    pawnAdvances(info, oneSqAdvanceQuiet, oneSqAdvancePromotion, Black, moves, numOfMoves);


    while (twoSqAdvance)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(twoSqAdvance));
        Move startSquare  = endSquare + 16;
        // flag for double push moves is 1
        Move move = startSquare | (endSquare << 6) | (1 << 12);

        if (!(info.pinned & (1ULL << startSquare)))
        {
            //std::cout << "INSERTING MOVE: " << move << std::endl;
            moves[numOfMoves++] = move;
        } else
        {

            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }

    // captures

    // left captures
    Bitboard blackPawnLeftCaptures = ((blackPawns & ~fileA) >> 9) & whitePieces;

    Bitboard blackPawnLeftQuietCaptures = blackPawnLeftCaptures & ~rank1;
    Bitboard blackPawnLeftPromoCaptures = blackPawnLeftCaptures & rank1;



    while (blackPawnLeftQuietCaptures) // non-promo left captures
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnLeftQuietCaptures));
        Move startSquare = endSquare + 9;

        Move move = startSquare | (endSquare << 6) | (4 << 12);

        if (!(info.pinned & (1ULL << startSquare)))
        {
            moves[numOfMoves++] = move;
        } else
        {
            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }

    // capture with promotion
    while (blackPawnLeftPromoCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnLeftPromoCaptures));
        Move startSquare = endSquare + 9;

        for (Move flag = 12; flag < 16; flag++)
        {
            Move move = startSquare | (endSquare << 6) | (flag << 12);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }


    // right captures
    Bitboard blackPawnRightCaptures = ((blackPawns & ~fileH) >> 7) & whitePieces;
    Bitboard blackPawnRightQuietCaptures = blackPawnRightCaptures & ~rank1;
    Bitboard blackPawnRightPromoCaptures = blackPawnRightCaptures & rank1;



    while (blackPawnRightQuietCaptures)
    {
        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnRightQuietCaptures));
        Move startSquare = endSquare + 7;
        Move move = startSquare | (endSquare << 6) | (4 << 12);


        if (!(info.pinned & (1ULL << startSquare)))
        {
            moves[numOfMoves++] = move;
        } else
        {
            if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
            {
                moves[numOfMoves++] = move;
            }
        }
    }


    // store right promo captures
    while (blackPawnRightPromoCaptures)
    {
        //std::cout << "BLACK PAWN RIGHT PROMO CAPTURES:";

        Move endSquare = static_cast<Move>(popLsbAndReturnIndex(blackPawnRightPromoCaptures));
        Move startSquare = endSquare + 7;
        for (Move flag = 12; flag < 16; flag++)
        {

            Move move = startSquare | (endSquare << 6) | (flag << 12);
            //printMove(move);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }



    // en-passant captures for black pawns
    if (pos.getEnpassantSquare() != NO_SQUARE)
    {
        int epEndSquare = pos.getEnpassantSquare();
        Bitboard epMask = 1ULL << epEndSquare;

        Bitboard leftEpCap = ((blackPawns & ~fileA) >> 9) & epMask;


        // left white ep
        if (leftEpCap)
        {
            int epStartSquare = epEndSquare + 9;

            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            // first check if pawn is pinned
            if (info.pinned & (1ULL << epStartSquare))
            {
                if (info.pinnedPieceLegalSquares[epStartSquare] & (1ULL << epEndSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
            // not pinned : we need to check if king is under attack then we can only do en passant if it is in the info.legal squares
            // the other case is the rook pin, we need to check if making ep gets king in check by rook type of piece
            else
            {
                // if king in check once
                if (info.numOfChecks)
                {
                    if ((info.legalSquaresMask & (1ULL << epEndSquare)) | (info.checkers & (1ULL << (epStartSquare - 1))  )  )
                    {
                        moves[numOfMoves++] = move;
                    }
                }
                // other case: rook type piece pin
                else
                {
                    if (!(squareUnderAttackByRookPiece(lsbIndex(pos.getPieceBitboard(bK)), White, pos, (occupied & ~(1ULL << epStartSquare) & ~(1ULL << (epStartSquare - 1))   )    )  )   )
                    {
                        moves[numOfMoves++] = move;
                    }
                }

            }

        }

        // right white ep captures


        Bitboard rightEpCap = ((blackPawns & ~fileH) >> 7) & epMask;


        // left white ep
        if (rightEpCap)
        {
            int epStartSquare = epEndSquare + 7;

            Move move = epStartSquare | (epEndSquare << 6) | (5 << 12);

            // first check if pawn is pinned
            if (info.pinned & (1ULL << epStartSquare))
            {
                if (info.pinnedPieceLegalSquares[epStartSquare] & (1ULL << epEndSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
            // not pinned : we need to check if king is under attack then we can only do en passant if it is in the info.legal squares
            // the other case is the rook pin, we need to check if making ep gets king in check by rook type of piece
            else
            {
                // if king in check once
                if (info.numOfChecks)
                {
                    if ((info.legalSquaresMask & (1ULL << epEndSquare)) | (info.checkers & (1ULL << (epStartSquare + 1))  )  )
                    {
                        moves[numOfMoves++] = move;
                    }
                }
                // other case: rook type piece pin
                else
                {
                    if (!(squareUnderAttackByRookPiece(lsbIndex(pos.getPieceBitboard(bK)), White, pos, (occupied & ~(1ULL << epStartSquare) & ~(1ULL << (epStartSquare + 1))   )    )  )   )
                    {
                        moves[numOfMoves++] = move;
                    }
                }

            }

        }

    }


}

// PAWN MOVES ----------------------------------------------------------------------------------------------------------------------------






// KNIGHT MOVES ----------------------------------------------------------------------------------------------------------------------------

// generate every legal knight move for either black or white
void generateKnightMoves(const legalityInformation& info, Bitboard knightPos, Bitboard allyPieces, Bitboard enemyPieces, Move moves[], int& numOfMoves)
{
    while (knightPos)
    {
        int startSquare = popLsbAndReturnIndex(knightPos);
        if (info.pinned & (1ULL << startSquare)) continue;

        if (info.numOfChecks == 1)
        {

        }
        Bitboard knightMoves = knightAttacks[startSquare] & ~allyPieces;
        if (info.numOfChecks == 1)
        {
            knightMoves &= info.legalSquaresMask;
        }
        Bitboard knightCaptures = knightMoves & enemyPieces;
        Bitboard knightQuietMoves = knightMoves & ~enemyPieces;


        while (knightCaptures)
        {
            int endSquare = popLsbAndReturnIndex(knightCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);
            moves[numOfMoves++] = move;
        }


        while (knightQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(knightQuietMoves);
            Move move = (startSquare) | (endSquare << 6);
            moves[numOfMoves++] = move;
        }
    }
}

// KNIGHT MOVES ----------------------------------------------------------------------------------------------------------------------------







// BISHOP MOVES ----------------------------------------------------------------------------------------------------------------------------

// generate every legal bishop move for either black or white
void generateBishopMoves(const legalityInformation& info, Bitboard bishopPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{

    while (bishopPos)
    {
        int startSquare = popLsbAndReturnIndex(bishopPos);
        Bitboard bishopMoves = getBishopAttacks(startSquare, occupied) & ~allyPieces;
        if (info.numOfChecks == 1)
        {
            bishopMoves &= info.legalSquaresMask;
        }
        Bitboard bishopCaptures = bishopMoves & enemyPieces;
        Bitboard bishopQuietMoves = bishopMoves & ~enemyPieces;



        while (bishopCaptures)
        {
            int endSquare = popLsbAndReturnIndex(bishopCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }


        while (bishopQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(bishopQuietMoves);
            Move move = (startSquare) | (endSquare << 6);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {


                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }
}

// BISHOP MOVES ----------------------------------------------------------------------------------------------------------------------------






// ROOK MOVES ----------------------------------------------------------------------------------------------------------------------------

// generate every legal rook move for either black or white
void generateRookMoves(const legalityInformation& info, Bitboard rookPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{

    while (rookPos)
    {
        int startSquare = popLsbAndReturnIndex(rookPos);

        Bitboard rookMoves = getRookAttacks(startSquare, occupied) & ~allyPieces;


        if (info.numOfChecks == 1)
        {
            rookMoves &= info.legalSquaresMask;
        }

        Bitboard rookCaptures = rookMoves & enemyPieces;
        Bitboard rookQuietMoves = rookMoves & ~enemyPieces;


        while (rookCaptures)
        {
            int endSquare = popLsbAndReturnIndex(rookCaptures);
            Move move = (startSquare) | (endSquare << 6) | (4 << 12);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {
                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }


        while (rookQuietMoves)
        {
            int endSquare = popLsbAndReturnIndex(rookQuietMoves);
            Move move = (startSquare) | (endSquare << 6);

            if (!(info.pinned & (1ULL << startSquare)))
            {
                moves[numOfMoves++] = move;
            } else
            {

                if (info.pinnedPieceLegalSquares[startSquare] & (1ULL << endSquare))
                {
                    moves[numOfMoves++] = move;
                }
            }
        }
    }
}

// ROOK MOVES ----------------------------------------------------------------------------------------------------------------------------





// QUEEN MOVES ----------------------------------------------------------------------------------------------------------------------------

// generate every legal queen move for either black or white using the rook and bishop moves generator functions
void generateQueenMoves(const legalityInformation& info, Bitboard queenPos, Bitboard allyPieces, Bitboard enemyPieces, Bitboard occupied, Move moves[], int& numOfMoves)
{
    generateRookMoves(info, queenPos, allyPieces, enemyPieces, occupied, moves, numOfMoves);
    generateBishopMoves(info, queenPos, allyPieces, enemyPieces, occupied, moves, numOfMoves);
}

// QUEEN MOVES ----------------------------------------------------------------------------------------------------------------------------







// KING MOVES ----------------------------------------------------------------------------------------------------------------------------

// generate legal king moves, quiet and captures
void generateKingMoves(Bitboard kingPos, Bitboard allyPieces, Bitboard enemyPieces, const Position& pos, Move moves[], int& numOfMoves)
{
    int startSquare = popLsbAndReturnIndex(kingPos);

    Bitboard startSqMask = 1ULL << startSquare;

    Bitboard kingMoves = kingAttacks[startSquare] & ~allyPieces;
    Bitboard kingCaptures = kingMoves & enemyPieces;
    Bitboard kingQuietMoves = kingMoves & ~enemyPieces;


    while (kingCaptures)
    {
        int endSquare = popLsbAndReturnIndex(kingCaptures);
        Move move = (startSquare) | (endSquare << 6) | (4 << 12);

        if (! (squareUnderAttack(endSquare, pos.isWhiteToMove() ? Black : White, pos, ((pos.getOccupiedBitboard()) & ~(startSqMask) )))) {moves[numOfMoves++] = move;}
    }



    while (kingQuietMoves)
    {
        int endSquare = popLsbAndReturnIndex(kingQuietMoves);
        Move move = (startSquare) | (endSquare << 6);
        if (! (squareUnderAttack(endSquare, pos.isWhiteToMove() ? Black : White, pos, ((pos.getOccupiedBitboard()) & ~(startSqMask) )))) {moves[numOfMoves++] = move;}
    }
}

// legal white king moves and castling
void generateWhiteKingMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard whiteKingBB = pos.getPieceBitboard(wK);
    Bitboard whitePiecesBB = pos.getWhiteBitboard();
    Bitboard blackPiecesBB = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();

    generateKingMoves(whiteKingBB, whitePiecesBB, blackPiecesBB, pos, moves, numOfMoves);

    // castling
    CastlingRights castleRights = pos.getCastlingRights();

    if (castleRights & 1) // if white kingside castling is allowed
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
                    //else
                    //{
                    //    std::cout << "g1 under attack\n";
                    //}
                }
                //else
                //{
                //    std::cout << "f1 under attack\n";
                //}
            }
            //else
            //{
            //    std::cout << "e1 under attack\n";
            //}
        }
    }

    if (castleRights & 2) // if white queenside castling is allowed
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
                    //else
                    //{
                    //    std::cout << "g1 under attack\n";
                    //}
                }
                //else
                //{
                //    std::cout << "f1 under attack\n";
                //}
            }
            //else
            //{
            //    std::cout << "e1 under attack\n";
            //}
        }
    }
}

// legal black king moves and castling
void generateBlackKingMoves(const Position& pos, Move moves[], int& numOfMoves)
{
    Bitboard blackKingBB = pos.getPieceBitboard(bK);
    Bitboard whitePiecesBB = pos.getWhiteBitboard();
    Bitboard blackPiecesBB = pos.getBlackBitboard();
    Bitboard occupied = pos.getOccupiedBitboard();

    generateKingMoves(blackKingBB, blackPiecesBB, whitePiecesBB, pos, moves, numOfMoves);

    // castling
    CastlingRights castleRights = pos.getCastlingRights();

    if (castleRights & 4) // if black kingside castling is allowed
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
                    //else
                    //{
                    //    std::cout << "g8 under attack\n";
                    //}
                }
                //else
                //{
                //    std::cout << "f8 under attack\n";
                //}
            }
            //else
            //{
            //    std::cout << "e8 under attack\n";
            //}
        }
    }

    if (castleRights & 8) // if black queenside castling is allowed
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
                    //else
                    //{
                    //    std::cout << "g1 under attack\n";
                    //}
                }
                //else
                //{
                //    std::cout << "f1 under attack\n";
                //}
            }
            //else
            //{
            //    std::cout << "e1 under attack\n";
            //}
        }
    }
}

// KING MOVES ----------------------------------------------------------------------------------------------------------------------------





// ALL LEGAL MOVES ----------------------------------------------------------------------------------------------------------------------------

// generates ONLY LEGAL moves, must receive a legalityInformation struct and a position argument
void generateLegalMoves(const legalityInformation& info, Position& pos, Move moves[], int &numOfMoves)
{
    Bitboard occupied = pos.getOccupiedBitboard();


    if (info.numOfChecks == 2)
    {
        pos.isWhiteToMove() ? generateWhiteKingMoves(pos, moves, numOfMoves) : generateBlackKingMoves(pos, moves, numOfMoves);
    }
    else
    {
        if (pos.isWhiteToMove())
        {
            //std::cout << "checking legal moves for white:" << std::endl;
            Bitboard allyPieces = pos.getWhiteBitboard();
            Bitboard enemyPieces = pos.getBlackBitboard();

            // legality : YES
            generateWhitePawnMoves(pos, info, moves, numOfMoves);

            // legality : YES
            generateKnightMoves(info, pos.getPieceBitboard(wN), allyPieces, enemyPieces, moves, numOfMoves);

            // legality : YES
            generateBishopMoves(info, pos.getPieceBitboard(wB), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateRookMoves(info, pos.getPieceBitboard(wR), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateQueenMoves(info, pos.getPieceBitboard(wQ), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateWhiteKingMoves(pos, moves, numOfMoves);
        } else
        {
            //std::cout << "checking legal moves for black:" << std::endl;
            Bitboard allyPieces = pos.getBlackBitboard();
            Bitboard enemyPieces = pos.getWhiteBitboard();

            // legality : YES
            generateBlackPawnMoves(pos, info, moves, numOfMoves);

            // legality : YES
            generateKnightMoves(info, pos.getPieceBitboard(bN), allyPieces, enemyPieces, moves, numOfMoves);

            // legality : YES
            generateBishopMoves(info, pos.getPieceBitboard(bB), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateRookMoves(info, pos.getPieceBitboard(bR), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateQueenMoves(info, pos.getPieceBitboard(bQ), allyPieces, enemyPieces, occupied, moves, numOfMoves);

            // legality : YES
            generateBlackKingMoves(pos, moves, numOfMoves);
        }
    }

}

// ALL LEGAL MOVES ----------------------------------------------------------------------------------------------------------------------------