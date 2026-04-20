#pragma once
#include "Piece.h"
#include "Board.h"
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <json.hpp>
using json = nlohmann::json;
#include <chrono>
#include <boost/asio/steady_timer.hpp>
#include "LessonStep.h"
#include <optional>
class Session;

class Player;

enum class GameState {
	Waiting,
	Playing,
	GameOver
};

struct ClockState
{
	int remaining_ms = 0;
	std::chrono::steady_clock::time_point lastStart;
};

class GameRoom :public std::enable_shared_from_this<GameRoom>
{
public:
	enum class Mode {
		PvP, //玩家对战
		PvE,  //玩家与AI对战
		Lesson //教学模式
	};

	using RoomID = uint64_t;
	GameRoom(boost::asio::io_context& io, RoomID roomId, Mode mode,
		int initialTimeMs = 300000,
		int incrementMs = 3000);
	~GameRoom() = default;

	// 开始对战
	void start(std::shared_ptr<Player> white, std::shared_ptr<Player> black);
	bool startLesson(std::shared_ptr<Player> player, const LessonStep& step);

	// 终止游戏
	void endGame(const std::string& result, const std::string& reason);

	// 处理玩家走棋
	void handleMove(std::shared_ptr<Player> player, const std::string& from, const std::string& to, char promotion = 'q');

	// 处理AI玩家走棋
	void maybeAIMove();

	// 处理教学走棋
	void handleLessonMove(std::shared_ptr<Player> player,
		const std::string& from, const std::string& to, char promotion = 'q');

	// 处理玩家主动认输
	void handleResign(const std::shared_ptr<Player> player);

	// 处理玩家Session断线
	void onPlayerDisconnected(const std::shared_ptr<Session>& session, const std::string& playerId);

	// 断线重连
	void reconnect(const std::shared_ptr<Session>& session, const std::string& playerId);

	// 通过playerId来找player
	std::shared_ptr<Player> findPlayerById(const std::string& id);

	// 获取当前轮
	Color currentTurn()const { return turn_; }

	// 获取房间状态
	bool isFull() const;
	bool isEmpty() const;
	// 获取房间号
	RoomID id()const { return roomId_; }

	// 重置游戏
	void resetGame();

private:
	void broadcast(const std::string& msg);
	void broadcastJson(const json& j);
	void broadcastMove(const std::string& from, const std::string& to, std::optional<char> promotion = std::nullopt);
	void broadcastState();
	void broadcastLessonState();
	//void broadcastGameOver(const std::string& result, const std::string& reason);

	// 在走棋后更新时钟
	void updateClockBeforeMove();

	void startClockTimer();
	void checkTimeout(const boost::system::error_code& ec);

	// 实现断线暂停时钟
	void pauseClock();
	void resumeClock();

private:
	boost::asio::strand<boost::asio::any_io_executor> strand_;
	boost::asio::steady_timer clockTimer_;
	Board board_;
	Color turn_ = Color::White; //白方先行
	RoomID roomId_;

	std::shared_ptr<Player> white_;
	std::shared_ptr<Player> black_;
	std::vector<std::shared_ptr<Player>> spectators_;

	GameState state_ = GameState::Waiting;

	int halfmoveClock_ = 0;
	int fullmoveNumber_ = 1;

	Mode mode_; //记录当前模式

	int initialTimeMs_ = 300000; //默认 5 分钟
	int incrementMs_ = 3000;	 //默认 +3 秒

	ClockState whiteClock_;
	ClockState blackClock_;

	bool clockPaused_ = false;

	// 教学模式
	int lessonStep_ = 0;
	std::vector<LessonStep> lessonSteps_;
	std::string lessonId_;
	std::string lessonTrackedPieceSquare_;
	std::vector<std::string> remainingLessonTargets_;
	int lessonMoveCount_ = 0;
};
