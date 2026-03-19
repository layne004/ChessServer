#pragma once
#include "Piece.h"
#include <string>
#include <boost/asio.hpp>
#include <json.hpp>
using json = nlohmann::json;

constexpr std::chrono::seconds DISCONNECT_TIMEOUT(30);

class Player {
public:
	Player();
	virtual ~Player() = default;

	virtual void send(const std::string& msg) = 0;

	virtual void sendJson(const json& j) = 0;

	virtual Color color() const = 0;

	virtual bool isAI() const = 0;

	bool connected()const { return connected_; }
	void setConnected(bool c) { connected_ = c; }

	void startDisconnectTimer(boost::asio::any_io_executor executor, std::function<void()> timeoutCallback);

	void cancelDisconnectTimer();

	const std::string& id()const { return id_; }

protected:
	std::string id_;

private:
	bool connected_ = true;
	std::unique_ptr<boost::asio::steady_timer> disconnectTimer_;
};