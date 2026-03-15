////本地控制台程序
//#include <iostream>
//#include <string>
//#include "Board.h"
//#include "MoveValidator.h"
//#include "CheckEvaluator.h"
//using namespace std;
//
//int main() {
//    // 创建棋盘
//    Board b;
//
//    // 白方先行
//    Color turn = Color::White;
//
//    // 交替下棋
//    while (true) {
//        b.print();
//        if (turn == Color::White) {
//            cout << "白方：";
//        }
//        else {
//            cout << "黑方：";
//        }
//        string from, to;
//        cin >> from >> to;
//        Move m = parseMove(from, to);
//        if (!MoveValidator::isValid(b, m, turn))
//            cout << "非法棋步，请重试"<<endl;
//        else{
//            b.applyMove(m);
//            turn = (turn == Color::White) ? Color::Black : Color::White;
//        }
//        cout << "---------------------------" << endl;
//
//        if (CheckEvaluator::isCheckmate(b, turn))
//        {
//            std::cout << "checkmate!\n";
//            b.print();
//            return 0;
//        }
// 
//    }
//
//    return 0;
//}


#include <boost/asio.hpp>
#include "Server.h"
#include <vector>
#include <thread>

int main()
{
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
}