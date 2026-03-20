#include "AIPlayer.h"

AIPlayer::AIPlayer(Color color)
	:color_(color)
{
#ifdef _WIN32
	engine_.start("./engine/stockfish-windows-x86-64-avx2.exe");
#else
	engine_.start("/usr/games/stockfish");
#endif
	
}

AIMove AIPlayer::think(const std::string& fen)
{
	std::string move = engine_.getBestMove(fen);

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
