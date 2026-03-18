#pragma once
#include <string>
// 坐标系统
// 我们统一约定
// row = 0 -> 黑方底线（8）
// row = 7 -> 白方底线（1）
// col = 0 -> a
// col = 7 -> h
// 比如 (0,0)就代表(a8)，(7,7)代表(h1)

struct Move {
    int fromRow, fromCol;
    int toRow, toCol;

	PieceType promotion = PieceType::Queen;
};

// 将棋步转换成坐标
inline Move parseMove(const std::string& from, const std::string& to, char promotion = 'q')
{
	Move m;
	m.fromRow = '8' - from[1];
	m.fromCol = from[0] - 'a';

	m.toRow = '8' - to[1];
	m.toCol = to[0] - 'a';

	switch (promotion)
	{
	case 'q': m.promotion = PieceType::Queen; break;
	case 'r': m.promotion = PieceType::Rook; break;
	case 'b': m.promotion = PieceType::Bishop; break;
	case 'n': m.promotion = PieceType::Knight; break;
	}

	return m;
}