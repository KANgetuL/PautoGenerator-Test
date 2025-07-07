# CMakeList.txt: PautoGenerator-Test 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("PautoGenerator-Test")

# 强制使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 源文件
set(SOURCES
    src/main.cpp
    src/Generator.cpp
    src/Generator.h
)

# 可执行文件。
add_executable(PautoGenerator-Test ${SOURCES})

# nlohmann/json
# 推荐直接下载单头文件 json.hpp 放在 src/include/json.hpp
# 或者用 vcpkg / conan 管理库，如果用 find_package 则需要 3.10+ 和 find_package(nlohmann_json CONFIG REQUIRED)

# 示例：把 include 加进去
target_include_directories(PautoGenerator-Test PRIVATE 
    src/include     #json.hpp
    src             #Generator.h
)


#if (CMAKE_VERSION VERSION_GREATER 3.12)
#  set_property(TARGET PautoGenerator-Test PROPERTY CXX_STANDARD 20)
#endif()

# TODO: 如有需要，请添加测试并安装目标。
