#pragma once

#include <optional>
#include <string>
#include <mutex>
#include <unordered_map>

struct LessonProgressRecord
{
	bool completed = false;
	std::optional<int> bestMoveCount;
	std::optional<int> lastMoveCount;
};

class Database
{
public:
	static Database& instance();

	bool upsertLessonProgress(const std::string& userId, const std::string& lessonId, int moveCount);
	std::unordered_map<std::string, LessonProgressRecord> getLessonProgress(const std::string& userId);

private:
	Database() = default;
	~Database() = default;
	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	bool ensureConnected();
	bool connectLocked();
	std::string escapeStringLocked(const std::string& value);

private:
	void* conn_ = nullptr;
	std::mutex mutex_;
};
