#include "ocean.hpp"

void Ocean::initializeVertices(uint32_t resolution)
{

	info("generating vertices");
	m_vertices = {};
	m_indices = {};

	size_t overall_size = resolution * resolution;
	m_vertices.resize(overall_size);

	float step = tile_size / resolution;
	float texStep = 1.0f / resolution;

	uint32_t row = 0;
	for (uint32_t column = 0; column < overall_size; column++) {
		if (column % resolution == 0 && column != 0)
			row++;

		m_vertices[column] = { {-0.5* tile_size + column % resolution * step, -0.5* tile_size + row * step, 0.0f}, {0.0f, 1.0f, 0.0f}, {texStep * column, texStep * row} };

	}

	info("generating indices");
	//generate the indeces row by row in clockwise fashion
	uint32_t a,b,c;
	a = 0;
	b = resolution;
	c = 1;
	for (int i = 0; a < overall_size && b < overall_size && c < overall_size; i++) {
		//the way the indices are done requires this part
		if (i % 2 == 0) {
			c = a+1;
		}
		else {
			c = b+1;
		}
		//end of row, jump to the next one
		if (c % resolution == 0) {
			a++;
			b++;
			c++;
		}
		//if the last vertex isnt in the indeces yet, continue adding them, otherwise break
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
	info("Set Up Ocean surface");
}

void Ocean::initializeWave(uint32_t resolution)
{

	m_waves.push_back(Gerstner(glm::vec2(1.0f, 0.0f), 5.0f, 64.0f, 128.0f));	
	//m_waves.push_back(Gerstner(glm::vec2(-0.50f, 3.0f), 0.50f, 0.10f, 0.4f));

	//m_waves.push_back(Gerstner(glm::vec2(1.0f, -3.5f), .120f, 1500.0f, 21.7f));
	//m_waves.push_back(Gerstner(glm::vec2(1.0f, 2.3f), .080f, 1000.0f, 35.55f));
	/*m_waves.push_back(Gerstner(glm::vec2(-5.0f, 1.0f), 0.240f, 1.50f, 1.5f));
	m_waves.push_back(Gerstner(glm::vec2(3.0f, -1.0f), 0.430f, 2.0f, 0.7f));*/
	//m_waves.push_back(Gerstner(glm::vec2(2.0f, 7.0f), 1.270f, 3.0f, 1.8f));
	//m_waves.push_back(Gerstner(glm::vec2(-3.0f, 4.0f), 0.820f, 6.0f, 1.3f));
	//m_waves.push_back(Gerstner(glm::vec2(56.0f, -34.0f), 1.670f, 4.0f, 1.1f));

}

Ocean::Ocean(uint32_t resolution, float tilesize)
{
	info("Setting up Ocean...");
	if (resolution > 64) {
		warn("WARNING: Entering resolutions higher than 128 might become very demanding and will require considerable time to generate the ocean surface and indices.");
	}
	this->resolution = resolution;
	tile_size = tilesize;
	initializeVertices(resolution);
	initializeWave(resolution);
	succ("Ocean successfully initialized");
}

std::vector<Vertex> Ocean::getVertices()
{
	return m_vertices;
}

std::vector<uint32_t> Ocean::getIndices()
{
	return m_indices;
}

//std::vector<glm::vec3> Ocean::getHeightmap()
//{
//}

std::vector<Displacement> Ocean::update_waves(float time){
	std::vector<Displacement> current_displacement = {};

	if (current_displacement.size() != m_vertices.size()) {
		current_displacement.resize(m_vertices.size());
	}
	for (Gerstner wave : m_waves) {
		current_displacement = wave.apply_wave(current_displacement, resolution, tile_size, time);
	}
	return current_displacement;
}