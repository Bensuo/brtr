#include "bvh.hpp"
#include "mesh.hpp"
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
			aabb volume;
			volume.tri.verts[0] = verts[indices[i*3]];
			volume.tri.verts[1] = verts[indices[i*3 + 1]];
			volume.tri.verts[2] = verts[indices[i*3 + 2]];
			volume.material = material_index;
			m_aabbs.emplace_back(volume);
		}
	}

	void bounding_volume_hierarchy::construct()
	{
		m_nodes.clear();
		for (int i = 0; i < m_aabbs.size(); ++i)
		{
			node n;
			n.aabb = i;
			m_nodes.push_back(n);
		}
		m_kernel->set_global_work_size(m_aabbs.size());
		m_aabbs_buffer = m_platform->create_buffer(buffer_access::read_write, sizeof(aabb), m_aabbs.size(), m_aabbs.data());
		m_nodes_buffer = m_platform->create_buffer(buffer_access::read_only, sizeof(node), m_nodes.size(), m_nodes.data());
		m_kernel->set_kernel_arg(buffer_operation::write, 0, m_aabbs_buffer);
		m_kernel->set_kernel_arg(buffer_operation::read, 0, m_aabbs_buffer);
		m_kernel->run();
		m_platform->run();
	}

	const std::vector<aabb>& bounding_volume_hierarchy::aabbs() const
	{
		return m_aabbs;
	}
}
