#pragma once
#include "Player.h"
#include "StockfishEngine.h"

class AIPlayer : public Player {
public:
	AIPlayer(Color color);

	void send(const std::string& msg) override {}

	void sendJson(const json&)override {
		// AI 不需要发网络消息
	}

	Color color() const override { return color_; }

	bool isAI()const override { return true; }

	std::pair<std::string, std::string> think(const std::string& fen);

private:
	Color color_;
	StockfishEngine engine_;
};