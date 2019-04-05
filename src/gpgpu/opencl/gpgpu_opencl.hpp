#pragma once

#include "gpgpu_platform.hpp"
#include <CL/cl.hpp>
#include <map>
#include <unordered_map>
#include <vector>

namespace brtr
{
    class gpgpu_opencl : public gpgpu_platform
    {
    public:
        gpgpu_opencl();
        void run() override;
        std::shared_ptr<buffer> create_buffer(
            buffer_access access, size_t stride, size_t size, void* data_ptr) override;
        std::unique_ptr<kernel> load_kernel(const std::string& path, const std::string& kernel_name) override;

    private:
        cl_mem_flags buffer_flags(buffer_access access);

        std::vector<cl::Platform> m_platforms;
        cl::Platform m_platform;
        cl::Device m_device;
        cl::CommandQueue m_queue;
        cl::Context m_context;
        std::unordered_map<std::string, cl::Program> m_programs;
        cl::Program m_program;
        // std::map<uint32_t, cl::Buffer> m_buffers;
        std::vector<cl::Image2D> m_image_buffers;
    };
} // namespace brtr
