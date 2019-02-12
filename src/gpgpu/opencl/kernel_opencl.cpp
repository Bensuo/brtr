#include "kernel_opencl.hpp"
#include "buffer_opencl.hpp"
#include "camera.hpp"
#include "gpgpu/parameter.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
namespace brtr
{
    kernel_opencl::kernel_opencl(
        cl::Program& program,
        const std::string& kernel_name,
        gpgpu_opencl& platform,
        cl::Device& device,
        cl::CommandQueue& queue,
        cl::Context& context)
        : m_platform(platform), m_device(device), m_queue(queue), m_context(context)
    {
        cl_int err;
        m_kernel = cl::Kernel(program, kernel_name.c_str(), &err);
    }

    void kernel_opencl::set_kernel_arg(buffer_operation operation, int index, std::shared_ptr<buffer> buf)
    {
        std::shared_ptr<buffer_opencl> buffer =
            std::dynamic_pointer_cast<buffer_opencl>(buf);
        m_kernel.setArg(index, buffer->m_buffer);
        switch (operation)
        {
        case buffer_operation::write:
            m_buffers_to_write.push_back(buffer);

            break;
        case buffer_operation::read:
            m_buffers_to_read.push_back(buffer);

            break;
        }
    }

    void kernel_opencl::set_kernel_arg(buffer_operation operation, int index, image_buffer buf)
    {
        /*m_kernel.setArg(index, m_buffers[buf.id]);
        switch (operation)
        {
        case buffer_operation::write:
            m_queue.enqueueWriteImage(m_image_buffers[buf.id], CL_TRUE,);
            break;
        case buffer_operation::read:
            m_queue.enqueueReadBuffer(m_buffers[buf.id], CL_TRUE, 0,
        buf.size*buf.stride, buf.data); break;

        }*/
    }

    void kernel_opencl::set_global_work_size(size_t size)
    {
        m_global_range = cl::NDRange{size};
    }

    void kernel_opencl::set_global_work_size(size_t x, size_t y)
    {
        m_global_range = cl::NDRange{x, y};
    }

    void kernel_opencl::set_local_work_size()
    {
        size_t size;
        m_kernel.getWorkGroupInfo(m_device, CL_KERNEL_WORK_GROUP_SIZE, &size);
        size_t target_x = static_cast<size_t>(sqrt(size));
        size_t target_y = static_cast<size_t>(sqrt(size));
        while (m_global_range[0] % target_x != 0)
        {
            target_x = target_x / 2;
        }
        while (m_global_range[1] % target_y != 0)
        {
            target_y = target_y / 2;
        }
        m_local_range = cl::NDRange{target_x, target_y};
    }

    void kernel_opencl::set_local_work_size(size_t x, size_t y)
    {
        m_local_range = cl::NDRange{x, y};
    }

    void kernel_opencl::run()
    {
        for (const auto& buffer : m_buffers_to_write)
        {
            m_queue.enqueueWriteBuffer(
                buffer->m_buffer,
                CL_TRUE,
                0,
                buffer->size * buffer->stride,
                buffer->data);
        }
        if (m_local_range[0] == 0)
        {
            m_queue.enqueueNDRangeKernel(
                m_kernel, cl::NullRange, m_global_range, cl::NullRange, NULL, &m_event);
        }
        else
        {
            m_queue.enqueueNDRangeKernel(
                m_kernel, cl::NullRange, m_global_range, m_local_range, NULL, &m_event);
        }

        for (const auto& buffer : m_buffers_to_read)
        {
            m_queue.enqueueReadBuffer(
                buffer->m_buffer,
                CL_TRUE,
                0,
                buffer->size * buffer->stride,
                buffer->data);
        }
        m_buffers_to_write.clear();
        m_buffers_to_read.clear();
    }

    void kernel_opencl::set_kernel_arg(int index, int val)
    {
        m_kernel.setArg(index, val);
    }

    void kernel_opencl::set_kernel_arg(int index, float val)
    {
        m_kernel.setArg(index, val);
    }

    void kernel_opencl::set_kernel_arg(int index, size_t size, void* arg_ptr)
    {
        m_kernel.setArg(index, size, arg_ptr);
    }

    glm::ivec2 kernel_opencl::get_local_work_size()
    {
        return {m_local_range[0], m_local_range[1]};
    }
} // namespace brtr
