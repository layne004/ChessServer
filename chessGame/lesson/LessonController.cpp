#include "LessonController.h"
#include <algorithm>
#include <array>

namespace
{
	const std::array<LessonStep, 6> kLessons = {
		LessonStep{
			"rook_1",
			"rook",
			"车",
			"车沿横线和竖线移动。移动这枚车，吃掉所有星星。",
			"8/2R5/8/8/8/8/8/8 w - - 0 1",
			Color::White,
			"c7",
			{"c5", "g5"},
			"做得很好，你已经掌握了车的直线移动。",
			"车不能斜着走，只能沿横线或竖线移动。"
		},
		LessonStep{
			"bishop_1",
			"bishop",
			"象",
			"象沿对角线移动。移动这枚象，吃掉所有星星。",
			"8/8/8/8/3B4/8/8/8 w - - 0 1",
			Color::White,
			"d4",
			{"g7", "a1"},
			"做得很好，你已经掌握了象的对角线移动。",
			"观察象所在格子的两条对角线，象只能沿对角线前进。"
		},
		LessonStep{
			"queen_1",
			"queen",
			"后",
			"后同时拥有车和象的走法。移动这枚后，吃掉所有星星。",
			"8/8/8/3Q4/8/8/8/8 w - - 0 1",
			Color::White,
			"d5",
			{"h5", "a2"},
			"做得很好，你已经掌握了后的综合走法。",
			"可以把后理解成“车 + 象”，既能直走，也能斜走。"
		},
		LessonStep{
			"king_1",
			"king",
			"王",
			"王每次只能向任意方向走一格。移动这枚王，吃掉所有星星。",
			"8/8/8/8/4K3/8/8/8 w - - 0 1",
			Color::White,
			"e4",
			{"e5", "f5"},
			"做得很好，你已经掌握了王的一格移动。",
			"王可以走向周围相邻的格子，但一次只能走一格。"
		},
		LessonStep{
			"knight_1",
			"knight",
			"马",
			"马走“日”字，并且可以跳过中间的棋子。移动这枚马，吃掉所有星星。",
			"8/8/8/8/4N3/8/8/8 w - - 0 1",
			Color::White,
			"e4",
			{"f6", "g5"},
			"做得很好，你已经掌握了马的跳跃走法。",
			"先朝一个方向走两格，再拐一格，就是马的走法。"
		},
		LessonStep{
			"pawn_1",
			"pawn",
			"兵",
			"白兵通常向前移动。移动这枚兵，吃掉前方的所有星星。",
			"8/8/8/8/8/8/4P3/8 w - - 0 1",
			Color::White,
			"e2",
			{"e3", "e4"},
			"做得很好，你已经掌握了兵的前进方式。",
			"白兵朝棋盘上方前进；在起始位置时，可以选择前进一格或两格。"
		}
	};
}

std::optional<LessonStep> LessonController::findLesson(const std::string& lessonId)
{
	const auto it = std::find_if(kLessons.begin(), kLessons.end(),
		[&lessonId](const LessonStep& step) { return step.lessonId == lessonId; });

	if (it == kLessons.end()) {
		return std::nullopt;
	}

	return *it;
}

std::optional<std::string> LessonController::findNextLessonId(const std::string& lessonId)
{
	for (std::size_t i = 0; i < kLessons.size(); ++i) {
		if (kLessons[i].lessonId != lessonId) {
			continue;
		}

		if (i + 1 >= kLessons.size()) {
			return std::nullopt;
		}

		return kLessons[i + 1].lessonId;
	}

	return std::nullopt;
}

std::vector<LessonStep> LessonController::listLessons()
{
	return std::vector<LessonStep>(kLessons.begin(), kLessons.end());
}

std::vector<std::string> LessonController::listLessonIds()
{
	std::vector<std::string> ids;
	ids.reserve(kLessons.size());
	for (const auto& step : kLessons) {
		ids.push_back(step.lessonId);
	}
	return ids;
}

bool LessonController::checkMove(const LessonStep& step, const std::string& from, const std::string& to)
{
	if (from != step.trackedPieceSquare) {
		return false;
	}

	return std::find(step.targetSquares.begin(), step.targetSquares.end(), to) != step.targetSquares.end();
}
