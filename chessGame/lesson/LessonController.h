#pragma once
#include <string>
#include "LessonStep.h"
class LessonController {
public:
	bool checkMove(const LessonStep& step, const std::string& from, const std::string& to);

	void nextStep();
};