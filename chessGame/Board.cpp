#include "Board.h"
#include <iostream>
using std::cout;
using std::endl;
#include <sstream>
using std::ostringstream;

Board::Board() {
	Board::init();
}

void Board::init() {
	// 清空
	for(int r = 0; r < 8; r++)
		for (int c = 0; c < 8; c++) {
			cells[r][c].reset();
		}

	// 黑方摆棋
	cells[0][0] = Piece{ PieceType::Rook, Color::Black };
	cells[0][1] = Piece{ PieceType::Knight, Color::Black };
	cells[0][2] = Piece{ PieceType::Bishop, Color::Black };
	cells[0][3] = Piece{ PieceType::Queen, Color::Black };
	cells[0][4] = Piece{ PieceType::King, Color::Black };
	cells[0][5] = Piece{ PieceType::Bishop, Color::Black };
	cells[0][6] = Piece{ PieceType::Knight, Color::Black };
	cells[0][7] = Piece{ PieceType::Rook, Color::Black };

	for (int i = 0; i < 8; i++) {
		cells[1][i] = Piece{ PieceType::Pawn, Color::Black };
	}

	// 白方摆棋
	cells[7][0] = Piece{ PieceType::Rook, Color::White };
	cells[7][1] = Piece{ PieceType::Knight, Color::White };
	cells[7][2] = Piece{ PieceType::Bishop, Color::White };
	cells[7][3] = Piece{ PieceType::Queen, Color::White };
	cells[7][4] = Piece{ PieceType::King, Color::White };
	cells[7][5] = Piece{ PieceType::Bishop, Color::White };
	cells[7][6] = Piece{ PieceType::Knight, Color::White };
	cells[7][7] = Piece{ PieceType::Rook, Color::White };

	for (int i = 0; i < 8; i++) {
		cells[6][i] = Piece{ PieceType::Pawn, Color::White };
	}
}

void Board::applyMove(const Move& move)
{
	cells[move.toRow][move.toCol] = cells[move.fromRow][move.fromCol];
	cells[move.fromRow][move.fromCol].reset();
}

void Board::print() const{

	for (int r = 0; r < 8; r++) {
		cout << 8 - r << " ";
		for (int c = 0; c < 8; c++) {
			if (!cells[r][c]) {
				cout << ". ";
			}
			else {
				char ch;
				switch (cells[r][c]->type) {
				case PieceType::King: ch = 'K'; break;
				case PieceType::Queen: ch = 'Q'; break;
				case PieceType::Rook: ch = 'R'; break;
				case PieceType::Bishop: ch = 'B'; break;
				case PieceType::Knight: ch = 'N'; break;
				case PieceType::Pawn: ch = 'P'; break;
				}
				if (cells[r][c]->color == Color::Black)
					ch = tolower(ch);
				cout << ch << " ";
			}
		}
		cout << endl;
	}
	cout << "  a b c d e f g h\n";
}

std::string Board::toString() const
{
	ostringstream oss;

	for (int r = 0; r < 8; r++) {
		oss << 8 - r << " ";
		for (int c = 0; c < 8; c++) {
			if (!cells[r][c]) {
				oss << ". ";
			}
			else {
				char ch;
				switch (cells[r][c]->type) {
				case PieceType::King: ch = 'K'; break;
				case PieceType::Queen: ch = 'Q'; break;
				case PieceType::Rook: ch = 'R'; break;
				case PieceType::Bishop: ch = 'B'; break;
				case PieceType::Knight: ch = 'N'; break;
				case PieceType::Pawn: ch = 'P'; break;
				}
				if (cells[r][c]->color == Color::Black)
					ch = tolower(ch);
				oss << ch << " ";
			}
		}
		oss << endl;
	}
	oss << "  a b c d e f g h\n";

	return oss.str();
}

std::string Board::toFEN(Color turn, int halfmove, int fullmove)const 
{
	std::ostringstream fen;

	// 棋盘部分
	for (int r = 0; r < 8; r++)
	{
		int emptyCount = 0;

		for (int c = 0; c < 8; c++)
		{
			if (!cells[r][c]) {
				emptyCount++;
			}
			else {
				if (emptyCount > 0) {
					fen << emptyCount;
					emptyCount = 0;
				}

				char ch;
				switch (cells[r][c]->type) {
					case PieceType::King: ch = 'K'; break;
					case PieceType::Queen: ch = 'Q'; break;
					case PieceType::Rook: ch = 'R'; break;
					case PieceType::Bishop: ch = 'B'; break;
					case PieceType::Knight: ch = 'N'; break;
					case PieceType::Pawn: ch = 'P'; break;
				}

				if (cells[r][c]->color == Color::Black)
					ch = tolower(ch);

				fen << ch;
			}
		}

		if (emptyCount > 0)
			fen << emptyCount;

		if (r != 7)
			fen << "/";
	}

	// 当前走棋方
	fen << " ";
	fen << (turn == Color::White ? "w" : "b");

	// 王车易位 (暂时不实现)
	fen << " -";

	// 吃过路兵 (暂时不实现)
	fen << " -";

	// 半回合计数
	fen << " "<<halfmove;

	// 全回合计数
	fen << " "<<fullmove;

	return fen.str();
}