#pragma once
#include "Piece.h"
#include "Board.h"

enum class GameResult {
	Ongoing,
	Checkmate,
	Stalemate
};

// ÆåÅÌ¾ÖÃæ×´Ì¬¼ì²âÆ÷
class CheckEvaluator
{
public:
	static bool isKingInCheck(const Board& board, Color kingColor);
	static bool isCheckmate(const Board& board, Color color);
	static bool isStalemate(const Board& board, Color color);
	static GameResult evaluate(const Board& board, Color color);
	static bool isSquareAttacked(const Board& board, int row, int col, Color enemy);

private:
	static bool findKing(const Board& board, Color color, int& kingRow, int& kingCol);
	static bool isPawnAttacking(const Board& board, const Move& m, Color pawnColor);
	static bool hasAnyLegalEscape(const Board& board, Color color);
	static bool canPieceEscape(const Board& board, int fr, int fc, Color color);

};

