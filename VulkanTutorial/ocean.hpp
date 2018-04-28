#pragma once

#include <vector>
#include <iostream>

//#include "application.hpp"
#include "vertex.hpp"
#include "logger.hpp"
#include "gerstner_waves.hpp"
#include "helper.hpp"

class Ocean
{
	float tile_size;
	static enum WaveType { GerstnerWaves, FFT };

	std::vector<Gerstner> m_waves;
	std::vector<Vertex> m_vertices = {}; //vertices of the plane
	std::vector<uint32_t> m_indices = {}; //indeces for draw order

	void initializeVertices(uint32_t resolution);
	void initializeWave(uint32_t resolution);


public:
	uint32_t resolution;

	Ocean(uint32_t resolution = 1024, float tilesize = 256);
	std::vector<Vertex> getVertices();
	std::vector<uint32_t> getIndices();
	//std::vector<glm::vec3> getHeightmap();
	std::vector<Displacement> update_waves(float time);
};

