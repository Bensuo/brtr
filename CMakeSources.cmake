set(BRTR_CPP
    src/ray_tracer/ray_tracer.cpp
    src/ray_tracer/mesh.cpp
    src/gpgpu/opencl/gpgpu_opencl.cpp
    src/gpgpu/opencl/kernel_opencl.cpp
    src/gpgpu/gpgpu_platform.cpp
    src/bvh/bvh.cpp
    src/denoiser/denoiser.cpp
    src/brtr_platform.cpp
    src/camera.cpp
)

set(BRTR_HPP

    src/gpgpu/opencl/gpgpu_opencl.hpp
    src/gpgpu/opencl/kernel_opencl.hpp
    src/gpgpu/opencl/buffer_opencl.hpp

)

set(BRTR_INC
    include/brtr/brtr.hpp
    include/brtr/ray_tracer.hpp
    include/brtr/gpgpu_platform.hpp
    include/brtr/buffer.hpp
    include/brtr/image_buffer.hpp
    include/brtr/mesh.hpp
    include/brtr/kernel.hpp
    include/brtr/bvh.hpp
    include/brtr/brtr_platform.hpp
    include/brtr/camera.hpp
    include/brtr/light.hpp
    include/brtr/denoiser.hpp
)