#include "gerstner_waves.hpp"


float Gerstner::get_w() {
	return sqrtf(g * get_K());
}

float Gerstner::get_K() {
	return 2 * PI / lambda;
}

float Gerstner::get_y() {
	//TODO: check if it really is the length
	return A * cosf(k.length() * x0.length() - get_w()*time);
}

glm::vec2 Gerstner::get_x() {
	return x0 - (k / get_K())*A*sinf(k.length() * x0.length() - get_w()*time);
}

//Applies this wave on top of a wavemap
void Gerstner::apply_wave(std::vector<glm::vec3>& current_waves, float time) {
	this->time = time;

	for (glm::vec3 dispersion : current_waves) {
		//For now its just adding waves ontop of each other, maybe rework that later on
		dispersion += glm::vec3(get_x(), get_y());
	}
}
