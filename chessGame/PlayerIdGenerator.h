#pragma once
#include <string>
#include <atomic>
#include <sstream>
// µİÔöID + Ê±¼ä´Á
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