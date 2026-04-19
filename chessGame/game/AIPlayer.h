#pragma once
#include "Player.h"
#include "StockfishEngine.h"

struct AIMove
{
	std::string from;
	std::string to;
	char promotion = 'q';
};

enum class AIDifficulty
{
	Easy,
	Medium,
	Hard
};

class AIPlayer : public Player {
public:
	AIPlayer(Color color, const std::string& level);

	void send(const std::string& msg) override {}

	void sendJson(const json&)override {
		// AI 不需要发网络消息
	}

	Color color() const override { return color_; }

	bool isAI()const override { return true; }

	// 同步
	AIMove think(const std::string& fen);
	// 异步
	void asyncThink(const std::string& fen, std::function<void(AIMove)> cb);

private:
	Color color_;
	int depth_;
	StockfishEngine engine_;
};