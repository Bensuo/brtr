#pragma once

#include "buffer.hpp"
#include "bvh.hpp"
#include "camera.hpp"
#include "denoiser.hpp"
#include <memory>
#include <vector>

namespace brtr
{
    struct point_light;
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
        float total_bvh;
        float bvh_gpu;
        float bvh_construction;
        float denoise;
    };
    class ray_tracer
    {
    public:
        ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h, int samples_per_pixel);
        void run();
        std::shared_ptr<buffer> result_buffer() const
        {
            return m_denoiser.result_buffer();
        }
        void add_mesh(mesh& mesh);
        void add_light(point_light& light);
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
        std::shared_ptr<buffer> m_result_buffer;
        std::vector<glm::vec3> m_random_dirs;
        std::shared_ptr<buffer> m_random_buffer;

        int m_width;
        int m_height;
        int m_samples_per_pixel;
        bounding_volume_hierarchy m_bvh;
        std::vector<mesh_material> m_materials;
        std::vector<point_light> m_lights;
        std::shared_ptr<buffer> m_lights_buffer;
        camera& m_camera;
        denoiser m_denoiser;
        // Stats
        tracer_stats stats;
    };
} // namespace brtr
