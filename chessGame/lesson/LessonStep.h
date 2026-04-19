#pragma once
#include "Piece.h"
#include <string>
#include <vector>

struct LessonStep {
	std::string lessonId;
	std::string pieceName;
	std::string title;
	std::string description;
	std::string fen;
	Color turn = Color::White;
	std::string trackedPieceSquare;
	std::vector<std::string> targetSquares;

	std::string successMessage;
	std::string hint;
};
