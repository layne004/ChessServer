#include "AIPlayer.h"

AIPlayer::AIPlayer(Color color)
	:color_(color)
{
	engine_.start("stockfish-windows-x86-64-avx2.exe");
}

std::pair<std::string, std::string> AIPlayer::think(const std::string& fen)
{
	std::string move = engine_.getBestMove(fen);

	std::string from = move.substr(0, 2);
	std::string to   = move.substr(2, 2);

	return { from, to };
}
