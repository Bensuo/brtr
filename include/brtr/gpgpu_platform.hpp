#pragma once
#include "buffer.hpp"
#include "image_buffer.hpp"
#include "kernel.hpp"
#include <string>

namespace brtr
{
	class gpgpu_platform
	{
	public:
		virtual ~gpgpu_platform();
		virtual std::unique_ptr<kernel> load_kernel(const std::string& path, const std::string& kernel_name) = 0;
		virtual std::shared_ptr<buffer> create_buffer(buffer_access access, size_t stride, size_t size, void* data_ptr) = 0;
		virtual void run() = 0;

	};
}
