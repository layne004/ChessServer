#pragma once
#include "Piece.h"
#include <json.hpp>
using json = nlohmann::json;

class Session;

class IPlayer {
public:
	virtual ~IPlayer() = default;

	virtual void sendJson(const json& j) = 0;

	virtual Color color() const = 0;

	virtual bool isAI() const = 0;

};

class NetworkPlayer : public IPlayer {
public:
	NetworkPlayer(std::shared_ptr<Session> session, Color c);

	void sendJson(const json& j) override;

	Color color() const override;

	bool isAI() const override;

private:
	std::shared_ptr<Session> session_;
	Color color_;
};