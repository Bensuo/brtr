#include "brtr_platform.hpp"
#include "gpgpu/opencl/gpgpu_opencl.hpp"

namespace brtr
{
	platform::platform()
	{
		m_gpgpu = std::make_shared<gpgpu_opencl>();
	}

	std::shared_ptr<gpgpu_platform> platform::gpgpu()
	{
		return m_gpgpu;
	}
}
