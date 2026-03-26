#include "PlayerIdGenerator.h"

std::atomic<uint64_t> PlayerIdGenerator::counter_{ 0 };