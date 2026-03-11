#include "GameRoom.h"
#include "MoveValidator.h"
#include "CheckEvaluator.h"
#include "Session.h"
#include <iostream>
//#include "Player.h"
#include "NetworkPlayer.h"

GameRoom::GameRoom(boost::asio::io_context& io, RoomID roomId)
	:strand_(boost::asio::make_strand(io)),roomId_(roomId)
{
	// 初始化
	board_.init();
}

void GameRoom::start(std::shared_ptr<Player> white, std::shared_ptr<Player> black)
{
	
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, white, black]() 
		{

			state_ = GameState::Playing;

			white_ = white;
			black_ = black;

			white->sendJson({
				{"type", "match_success"},
				{"room_id", id()},
				{"color", "white"}
				});

			black->sendJson({
				{"type", "match_success"},
				{"room_id", id()},
				{"color", "black"}
				});

			broadcastState();
		}
	);
}

void GameRoom::endGame(const std::string& result, const std::string& reason)
{
	state_ = GameState::GameOver;

	broadcastJson({
		{"type", "game_over"},
		{"result", result},
		{"reason", reason }
	});

	if (white_) {
		if (auto netp = std::dynamic_pointer_cast<NetworkPlayer>(white_)) {
			if (auto s = netp->getSession())
				s->clearRoom();
		}
	}

	if (black_) {
		if (auto netp = std::dynamic_pointer_cast<NetworkPlayer>(black_)) {
			if (auto s = netp->getSession())
				s->clearRoom();
		}
	}

	white_.reset();
	black_.reset();
}

void GameRoom::handleMove(std::shared_ptr<Player> player, const std::string& from, const std::string& to)
{
	if (state_ != GameState::Playing)
	{
		player->sendJson(
			{
				{"type", "error"},
				{"message", "Game not in playing state"}
			}
		);
		return;
	}
	auto self = shared_from_this();

	boost::asio::post(
		strand_,
		[this, self, player, from, to]()
		{

			if (player->color() != turn_) {
				player->sendJson({
					{"type", "error"},
					{"message", "Not your turn"}
				});
				return;
			}

			Move move = parseMove(from, to);

			if (!MoveValidator::isValid(board_, move, turn_))
			{
				player->sendJson(
					{
						{"type", "error"},
						{"message", "Invalid move"}
					}
				);
				return;
			}

			//判断是否吃子或兵移动
			bool isCapture = board_.cells[move.toRow][move.toCol].has_value();
			auto piece = board_.cells[move.fromRow][move.fromCol];

			board_.applyMove(move);

			// 更新 halfmove 和 fullmove
			bool isPawnMove = false;
			
			if (piece && piece->type == PieceType::Pawn)
				isPawnMove = true;

			if (isPawnMove || isCapture)
				halfmoveClock_ = 0;
			else
				halfmoveClock_++;

			if (turn_ == Color::Black)
				fullmoveNumber_++;

			//切换走棋方
			turn_ =
				(self->turn_ == Color::White)
				? Color::Black
				: Color::White;

			GameResult result =
				CheckEvaluator::evaluate(board_, turn_);

			
			if (result == GameResult::Checkmate) {
				std::string winner = (turn_ == Color::White)
					? "black_win"
					: "white_win";
				endGame(winner, "checkmate");
				return;
			}
			else if (result == GameResult::Stalemate) {
				endGame("no_winner", "stalemate");
				return;
			}

			//broadcastState();
			broadcastMove(from, to);
		}
	);

}

void GameRoom::handleResign(const std::shared_ptr<Player> player)
{
	
	auto self = shared_from_this();

	boost::asio::post(strand_, 
		[this, self, player]() 
		{
			Color color = player->color();

			std::string winner =
				(color == Color::White) ? "black_win" : "white_win";

			std::string reason = "resign";

			endGame(winner, reason);

		}
	);
}

void GameRoom::onPlayerDisconnected(const std::shared_ptr<Session>& session)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, session]() {
			if (state_ == GameState::GameOver)
				return;

			auto player = findPlayer(session);

			if (!player)
				return;

			player->setConnected(false);

			std::cout << "Player disconnected in room " << roomId_ << std::endl;

			player->startDisconnectTimer(strand_.get_inner_executor(), 
				[this, self, player]()
				{
					if (!player->connected())
					{
						std::string winner =
							(player->color() == Color::White) ? "black_win" : "white_win";

						endGame(winner, "disconnect_timeout");
					}
				}
			);
		}
	);
}

void GameRoom::reconnect(const std::shared_ptr<Session>& session)
{
	auto self = shared_from_this();

	boost::asio::post(strand_, 
		[this, self, session]()
		{
			bool found = false;
			for (auto player : { white_, black_ })
			{
				auto net = std::dynamic_pointer_cast<NetworkPlayer>(player);

				if (!net)
					continue;

				if (!player->connected())
				{
					net->setSession(session);
					player->setConnected(true);
					player->cancelDisconnectTimer(); //取消断线计时

					session->setRoom(shared_from_this());
					session->setPlayer(player);

					found = true;
					broadcastState();
					break;
				}
			}

			if (found) {
				session->sendJson({
					{"type", "reconnect_success"},
					{"room_id", roomId_}
				});
			}
			else {
				session->sendJson({
					{"type", "error"},
					{"message", "reconnect failed: no disconnected player"}
				});
			}
		}
	);

}

std::shared_ptr<Player> GameRoom::findPlayer(const std::shared_ptr<Session>& session)
{
	if (auto net = std::dynamic_pointer_cast<NetworkPlayer>(white_)) {
		if (net->getSession() == session)
			return white_;
	}

	if (auto net = std::dynamic_pointer_cast<NetworkPlayer>(black_)) {
		if (net->getSession() == session)
			return black_;
	}

	return nullptr;
}

bool GameRoom::isFull() const 
{ 
	return white_ && black_;
}

bool GameRoom::isEmpty() const
{
	return !white_ && !black_;
}

void GameRoom::resetGame()
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self]() 
		{
			board_.init();
			turn_ = Color::White;

			broadcast("RESET");
		}
	);

}

void GameRoom::broadcast(const std::string& msg)
{
	if (white_) {
		white_->send(msg);
	}
	if (black_) {
		black_->send(msg);
	}

}

void GameRoom::broadcastJson(const json& j)
{
	if (white_) {
		white_->sendJson(j);
	}
	if (black_) {
		black_->sendJson(j);
	}
}

void GameRoom::broadcastMove(const std::string& from, const std::string& to)
{
	json j = {
		{"type","move"},
		{"from",from},
		{"to",to},
		{"fen", board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_)},
		{ "turn", turn_ == Color::White ? "white" : "black" }
	};

	broadcastJson(j);
}

void GameRoom::broadcastState()
{
	json j = {
		{"type", "state_update"},
		{"fen", board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_)},
		{"turn", turn_ == Color::White ? "white" : "black"}
	};

	broadcastJson(j);
}

