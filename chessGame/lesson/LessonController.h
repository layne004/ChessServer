#pragma once
#include <optional>
#include <string>
#include <vector>
#include "LessonStep.h"

class LessonController {
public:
	static std::optional<LessonStep> findLesson(const std::string& lessonId);
	static std::optional<std::string> findNextLessonId(const std::string& lessonId);
	static std::vector<LessonStep> listLessons();
	static std::vector<std::string> listLessonIds();
	static bool checkMove(const LessonStep& step, const std::string& from, const std::string& to);
};
