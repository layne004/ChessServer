#include "GameRoom.h"
#include "MoveValidator.h"
#include "CheckEvaluator.h"
#include "Session.h"
#include <iostream>

GameRoom::GameRoom(boost::asio::io_context& io)
	:strand_(boost::asio::make_strand(io))
{
	// 初始化
	board_.init();
}

void GameRoom::start(std::shared_ptr<Session> p1, std::shared_ptr<Session> p2)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, p1, p2]() 
		{

			p1_ = p1;
			p2_ = p2;

			p1->sendJson({
				{"type", "match_success"},
				{"color", "white"}
				});

			p2->sendJson({
				{"type", "match_success"},
				{"color", "black"}
				});

			broadcastState();
		}
	);
}

void GameRoom::join(std::shared_ptr<Session> player)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, player] {

			if (p1_.expired()) {
				p1_ = player;
			}
			else if (p2_.expired())
			{
				p2_ = player;
				broadcast("GAME START\n");
			}

		});

}

void GameRoom::leave(const std::shared_ptr<Session>& player)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, player] 
		{
			if (auto p1 = p1_.lock()) {
				if (p1 == player)
				{
					p1_.reset();
				}
			}

			if (auto p2 = p2_.lock())
			{
				if (p2 == player)
				{
					p2_.reset();
				}
			}
		});

}

//tofix 非线程安全
Color GameRoom::getPlayerColor(const std::shared_ptr<Session>& player) const
{
	if (auto p1 = p1_.lock()) {
		if(p1 == player)
			return Color::White;
	} 
	if (auto p2 = p2_.lock()) {
		if (p2 == player)
			return Color::Black;
	}
	throw std::runtime_error("Unknown player");
}

void GameRoom::handleMove(std::shared_ptr<Session> playerSession, Color player, const std::string& from, const std::string& to)
{
	auto self = shared_from_this();

	boost::asio::post(
		strand_,
		[this, self, playerSession, player, from, to]()
		{
			if (player != turn_) {
				playerSession->sendJson({
					{"type", "error"},
					{"message", "Not your turn"}
				});
				return;
			}

			Move move = parseMove(from, to);

			if (!MoveValidator::isValid(board_, move, turn_))
			{
				playerSession->sendJson(
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

void GameRoom::handleResign(const std::shared_ptr<Session> player)
{
	auto self = shared_from_this();

	boost::asio::post(strand_, 
		[this, self, player]() 
		{
			Color color = getPlayerColor(player);

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
	auto p1 = p1_.lock();
	auto p2 = p2_.lock();

	return p1 && p2;
}

bool GameRoom::isEmpty() const
{
	return p1_.expired() && p2_.expired();
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
	if (auto p1 = p1_.lock()) {
		p1->send(msg);
	}
	if (auto p2 = p2_.lock()) {
		p2->send(msg);
	}

}

void GameRoom::broadcastJson(const json& j)
{
	if (auto p1 = p1_.lock()) {
		p1->sendJson(j);
	}
	if (auto p2 = p2_.lock()) {
		p2->sendJson(j);
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

