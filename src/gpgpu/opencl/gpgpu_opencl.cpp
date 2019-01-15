#include "gpgpu_opencl.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include "kernel_opencl.hpp"
#include "buffer_opencl.hpp"

#define CL_TARGET_OPENCL_VERSION 120
namespace brtr
{
	gpgpu_opencl::gpgpu_opencl()
	{
		cl::Platform::get(&m_platforms);
		if (m_platforms.size() == 0)
		{
			throw std::runtime_error("Error: No OpenCL Platforms Found.");
		}
		m_platform = m_platforms[0];
		for (const auto& p : m_platforms)
		{
			std::string s;
			p.getInfo(CL_PLATFORM_VENDOR, &s);
			if (std::string::npos != s.find("NVIDIA"))
			{
				m_platform = p;
				break;
			}
		}

		std::vector<cl::Device> devices;
		m_platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
		if (devices.size() == 0)
		{
			throw std::runtime_error("Error: No OpenCL devices found");
		}
		m_device = devices[0];

		m_context = cl::Context(m_device);
		m_queue = cl::CommandQueue(m_context, m_device, CL_QUEUE_PROFILING_ENABLE);


	}


	void gpgpu_opencl::run()
	{

		m_queue.flush();
		m_queue.finish();
	}



	std::shared_ptr<buffer> gpgpu_opencl::create_buffer(buffer_access access, size_t stride, size_t size, void* data_ptr)
	{
		return std::make_shared<buffer_opencl>(m_context, buffer_flags(access), data_ptr, stride, size);
	}

	image_buffer gpgpu_opencl::create_image_buffer(buffer_access access, size_t width, size_t height, void* data_ptr)
	{
		cl::ImageFormat format{ CL_RGBA, CL_UNORM_INT8 };
		m_image_buffers.emplace_back(m_context, buffer_flags(access), format, width, height);
		return { data_ptr, m_image_buffers.size() - 1, width, height };
	}

	std::unique_ptr<kernel> gpgpu_opencl::load_kernel(const std::string& path, const std::string& kernel_name)
	{
		if (m_programs.find(path) == m_programs.end())
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				throw std::runtime_error("Error: Could not open Kernel File at " + path);
			}
			std::ostringstream contents;
			contents << file.rdbuf();
			file.close();
			std::string src = contents.str();

			cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));
			auto program = cl::Program(m_context, sources);

			program.build();

			std::cout << "Build Log: " << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_device) << std::endl;
			m_programs[path] = program;
		}
		return std::make_unique<kernel_opencl>(m_programs[path], kernel_name, *this, m_device, m_queue, m_context);
	}


	cl_mem_flags gpgpu_opencl::buffer_flags(buffer_access access)
	{
		switch (access)
		{
		case buffer_access::read_only:
			return CL_MEM_READ_ONLY;
		case buffer_access::write_only:
			return CL_MEM_WRITE_ONLY;
		case buffer_access::read_write:
			return CL_MEM_READ_WRITE;
		default:
			return CL_MEM_READ_WRITE;
		}
	}

}
