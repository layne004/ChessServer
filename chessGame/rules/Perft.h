#pragma once

#include "Board.h"
#include "Move.h"
#include <vector>

class Perft
{
public:

	static uint64_t run(Board board, Color turn, int depth);

private:

	static void generateMoves(
		const Board& board,
		Color turn,
		std::vector<Move>& moves);
};