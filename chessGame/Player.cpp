#include "Player.h"
#include "Session.h"

NetworkPlayer::NetworkPlayer(std::shared_ptr<Session> session, Color c)
{
}

void NetworkPlayer::sendJson(const json& j)
{
	session_->sendJson(j);
}

Color NetworkPlayer::color()const
{
	return color_;
}

bool NetworkPlayer::isAI()const 
{
	return false;
}