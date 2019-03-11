#include "denoiser.hpp"
#include <chrono>

namespace brtr
{
    denoiser::denoiser(
        std::shared_ptr<platform> platform,
        std::shared_ptr<buffer> image_buffer,
        int w,
        int h,
        bool median = true)
        : m_gpgpu(platform->gpgpu()),
          m_image_buffer(image_buffer),
          m_result_data(w * h * 4),
          m_radius(4)
    {
        std::string kernel = median ? "denoise-median.cl" : "denoise.cl";
        m_denoise_kernel =
            m_gpgpu->load_kernel("../../src/gpgpu/opencl/" + kernel, "denoise");
        m_denoise_kernel->set_global_work_size(w, h);
        m_denoise_kernel->set_local_work_size();

        create_filter(2.0f);
        // m_filter.assign(49, 1.0f / 49);
        m_filter_buffer = m_gpgpu->create_buffer(
            buffer_access::read_only, sizeof(float), m_filter.size(), m_filter.data());
        m_result_buffer = m_gpgpu->create_buffer(
            buffer_access::write_only,
            m_image_buffer->stride,
            m_image_buffer->size,
            m_result_data.data());
    }

    void denoiser::run()
    {
        std::chrono::high_resolution_clock::time_point start =
            std::chrono::high_resolution_clock::now();
        m_denoise_kernel->set_kernel_arg(buffer_operation::none, 0, m_image_buffer);
        m_denoise_kernel->set_kernel_arg(buffer_operation::none, 1, m_result_buffer);
        m_denoise_kernel->set_kernel_arg(buffer_operation::write, 2, m_filter_buffer);
        auto work_size = m_denoise_kernel->get_local_work_size();
        m_denoise_kernel->set_kernel_arg(
            3,
            sizeof(uint8_t) * 4 * (work_size.x + (m_radius - 1) * 2) *
                (work_size.y + (m_radius - 1) * 2),
            nullptr);
        m_denoise_kernel->run();
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();

        m_execution_time = (end - start).count() / 1000000.0f;
    }

    void denoiser::create_filter(float strength)
    {
        int filterSize = (int)std::ceil(3.0f * strength);
        m_radius = filterSize + 1;
        m_filter.assign((filterSize * 2 + 1) * (filterSize * 2 + 1), 0.0f);

        float sum = 0.0f;
        for (int i = -filterSize; i < filterSize + 1; ++i)
        {
            for (int j = -filterSize; j < filterSize + 1; ++j)
            {
                float tmp = std::exp(
                    -(static_cast<float>(i * i + j * j) / (2 * strength * strength)));
                sum += tmp;
                int index = i + filterSize + (j + filterSize) * (filterSize * 2 + 1);
                m_filter[index] = tmp;
            }
        }
        for (int i = 0; i < m_filter.size(); ++i)
        {
            m_filter[i] = m_filter[i] / sum;
        }
    }
} // namespace brtr
