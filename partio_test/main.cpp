# include<iostream>
# include"Partio.h"
using namespace Partio;

int main() {
	Partio::ParticlesDataMutable* data = Partio::read("test.bgeo");
	std::cout << "Number of particles " << data->numParticles() << std::endl;
}
