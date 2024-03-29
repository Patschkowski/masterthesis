cmake_minimum_required(VERSION 3.28)

project(OpenCL-Wrapper)

find_package(OpenCL REQUIRED)

set(OclWrapper_HDRS
  Code/inc/ocl_buffer.h
  Code/inc/ocl_context.h
  Code/inc/ocl_device.h
  Code/inc/ocl_device_type.h
  Code/inc/ocl_event.h
  Code/inc/ocl_event_list.h
  Code/inc/ocl_image.h
  Code/inc/ocl_kernel.h
  Code/inc/ocl_memory.h
  Code/inc/ocl_platform.h
  Code/inc/ocl_program.h
  Code/inc/ocl_query.h
  Code/inc/ocl_queue.h
  Code/inc/ocl_sampler.h
  Code/inc/ocl_wrapper.h
  Code/inc/utl_args.h
  Code/inc/utl_assert.h
  Code/inc/utl_dim.h
  Code/inc/utl_flags.h
  Code/inc/utl_matrix.h
  Code/inc/utl_profile_pass.h
  Code/inc/utl_profile_pass_manager.h
  Code/inc/utl_storage.h
  Code/inc/utl_stream.h
  Code/inc/utl_timer.h
  Code/inc/utl_type.h
  Code/inc/utl_utils.h
)

set(OclWrapper_SRCS
  Code/src/ocl_buffer.cpp
  Code/src/ocl_context.cpp
  Code/src/ocl_device.cpp
  Code/src/ocl_device_type.cpp
  Code/src/ocl_event.cpp
  Code/src/ocl_event_list.cpp
  Code/src/ocl_image.cpp
  Code/src/ocl_kernel.cpp
  Code/src/ocl_memory.cpp
  Code/src/ocl_platform.cpp
  Code/src/ocl_program.cpp
  Code/src/ocl_query.cpp
  Code/src/ocl_queue.cpp
  Code/src/ocl_sampler.cpp
  Code/src/utl_args.cpp
  Code/src/utl_dim.cpp
  Code/src/utl_storage.cpp
  Code/src/utl_stream.cpp
  Code/src/utl_timer.cpp
  Code/src/utl_type.cpp
)

add_library(OclWrapper STATIC ${OclWrapper_HDRS} ${OclWrapper_SRCS})
target_compile_features(OclWrapper PUBLIC cxx_std_17)

add_executable(platform Tutorial/1.platform/platform.cpp)
target_link_libraries(platform OclWrapper OpenCL::OpenCL)

add_executable(context Tutorial/2.context/context.cpp)
target_link_libraries(context OclWrapper OpenCL::OpenCL)

add_executable(queue Tutorial/3.queue/queue.cpp)
target_link_libraries(queue OclWrapper OpenCL::OpenCL)

add_executable(program Tutorial/4.program/program.cpp)
target_link_libraries(program OclWrapper OpenCL::OpenCL)

add_executable(buffer Tutorial/5.buffer/buffer.cpp)
target_link_libraries(buffer OclWrapper OpenCL::OpenCL)

add_executable(kernel Tutorial/6.kernel/kernel.cpp)
target_link_libraries(kernel OclWrapper OpenCL::OpenCL)

add_executable(events Tutorial/7.events/events.cpp)
target_link_libraries(events OclWrapper OpenCL::OpenCL)

add_executable(matrix Tutorial/8.matrix/matrix.cpp)
target_link_libraries(matrix OclWrapper OpenCL::OpenCL)

add_executable(minimum Tutorial/9.minimum/minimum.cpp)
target_link_libraries(minimum OclWrapper OpenCL::OpenCL)

add_executable(image Tutorial/10.image/image.cpp)
target_link_libraries(image OclWrapper OpenCL::OpenCL)
