#pragma once
#include "buffer.hpp"
#include <CL/cl.hpp>

namespace brtr
{
	class buffer_opencl : public buffer
	{
		
	public:
		cl::Buffer m_buffer;
		buffer_opencl(cl::Context context, cl_mem_flags flags, void* data_ptr, size_t stride, size_t size)
			: buffer(data_ptr, stride, size),
		m_buffer(context, flags, size *stride)
		{
			
		}
	};
}
