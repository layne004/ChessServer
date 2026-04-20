#include "Board.h"
#include <iostream>
using std::cout;
using std::endl;
#include <sstream>
using std::ostringstream;
#include <cctype>
#include <vector>

Board::Board() {
	Board::init();
}

void Board::init() {
	// 清空
	for (int r = 0; r < 8; r++)
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

	whiteKingSideCastle = true;
	whiteQueenSideCastle = true;
	blackKingSideCastle = true;
	blackQueenSideCastle = true;

	enPassantRow = -1;
	enPassantCol = -1;
}

bool Board::loadFromFEN(const std::string& fen, Color& turn, int& halfmove, int& fullmove)
{
	std::istringstream iss(fen);
	std::string placement;
	std::string activeColor;
	std::string castling;
	std::string enPassant;

	if (!(iss >> placement >> activeColor >> castling >> enPassant >> halfmove >> fullmove)) {
		return false;
	}

	for (int r = 0; r < 8; ++r) {
		for (int c = 0; c < 8; ++c) {
			cells[r][c].reset();
		}
	}

	int row = 0;
	int col = 0;
	for (char ch : placement)
	{
		if (ch == '/') {
			if (col != 8) {
				return false;
			}
			++row;
			col = 0;
			continue;
		}

		if (std::isdigit(static_cast<unsigned char>(ch))) {
			col += ch - '0';
			if (col > 8) {
				return false;
			}
			continue;
		}

		if (row >= 8 || col >= 8) {
			return false;
		}

		Color color = std::isupper(static_cast<unsigned char>(ch)) ? Color::White : Color::Black;
		char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

		PieceType type;
		switch (lower)
		{
		case 'k': type = PieceType::King; break;
		case 'q': type = PieceType::Queen; break;
		case 'r': type = PieceType::Rook; break;
		case 'b': type = PieceType::Bishop; break;
		case 'n': type = PieceType::Knight; break;
		case 'p': type = PieceType::Pawn; break;
		default:
			return false;
		}

		cells[row][col] = Piece{ type, color };
		++col;
	}

	if (row != 7 || col != 8) {
		return false;
	}

	if (activeColor == "w") {
		turn = Color::White;
	}
	else if (activeColor == "b") {
		turn = Color::Black;
	}
	else {
		return false;
	}

	whiteKingSideCastle = castling.find('K') != std::string::npos;
	whiteQueenSideCastle = castling.find('Q') != std::string::npos;
	blackKingSideCastle = castling.find('k') != std::string::npos;
	blackQueenSideCastle = castling.find('q') != std::string::npos;

	if (enPassant == "-") {
		enPassantRow = -1;
		enPassantCol = -1;
	}
	else {
		if (enPassant.size() != 2) {
			return false;
		}
		enPassantCol = enPassant[0] - 'a';
		enPassantRow = '8' - enPassant[1];
		if (!isInsideBoard(enPassantRow, enPassantCol)) {
			return false;
		}
	}

	return true;
}

bool Board::loadFromFEN(const std::string& fen, Color& turn)
{
	int halfmove = 0;
	int fullmove = 1;
	return loadFromFEN(fen, turn, halfmove, fullmove);
}

void Board::applyMove(const Move& move)
{
	auto piece = cells[move.fromRow][move.fromCol];

	if (!piece)
		return;

	// 保存旧 enPassant
	int oldEnPassantRow = enPassantRow;
	int oldEnPassantCol = enPassantCol;

	// 清空 enPassant
	enPassantRow = -1;
	enPassantCol = -1;

	// 处理Rook被吃
	auto captured = cells[move.toRow][move.toCol];

	if (captured && captured->type == PieceType::Rook)
	{
		if (move.toRow == 7 && move.toCol == 0)
			whiteQueenSideCastle = false;

		if (move.toRow == 7 && move.toCol == 7)
			whiteKingSideCastle = false;

		if (move.toRow == 0 && move.toCol == 0)
			blackQueenSideCastle = false;

		if (move.toRow == 0 && move.toCol == 7)
			blackKingSideCastle = false;
	}

	// 处理过路兵被吃
	if (piece->type == PieceType::Pawn)
	{
		if (move.toRow == oldEnPassantRow && move.toCol == oldEnPassantCol)
		{
			int row = (piece->color == Color::White) ? move.toRow + 1 : move.toRow - 1;
			cells[row][move.toCol].reset();
		}
	}

	// 王车易位
	if (piece->type == PieceType::King)
	{
		int diff = move.toCol - move.fromCol;

		if (abs(diff) == 2) {
			if (diff == 2)
			{
				cells[move.fromRow][5] = cells[move.fromRow][7];
				cells[move.fromRow][7].reset();
			}
			else if (diff == -2)
			{
				cells[move.fromRow][3] = cells[move.fromRow][0];
				cells[move.fromRow][0].reset();
			}
		}
	}

	// 王移动
	if (piece->type == PieceType::King)
	{
		if (piece->color == Color::White)
		{
			whiteKingSideCastle = false;
			whiteQueenSideCastle = false;
		}
		else
		{
			blackKingSideCastle = false;
			blackQueenSideCastle = false;
		}
	}

	// 车移动
	if (piece->type == PieceType::Rook)
	{
		if (move.fromRow == 7 && move.fromCol == 0)
			whiteQueenSideCastle = false;

		if (move.fromRow == 7 && move.fromCol == 7)
			whiteKingSideCastle = false;

		if (move.fromRow == 0 && move.fromCol == 0)
			blackQueenSideCastle = false;

		if (move.fromRow == 0 && move.fromCol == 7)
			blackKingSideCastle = false;
	}

	// 兵走两格
	if (piece && piece->type == PieceType::Pawn)
	{
		int diff = move.toRow - move.fromRow;

		if (abs(diff) == 2)
		{
			enPassantRow = (move.fromRow + move.toRow) / 2;
			enPassantCol = move.fromCol;
		}
	}

	cells[move.toRow][move.toCol] = piece;
	cells[move.fromRow][move.fromCol].reset();

	// 兵升变
	if (piece->type == PieceType::Pawn)
	{
		if (piece->color == Color::White && move.toRow == 0)
		{
			cells[move.toRow][move.toCol] = Piece{ move.promotion, Color::White };
		}

		if (piece->color == Color::Black && move.toRow == 7)
		{
			cells[move.toRow][move.toCol] = Piece{ move.promotion, Color::Black };
		}
	}
}

void Board::print() const {
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

	// 王车易位
	fen << " ";

	std::string castling = "";

	if (whiteKingSideCastle) castling += "K";
	if (whiteQueenSideCastle) castling += "Q";
	if (blackKingSideCastle) castling += "k";
	if (blackQueenSideCastle) castling += "q";

	if (castling.empty())
		castling = "-";

	fen << castling;

	// 吃过路兵
	fen << " ";

	if (enPassantRow == -1)
	{
		fen << "-";
	}
	else {
		char file = 'a' + enPassantCol;
		char rank = '8' - enPassantRow;

		fen << file << rank;
	}

	// 半回合计数
	fen << " " << halfmove;

	// 全回合计数
	fen << " " << fullmove;

	return fen.str();
}