#pragma once

#include "brtr_platform.hpp"
#include "gpgpu_platform.hpp"
#include "mesh.hpp"
#include <glm/gtc/type_aligned.hpp>
#include <vector>

namespace brtr
{
    class mesh;

    struct triangle
    {
        vertex verts[3];
    };
    struct aabb
    {
        glm::aligned_vec3 min;
        glm::aligned_vec3 max;
    };

    struct node
    {
        int parent;
        int children[2];
        aabb bounds;
        unsigned leaf_node[2];
    };

    struct leaf_node
    {
        int parent;
        aabb bounds;
        triangle tri;
        unsigned material;
        unsigned morton;
    };

    struct bvh_stats
    {
        float morton_time;
        float generate_time;
        float construction_time;
        float sort_time;
        float expansion_time;
        int object_count;
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
        float get_last_execution_time_gpu() const
        {
            return m_kernel->get_last_execution_time();
        }
        float get_last_construction_time() const
        {
            return m_construction_time;
        }

        bvh_stats stats()
        {
            bvh_stats val;
            val.morton_time = m_morton_time;
            val.generate_time = m_generate_time;
            val.construction_time = m_construction_time;
            val.sort_time = m_sort_time;
            val.expansion_time = m_expansion_time;
            val.object_count = m_leaf_nodes.size();
            return val;
        }

    private:
        std::shared_ptr<gpgpu_platform> m_platform;
        std::unique_ptr<kernel> m_kernel;
        std::unique_ptr<kernel> m_generate_kernel;
        std::shared_ptr<buffer> m_node_flags_buffer;
        std::vector<node> m_nodes;
        std::vector<unsigned> m_morton_codes;
        std::shared_ptr<buffer> m_morton_buffer;
        std::vector<leaf_node> m_leaf_nodes;
        std::shared_ptr<buffer> m_leaf_nodes_buffer;
        std::shared_ptr<buffer> m_nodes_buffer;

        float m_morton_time;
        float m_generate_time;
        float m_construction_time;
        float m_sort_time;
        float m_expansion_time;
    };
} // namespace brtr
