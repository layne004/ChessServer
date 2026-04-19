#include "LessonController.h"
#include <algorithm>
#include <array>

namespace
{
	const std::array<LessonStep, 6> kLessons = {
		LessonStep{
			"rook_1",
			"rook",
			"Rook",
			"The rook moves in straight lines. Collect every star with the rook.",
			"8/2R5/8/8/8/8/8/8 w - - 0 1",
			Color::White,
			"c7",
			{"c5", "g5"},
			"Good. You collected all rook targets.",
			"The rook does not move diagonally."
		},
		LessonStep{
			"bishop_1",
			"bishop",
			"Bishop",
			"The bishop moves on diagonals. Collect every star with the bishop.",
			"8/8/8/8/3B4/8/8/8 w - - 0 1",
			Color::White,
			"d4",
			{"g7", "a1"},
			"Good. You collected all bishop targets.",
			"Follow the diagonal from the bishop's square."
		},
		LessonStep{
			"queen_1",
			"queen",
			"Queen",
			"The queen combines rook and bishop movement. Collect every star.",
			"8/8/8/3Q4/8/8/8/8 w - - 0 1",
			Color::White,
			"d5",
			{"h5", "a2"},
			"Good. You collected all queen targets.",
			"Think of the queen as rook plus bishop."
		},
		LessonStep{
			"king_1",
			"king",
			"King",
			"The king moves one square in any direction. Collect every nearby star.",
			"8/8/8/8/4K3/8/8/8 w - - 0 1",
			Color::White,
			"e4",
			{"e5", "f5"},
			"Good. You collected all king targets.",
			"The king can step to any adjacent square, but only one."
		},
		LessonStep{
			"knight_1",
			"knight",
			"Knight",
			"The knight moves in an L shape. Collect every star with knight jumps.",
			"8/8/8/8/4N3/8/8/8 w - - 0 1",
			Color::White,
			"e4",
			{"f6", "g5"},
			"Good. You collected all knight targets.",
			"Two squares one way, then one square sideways."
		},
		LessonStep{
			"pawn_1",
			"pawn",
			"Pawn",
			"A white pawn moves forward. Collect every star in front of the pawn.",
			"8/8/8/8/8/8/4P3/8 w - - 0 1",
			Color::White,
			"e2",
			{"e3", "e4"},
			"Good. You collected all pawn targets.",
			"White pawns move toward rank 8."
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
