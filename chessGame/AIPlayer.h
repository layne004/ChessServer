#pragma once
#include "Player.h"

class Move;
class Board;

class AIPlayer : public Player {
public:
	AIPlayer(Color c);

	void send(const std::string& msg) override {

	}

	void sendJson(const json&)override {
		// AI 不需要发网络消息
	}

	Color color() const override { return color_; }

	bool isAI()const override { return true; }

	Move generateMove(const Board& board);
private:
	Color color_;
};