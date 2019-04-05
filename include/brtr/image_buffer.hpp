#pragma once
#include "buffer.hpp"

namespace brtr
{

	struct image_buffer
	{
		void* data;
		size_t id;
		size_t width;
		size_t height;
		image_buffer(void* data_ptr, size_t id, size_t width, size_t height)
			:data(data_ptr), id(id), width(width), height(height)
		{
			
		}
	};
}