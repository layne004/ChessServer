#pragma once
// 枚举类型（棋子颜色、类型）
enum class Color {
    White,
    Black
};

enum class PieceType {
    King,
    Queen,
    Rook,
    Bishop,
    Knight,
    Pawn
};

// 棋子结构
struct Piece {
    PieceType type;
    Color color;
};
