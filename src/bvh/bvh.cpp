#include "bvh.hpp"
#include "mesh.hpp"
#include <algorithm>
#include <chrono>

namespace brtr
{
    bounding_volume_hierarchy::bounding_volume_hierarchy(std::shared_ptr<platform> platform)
        : m_platform(platform->gpgpu()),
          m_construction_time(0),
          m_sort_time(0),
          m_expansion_time(0)
    {
        m_kernel = m_platform->load_kernel(
            "../../src/gpgpu/opencl/bvh.cl", "calc_bounding_boxes");

        m_generate_kernel = m_platform->load_kernel(
            "../../src/gpgpu/opencl/bvh.cl", "generate_hierarchy");
    }

    void bounding_volume_hierarchy::add_mesh(const mesh& model_mesh, int material_index)
    {
        const auto& verts = model_mesh.vertices();
        const auto& indices = model_mesh.indices();
        for (int i = 0; i < indices.size() / 3; i++)
        {
            leaf_node volume;
            volume.tri.verts[0] = verts[indices[i * 3]];
            volume.tri.verts[1] = verts[indices[i * 3 + 1]];
            volume.tri.verts[2] = verts[indices[i * 3 + 2]];
            volume.material = material_index;
            m_leaf_nodes.emplace_back(volume);
        }
    }

    void bounding_volume_hierarchy::construct()
    {
        std::chrono::high_resolution_clock::time_point construction_start =
            std::chrono::high_resolution_clock::now();
        size_t depth = 0;

        const auto expand_aabb = [](aabb& surround, aabb& new_box, float margin) {
            glm::vec3 min;
            min.x = glm::min(surround.min.x, new_box.min.x);
            min.y = glm::min(surround.min.y, new_box.min.y);
            min.z = glm::min(surround.min.z, new_box.min.z);
            glm::vec3 max;
            max.x = glm::max(surround.max.x, new_box.max.x);
            max.y = glm::max(surround.max.y, new_box.max.y);
            max.z = glm::max(surround.max.z, new_box.max.z);
            surround.min = min - glm::vec3(margin);
            surround.max = max + glm::vec3(margin);
        };

        m_nodes.clear();
        m_nodes.resize(m_leaf_nodes.size() - 1);
        m_morton_codes.clear();
        m_morton_codes.resize(m_leaf_nodes.size(), 0);

        m_kernel->set_global_work_size(m_leaf_nodes.size());
        m_leaf_nodes_buffer = m_platform->create_buffer(
            buffer_access::read_write,
            sizeof(leaf_node),
            m_leaf_nodes.size(),
            m_leaf_nodes.data());
        m_nodes_buffer = m_platform->create_buffer(
            buffer_access::read_write, sizeof(node), m_nodes.size(), m_nodes.data());
        m_kernel->set_kernel_arg(buffer_operation::write, 0, m_leaf_nodes_buffer);
        m_kernel->set_kernel_arg(buffer_operation::read, 0, m_leaf_nodes_buffer);

        m_kernel->run();
        m_platform->run();
        std::chrono::high_resolution_clock::time_point sort_start =
            std::chrono::high_resolution_clock::now();
        std::sort(
            std::begin(m_leaf_nodes),
            std::end(m_leaf_nodes),
            [](const leaf_node& a, const leaf_node& b) {
                return a.morton < b.morton;
            });

        for (int i = 0; i < m_leaf_nodes.size(); ++i)
        {
            m_morton_codes[i] = m_leaf_nodes[i].morton;
        }
        std::chrono::high_resolution_clock::time_point sort_end =
            std::chrono::high_resolution_clock::now();

        m_generate_kernel->set_global_work_size(m_nodes.size());
        m_generate_kernel->set_kernel_arg(buffer_operation::write, 0, m_nodes_buffer);
        m_generate_kernel->set_kernel_arg(buffer_operation::read, 0, m_nodes_buffer);
        m_generate_kernel->set_kernel_arg(buffer_operation::write, 1, m_leaf_nodes_buffer);
        m_generate_kernel->set_kernel_arg(buffer_operation::read, 1, m_leaf_nodes_buffer);
        m_morton_buffer = m_platform->create_buffer(
            buffer_access::read_only,
            sizeof(unsigned),
            m_morton_codes.size(),
            m_morton_codes.data());
        m_generate_kernel->set_kernel_arg(buffer_operation::write, 2, m_morton_buffer);
        m_generate_kernel->run();
        m_platform->run();

        std::chrono::high_resolution_clock::time_point expansion_start =
            std::chrono::high_resolution_clock::now();
        float margin = 0.0f;
        for (int i = 0; i < m_leaf_nodes.size(); ++i)
        {
            int next = m_leaf_nodes[i].parent;
            while (true)
            {
                node n = m_nodes[next];
                if (n.children[0] == -1)
                {
                    expand_aabb(
                        m_nodes[next].bounds, m_leaf_nodes[n.leaf_node[0]].bounds, margin);
                }
                else
                {
                    expand_aabb(m_nodes[next].bounds, m_nodes[n.children[0]].bounds, margin);
                }
                if (n.children[1] == -1)
                {
                    expand_aabb(
                        m_nodes[next].bounds, m_leaf_nodes[n.leaf_node[1]].bounds, margin);
                }
                else
                {
                    expand_aabb(m_nodes[next].bounds, m_nodes[n.children[1]].bounds, margin);
                }
                next = n.parent;
                if (next == -1)
                {
                    break;
                }
            }
        }

        std::chrono::high_resolution_clock::time_point expansion_end =
            std::chrono::high_resolution_clock::now();
        m_sort_time = (sort_end - sort_start).count() / 1000000.0f;
        m_expansion_time = (expansion_end - expansion_start).count() / 1000000.0f;
        m_morton_time = m_kernel->get_last_execution_time();
        m_generate_time = m_generate_kernel->get_last_execution_time();
        m_construction_time = (expansion_end - construction_start).count() / 1000000.0f;
    }

    const std::vector<leaf_node>& bounding_volume_hierarchy::leaf_nodes() const
    {
        return m_leaf_nodes;
    }
} // namespace brtr
