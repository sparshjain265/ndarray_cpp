# ─── Minimum Version ─────────────────────────────────────────────────────────
# CMake 3.25 is required for two key features we rely on:
#   • The SYSTEM property on imported targets (silences warnings from
#     third-party headers without manual -isystem trickery).
#   • Improved generator-expression support used in per-config options.
cmake_minimum_required(VERSION 3.25)

# ─── Project Declaration ─────────────────────────────────────────────────────
# Declares the project name, version, the languages used (CXX = C++), and an
# optional human-readable description.  CMake sets PROJECT_NAME,
# PROJECT_VERSION, etc. from these fields.
project(cpp_eigen_opencv
    VERSION     0.1.0
    LANGUAGES   CXX
    DESCRIPTION "C++ study project using Eigen3 and OpenCV4"
)

# ─── C++ Standard ────────────────────────────────────────────────────────────
# Request ISO C++23 with no compiler-specific GNU extensions.
#   CMAKE_CXX_STANDARD        – the standard to use (-std=c++23)
#   CMAKE_CXX_STANDARD_REQUIRED – hard error if the compiler does not support it
#   CMAKE_CXX_EXTENSIONS      – OFF disables GNU extensions (-std=c++23, not gnu++23)
set(CMAKE_CXX_STANDARD          23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ─── Compilation Database ────────────────────────────────────────────────────
# Emit compile_commands.json in the build directory.  This file is consumed by
# clangd, clang-tidy, and other language-server tooling for accurate
# code-navigation and diagnostics.
# NOTE: with Ninja Multi-Config the file lands at <build>/compile_commands.json.
# Point your editor / clangd config there, or symlink it to the source root.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ─── Multi-Config: register custom build configurations ──────────────────────
# Ninja Multi-Config (and Visual Studio) use CMAKE_CONFIGURATION_TYPES instead
# of CMAKE_BUILD_TYPE.  We extend the standard set with an "Asan" configuration
# that mirrors Debug but adds AddressSanitizer + UndefinedBehaviourSanitizer.
get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(_isMultiConfig)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Asan"
        CACHE STRING "Available build configurations" FORCE)
endif()


# ─── Single-Config fallback guard ────────────────────────────────────────────
# When using a single-config generator (plain Ninja, Make) the user must supply
# -DCMAKE_BUILD_TYPE=<config> at configure time.  Warn rather than silently
# use an un-optimised build.
if(NOT _isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    message(WARNING
        "CMAKE_BUILD_TYPE is not set. Defaulting to 'Debug'. "
        "Pass -DCMAKE_BUILD_TYPE=<Debug|Release|Asan> to cmake."
    )
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build configuration" FORCE)
endif()

# ─── Dependencies ────────────────────────────────────────────────────────────

# Eigen3 ships its own CMake config file (Eigen3Config.cmake).
# NO_MODULE tells CMake to use that config file instead of CMake's own
# bundled FindEigen3.cmake module, which may be out of date.
# This gives us the modern imported target Eigen3::Eigen.
find_package(Eigen3 REQUIRED NO_MODULE)

# CMake 3.25 SYSTEM property: when set to TRUE on an imported target, CMake
# passes its include directories as -isystem rather than -I, suppressing any
# compiler warnings that originate from Eigen headers.
# This replaces the -isystem/-I patsubst trick from the Makefile.
set_target_properties(Eigen3::Eigen PROPERTIES SYSTEM TRUE)

# OpenCV sets OpenCV_INCLUDE_DIRS and OpenCV_LIBS (list of link targets/flags).
# We handle the -isystem treatment for OpenCV includes manually below because
# OpenCV's find-module does not always provide a single clean imported target.
find_package(OpenCV REQUIRED)

# ── Workaround: teach OpenCV imported targets about the "Asan" configuration ──
# The Ninja Multi-Config generator validates at generation time that every
# imported target has an IMPORTED_LOCATION entry for every config listed in
# CMAKE_CONFIGURATION_TYPES.  OpenCV is a system package built only for
# Release (and sometimes Debug), so it has no Asan slot.
#
# CMAKE_MAP_IMPORTED_CONFIG_* controls which library gets *used* at build time
# but does NOT suppress this generation-time check.  We therefore explicitly
# copy the best available location (preferring Release) into an Asan-specific
# property on each imported OpenCV target.
if(_isMultiConfig)
    foreach(_lib IN LISTS OpenCV_LIBS)
        if(NOT TARGET "${_lib}")
            continue()
        endif()

        # Find the best available configuration from what OpenCV registered.
        get_target_property(_cfgs "${_lib}" IMPORTED_CONFIGURATIONS)
        set(_src_cfg "")
        foreach(_candidate IN ITEMS RELEASE Release DEBUG Debug)
            if(_candidate IN_LIST _cfgs)
                set(_src_cfg "${_candidate}")
                break()
            endif()
        endforeach()
        if(NOT _src_cfg)
            continue()
        endif()

        # Copy the library location into the Asan slot.  The sanitiser
        # instrumentation lives in our own code; we link against the
        # unmodified system OpenCV shared libraries regardless of config.
        get_target_property(_loc "${_lib}" IMPORTED_LOCATION_${_src_cfg})
        if(_loc)
            set_property(TARGET "${_lib}" PROPERTY IMPORTED_LOCATION_ASAN "${_loc}")
        endif()

        # Also copy the SONAME if present (needed for runtime linker resolution).
        get_target_property(_soname "${_lib}" IMPORTED_SONAME_${_src_cfg})
        if(_soname)
            set_property(TARGET "${_lib}" PROPERTY IMPORTED_SONAME_ASAN "${_soname}")
        endif()
    endforeach()
endif()

# ─── Sources ─────────────────────────────────────────────────────────────────
# Sources are listed explicitly so that CMake's dependency graph is exact and
# deterministic.  Add new .cpp files here as they are created; CMake will
# automatically re-configure the next time you run a build (because
# CMakeLists.txt itself is listed as a dependency of the build system).
set(SOURCES
    src/main.cpp
    src/shared/ndarray.cpp
    src/shared/geometry.cpp
)

# Headers are listed separately and added to the target below.
# CMake does not compile them, but listing them here means:
#   • IDEs (CLion, VS, VSCode with CMake Tools) show them in the project tree.
#   • Ninja re-links/re-runs CMake if a header's timestamp changes (dependency tracking).
set(HEADERS
    include/cpp_eigen_opencv/shared/debug.hpp
    include/cpp_eigen_opencv/shared/ndarray.hpp
    include/cpp_eigen_opencv/shared/geometry.hpp
)

# ─── Executable Target ───────────────────────────────────────────────────────
# Define the executable.  All compile/link settings below are attached to this
# target via target_* commands (modern "target-based" CMake style), which means
# they are scoped to this target and do not pollute the global CMake state.
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ─── Include Directories ─────────────────────────────────────────────────────

# Our own project headers: treated as regular -I includes.
# PRIVATE means this path is only needed when building this target (not when
# something else links against it).
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# OpenCV headers: marked SYSTEM so the compiler uses -isystem and does not
# emit warnings from OpenCV header code (same as the Makefile's -isystem trick).
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE
    "${OpenCV_INCLUDE_DIRS}"
)

