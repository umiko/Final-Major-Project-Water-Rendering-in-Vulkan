#include "ocean.hpp"

void Ocean::initializeVertices(uint32_t resolution)
{
	m_vertices = {};
	m_indices = {};

	size_t overall_size = resolution * resolution;
	m_vertices.resize(overall_size);

	float step = tile_size / resolution;
	float texStep = 1.0f / resolution;

	uint16_t row = 0;
	for (uint32_t column = 0; column < overall_size; column++) {
		if (column % resolution == 0 && column != 0)
			row++;

		m_vertices[column] = { {-0.5* tile_size + column % resolution * step, -0.5* tile_size + row * step, 0.0f}, {0.0f, 1.0f, 0.0f}, {texStep * column, texStep * row} };

	}

	info("generating indices");

	uint32_t a,b,c;
	a = 0;
	b = resolution;
	c = 1;
	for (int i = 0; a < overall_size && b < overall_size && c < overall_size; i++) {
		

		if (i % 2 == 0) {
			c = a+1;
		}
		else {
			c = b+1;
		}

		if (c % resolution == 0) {
			a++;
			b++;
			c++;
		}
		if (a < overall_size && b < overall_size && c < overall_size) {
			m_indices.push_back(a);
			
			m_indices.push_back(c);
			m_indices.push_back(b);
		}
		else {
			break;
		}

		if (i % 2 == 0) {
			a = c;
		}
		else {
			b = c;
		}
	}
	info("fettig");
}

void Ocean::initializeHeightmap(uint32_t resolution)
{
}

Ocean::Ocean(uint32_t resolution, float tilesize)
{
	tile_size = tilesize;
	initializeVertices(resolution);
	initializeHeightmap(resolution);
}

std::vector<Vertex> Ocean::getVertices()
{
	return m_vertices;
}

std::vector<uint32_t> Ocean::getIndices()
{
	return m_indices;
}

std::vector<float> Ocean::getHeightmap()
{
	return m_heightmap;
}
