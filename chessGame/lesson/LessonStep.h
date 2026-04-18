#pragma once
#include <string>
#include <vector>

struct LessonStep {
	std::string fen;

	std::string from;
	std::string to;

	std::vector<std::string> alternativeMoves;

	std::string hint;

	bool requireExact = true;
};