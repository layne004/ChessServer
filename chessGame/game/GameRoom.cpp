#include "GameRoom.h"
#include "MoveValidator.h"
#include "CheckEvaluator.h"
#include "Session.h"
#include <iostream>
#include "NetworkPlayer.h"
#include "AIPlayer.h"
using namespace std::chrono;

GameRoom::GameRoom(boost::asio::io_context& io, RoomID roomId, Mode mode,
	int initialTimeMs,
	int incrementMs)
	:strand_(boost::asio::make_strand(io)), clockTimer_(io), roomId_(roomId),mode_(mode),
	initialTimeMs_(initialTimeMs), incrementMs_(incrementMs)
{
	// 初始化
	board_.init();
}

void GameRoom::updateClockBeforeMove()
{
	if (mode_ == Mode::PvE)
		return;

	auto now = steady_clock::now();

	ClockState& current = (turn_ == Color::White) ? whiteClock_ : blackClock_;

	int used = duration_cast<milliseconds>(now - current.lastStart).count();

	current.remaining_ms -= used;

	if (current.remaining_ms <= 0)
	{
		std::string winner =
			(turn_ == Color::White) ? "black_win" : "white_win";

		endGame(winner, "timeout");
		return;
	}

	current.remaining_ms += incrementMs_;
}

void GameRoom::startClockTimer()
{
	// 人机对战无时间限制
	if (mode_ == Mode::PvE)
		return;

	auto self = shared_from_this();

	clockTimer_.expires_after(std::chrono::milliseconds(200));

	clockTimer_.async_wait(
		boost::asio::bind_executor(
			strand_,
			[this, self](const boost::system::error_code& ec)
			{
				checkTimeout(ec);
			}
		)
	);
}

void GameRoom::checkTimeout(const boost::system::error_code& ec)
{
	if (ec || state_ != GameState::Playing)
		return;

	auto now = steady_clock::now();

	ClockState& current = (turn_ == Color::White) ? whiteClock_ : blackClock_;

	int elapsed = duration_cast<milliseconds>(now - current.lastStart).count();

	if (elapsed >= current.remaining_ms)
	{
		std::string winner =
			(turn_ == Color::White) ? "black_win" : "white_win";

		endGame(winner, "timeout");
		return;
	}

	startClockTimer();
}

void GameRoom::pauseClock()
{
	if (clockPaused_ || state_ != GameState::Playing)
		return;

	auto now = steady_clock::now();

	// 结算当前玩家时间
	ClockState& current = (turn_ == Color::White) ? whiteClock_ : blackClock_;

	int used = duration_cast<milliseconds>(now - current.lastStart).count();

	current.remaining_ms -= used;

	if (current.remaining_ms <= 0)
	{
		std::string winner = (turn_ == Color::White) ? "black_win" : "white_win";
		endGame(winner, "timeout");
		return;
	}

	// 标记暂停
	clockPaused_ = true;

	// 停止定时器
	clockTimer_.cancel();

	broadcastState();
	std::cout << "[Clock] paused in room " << roomId_ << std::endl;
}

void GameRoom::resumeClock()
{
	if (!clockPaused_ || state_ != GameState::Playing)
		return;

	auto now = steady_clock::now();

	// 当前玩家重新开始计时
	ClockState& current = (turn_ == Color::White) ? whiteClock_ : blackClock_;

	current.lastStart = now;

	clockPaused_ = false;

	startClockTimer();

	broadcastState();
	std::cout << "[Clock] resumed in room " << roomId_ << std::endl;
}

void GameRoom::start(std::shared_ptr<Player> white, std::shared_ptr<Player> black)
{
	
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, white, black]() 
		{

			state_ = GameState::Playing;
			startClockTimer();

			//初始化棋钟
			auto now = std::chrono::steady_clock::now();

			whiteClock_.remaining_ms = initialTimeMs_;
			blackClock_.remaining_ms = initialTimeMs_;

			whiteClock_.lastStart = now; //白方先计时

			white_ = white;
			black_ = black;

			white->sendJson({
				{"type", "match_success"},
				{"player_id", white->id()},
				{"room_id", id()},
				{"color", "white"}
				});

			black->sendJson({
				{"type", "match_success"},
				{"player_id", black->id()},
				{"room_id", id()},
				{"color", "black"}
				});

			broadcastState();

			maybeAIMove();
		}
	);
}