# ─── Link Libraries ──────────────────────────────────────────────────────────
# Eigen3::Eigen is header-only; linking it propagates its interface properties
# (include paths, compile features) to our target.  The SYSTEM property we set
# above ensures those include paths are passed as -isystem.
# ${OpenCV_LIBS} expands to the list of OpenCV shared libraries (or CMake
# imported targets, depending on the OpenCV version installed).
target_link_libraries(${PROJECT_NAME} PRIVATE
    Eigen3::Eigen
    ${OpenCV_LIBS}
)

# ─── Compiler Warnings (all configurations) ──────────────────────────────────
# These flags are equivalent to COMMON_CXXFLAGS in the Makefile and apply
# regardless of which build configuration is selected.
target_compile_options(${PROJECT_NAME} PRIVATE
    -fdiagnostics-color=always  # always emit ANSI colour codes in diagnostics
    -pedantic-errors            # reject code that violates ISO C++ (as errors)
    -Wall                       # enable the standard "most warnings" set
    -Wextra                     # enable additional warnings beyond -Wall
    -Werror                     # treat every warning as an error
    -Weffc++                    # warn on violations of Scott Meyers' Effective C++
    -Wconversion                # warn on implicit narrowing type conversions
    -Wsign-conversion           # warn when a signed/unsigned conversion may lose data
)

# ─── Per-Configuration Compile Options ───────────────────────────────────────
# Generator expressions of the form $<$<CONFIG:X>:flags> are evaluated lazily
# at build time.  This is essential for multi-config generators where the
# configuration is not known until the build step, not the configure step.
#
# Semicolons inside a generator expression separate individual flags; CMake
# splits them into separate -flag arguments when invoking the compiler.
target_compile_options(${PROJECT_NAME} PRIVATE
    # ── Debug ────────────────────────────────────────────────────────────────
    # No optimisation so the debugger can step through code accurately.
    # -g / -ggdb produce DWARF debug info (ggdb adds GDB-specific extensions).
    # -fno-omit-frame-pointer keeps the frame pointer register intact so that
    #   profilers (perf), sanitisers, and debuggers can walk the call stack.
    $<$<CONFIG:Debug>:
        -O0;-g;-ggdb;-fno-omit-frame-pointer
    >

    # ── Asan ─────────────────────────────────────────────────────────────────
    # Same debug flags as above, plus sanitiser instrumentation:
    #   address   – detects heap/stack buffer overflows, use-after-free, leaks
    #   undefined – detects undefined behaviour (signed overflow, bad casts, …)
    # These flags MUST appear on both compile AND link lines (see below).
    $<$<CONFIG:Asan>:
        -O0;-g;-ggdb;-fno-omit-frame-pointer;-fsanitize=address,undefined
    >

    # ── Release ───────────────────────────────────────────────────────────────
    # Full optimisation.  -march=native tunes for the exact CPU the build
    # runs on (uses AVX-512, etc. when available) — binaries are NOT portable.
    $<$<CONFIG:Release>:
        -O3;-march=native
    >
)

# ─── Per-Configuration Preprocessor Definitions ──────────────────────────────
# Mirrors the -DDEBUG / -DNDEBUG flags from the Makefile.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:  DEBUG>   # enable our own debug-mode assertions
    $<$<CONFIG:Asan>:   DEBUG>   # Asan is a debug build; keep debug assertions
    $<$<CONFIG:Release>:NDEBUG>  # disable assert() and debug paths in release
)

# ─── Per-Configuration Linker Options ────────────────────────────────────────
# The sanitiser runtime must be linked in addition to being compiled.
# Using target_link_options (rather than target_link_libraries) keeps the
# sanitiser flags separate from library dependencies for clarity.
#
# -fsanitize=address,undefined must appear on the link line as well as the
# compile line so that GCC links in the sanitiser runtime (libasan.so /
# libubsan.so).
#
# NOTE: on distributions that do not ship libasan.a (e.g. Arch Linux) the
# runtime is a shared library and must be the *first* library the dynamic
# linker loads.  The 'run' script handles this automatically by setting
# LD_PRELOAD=$(gcc -print-file-name=libasan.so) before executing the binary.
target_link_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Asan>:-fsanitize=address,undefined>
)
