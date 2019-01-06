#pragma once

namespace brtr
{
	enum class buffer_access
	{
		read_only,
		write_only,
		read_write
	};
	enum class buffer_operation
	{
		write,
		read
	};
	class buffer
	{
	public:
		void* data;
		size_t stride;
		size_t size;
		buffer()
		{
			
		}
		buffer(void* data_ptr, size_t stride, size_t size)
			:data(data_ptr), stride(stride), size(size)
		{
			
		}
		virtual ~buffer()
		{
			
		}
	};
}