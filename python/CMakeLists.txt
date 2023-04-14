project(python_ext)

find_package(Python 3.9 COMPONENTS Interpreter Development.Module REQUIRED)
set(CMAKE_CXX_STANDARD 20)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

# Create CMake targets for all Python components needed by nanobind
if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.26)
    find_package(Python 3.8 COMPONENTS Interpreter Development.Module Development.SABIModule REQUIRED)
else ()
    find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
endif ()

# Detect the installed nanobind package and import it into CMake
execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import nanobind; print(nanobind.cmake_dir())"
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)

nanobind_add_module(
        _py_adverbs
        NOSTRIP
        NB_STATIC
        _py_adverbs.cpp
)
target_link_libraries(
        _py_adverbs
        PUBLIC
        adverbs
)
