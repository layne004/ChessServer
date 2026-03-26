#include "Perft.h"
#include "CheckEvaluator.h"
#include "MoveValidator.h"

uint64_t Perft::run(Board board, Color turn, int depth)
{
	if (depth == 0)
		return 1;

	std::vector<Move> moves;

	generateMoves(board, turn, moves);

	uint64_t nodes = 0;

	for (const auto& m : moves)
	{
		Board next = board;

		next.applyMove(m);

		if (CheckEvaluator::isKingInCheck(next, turn))
			continue;

		Color nextTurn = (turn == Color::White) ? Color::Black : Color::White;

		nodes += run(next, nextTurn, depth - 1);
	}

	return nodes;
}

void Perft::generateMoves(const Board& board, Color turn, std::vector<Move>& moves)
{
	for (int r = 0; r < 8; r++)
	{
		for (int c = 0; c < 8; c++)
		{
			auto p = board.cells[r][c];

			if (!p || p->color != turn)
				continue;

			for (int tr = 0; tr < 8; tr++)
			{
				for (int tc = 0; tc < 8; tc++)
				{
					Move m{ r,c,tr,tc };

					if (!MoveValidator::isValid(board, m, turn))
						continue;

					moves.push_back(m);
				}
			}
		}
	}
}