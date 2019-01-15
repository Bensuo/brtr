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
	};

	struct node
	{
		int children[4];
		aabb bounds;
		unsigned leaf_node;
	};

	struct leaf_node
	{
		aabb bounds;
		triangle tri;
		unsigned material;
		unsigned morton;
	};

	class bounding_volume_hierarchy
	{
	public:
		bounding_volume_hierarchy(std::shared_ptr<platform> platform);
		void add_mesh(const mesh& model_mesh, int material_index);
		void construct();
		const std::vector<leaf_node>& leaf_nodes() const;
		std::shared_ptr<buffer> aabbs_buffer() const
		{
			return m_leaf_nodes_buffer;
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
		std::vector<leaf_node> m_leaf_nodes;
		std::shared_ptr<buffer> m_leaf_nodes_buffer;
		std::shared_ptr<buffer> m_nodes_buffer;
	};
}
