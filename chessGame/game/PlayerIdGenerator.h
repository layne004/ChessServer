#pragma once
#include <string>
#include <atomic>
#include <sstream>
// 递增ID + 时间戳
class PlayerIdGenerator {
public:
	static std::string generate()
	{
		uint64_t id = ++counter_;
		std::ostringstream oss;
		oss << "P" << id;
		return oss.str();
	}

private:
	static std::atomic<uint64_t> counter_;
};