#pragma once
// 函数声明中vector需要知道Move类型的完整定义，所以此处使用前向声明会报错
// 直接引入对应头文件就行
#include "Board.h"
#include "Move.h"
#include <vector>


class MoveGenerator {
public:
	// 返回某个棋子的合法走法
	static std::vector<Move> generateForSquare(const Board& board, int row, int col, Color turn);

	// 返回当前玩家所有合法走法
	static std::vector<Move> generateAll(const Board& board, Color turn);
};