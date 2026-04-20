#include "Player.h"
#include "PlayerIdGenerator.h"
Player::Player()
{
	id_ = PlayerIdGenerator::generate();
}

void Player::startDisconnectTimer(boost::asio::any_io_executor executor, std::function<void()> timeoutCallback)
{
	if (!disconnectTimer_)
		disconnectTimer_ = std::make_unique<boost::asio::steady_timer>(executor);

	disconnectTimer_->expires_after(DISCONNECT_TIMEOUT);

	disconnectTimer_->async_wait(
		[cb = std::move(timeoutCallback)](const boost::system::error_code& ec)
		{
			if (!ec && cb) {
				cb();
			}
		}
	);
}

void Player::cancelDisconnectTimer()
{
	if (disconnectTimer_)
	{
		disconnectTimer_->cancel();
	}
}