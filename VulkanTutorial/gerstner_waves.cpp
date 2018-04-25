#include "gerstner_waves.hpp"

void Gerstner::resize_offset_field(uint32_t resolution)
{
	offset_field.resize(resolution);
	for(int i = 0; i < resolution; i++)
	{
		offset_field[i].resize(resolution);
	}
}
