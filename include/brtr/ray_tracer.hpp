#pragma once

#include <memory>
#include "buffer.hpp"
#include <vector>
#include "bvh.hpp"
#include "camera.hpp"

namespace brtr
{
	class platform;
	class gpgpu_platform;
	class kernel;
	class ray_tracer
	{
	public:
		ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h, int samples_per_pixel);
		void run();
		std::shared_ptr<buffer> result_buffer() const
		{
			return m_result_buffer;
		}
		void add_mesh(mesh& mesh);
	private:
		std::shared_ptr<gpgpu_platform> m_gpgpu;
		std::unique_ptr<kernel> m_raytrace_kernel;
		std::unique_ptr<kernel> m_aabb_kernel;
		std::vector<uint8_t> m_image_data;
		std::shared_ptr<buffer> m_result_buffer;
		int m_width;
		int m_height;
		int m_samples_per_pixel;
		bounding_volume_hierarchy m_bvh;
		std::vector<mesh_material> m_materials;
		camera& m_camera;
		
	};
}
