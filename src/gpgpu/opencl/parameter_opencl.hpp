#pragma once
#include "gpgpu/parameter.hpp"
#include "gpgpu/opencl/kernel_opencl.hpp"
namespace brtr
{
	template <typename T>
	class parameter_opencl : parameter_base<parameter_opencl<T>>
	{
	public:
		parameter_opencl(kernel_opencl& kernel, int index, T& value)
			: m_kernel(kernel), m_value(value)
		{
			
		}
	protected:
		void implementation() const
		{
			m_kernel.cl_kernel().setArg(m_index, m_value);
		}

		kernel_opencl& m_kernel;
		T& m_value;
		int m_index;
	};
}