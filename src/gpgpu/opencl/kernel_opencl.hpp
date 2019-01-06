#pragma once

#include "kernel.hpp"
#include <CL/cl.hpp>
#include "gpgpu_opencl.hpp"
#include "buffer_opencl.hpp"

namespace brtr
{
	class kernel_opencl : public kernel
	{
	public:
		kernel_opencl(
			cl::Program& program,
			const std::string& kernel_name,
			gpgpu_opencl& platform,
			cl::Device& device,
			cl::CommandQueue& queue,
			cl::Context& context);
		void set_kernel_arg(buffer_operation operation, int index, std::shared_ptr<buffer> buf) override;
		void set_kernel_arg(buffer_operation operation, int index, image_buffer buf) override;
		void set_global_work_size(size_t size) override;
		void set_global_work_size(size_t x, size_t y) override;
		void set_local_work_size() override;
		void set_local_work_size(size_t x, size_t y) override;

		void run() override;
		void set_kernel_arg(int index, int val) override;
		void set_kernel_arg(int index, float val) override;
		const cl::Kernel& cl_kernel() const
		{
			return m_kernel;
		}
	private:
		gpgpu_opencl& m_platform;
		cl::Device m_device;
		cl::Kernel m_kernel;
		cl::CommandQueue m_queue;
		cl::Context m_context;
		cl::NDRange m_global_range;
		cl::NDRange m_local_range;
		std::vector<std::shared_ptr<buffer_opencl>> m_buffers_to_write;
		std::vector<std::shared_ptr<buffer_opencl>> m_buffers_to_read;
	};
}