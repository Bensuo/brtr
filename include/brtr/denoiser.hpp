#pragma once
#include "brtr_platform.hpp"
#include "gpgpu_platform.hpp"
#include <vector>

namespace brtr
{
    class denoiser
    {
    public:
        denoiser(std::shared_ptr<platform> platform, std::shared_ptr<buffer> image_buffer, int w, int h, bool median);
        void run();
        std::shared_ptr<buffer> result_buffer() const
        {
            return m_result_buffer;
        }
        float execution_time()
        {
            return m_denoise_kernel->get_last_execution_time();
        }
    private:
        void create_filter(float strength);
        std::shared_ptr<gpgpu_platform> m_gpgpu;
        std::unique_ptr<kernel> m_denoise_kernel;
        std::shared_ptr<buffer> m_image_buffer;
        std::shared_ptr<buffer> m_result_buffer;
        std::shared_ptr<buffer> m_filter_buffer;
        std::vector<float> m_filter;
        std::vector<uint8_t> m_result_data;
        float m_radius;
        float m_execution_time;
    };
} // namespace brtr
