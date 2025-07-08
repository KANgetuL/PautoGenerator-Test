# CMakeList.txt: PautoGenerator-Test 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)


# 启用 vcpkg 工具链
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()



# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PautoGenerator-Test")

# 强制使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#查找依赖库
# 使用 find_package 查找依赖
find_package(SndFile REQUIRED)
find_package(FFTW3 REQUIRED)
find_package(FFTW3f REQUIRED)
find_package(nlohmann_json 3.12.0 REQUIRED)

# 源文件
set(SOURCES
    src/main.cpp
    src/Generator.cpp
    src/Generator.h
    src/AudioProcessor.h
    src/AudioProcessor.cpp)

# 可执行文件。
add_executable(PautoGenerator-Test ${SOURCES})

# nlohmann/json
# 示例：把 include 加进去
target_include_directories(PautoGenerator-Test PRIVATE 
    src/include     #json.hpp
    src             #Generator.h
    ${SNDFILE_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}
)

# 链接库
target_link_libraries(PautoGenerator-Test PRIVATE
    SndFile::sndfile  # 使用 IMPORTED 目标
    FFTW3::fftw3      # 使用 IMPORTED 目标
    FFTW3f:fftw3f
    nlohmann_json::nlohmann_json
)

# 添加库文件目录
target_link_directories(PautoGenerator-Test PRIVATE
    ${SndFile_LIBRARY_DIRS}
    ${FFTW3_LIBRARY_DIRS}
)

# 确保链接器能找到库文件
if(WIN32)
    # 将库文件目录添加到链接器搜索路径
    link_directories(${SndFile_LIBRARY_DIRS} ${FFTW3_LIBRARY_DIRS})
    
    # 添加库文件
    target_link_libraries(PautoGenerator-Test PRIVATE
        ${SndFile_LIBRARIES}
        ${FFTW3_LIBRARIES}
        fftw3f.lib
    )
endif()


#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET PautoGenerator-Test PROPERTY CXX_STANDARD 20)
#endif()

# 打印库路径以便调试
message(STATUS "SndFile libraries: ${SndFile_LIBRARIES}")
message(STATUS "FFTW3 libraries: ${FFTW3_LIBRARIES}")
message(STATUS "FFTW3 library dirs: ${FFTW3_LIBRARY_DIRS}")

# TODO: 如有需要，请添加测试并安装目标。
