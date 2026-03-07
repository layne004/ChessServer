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

};