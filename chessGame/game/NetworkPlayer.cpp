#include "NetworkPlayer.h"
#include "Session.h"

NetworkPlayer::NetworkPlayer(std::shared_ptr<Session> session, Color c)
	:session_(session), color_(c)
{
}

void NetworkPlayer::send(const std::string& msg)
{
	if (auto session = session_.lock())
		session->send(msg);
}

void NetworkPlayer::sendJson(const json& j)
{
	if (auto session = session_.lock())
		session->sendJson(j);
}

Color NetworkPlayer::color()const
{
	return color_;
}

bool NetworkPlayer::isAI()const
{
	return false;
}

std::shared_ptr<Session> NetworkPlayer::getSession() const
{
	return session_.lock();
}