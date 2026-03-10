#pragma once
#include "Piece.h"
#include <string>
#include <json.hpp>
using json = nlohmann::json;

class Player {
public:
	virtual ~Player() = default;

	virtual void send(const std::string& msg) = 0;

	virtual void sendJson(const json& j) = 0;

	virtual Color color() const = 0;

	virtual bool isAI() const = 0;

	bool connected()const { return connected_; }
	void setConnected(bool c) { connected_ = c; }

private:
	bool connected_ = true;
	uint64_t roomId_;
};