#pragma once
#include "Board.h"
class MoveValidator
{
public:
	static bool isValid(const Board& board, const Move& move, Color currentTurn);
};
