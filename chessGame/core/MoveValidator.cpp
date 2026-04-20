#include "MoveValidator.h"
#include "Rules.h"

bool MoveValidator::isValid(const Board& board, const Move& move, Color currentTurn)
{
	int fr = move.fromRow;
	int fc = move.fromCol;
	int tr = move.toRow;
	int tc = move.toCol;

	// 1.起点、终点是否在棋盘内
	if (!Board::isInsideBoard(fr, fc) || !Board::isInsideBoard(tr, tc))
		return false;

	// 2.起点必须有棋子
	if (!board.cells[fr][fc])
		return false;

	const Piece& piece = *board.cells[fr][fc];

	// 3.必须走当前回合的棋子
	if (piece.color != currentTurn)
		return false;

	// 4.目标位置不能是己方棋子
	if (board.cells[tr][tc] && board.cells[tr][tc]->color == currentTurn)
		return false;

	// 5.根据棋子类型判断走法是否合法
	switch (piece.type) {
	case PieceType::King:
		return isValidKingMove(board, move, currentTurn);

	case PieceType::Queen:
		return isValidQueenMove(board, move);

	case PieceType::Rook:
		return isValidRookMove(board, move);

	case PieceType::Bishop:
		return isValidBishopMove(board, move);

	case PieceType::Knight:
		return isValidKnightMove(board, move);

	case PieceType::Pawn:
		return isValidPawnMove(board, move);

	default:
		return false;
	}
}