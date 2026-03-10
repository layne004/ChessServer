#pragma once
#include "Player.h"

class Session;

class NetworkPlayer : public Player {
public:
	NetworkPlayer(std::shared_ptr<Session> session, Color c);

	void send(const std::string& msg) override;

	void sendJson(const json& j) override;

	Color color() const override;

	bool isAI() const override;

	std::shared_ptr<Session> getSession();

	void setSession(std::shared_ptr<Session> s) { session_ = s; }

private:
	std::weak_ptr<Session> session_;
	Color color_;
};