#include "Rules.h"
#include "CheckEvaluator.h"
#include <iostream>

bool isValidKingMove(const Board& board, const Move& m, Color currentTurn)
{
	int dr = abs(m.toRow - m.fromRow);
	int dc = abs(m.toCol - m.fromCol);

	if (dr == 0 && dc == 2)
	{
		if (currentTurn == Color::White)
		{
			if (m.toCol == 6 && board.whiteKingSideCastle)
			{
				if (!board.cells[7][5] && !board.cells[7][6])
					return true;
			}

			if (m.toCol == 2 && board.whiteQueenSideCastle)
			{
				if (!board.cells[7][1] && !board.cells[7][2] && !board.cells[7][3])
					return true;
			}
		}
		else
		{
			if (m.toCol == 6 && board.blackKingSideCastle)
			{
				if (!board.cells[0][5] && !board.cells[0][6])
					return true;
			}

			if (m.toCol == 2 && board.blackQueenSideCastle)
			{
				if (!board.cells[0][1] && !board.cells[0][2] && !board.cells[0][3])
					return true;
			}
		}
	}

	if (dr > 1 || dc > 1)
		return false;

	// 模拟走棋
	Board temp = board;
	temp.applyMove(m);

	// 判断王是否被将
	return !CheckEvaluator::isKingInCheck(temp, currentTurn);
}

bool isValidQueenMove(const Board& board, const Move& m)
{
	return isValidRookMove(board, m) || isValidBishopMove(board, m);
}

bool isValidRookMove(const Board& board, const Move& m) {
	// 十字形，行和列不会同时改变
	if (m.fromRow != m.toRow && m.fromCol != m.toCol)
		return false;

	// 行列方向
	int dr = (m.fromRow == m.toRow) ? 0 : (m.toRow > m.fromRow ? 1 : -1);
	int dc = (m.fromCol == m.toCol) ? 0 : (m.toCol > m.fromCol ? 1 : -1);

	// 从起点开始，逐步向目标位置移动，同时检查路上格子是否已有棋子
	int r = m.fromRow + dr;
	int c = m.fromCol + dc;

	while (r != m.toRow || c != m.toCol) {
		if (board.cells[r][c])
			return false;
		r += dr;
		c += dc;
	}

	//debug
	//std::cout << "Rook: " << "(" << m.fromRow << "," << m.fromCol << ") -> (" 
	//	<< m.toRow << "," << m.toCol << ")" << std::endl;

	return true;
}

bool isValidBishopMove(const Board& board, const Move& m)
{
	int dr = m.toRow - m.fromRow;
	int dc = m.toCol - m.fromCol;

	if(abs(dr) != abs(dc))
		return false;

	dr = dr > 0 ? 1 : -1;
	dc = dc > 0 ? 1 : -1;

	int r = m.fromRow + dr;
	int c = m.fromCol + dc;

	while (r != m.toRow || c != m.toCol) {
		if (board.cells[r][c])
			return false;

		r += dr;
		c += dc;
	}
	return true;
}

bool isValidKnightMove(const Board& board, const Move& m)
{
	int dr = abs(m.toRow - m.fromRow);
	int dc = abs(m.toCol - m.fromCol);
	return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
}

bool isValidPawnMove(const Board& board, const Move& m) {
	const Piece& pawn = *board.cells[m.fromRow][m.fromCol];
	int dir = (pawn.color == Color::White) ? -1 : 1;

	int dr = m.toRow - m.fromRow;
	int dc = abs(m.toCol - m.fromCol);

	// 向前走一格
	if (dc == 0 && dr == dir && !board.cells[m.toRow][m.toCol])
		return true;

	// 起始位置向前走两格
	int startRow = (pawn.color == Color::White) ? 6 : 1;
	if (m.fromRow == startRow && dc == 0 && dr == 2 * dir
		&& !board.cells[m.fromRow + dir][m.fromCol] && !board.cells[m.toRow][m.toCol])
		return true;

	// 斜吃子
	if (dc == 1 && dr == dir
		&& board.cells[m.toRow][m.toCol] 
		&& board.cells[m.toRow][m.toCol]->color != pawn.color)
		return true;

	// 吃过路兵
	if (dc == 1 && dr == dir)
	{
		if (m.toRow == board.enPassantRow &&
			m.toCol == board.enPassantCol)
			return true;
	}

	// 升变检查
	if (pawn.color == Color::White && m.toRow == 0)
	{
		if (m.promotion == PieceType::King || m.promotion == PieceType::Pawn)
			return false;
	}

	if (pawn.color == Color::Black && m.toRow == 7)
	{
		if (m.promotion == PieceType::King || m.promotion == PieceType::Pawn)
			return false;
	}

	return false;
}