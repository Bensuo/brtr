#pragma once 
#include "gpgpu_platform.hpp"

namespace brtr
{
    class platform
    {
    public:
		platform();
		std::shared_ptr<gpgpu_platform> gpgpu();
    private:
		std::shared_ptr<gpgpu_platform> m_gpgpu;
    };
}
