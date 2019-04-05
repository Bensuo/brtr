#pragma once

#include "buffer.hpp"
#include "bvh.hpp"
#include "camera.hpp"
#include "denoiser.hpp"
#include "light.hpp"
#include <memory>
#include <vector>

namespace brtr
{
    class platform;
    class gpgpu_platform;
    class kernel;
    struct tracer_stats
    {
        float lifetime_overall{0.0f};
        float lifetime_kernel{0.0f};
        float lifetime_bvh_gpu{0.0f};
        float lifetime_bvh_construction{0.0f};
        float num_iterations{0.0f};
        float total_overall;
        float total_raytrace;
        bvh_stats bvh;
        float denoise;
        float denoise_median;
        float last_frame_time;
        float update_dirs;
    };
    class ray_tracer
    {
    public:
        ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h);
        void run();
        std::shared_ptr<buffer> result_buffer() const
        {
            return m_denoiser_median.result_buffer();
        }
        void add_mesh(mesh& mesh);
        void set_directional_light(directional_light& light);
        tracer_stats get_stats() const
        {
            return stats;
        }

    private:
        void generate_random_dirs();
        void update_random_dirs();
        std::shared_ptr<gpgpu_platform> m_gpgpu;
        std::unique_ptr<kernel> m_raytrace_kernel;
        std::unique_ptr<kernel> m_aabb_kernel;
        std::vector<uint8_t> m_image_data;
        std::vector<uint8_t> m_direct_image;
        std::vector<uint8_t> m_indirect_image;
        std::shared_ptr<buffer> m_direct_buffer;
        std::shared_ptr<buffer> m_indirect_buffer;
        std::shared_ptr<buffer> m_result_buffer;
        std::vector<glm::vec3> m_random_dirs;
        std::shared_ptr<buffer> m_random_buffer;
        int m_width;
        int m_height;
        bounding_volume_hierarchy m_bvh;
        std::vector<mesh_material> m_materials;
        std::shared_ptr<buffer> m_lights_buffer;
        camera& m_camera;
        denoiser m_denoiser;
        denoiser m_denoiser_median;
        directional_light m_directional_light;
        // Stats
        tracer_stats stats;
    };
} // namespace brtr
