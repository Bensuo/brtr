#pragma once

#include <vector>
#include "mesh.hpp"
#include "gpgpu_platform.hpp"
#include "brtr_platform.hpp"
#include <glm/gtc/type_aligned.hpp>

namespace brtr
{
	class mesh;
	
	struct triangle
	{
		vertex verts[3];
	};
	struct aabb
	{
		glm::aligned_vec3 position;
		glm::aligned_vec3 min;
		glm::aligned_vec3 max;
		triangle tri;
		unsigned material;
		unsigned morton;
	};

	struct node
	{
		int children[4];
		int aabb;
	};


	class bounding_volume_hierarchy
	{
	public:
		bounding_volume_hierarchy(std::shared_ptr<platform> platform);
		void add_mesh(const mesh& model_mesh, int material_index);
		void construct();
		const std::vector<aabb>& aabbs() const;
		std::shared_ptr<buffer> aabbs_buffer() const
		{
			return m_aabbs_buffer;
		}
		std::shared_ptr<buffer> nodes_buffer() const
		{
			return m_nodes_buffer;
		}
		const std::vector<node>& nodes() const
		{
			return m_nodes;
		}
	private:
		std::shared_ptr<gpgpu_platform> m_platform;
		std::unique_ptr<kernel> m_kernel;
		std::vector<node> m_nodes;
		std::vector<aabb> m_aabbs;
		std::shared_ptr<buffer> m_aabbs_buffer;
		std::shared_ptr<buffer> m_nodes_buffer;
	};
}
