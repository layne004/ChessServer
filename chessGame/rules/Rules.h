#pragma once
#include "Board.h"
bool isValidKingMove(const Board& board, const Move& m, Color currentTurn);
bool isValidQueenMove(const Board& board, const Move& m);
bool isValidRookMove(const Board& board, const Move& m);
bool isValidBishopMove(const Board& board, const Move& m);
bool isValidKnightMove(const Board& board, const Move& m);
bool isValidPawnMove(const Board& board, const Move& m);
