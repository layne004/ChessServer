#include "MoveGenerator.h"
#include "MoveValidator.h"
#include "CheckEvaluator.h"

std::vector<Move> MoveGenerator::generateForSquare(const Board& board, int row, int col, Color turn)
{
	std::vector<Move> result;

	// 若该位置无棋子则返回空result
	if (!board.cells[row][col])
		return result;

	for (int tr = 0; tr < 8; ++tr)
	{
		for (int tc = 0; tc < 8; ++tc)
		{
			Move m{ row, col, tr, tc };

			if (!MoveValidator::isValid(board, m, turn))
				continue;

			Board tempBoard = board;
			tempBoard.applyMove(m);

			// 走法不能使己方王被将，即不能送棋
			if (!CheckEvaluator::isKingInCheck(tempBoard, turn))
				result.push_back(m);
		}
	}

	return result;
}

std::vector<Move> MoveGenerator::generateAll(const Board& board, Color turn)
{
	std::vector<Move> result;

	for (int r = 0; r < 8; ++r)
	{
		for (int c = 0; c < 8; ++c)
		{
			if (board.cells[r][c] && board.cells[r][c]->color == turn)
			{
				auto moves = generateForSquare(board, r, c, turn);
				result.insert(result.end(), moves.begin(), moves.end());
			}
		}
	}

	return result;
}