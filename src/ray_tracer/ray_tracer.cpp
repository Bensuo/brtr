#include "ray_tracer.hpp"
#include "brtr_platform.hpp"
#include "buffer.hpp"
#include "gpgpu/opencl/gpgpu_opencl.hpp"
#include "gpgpu_platform.hpp"
#include "kernel.hpp"
#include "light.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>

namespace brtr
{
    ray_tracer::ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h, int samples_per_pixel)
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
          m_samples_per_pixel(samples_per_pixel),
          m_bvh(platform),
          m_camera(cam),
          m_denoiser(platform, m_indirect_buffer, w, h, false),
          m_denoiser_median(platform, m_indirect_buffer, w, h, true)
    {
        std::cout << "Ray_tracer constructed\n";
        m_raytrace_kernel = m_gpgpu->load_kernel(
            "../../src/gpgpu/opencl/raytracing.cl", "ray_trace");
        m_combine_kernel =
            m_gpgpu->load_kernel("../../src/gpgpu/opencl/combine.cl", "combine");
        m_raytrace_kernel->set_global_work_size(m_width, m_height);
        m_raytrace_kernel->set_local_work_size();
        m_combine_kernel->set_global_work_size(m_width, m_height);
        m_combine_kernel->set_local_work_size();
        m_random_dirs.resize(1 << 16);
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
        std::chrono::high_resolution_clock::time_point start =
            std::chrono::high_resolution_clock::now();
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
        static auto rng = std::default_random_engine{};
        update_random_dirs();
        std::shuffle(m_random_dirs.begin(), m_random_dirs.end(), rng);
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 6, m_random_buffer);
        m_lights_buffer = m_gpgpu->create_buffer(
            buffer_access::read_only,
            sizeof(point_light),
            m_lights.size(),
            m_lights.data());
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 7, m_lights_buffer);
        m_raytrace_kernel->set_kernel_arg(buffer_operation::none, 8, m_indirect_buffer);
        m_raytrace_kernel->run();

        // m_denoiser.run();
        // m_denoiser_median.run();
        m_combine_kernel->set_kernel_arg(buffer_operation::none, 0, m_direct_buffer);
        m_combine_kernel->set_kernel_arg(buffer_operation::none, 1, m_indirect_buffer);
        m_combine_kernel->set_kernel_arg(buffer_operation::read, 2, m_result_buffer);
        m_combine_kernel->run();
        m_gpgpu->run();
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();
        stats.total_overall = (end - start).count() / 1000000.0f;
        stats.total_bvh = m_bvh.get_last_execution_time_gpu() +
                          m_bvh.get_last_construction_time();
        stats.bvh_construction = m_bvh.get_last_construction_time();
        stats.bvh_gpu = m_bvh.get_last_execution_time_gpu();
        stats.total_raytrace = m_raytrace_kernel->get_last_execution_time();
        stats.lifetime_overall += stats.total_overall;
        stats.lifetime_kernel += stats.total_raytrace;
        stats.lifetime_bvh_gpu += stats.bvh_gpu;
        stats.lifetime_bvh_construction += stats.bvh_construction;
        stats.num_iterations++;
        stats.denoise = m_denoiser.execution_time();
    }

    void ray_tracer::add_mesh(mesh& mesh)
    {
        m_materials.push_back(mesh.material());
        m_bvh.add_mesh(mesh, m_materials.size() - 1);
    }

    void ray_tracer::add_light(point_light& light)
    {
        m_lights.push_back(light);
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

        for (auto& dir : m_random_dirs)
        {
            dir = random_in_unit_sphere();
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
        glm::vec3 p{0.0f};
        for (int i = 0; i < m_random_dirs.size() / step; ++i)
        {
            p.x = 2.0f * rand_float() - 1.0f;
            p.y = 2.0f * rand_float() - 1.0f;
            p.z = 2.0f * rand_float() - 1.0f;
            glm::normalize(p);
            m_random_dirs[i * step + offset] = p;
        }
        offset = offset >= step - 1 ? 0 : offset + 1;
    }

} // namespace brtr
