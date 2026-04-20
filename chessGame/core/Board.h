#pragma once
#include <optional>
#include "Piece.h"
#include "Move.h"
#include <string>

class Board {
public:
	std::optional <Piece> cells[8][8]; //optional可以表示棋格为空

	bool whiteKingSideCastle = true;
	bool whiteQueenSideCastle = true;
	bool blackKingSideCastle = true;
	bool blackQueenSideCastle = true;

	int enPassantRow = -1;
	int enPassantCol = -1;

	Board();

	// 检查坐标是否在棋盘内
	static bool isInsideBoard(int row, int col) {
		return row >= 0 && row < 8 && col >= 0 && col < 8;
	}

	void init();
	bool loadFromFEN(const std::string& fen, Color& turn);
	bool loadFromFEN(const std::string& fen, Color& turn, int& halfmove, int& fullmove);
	void applyMove(const Move& move);
	void print() const;
	std::string toString() const;
	std::string toFEN(Color turn, int halfmove, int fullmove) const;
};
