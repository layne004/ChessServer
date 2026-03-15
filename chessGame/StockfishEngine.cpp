#include "StockfishEngine.h"
#include <iostream>
#include <Windows.h>

StockfishEngine::StockfishEngine() {}

StockfishEngine::~StockfishEngine() 
{
#ifdef _WIN32
	if (processHandle)
		CloseHandle(processHandle);
#else
	if (pid > 0)
		kill(pid, SIGTERM);
#endif

}

#ifdef _WIN32

bool StockfishEngine::start(const std::string& path)
{
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE inRead, inWrite;
    HANDLE outRead, outWrite;

    CreatePipe(&inRead, &inWrite, &sa, 0);
    CreatePipe(&outRead, &outWrite, &sa, 0);

    STARTUPINFOA si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = inRead;
    si.hStdOutput = outWrite;
    si.hStdError = outWrite;

    PROCESS_INFORMATION pi;

    if (!CreateProcessA(
        NULL,
        (LPSTR)path.c_str(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
    {
        return false;
    }

    processHandle = pi.hProcess;

    writePipe = inWrite;
    readPipe = outRead;

    CloseHandle(inRead);
    CloseHandle(outWrite);

    sendCommand("uci");

    return true;
}

#endif

#ifndef _WIN32

bool StockfishEngine::start(const std::string& path)
{
    pipe(writePipe);
    pipe(readPipe);

    pid = fork();

    if (pid == 0)
    {
        dup2(writePipe[0], STDIN_FILENO);
        dup2(readPipe[1], STDOUT_FILENO);

        execl(path.c_str(), path.c_str(), NULL);

        exit(1);
    }

    close(writePipe[0]);
    close(readPipe[1]);

    sendCommand("uci");

    return true;
}

#endif

void StockfishEngine::sendCommand(const std::string& cmd)
{
#ifdef _WIN32

    DWORD written;
    std::string s = cmd + "\n";

    WriteFile(writePipe, s.c_str(), s.size(), &written, NULL);

#else

    std::string s = cmd + "\n";

    write(writePipe[1], s.c_str(), s.size());

#endif
}

std::string StockfishEngine::readLine()
{
	char buffer[256];

#ifdef _WIN32

    DWORD read;
    if (!ReadFile(readPipe, buffer, sizeof(buffer) - 1, &read, NULL))
        return "";

    buffer[read] = 0;
    return std::string(buffer);

#else

    int n = read(readPipe[0], buffer, sizeof(buffer) - 1);

    if (n <= 0)
        return "";

    buffer[n] = 0;
    return std::string(buffer);

#endif
}

std::string StockfishEngine::getBestMove(const std::string& fen)
{
	sendCommand("position fen " + fen);
	sendCommand("go movetime 500");

	while (true)
	{
		auto line = readLine();

		if (line.find("bestmove") != std::string::npos)
		{
            std::string move = line.substr(9);

            auto pos = move.find(' ');

            if (pos != std::string::npos)
                move = move.substr(0, pos);

            return move;
		}
	}
}
