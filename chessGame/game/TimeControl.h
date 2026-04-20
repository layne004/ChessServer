#pragma once
#include <cstdint>

struct TimeControl
{
	int initial_ms;		// A:初始时间
	int increment_ms;	// B:每步加时

	TimeControl(int initial = 300000, int increment = 3000)
		: initial_ms(initial), increment_ms(increment) {
	}
};