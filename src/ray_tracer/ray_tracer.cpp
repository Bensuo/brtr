#include "ray_tracer.hpp"
#include "brtr_platform.hpp"
#include "buffer.hpp"
#include "gpgpu/opencl/gpgpu_opencl.hpp"
#include "gpgpu_platform.hpp"
#include "kernel.hpp"
#include <chrono>
#include <iostream>

namespace brtr
{
    ray_tracer::ray_tracer(std::shared_ptr<platform> platform, camera& cam, int w, int h, int samples_per_pixel)
        : m_gpgpu(platform->gpgpu()),
          m_image_data(w * h * 4, 0),
          m_result_buffer(m_gpgpu->create_buffer(
              buffer_access::write_only, sizeof(cl_uchar4), w * h, m_image_data.data())),
          m_width(w),
          m_height(h),
          m_samples_per_pixel(samples_per_pixel),
          m_bvh(platform),
          m_camera(cam)
    {
        std::cout << "Ray_tracer constructed\n";
        m_raytrace_kernel = m_gpgpu->load_kernel(
            "../../src/gpgpu/opencl/raytracing.cl", "ray_trace");
        m_raytrace_kernel->set_global_work_size(m_width, m_height);
        m_raytrace_kernel->set_local_work_size();
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
        m_raytrace_kernel->set_kernel_arg(buffer_operation::read, 4, m_result_buffer);
        camera_gpu cam = m_camera.gpu();
        std::shared_ptr<buffer> camera_buffer = m_gpgpu->create_buffer(
            buffer_access::read_only, sizeof(camera_gpu), 1, &cam);
        m_raytrace_kernel->set_kernel_arg(buffer_operation::write, 5, camera_buffer);
        m_raytrace_kernel->run();
        std::chrono::high_resolution_clock::time_point bvh =
            std::chrono::high_resolution_clock::now();
        m_gpgpu->run();
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();
        stats.total_overall = (end - start).count() / 1000000.0f;
        stats.total_bvh = m_bvh.get_last_execution_time_gpu() +
                          m_bvh.get_last_construction_time();
        stats.bvh_construction = m_bvh.get_last_construction_time();
        stats.bvh_gpu = m_bvh.get_last_execution_time_gpu();
        stats.total_raytrace = m_raytrace_kernel->get_last_execution_time();
    }

    void ray_tracer::add_mesh(mesh& mesh)
    {
        m_materials.push_back(mesh.material());
        m_bvh.add_mesh(mesh, m_materials.size() - 1);
    }
} // namespace brtr
