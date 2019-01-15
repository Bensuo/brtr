#include "bvh.hpp"
#include "mesh.hpp"
#include <algorithm>

namespace brtr
{
	bounding_volume_hierarchy::bounding_volume_hierarchy(std::shared_ptr<platform> platform)
		: m_platform(platform->gpgpu())
	{
		m_kernel = m_platform->load_kernel("../../src/gpgpu/opencl/bvh.cl", "calc_bounding_boxes");
		
	}

	void bounding_volume_hierarchy::add_mesh(const mesh& model_mesh, int material_index)
	{
		const auto& verts = model_mesh.vertices();
		const auto& indices = model_mesh.indices();
		for (int i = 0; i < indices.size()/3; i++)
		{
			leaf_node volume;
			volume.tri.verts[0] = verts[indices[i*3]];
			volume.tri.verts[1] = verts[indices[i*3 + 1]];
			volume.tri.verts[2] = verts[indices[i*3 + 2]];
			volume.material = material_index;
			m_leaf_nodes.emplace_back(volume);
		}
	}

	void bounding_volume_hierarchy::construct()
	{
		size_t depth = 0;

		const auto expand_aabb = [](aabb& surround, aabb& new_box)
		{

			glm::vec3 min;
			min.x = glm::min(surround.min.x, new_box.min.x);
			min.y = glm::min(surround.min.y, new_box.min.y);
			min.z = glm::min(surround.min.z, new_box.min.z);
			glm::vec3 max;	 				 
			max.x = glm::max(surround.max.x, new_box.max.x);
			max.y = glm::max(surround.max.y, new_box.max.y);
			max.z = glm::max(surround.max.z, new_box.max.z);
			surround.min = min;
			surround.max = max;
			surround.position = min + (max - min) * 0.5f;
		};

		
		int rounded_nodes = m_leaf_nodes.size() + m_leaf_nodes.size() % 4;
		int total_nodes = 0;
		int to_add = rounded_nodes;
		while(true)
		{
			total_nodes += to_add;
			to_add /= 4;
			if(to_add == 1)
			{
				total_nodes += to_add;
				break;
			}
		}
		m_nodes.clear();
		m_nodes.resize(total_nodes);

		//for (int i = m_leaf_nodes.size() - 1; i >= 0 ; ++i)
		//{
		//	m_nodes[m_nodes.size() - i].leaf_node = m_leaf_nodes.size() - 1 - i;
		//}
		
		m_kernel->set_global_work_size(m_leaf_nodes.size());
		m_leaf_nodes_buffer = m_platform->create_buffer(buffer_access::read_write, sizeof(leaf_node), m_leaf_nodes.size(), m_leaf_nodes.data());
		m_nodes_buffer = m_platform->create_buffer(buffer_access::read_only, sizeof(node), m_nodes.size(), m_nodes.data());
		m_kernel->set_kernel_arg(buffer_operation::write, 0, m_leaf_nodes_buffer);
		m_kernel->set_kernel_arg(buffer_operation::read, 0, m_leaf_nodes_buffer);
		m_kernel->run();
		m_platform->run();

		std::sort(std::begin(m_leaf_nodes), std::end(m_leaf_nodes), [](const leaf_node& a, const leaf_node& b) {return a.morton < b.morton; });
		for (int i = 0; i < m_leaf_nodes.size(); ++i)
		{
			int index = m_nodes.size() - (m_leaf_nodes.size() - i);
			m_nodes[index].leaf_node = i + 1;
			m_nodes[index].bounds = m_leaf_nodes[i].bounds;
		}
		for (int i = 0; i < m_nodes.size(); ++i)
		{

				for (int j = 0; j < 4; ++j)
				{
					int index = 4 * i + (j + 1);
					if ( m_nodes[i].leaf_node == 0 && index < m_nodes.size())
					{
						
						m_nodes[i].children[j] = index;
					}
					else
					{
						m_nodes[i].children[j] = 0;
					}

				}
			

		}
		
		for (int i = m_nodes.size() - m_leaf_nodes.size(); i >= 0; i--)
		{
			for (int j = 0; j < 4; ++j)
			{
				if (m_nodes[i].children[j] != 0)
				{
					expand_aabb(m_nodes[i].bounds, m_nodes[m_nodes[i].children[j]].bounds);
				}
			}

		}
	}

	const std::vector<leaf_node>& bounding_volume_hierarchy::leaf_nodes() const
	{
		return m_leaf_nodes;
	}
}
