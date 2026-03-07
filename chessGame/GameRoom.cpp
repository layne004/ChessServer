#include "GameRoom.h"
#include "MoveValidator.h"
#include "CheckEvaluator.h"
#include "Session.h"
#include <iostream>
//#include "Player.h"
#include "NetworkPlayer.h"

GameRoom::GameRoom(boost::asio::io_context& io)
	:strand_(boost::asio::make_strand(io))
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

			white_ = white;
			black_ = black;

			white->sendJson({
				{"type", "match_success"},
				{"color", "white"}
				});

			black->sendJson({
				{"type", "match_success"},
				{"color", "black"}
				});

			broadcastState();
		}
	);
}

void GameRoom::leave(const std::shared_ptr<Session>& playerSession)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, playerSession] 
		{
			if (white_) {
				auto netp = std::dynamic_pointer_cast<NetworkPlayer>(white_);
				if (netp && netp->getSession() == playerSession)
				{
					white_.reset();
				}
			}

			if (black_) {
				auto netp = std::dynamic_pointer_cast<NetworkPlayer>(black_);
				if (netp && netp->getSession() == playerSession)
				{
					black_.reset();
				}
			}
		});

}

void GameRoom::handleMove(std::shared_ptr<Player> player, const std::string& from, const std::string& to)
{
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
				broadcastGameOver();
				return;
			}

			broadcastState();
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

			broadcastJson({
				{"type", "game_over"},
				{"result", winner},
				{"reason", "resign"}
			});
		}
	);
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

void GameRoom::broadcastState()
{
	json j = {
		{"type", "state_update"},
		{"fen", board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_)},
		{"turn", turn_ == Color::White ? "white" : "black"}
	};

	broadcastJson(j);
}

void GameRoom::broadcastGameOver()
{
	json j = {
		{"type", "game_over"},
		{"result", turn_ == Color::White?"black_win":"white_win"}
	};

	broadcastJson(j);
}

