#include "Database.h"
#include <mysql.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
	std::string getEnvOrDefault(const char* name, const char* fallback)
	{
	#ifdef _MSC_VER
		char* value = nullptr;
		size_t len = 0;

		if (_dupenv_s(&value, &len, name) != 0 || value == nullptr || *value == '\0') {
			if (value)
				free(value);
			return fallback;
		}
		
		std::string result(value);
		free(value);
		return result;
	#else
		const char* value = std::getenv(name);
		if (!value || *value == '\0')
			return fallback;
		return value;
	#endif
	}

	unsigned int getEnvPort(const char* name, unsigned int fallback)
	{
		try {
			std::string value = getEnvOrDefault(name, "");
			if (value.empty())
			{
				return fallback;
			}
			return static_cast<unsigned int>(std::stoul(value));
		}
		catch (...) {
			return fallback;
		}
	}
}

Database& Database::instance()
{
	static Database db;
	return db;
}

bool Database::ensureConnected()
{
	std::lock_guard<std::mutex> lock(mutex_);
	if (conn_ != nullptr) {
		return true;
	}

	return connectLocked();
}

bool Database::connectLocked()
{
	MYSQL* mysql = mysql_init(nullptr);
	if (!mysql) {
		std::cerr << "[Database] mysql_init failed" << std::endl;
		return false;
	}

	const std::string host = getEnvOrDefault("MYSQL_HOST", "127.0.0.1");
	const std::string user = getEnvOrDefault("MYSQL_USER", "root");
	const std::string password = getEnvOrDefault("MYSQL_PASSWORD", "040812");
	const std::string database = getEnvOrDefault("MYSQL_DATABASE", "chess_server");
	const unsigned int port = getEnvPort("MYSQL_PORT", 3306);

	mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

	if (!mysql_real_connect(mysql, host.c_str(), user.c_str(), password.c_str(),
		database.c_str(), port, nullptr, 0)) {
		std::cerr << "[Database] mysql_real_connect failed: " << mysql_error(mysql) << std::endl;
		mysql_close(mysql);
		return false;
	}

	conn_ = mysql;
	return true;
}

std::string Database::escapeStringLocked(const std::string& value)
{
	MYSQL* mysql = static_cast<MYSQL*>(conn_);
	std::vector<char> buffer(value.size() * 2 + 1, '\0');
	const auto escapedSize = mysql_real_escape_string(
		mysql, buffer.data(), value.c_str(), static_cast<unsigned long>(value.size()));
	return std::string(buffer.data(), escapedSize);
}

bool Database::upsertLessonProgress(const std::string& userId, const std::string& lessonId, int moveCount)
{
	if (userId.empty() || lessonId.empty()) {
		return false;
	}

	if (!ensureConnected()) {
		return false;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	MYSQL* mysql = static_cast<MYSQL*>(conn_);
	if (!mysql) {
		return false;
	}

	const std::string escapedUserId = escapeStringLocked(userId);
	const std::string escapedLessonId = escapeStringLocked(lessonId);

	std::ostringstream sql;
	sql
		<< "INSERT INTO lesson_progress "
		<< "(user_id, lesson_id, completed, best_move_count, last_move_count, completed_at) VALUES ("
		<< "'" << escapedUserId << "',"
		<< "'" << escapedLessonId << "',"
		<< "1,"
		<< moveCount << ","
		<< moveCount << ","
		<< "NOW()) "
		<< "ON DUPLICATE KEY UPDATE "
		<< "completed = 1, "
		<< "best_move_count = IF(best_move_count IS NULL, VALUES(best_move_count), LEAST(best_move_count, VALUES(best_move_count))), "
		<< "last_move_count = VALUES(last_move_count), "
		<< "completed_at = NOW()";

	if (mysql_query(mysql, sql.str().c_str()) != 0) {
		std::cerr << "[Database] upsertLessonProgress failed: " << mysql_error(mysql) << std::endl;
		return false;
	}

	return true;
}

std::unordered_map<std::string, LessonProgressRecord> Database::getLessonProgress(const std::string& userId)
{
	std::unordered_map<std::string, LessonProgressRecord> result;
	if (userId.empty()) {
		return result;
	}

	if (!ensureConnected()) {
		return result;
	}

	std::lock_guard<std::mutex> lock(mutex_);
	MYSQL* mysql = static_cast<MYSQL*>(conn_);
	if (!mysql) {
		return result;
	}

	const std::string escapedUserId = escapeStringLocked(userId);
	const std::string sql =
		"SELECT lesson_id, completed, best_move_count, last_move_count "
		"FROM lesson_progress WHERE user_id = '" + escapedUserId + "'";

	if (mysql_query(mysql, sql.c_str()) != 0) {
		std::cerr << "[Database] getLessonProgress failed: " << mysql_error(mysql) << std::endl;
		return result;
	}

	MYSQL_RES* res = mysql_store_result(mysql);
	if (!res) {
		if (mysql_field_count(mysql) != 0) {
			std::cerr << "[Database] mysql_store_result failed: " << mysql_error(mysql) << std::endl;
		}
		return result;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != nullptr) {
		LessonProgressRecord record;
		record.completed = row[1] != nullptr && std::string(row[1]) != "0";
		if (row[2] != nullptr) {
			record.bestMoveCount = std::atoi(row[2]);
		}
		if (row[3] != nullptr) {
			record.lastMoveCount = std::atoi(row[3]);
		}

		result[row[0] ? row[0] : ""] = record;
	}

	mysql_free_result(res);
	return result;
}
