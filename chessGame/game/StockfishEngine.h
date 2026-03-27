#pragma once
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

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
	using Callback = std::function<void(const std::string&)>;

	StockfishEngine();
	~StockfishEngine();

	bool start(const std::string& path);

	void sendCommand(const std::string& cmd);

	std::string readLine();

	// 旧同步接口
	std::string getBestMove(const std::string& fen);

	// 新异步接口
	void asyncGetBestMove(const std::string& fen, Callback cb);

private:
	void workerLoop();

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

	struct Task
	{
		std::string fen;
		Callback cb;
	};

	std::thread worker_;
	std::mutex mutex_;
	bool running_ = false;
	std::condition_variable cv_;
	std::queue<Task> tasks_;
};

