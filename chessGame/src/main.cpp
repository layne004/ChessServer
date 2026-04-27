//Perft测试成功
//运行结果：（标准答案）
//perft1: 20
//perft2: 400
//perft3 : 8902
//perft4 : 197281
//#include "Perft.h"
//#include <iostream>
//
//int main()
//{
//	Board board;
//
//	std::cout << "perft1: "
//		<< Perft::run(board, Color::White, 1)
//		<< std::endl;
//
//	std::cout << "perft2: "
//		<< Perft::run(board, Color::White, 2)
//		<< std::endl;
//
//	std::cout << "perft3: "
//		<< Perft::run(board, Color::White, 3)
//		<< std::endl;
//
//	std::cout << "perft4: "
//		<< Perft::run(board, Color::White, 4)
//		<< std::endl;
//
//	return 0;
//}

#include <boost/asio.hpp>
#include "Server.h"
#include <vector>
#include <thread>
#include "Perft.h"
#include <iostream>
#include "StockfishEngine.h"

int main()
{
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
#endif
	boost::asio::io_context io;

	// 防止 io.run() 提前退出
	auto work_guard = boost::asio::make_work_guard(io);

	Server server(io, 12345);

	// 启动线程池
	std::vector<std::thread> threads;

	unsigned int threadCount = std::thread::hardware_concurrency();

	for (unsigned int i = 0; i < threadCount; ++i)
	{
		threads.emplace_back([&io]() {
			io.run();
			});
	}

	for (auto& t : threads)
	{
		t.join();
	}

	return 0;
}