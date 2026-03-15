#pragma once
#include <string>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

class StockfishEngine
{
public:
	StockfishEngine();
	~StockfishEngine();

	bool start(const std::string& path);

	void sendCommand(const std::string& cmd);

	std::string readLine();

	std::string getBestMove(const std::string& fen);

private:
	
#ifdef _WIN32
	HANDLE processHandle = NULL;
	HANDLE writePipe = NULL;
	HANDLE readPipe = NULL;
#else
	int writePipe[2];
	int readPipe[2];
	pid_t pid = -1;
#endif

};

