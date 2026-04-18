#include "StockfishEngine.h"

StockfishEngine::StockfishEngine() {}

StockfishEngine::~StockfishEngine() 
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        running_ = false;
    }

    cv_.notify_all();

    if (worker_.joinable())
        worker_.join();
    
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

    running_ = true;
    worker_ = std::thread(&StockfishEngine::workerLoop, this);

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

    running_ = true;
    worker_ = std::thread(&StockfishEngine::workerLoop, this);

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
    static std::string cache;
    char buf[256];

    while (true)
    {
        auto pos = cache.find('\n');
        if (pos != std::string::npos)
        {
            std::string line = cache.substr(0, pos);
            cache.erase(0, pos + 1);
            return line;
        }

#ifdef _WIN32
        DWORD read;
        if (!ReadFile(readPipe, buf, sizeof(buf), &read, NULL) || read == 0)
            return "";
#else
        int read = ::read(readPipe[0], buf, sizeof(buf));
        if (read <= 0)
            return "";
#endif

        cache.append(buf, read);
    }
}

std::string StockfishEngine::getBestMove(const std::string& fen, int depth)
{
	sendCommand("position fen " + fen);
    sendCommand("go depth " + std::to_string(depth));

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

void StockfishEngine::asyncGetBestMove(const std::string& fen, int depth, Callback cb)
{
    // 开线程 -> 压入队列并唤醒worker;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push(Task{ fen, depth, cb });
    }
    cv_.notify_one();
    
}

void StockfishEngine::workerLoop()
{
    while (true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex_);

            cv_.wait(lock, [this] {return !tasks_.empty() || !running_; });

            if (tasks_.empty() && !running_)
                return;

            task = tasks_.front();
            tasks_.pop();
        }

        std::string move = getBestMove(task.fen, task.depth);
        if (task.cb)
            task.cb(move);
    }
}
