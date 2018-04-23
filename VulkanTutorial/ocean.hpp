#pragma once

#include <vector>
#include <iostream>

//#include "application.hpp"
#include "vertex.hpp"
#include "logger.hpp"

class Ocean
{
	float tile_size;

	std::vector<Vertex> m_vertices = {};
	std::vector<uint32_t> m_indices = {};
	std::vector<float> m_heightmap = {};

	void initializeVertices(uint32_t resolution);
	void initializeHeightmap(uint32_t resolution);


public:
	Ocean(uint32_t resolution = 1024, float tilesize = 3.0f);
	std::vector<Vertex> getVertices();
	std::vector<uint32_t> getIndices();
	std::vector<float> getHeightmap();
};

