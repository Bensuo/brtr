#include "ray_tracer.hpp"
#include <iostream>
#include "glm/glm.hpp"
#include "CL/cl.hpp"
namespace brtr
{
	ray_tracer::ray_tracer()
	{
		std::cout << "Ray_tracer constructed\n";
	}

	void ray_tracer::run()
	{
		std::cout << "Run()\n";
		glm::vec3 a{ 0,1,0 };
		cl::Buffer buf;
	}
}
