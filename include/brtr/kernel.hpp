#pragma once

#include <any>
#include <memory>

namespace brtr
{
    class parameter;
    struct buffer;
    enum class buffer_operation;
    struct image_buffer;

    class kernel
    {
    public:
        virtual ~kernel() = default;
        virtual void run() = 0;
        virtual void set_kernel_arg(int index, int val) = 0;
        virtual void set_kernel_arg(int index, float val) = 0;

        virtual void set_kernel_arg(
            buffer_operation operation, int index, std::shared_ptr<buffer> buf) = 0;
        virtual void set_kernel_arg(buffer_operation operation, int index, image_buffer buf) = 0;
        virtual void set_global_work_size(size_t size) = 0;
        virtual void set_global_work_size(size_t x, size_t y) = 0;
        virtual void set_local_work_size() = 0;
        virtual void set_local_work_size(size_t x, size_t y) = 0;
        virtual void set_last_execution_time(float t) = 0;
        virtual float get_last_execution_time() = 0;
    };
} // namespace brtr
