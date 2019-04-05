#include "ray_tracer.hpp"
#include "brtr_platform.hpp"
#include "buffer.hpp"
#include "gpgpu/opencl/gpgpu_opencl.hpp"
#include "gpgpu_platform.hpp"
#include "kernel.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>

namespace brtr
{
    ray_tracer::ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h)
        : m_gpgpu(platform->gpgpu()),
          m_image_data(w * h * 4, 0),
          m_result_buffer(m_gpgpu->create_buffer(
              buffer_access::write_only, sizeof(cl_uchar4), w * h, m_image_data.data())),
          m_direct_buffer(m_gpgpu->create_buffer(
              buffer_access::read_write, sizeof(cl_uchar4), w * h, nullptr)),
          m_indirect_buffer(m_gpgpu->create_buffer(
              buffer_access::read_write, sizeof(cl_uchar4), w * h, nullptr)),
          m_width(w),
          m_height(h),
          m_bvh(platform),
          m_camera(cam),
          m_denoiser(platform, m_direct_buffer, w, h, false),
          m_denoiser_median(platform, m_denoiser.result_buffer(), w, h, true)
    {
        std::cout << "Ray_tracer constructed\n";
        m_raytrace_kernel = m_gpgpu->load_kernel(
            "../../src/gpgpu/opencl/raytracing.cl", "ray_trace");
        m_raytrace_kernel->set_global_work_size(m_width, m_height);
        m_raytrace_kernel->set_local_work_size();

        m_random_dirs.resize(1 << 17);
        generate_random_dirs();
        m_random_buffer = m_gpgpu->create_buffer(
            buffer_access::read_write,
            sizeof(float),
            m_random_dirs.size() * 3,
            m_random_dirs.data());
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 6, m_random_buffer);
    }

    void ray_tracer::run()
    {
        m_bvh.construct();
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 0, m_bvh.nodes_buffer());
        m_raytrace_kernel->set_kernel_arg(1, (int)m_bvh.nodes().size());
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 2, m_bvh.aabbs_buffer());
        std::shared_ptr<buffer> buf = m_gpgpu->create_buffer(
            buffer_access::read_only,
            sizeof(mesh_material),
            m_materials.size(),
            m_materials.data());
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 3, buf);
        m_raytrace_kernel->set_kernel_arg(buffer_operation::none, 4, m_direct_buffer);
        camera_gpu cam = m_camera.gpu();
        std::shared_ptr<buffer> camera_buffer = m_gpgpu->create_buffer(
            buffer_access::read_only, sizeof(camera_gpu), 1, &cam);
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 5, camera_buffer);

        std::chrono::high_resolution_clock::time_point start =
            std::chrono::high_resolution_clock::now();
        update_random_dirs();
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();

        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 6, m_random_buffer);

        m_raytrace_kernel->set_kernel_arg(7, sizeof(directional_light), &m_directional_light);
        m_raytrace_kernel->run();

        m_denoiser.run();
        m_denoiser_median.run();

        m_gpgpu->run();

        stats.update_dirs = (end - start).count() / 1000000.0f;
        stats.bvh = m_bvh.stats();

        stats.total_raytrace = m_raytrace_kernel->get_last_execution_time();
        stats.lifetime_overall += stats.total_overall;
        stats.lifetime_kernel += stats.total_raytrace;
        stats.num_iterations++;
        stats.denoise = m_denoiser.execution_time();
        stats.denoise_median = m_denoiser_median.execution_time();
    }

    void ray_tracer::add_mesh(mesh& mesh)
    {
        m_materials.push_back(mesh.material());
        m_bvh.add_mesh(mesh, m_materials.size() - 1);
    }

    void ray_tracer::set_directional_light(directional_light& light)
    {
        m_directional_light = light;
    }

    void ray_tracer::generate_random_dirs()
    {
        static auto rand_float = []() {
            return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        };
        static auto random_in_unit_sphere = [&]() {
            glm::vec3 p{0.0f};
            do
            {
                p.x = 2.0f * rand_float() - 1.0f;
                p.y = 2.0f * rand_float() - 1.0f;
                p.z = 2.0f * rand_float() - 1.0f;
                /*p = 2.0f * glm::vec3(rand_float(), rand_float(), rand_float())
                   - glm::vec3(1.0f);*/
            } while (length(p) >= 1.0f);
            return p;
        };

        static auto random_in_cosine_hemisphere = [&]() {
            float u1 = rand_float();
            float u2 = rand_float();

            const float r = std::sqrt(u1);
            const float theta = 2 * glm::pi<float>() * u2;

            const float x = r * std::cos(theta);
            const float y = r * std::sin(theta);

            return glm::vec3(x, y, std::sqrt(std::max(0.0f, 1 - u1)));
        };
        for (auto& dir : m_random_dirs)
        {
            dir = random_in_unit_sphere();
            // dir = random_in_cosine_hemisphere();
        }
        /*for (int i = 0; i < m_random_dirs.size(); ++i)
        {
            const auto& v = random_in_unit_sphere();
            m_random_dirs[i].x = v.x;
            m_random_dirs[i].y = v.y;
            m_random_dirs[i].z = v.z;
        }*/
    }

    void ray_tracer::update_random_dirs()
    {
        static int step = 4;
        static int offset = 0;
        static auto rand_float = []() {
            return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        };
        static auto random_in_unit_sphere = [&]() {
            glm::vec3 p{0.0f};
            do
            {
                p.x = 2.0f * rand_float() - 1.0f;
                p.y = 2.0f * rand_float() - 1.0f;
                p.z = 2.0f * rand_float() - 1.0f;
                /*p = 2.0f * glm::vec3(rand_float(), rand_float(), rand_float())
                   - glm::vec3(1.0f);*/
            } while (length(p) >= 1.0f);
            return p;
        };

        static auto random_in_cosine_hemisphere = [&]() {
            float u1 = rand_float();
            float u2 = rand_float();

            const float r = std::sqrt(u1);
            const float theta = 2 * glm::pi<float>() * u2;

            const float x = r * std::cos(theta);
            const float y = r * std::sin(theta);

            return glm::vec3(x, y, std::sqrt(std::max(0.0f, 1 - u1)));
        };

        glm::vec3 p{0.0f};
        for (int i = 0; i < m_random_dirs.size(); ++i)
        {
            m_random_dirs[i] = random_in_cosine_hemisphere();
        }
        offset = offset >= step - 1 ? 0 : offset + 1;
    }

} // namespace brtr
