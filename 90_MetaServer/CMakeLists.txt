#
# Copyright (c) 2008-2021 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

if (NOT URHO3D_PHYSICS)
    return ()
endif ()

# Define target name
set (TARGET_NAME 90_MetaServer)


#修复mac 无法找到openssl 库
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(OpenSSL REQUIRED)
    include_directories(${OPENSSL_INCLUDE_DIR})
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    message(STATUS "current platform: Windows")
elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    message(STATUS "current platform: FreeBSD")
else()
    include_directories(/usr/local/opt/openssl@1.1/include)
    link_directories(/usr/local/opt/openssl@1.1/lib)


    set(OPENSSL_INCLUDE_DIR /usr/local/opt/openssl@1.1/include)
    set(CRYPTO_LIB /usr/local/opt/openssl@1.1/lib)
    set(OPENSSL_LIBRARIES crypto)
endif()


## add for beast
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set_property (GLOBAL PROPERTY USE_FOLDERS ON)
if (MSVC)
set (CMAKE_VERBOSE_MAKEFILE FALSE)

add_definitions (-D_WIN32_WINNT=0x0601)
add_definitions (-D_SCL_SECURE_NO_WARNINGS=1)
add_definitions (-D_CRT_SECURE_NO_WARNINGS=1)

add_compile_options(
/bigobj       # large object file format
/permissive-  # strict C++
#/wd4503      # decorated name length exceeded, name was truncated
/W4           # enable all warnings
/MP           # Multi-processor compilation
)

set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_STATIC_RUNTIME ON)

set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ob2 /Oi /Ot /GL /MT")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Oi /Ot /MT")

set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")

# for RelWithDebInfo builds, disable incremental linking
# since CMake sets it ON by default for that build type and it
# causes warnings
#
string (REPLACE "/INCREMENTAL" "/INCREMENTAL:NO" replacement_flags
${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})
set (CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO ${replacement_flags})

else()
set (THREADS_PREFER_PTHREAD_FLAG ON)
find_package (Threads)

set( CMAKE_CXX_FLAGS
"${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -Wpedantic -Wno-unused-parameter")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wrange-loop-analysis")
endif ()
endif()

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)



# Must come before Boost includes, otherwise the
# IDE sees the wrong file due to boost/ symlinks.
include_directories (include)
#-------------------------------------------------------------------------------
#
# Boost
#
if(ANDROID)
    get_filename_component (BOOST_ROOT ${PROJECT_SOURCE_DIR}/../../../../dev_tools/boost/1.70.0 ABSOLUTE)
    include_directories (${BOOST_ROOT})
    link_directories(${PROJECT_SOURCE_DIR}/../../../../dev_tools/boost/1.70.0/stage_android/debug/armeabi-v7a)
elseif(APPLE)
    get_filename_component (BOOST_ROOT ${PROJECT_SOURCE_DIR}/../../../../dev_tools/boost/1.70.0 ABSOLUTE)
    include_directories (${BOOST_ROOT})
    link_directories(${BOOST_ROOT}/stage_osx/lib)
elseif(UNIX)
    get_filename_component (BOOST_ROOT ${PROJECT_SOURCE_DIR}/../../../../dev_tools/boost/1.70.0 ABSOLUTE)
    include_directories (${BOOST_ROOT})
    link_directories(${PROJECT_SOURCE_DIR}/../../../../dev_tools/boost/1.70.0/stage_linux/lib)
else()
    message(FATAL_ERROR "未知的操作系统: ${CMAKE_SYSTEM}")
endif()

# VFALCO I want static but "b2 stage" builds a minimal set which excludes static
add_definitions (-DBOOST_ALL_STATIC_LINK=1)

add_definitions (-DBOOST_ASIO_NO_DEPRECATED=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_BIND=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_REGEX=1)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1)


include_directories(${PROJECT_SOURCE_DIR}/../../../moneytalks/include)
include_directories(${PROJECT_SOURCE_DIR}/../../android/launcher-app/src/main/cpp/include)

#message(${CMAKE_SOURCE_DIR})


# add by copyleft
file(GLOB_RECURSE project_headers *.h *.hpp)
file(GLOB_RECURSE project_c *.c)
file(GLOB_RECURSE project_cpps *.cpp *.cc)
file(GLOB_RECURSE project_res *.rc *.ico)
set(all_files ${project_headers} ${project_cpps} ${project_c} ${project_res})

# Define source files
define_source_files (EXTRA_H_FILES ${COMMON_SAMPLE_H_FILES} ${all_files})

# Setup target with resource copying
setup_main_executable ()

# Setup test cases
setup_test ()

# add for facepower
if(ANDROID_TOOLCHAIN)
    link_directories(${PROJECT_SOURCE_DIR}/../../android/launcher-app/libs/${ANDROID_ABI})

    add_library(facepower SHARED IMPORTED)
    set_target_properties(facepower PROPERTIES IMPORTED_LOCATION
        ${PROJECT_SOURCE_DIR}/../../android/launcher-app/libs/${ANDROID_ABI}/libfacepower.so)
    target_link_libraries(${TARGET_NAME} facepower)
elseif(APPLE)
    link_directories(/Users/leeco/breeze/prj_build/moneytalks/output/libs/Debug)

    target_link_libraries(${TARGET_NAME} boost_system ${OPENSSL_LIBRARIES} /Users/leeco/breeze/prj_build/moneytalks/output/libs/Debug/libfacepower.dylib)
elseif(UNIX)
    message(STATUS "当前平台:${CMAKE_SYSTEM} Linux 仅编译服务器端程序 无需连接facepower....")
else()
    message(FATAL_ERROR "未知的操作系统: ${CMAKE_SYSTEM}")
endif()
