#include "AIPlayer.h"

static int levelToDepth(const std::string& level)
{
	if (level == "easy") return 4;
	if (level == "hard") return 12;
	return 8;
}

AIPlayer::AIPlayer(Color color, const std::string& level)
	:color_(color), depth_(levelToDepth(level))
{
#ifdef _WIN32
	engine_.start("./engine/stockfish-windows-x86-64-avx2.exe");
#else
	engine_.start("/usr/games/stockfish");
#endif
	
}

AIMove AIPlayer::think(const std::string& fen)
{
	std::string move = engine_.getBestMove(fen, depth_);

	AIMove result;

	result.from = move.substr(0, 2);
	result.to   = move.substr(2, 2);

	if (move.size() == 5)
	{
		result.promotion = move[4];
	}
	else
	{
		result.promotion = 'q';
	}

	return result;
}

void AIPlayer::asyncThink(const std::string& fen, std::function<void(AIMove)> cb)
{
	engine_.asyncGetBestMove(fen, depth_, 
		[cb](const std::string& moveStr) {
			AIMove result;

			result.from = moveStr.substr(0, 2);
			result.to = moveStr.substr(2, 2);

			result.promotion = moveStr.size() == 5 ? moveStr[4] : 'q';

			cb(result);
	});
}
