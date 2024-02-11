cmake_minimum_required(VERSION 3.28)

project(Masterthesis)

if(CMAKE_COMPILER_IS_GNUC OR CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(-Wall -Wextra #-Werror 
   -pedantic #-pedantic-errors 
   -ggdb)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
  add_definitions(#-Weffc++
    -std=c++11)
endif()

find_package(OpenCL REQUIRED)
set(OclWrapper_HDRS
  ../OpenCL-Wrapper/Code/inc/ocl_buffer.h
  ../OpenCL-Wrapper/Code/inc/ocl_context.h
  ../OpenCL-Wrapper/Code/inc/ocl_device.h
  ../OpenCL-Wrapper/Code/inc/ocl_device_type.h
  ../OpenCL-Wrapper/Code/inc/ocl_event.h
  ../OpenCL-Wrapper/Code/inc/ocl_event_list.h
  ../OpenCL-Wrapper/Code/inc/ocl_image.h
  ../OpenCL-Wrapper/Code/inc/ocl_kernel.h
  ../OpenCL-Wrapper/Code/inc/ocl_memory.h
  ../OpenCL-Wrapper/Code/inc/ocl_platform.h
  ../OpenCL-Wrapper/Code/inc/ocl_program.h
  ../OpenCL-Wrapper/Code/inc/ocl_query.h
  ../OpenCL-Wrapper/Code/inc/ocl_queue.h
  ../OpenCL-Wrapper/Code/inc/ocl_sampler.h
  ../OpenCL-Wrapper/Code/inc/ocl_wrapper.h
  ../OpenCL-Wrapper/Code/inc/utl_args.h
  ../OpenCL-Wrapper/Code/inc/utl_assert.h
  ../OpenCL-Wrapper/Code/inc/utl_dim.h
  ../OpenCL-Wrapper/Code/inc/utl_flags.h
  ../OpenCL-Wrapper/Code/inc/utl_matrix.h
  ../OpenCL-Wrapper/Code/inc/utl_profile_pass.h
  ../OpenCL-Wrapper/Code/inc/utl_profile_pass_manager.h
  ../OpenCL-Wrapper/Code/inc/utl_storage.h
  ../OpenCL-Wrapper/Code/inc/utl_stream.h
  ../OpenCL-Wrapper/Code/inc/utl_timer.h
  ../OpenCL-Wrapper/Code/inc/utl_type.h
  ../OpenCL-Wrapper/Code/inc/utl_utils.h
)

set(OclWrapper_SRCS
  ../OpenCL-Wrapper/Code/src/ocl_buffer.cpp
  ../OpenCL-Wrapper/Code/src/ocl_context.cpp
  ../OpenCL-Wrapper/Code/src/ocl_device.cpp
  ../OpenCL-Wrapper/Code/src/ocl_device_type.cpp
  ../OpenCL-Wrapper/Code/src/ocl_event.cpp
  ../OpenCL-Wrapper/Code/src/ocl_event_list.cpp
  ../OpenCL-Wrapper/Code/src/ocl_image.cpp
  ../OpenCL-Wrapper/Code/src/ocl_kernel.cpp
  ../OpenCL-Wrapper/Code/src/ocl_memory.cpp
  ../OpenCL-Wrapper/Code/src/ocl_platform.cpp
  ../OpenCL-Wrapper/Code/src/ocl_program.cpp
  ../OpenCL-Wrapper/Code/src/ocl_query.cpp
  ../OpenCL-Wrapper/Code/src/ocl_queue.cpp
  ../OpenCL-Wrapper/Code/src/ocl_sampler.cpp
  ../OpenCL-Wrapper/Code/src/utl_args.cpp
  ../OpenCL-Wrapper/Code/src/utl_dim.cpp
  ../OpenCL-Wrapper/Code/src/utl_storage.cpp
  ../OpenCL-Wrapper/Code/src/utl_stream.cpp
  ../OpenCL-Wrapper/Code/src/utl_timer.cpp
  ../OpenCL-Wrapper/Code/src/utl_type.cpp
)

add_library(OclWrapper STATIC ${OclWrapper_HDRS} ${OclWrapper_SRCS})
target_compile_features(OclWrapper PUBLIC cxx_std_20)
target_include_directories(OclWrapper PUBLIC ${OpenCL_INCLUDE_DIRS} ../OpenCL-Wrapper/Code/inc)
target_link_libraries(OclWrapper PUBLIC OpenCL::OpenCL)

add_executable(volkov_2008 OtherWork/volkov_2008.cpp)
target_link_libraries(volkov_2008 OclWrapper)

add_executable(torres_2011 OtherWork/torres_2011.cpp)
target_link_libraries(torres_2011 OclWrapper)

add_executable(matsumoto_2012 
  OtherWork/Matsumoto2012/BasicTemplate.hpp
  OtherWork/Matsumoto2012/DoubleBufferingTemplate.hpp
  OtherWork/Matsumoto2012/KernelTemplate.hpp
  OtherWork/Matsumoto2012/PipeliningTemplate.hpp
  OtherWork/Matsumoto2012/utl_matrix2.hpp
  OtherWork/Matsumoto2012/matsumoto_2012.cpp
)
target_link_libraries(matsumoto_2012 OclWrapper)

add_executable(djinevski_2013 OtherWork/djinevski_2013.cpp)
target_link_libraries(djinevski_2013 OclWrapper)

find_package(MPI REQUIRED)
find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
file(GLOB MUESLI_SRCS "OtherWork/Muesli/*.cpp")
file(GLOB MUESLI_HDRS "OtherWork/Muesli/*.h")
find_path(MUESLI_INCLUDE_DIR Muesli.h HINTS OtherWork/Muesli/)
add_executable(gemm_in_muesli ${MUESLI_HDRS} ${MUESLI_SRCS} OtherWork/gemm_in_muesli.cpp)
#target_include_directories(gemm_in_muesli PRIVATE ${MPI_INCLUDE_PATH} ${MUESLI_INCLUDE_DIR})
include_directories(${MUESLI_INCLUDE_DIR})
target_link_libraries(gemm_in_muesli MPI::MPI_CXX OclWrapper)

add_executable(kernel_parameters Microbenchmarks/KernelParameters.cpp)
target_link_libraries(kernel_parameters OclWrapper)

add_executable(fastest_dot_product Microbenchmarks/FastestDotProduct.cpp)
target_link_libraries(fastest_dot_product OclWrapper)

add_executable(kernel_runner Kernels/KernelRunner.cpp)
target_link_libraries(kernel_runner OclWrapper)
