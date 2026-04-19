#include "CheckEvaluator.h"
#include "Rules.h"
#include "MoveValidator.h"
#include "MoveGenerator.h"

bool CheckEvaluator::isKingInCheck(const Board& board, Color kingColor)
{
	// 1.获取王的位置
	int kingRow = -1, kingCol = -1;

	if (!findKing(board, kingColor, kingRow, kingCol))
		return false; //理论上不会发生

	// 获取敌方颜色
	Color enemyColor = (kingColor == Color::White) ? Color::Black : Color::White;

	// 遍历棋局判断 敌方棋子是否能合法地走到王的位置
	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			// 若该棋格无棋子 则跳过
			if (!board.cells[r][c])
				continue;

			const Piece& p = *board.cells[r][c];
			// 若该棋格棋子与王同色 则跳过
			if (p.color == kingColor)
				continue;

			// 模拟敌方棋子将王
			Move attackMove = Move{ r, c, kingRow, kingCol };

			switch (p.type) {
				// 此处不调用isValidKingMove的原因是防止死循环
				// 因为在 isValidKingMove 中调用了本函数 isKingInCheck
				case PieceType::King:
					if (abs(attackMove.fromRow - attackMove.toRow) <= 1 &&
						abs(attackMove.fromCol - attackMove.toCol) <= 1)
						return true;
					break;

				case PieceType::Queen:
					if (isValidQueenMove(board, attackMove))
						return true;
					break;

				case PieceType::Rook:
					if (isValidRookMove(board, attackMove))
						return true;
					break;

				case PieceType::Bishop:
					if (isValidBishopMove(board, attackMove))
						return true;
					break;

				case PieceType::Knight:
					if (isValidKnightMove(board, attackMove))
						return true;
					break;

				// 兵的攻击判断需要单独写，因为兵直着走，斜着吃子
				case PieceType::Pawn:
					if (isPawnAttacking(board, attackMove, enemyColor))
						return true;
					break;

			}
		}
	}

	return false;
}

bool CheckEvaluator::isCheckmate(const Board& board, Color color)
{
	if (!isKingInCheck(board, color))
		return false;

	// 尝试所有可能走法
	return !hasAnyLegalEscape(board, color);
}

bool CheckEvaluator::isStalemate(const Board& board, Color color)
{
	if (isKingInCheck(board, color))
		return false;

	return !hasAnyLegalEscape(board, color);
}

GameResult CheckEvaluator::evaluate(const Board& board, Color currentTurn)
{
	if (isCheckmate(board, currentTurn))
		return GameResult::Checkmate;

	if (isStalemate(board, currentTurn))
		return GameResult::Stalemate;

	return GameResult::Ongoing;
}

// 辅助函数
bool CheckEvaluator::findKing(const Board& board, Color color, int& kingRow, int& kingCol)
{
	for (int r = 0; r < 8; r++)
		for (int c = 0; c < 8; c++) {
			if (board.cells[r][c]
				&& board.cells[r][c]->type == PieceType::King
				&& board.cells[r][c]->color == color)
			{
				kingRow = r;
				kingCol = c;
				return true;
			}
		}

	return false;
}

bool CheckEvaluator::isPawnAttacking(const Board& board, const Move& m, Color pawnColor)
{
	int dir = (pawnColor == Color::White) ? -1 : 1;

	return (m.toRow == m.fromRow + dir &&
		abs(m.toCol - m.fromCol) == 1);
}

bool CheckEvaluator::hasAnyLegalEscape(const Board& board, Color color)
{
	return !MoveGenerator::generateAll(board, color).empty();
}

bool CheckEvaluator::isSquareAttacked(const Board& board, int row, int col, Color enemy)
{
	//遍历棋盘
	//找到对方棋子
	//判断它是否可以攻击目标格

	for (int r = 0; r < 8; r++)
	{
		for (int c = 0; c < 8; c++)
		{
			auto p = board.cells[r][c];

			if (!p || p->color != enemy)
				continue;

			Move m;
			m.fromRow = r;
			m.fromCol = c;
			m.toRow = row;
			m.toCol = col;

			switch (p->type)
			{
			case PieceType::Pawn:
			{
				int dir = (enemy == Color::White) ? -1 : 1;

				if (row == r + dir && abs(col - c) == 1)
					return true;
				break;
			}

			case PieceType::Knight:
				if (isValidKnightMove(board, m))
					return true;
				break;

			case PieceType::Bishop:
				if (isValidBishopMove(board, m))
					return true;
				break;

			case PieceType::Rook:
				if (isValidRookMove(board, m))
					return true;
				break;

			case PieceType::Queen:
				if (isValidQueenMove(board, m))
					return true;
				break;

			case PieceType::King:
				if (abs(row - r) <= 1 && abs(col - c) <= 1)
					return true;
				break;
			}
		}
	}

	return false;
}