void GameRoom::endGame(const std::string& result, const std::string& reason)
{
	state_ = GameState::GameOver;
	clockTimer_.cancel();

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

void GameRoom::handleMove(std::shared_ptr<Player> player, const std::string& from, const std::string& to, char promotion)
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

	if (clockPaused_)
	{
		player->sendJson(
			{
				{"type", "error"},
				{"message", "Game paused due to disconnect"}
			}
		);
		return;
	}

	auto self = shared_from_this();

	boost::asio::post(
		strand_,
		[this, self, player, from, to, promotion]()
		{
			updateClockBeforeMove();
			if (state_ == GameState::GameOver)
				return;

			if (player->color() != turn_) {
				player->sendJson({
					{"type", "error"},
					{"message", "Not your turn"}
				});
				return;
			}

			Move move = parseMove(from, to, promotion);

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
			auto piece = board_.cells[move.fromRow][move.fromCol];
			bool isCapture = board_.cells[move.toRow][move.toCol].has_value()||
				(piece && piece->type == PieceType::Pawn &&
					move.toRow == board_.enPassantRow &&
					move.toCol == board_.enPassantCol);

			//判断是否有升变
			bool isPromotion = false;

			if (piece && piece->type == PieceType::Pawn)
			{
				if ((turn_ == Color::White && move.toRow == 0) ||
					(turn_ == Color::Black && move.toRow == 7))
				{
					isPromotion = true;
				}
			}
			

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

			// 50步判断
			if (halfmoveClock_ >= 100)
			{
				endGame("stalemate", "draw_50move");
				return;
			}

			//切换走棋方
			turn_ =
				(turn_ == Color::White)
				? Color::Black
				: Color::White;

			// 启动下一方计时
			auto now = steady_clock::now();
			ClockState& next = (turn_ == Color::White) ? whiteClock_ : blackClock_;
			next.lastStart = now;

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
				endGame("stalemate", "stalemate");
				return;
			}

			if (isPromotion)
			{
				broadcastMove(from, to, promotion);
			}
			else {
				broadcastMove(from, to);
			}
			

			maybeAIMove();
		}
	);

}

void GameRoom::maybeAIMove()
{
	std::shared_ptr<Player> current = (turn_ == Color::White) ? white_ : black_;

	if (!current->isAI() || mode_ == Mode::PvP)
		return;

	auto ai = std::dynamic_pointer_cast<AIPlayer>(current);

	std::string fen = board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_);

	ai->asyncThink(fen, [weak = weak_from_this(), current](AIMove move)
		{
			if(auto self = weak.lock())
			{
				self->handleMove(current, move.from, move.to, move.promotion);
			}
		});
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

void GameRoom::onPlayerDisconnected(const std::shared_ptr<Session>& session, const std::string& playerId)
{
	auto self = shared_from_this();

	boost::asio::post(strand_,
		[this, self, session, playerId]() {
			if (state_ == GameState::GameOver)
				return;

			auto player = findPlayerById(playerId);

			if (!player)
				return;

			player->setConnected(false);
			pauseClock();

			std::cout << "Session disconnect: Player "<<player->id()<<" disconnected in room " << roomId_ << std::endl;

			// 防止出现网络抖动而启动多个Timer
			player->cancelDisconnectTimer();
			player->startDisconnectTimer(strand_.get_inner_executor(), 
				[this, self, player]()
				{
					if (state_ == GameState::GameOver)
						return;

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

void GameRoom::reconnect(const std::shared_ptr<Session>& session, const std::string& playerId)
{
	auto self = shared_from_this();

	boost::asio::post(strand_, 
		[this, self, session, playerId]()
		{
			auto player = findPlayerById(playerId);

			if (!player)
			{
				session->sendJson({
					{"type", "error"},
					{"message", "reconnect failed: invalid player id"}
				});
				return;
			}

			if (player->connected())
			{
				session->sendJson(
					{
						{"type", "error"},
						{"message", "reconnect failed: player already connected"}
					}
				);
				return;
			}

			auto net = std::dynamic_pointer_cast<NetworkPlayer>(player);
			if (!net)
				return;

			net->setSession(session);
			player->setConnected(true);
			player->cancelDisconnectTimer(); //取消断线计时

			if (white_ && black_ && white_->connected() && black_->connected())
			{
				resumeClock();
			}

			session->setRoom(shared_from_this());
			session->setPlayer(player);

			session->sendJson({
				{"type", "reconnect_success"},
				{"room_id", roomId_},
				{"player_id", player->id()},
				});

			broadcastState();

		}
	);

}

std::shared_ptr<Player> GameRoom::findPlayerById(const std::string& id)
{
	if (white_ && white_->id() == id)
	{
		return white_;
	}

	if (black_ && black_->id() == id)
	{
		return black_;
	}

	for(auto&s:spectators_)
		if (s && s->id() == id)
		{
			return s;
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

void GameRoom::broadcastMove(const std::string& from, const std::string& to, std::optional<char> promotion)
{
	json j = {
		{"type","move"},
		{"from",from},
		{"to",to},
		{"fen", board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_)},
		{ "turn", turn_ == Color::White ? "white" : "black" }
	};

	if (mode_ == Mode::PvP)
	{
		j["white_time"] = whiteClock_.remaining_ms;
		j["black_time"] = blackClock_.remaining_ms;
	}

	if (promotion.has_value())
	{
		j["promotion"] = std::string(1, promotion.value());
	}

	broadcastJson(j);
}

void GameRoom::broadcastState()
{
	json j = {
		{"type", "state_update"},
		{"fen", board_.toFEN(turn_, halfmoveClock_, fullmoveNumber_)},
		{"turn", turn_ == Color::White ? "white" : "black"}
	};

	if (mode_ == Mode::PvP)
	{
		j["white_time"] = whiteClock_.remaining_ms;
		j["black_time"] = blackClock_.remaining_ms;
	}

	broadcastJson(j);
}

