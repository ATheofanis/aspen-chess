//
// Created by theoa on 13/05/2026.
//

#pragma once

#include "Position.h"

void generatePseudoLegalCapAndPromoMoves(const Position& pos, Move moves[], int &numOfMoves);
void generatePseudoLegalQuietMoves(const Position& pos, Move moves[], int &numOfMoves);