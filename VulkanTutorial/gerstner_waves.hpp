#pragma once
#include <glm\vec3.hpp>
#include <vector>

class Gerstner {
	std::vector<std::vector<glm::vec3>> offset_field;

	void resize_offset_field(uint32_t resolution);
};