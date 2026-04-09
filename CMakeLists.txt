cmake_minimum_required(VERSION 3.20)

if(MSVC AND NOT DEFINED CMAKE_GENERATOR_PLATFORM)
    set(CMAKE_GENERATOR_PLATFORM x64 CACHE STRING "Target platform" FORCE)
endif()

project(MCTS_Allocation VERSION 1.0 LANGUAGES CXX)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "This project requires an x64 Visual Studio generator because the bundled OR-Tools Windows package is x64-only. Reconfigure with -A x64 or delete the existing build directory.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

# ----------------------------
# OpenMP (optionnel)
# ----------------------------
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP trouve, multi-thread active.")
else()
    message(WARNING "OpenMP non trouve.")
endif()

# ----------------------------
# OR-Tools auto-download
# ----------------------------
# Tu peux overrider ORTOOLS_VERSION et ORTOOLS_ARCHIVE via -D...
set(ORTOOLS_VERSION "v9.11" CACHE STRING "OR-Tools release tag")
set(ORTOOLS_BUILD_ID "4210" CACHE STRING "OR-Tools build id in release filename")
set(ORTOOLS_ARCHIVE "" CACHE STRING "OR-Tools archive name (auto-detected if empty)")

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ORTOOLS_CPU_RAW)
if(ORTOOLS_CPU_RAW MATCHES "^(x86_64|amd64)$")
    set(ORTOOLS_CPU "x86_64")
elseif(ORTOOLS_CPU_RAW MATCHES "^(arm64|aarch64)$")
    set(ORTOOLS_CPU "arm64")
else()
    set(ORTOOLS_CPU "${ORTOOLS_CPU_RAW}")
endif()

if(ORTOOLS_ARCHIVE STREQUAL "")
    if(WIN32)
        if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR "OR-Tools prebuilt Windows package requires x64.")
        endif()
        # Nom exact de l'archive C++ Windows publiee par OR-Tools pour VS2022 x64.
        set(ORTOOLS_ARCHIVE "or-tools_x64_VisualStudio2022_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.zip")
    elseif(APPLE)
        # OR-Tools package names include a macOS platform token; tweak if Google changes naming.
        if(ORTOOLS_CPU STREQUAL "arm64")
            set(ORTOOLS_MACOS_TAG "macOS-14.7" CACHE STRING "macOS package tag used in OR-Tools archive name")
            set(ORTOOLS_ARCHIVE "or-tools_arm64_${ORTOOLS_MACOS_TAG}_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.tar.gz")
        elseif(ORTOOLS_CPU STREQUAL "x86_64")
            set(ORTOOLS_MACOS_TAG "macOS-13.7" CACHE STRING "macOS package tag used in OR-Tools archive name")
            set(ORTOOLS_ARCHIVE "or-tools_x86_64_${ORTOOLS_MACOS_TAG}_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.tar.gz")
        else()
            message(FATAL_ERROR "Unsupported macOS CPU '${ORTOOLS_CPU}'. Set ORTOOLS_ARCHIVE manually.")
        endif()
    elseif(UNIX)
        # Linux package token differs by distro; override ORTOOLS_LINUX_TAG or ORTOOLS_ARCHIVE if needed.
        set(ORTOOLS_LINUX_TAG "Ubuntu-22.04" CACHE STRING "Linux package tag used in OR-Tools archive name")
        if(ORTOOLS_CPU STREQUAL "x86_64")
            set(ORTOOLS_ARCHIVE "or-tools_x86_64_${ORTOOLS_LINUX_TAG}_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.tar.gz")
        else()
            message(FATAL_ERROR "Unsupported Linux CPU '${ORTOOLS_CPU}' for default OR-Tools package. Set ORTOOLS_ARCHIVE manually.")
        endif()
    else()
        message(FATAL_ERROR "Plateforme non supportee par ce template.")
    endif()
endif()

set(ORTOOLS_URL "https://github.com/google/or-tools/releases/download/${ORTOOLS_VERSION}/${ORTOOLS_ARCHIVE}" CACHE STRING "OR-Tools download URL" FORCE)

set(DEPS_DIR "${CMAKE_BINARY_DIR}/_deps")
set(ORTOOLS_DIR "${DEPS_DIR}/ortools")

# Some previous failed runs can leave a file named "ortools" instead of a directory.
if(EXISTS "${ORTOOLS_DIR}" AND NOT IS_DIRECTORY "${ORTOOLS_DIR}")
    file(REMOVE "${ORTOOLS_DIR}")
endif()

# Prefer an already extracted OR-Tools directory if one exists.
set(ORTOOLS_ROOT_DIR "")
file(GLOB ORTOOLS_EXTRACTED_CANDIDATES LIST_DIRECTORIES true "${DEPS_DIR}/or-tools*")
foreach(CANDIDATE IN LISTS ORTOOLS_EXTRACTED_CANDIDATES)
    if(IS_DIRECTORY "${CANDIDATE}")
        set(ORTOOLS_ROOT_DIR "${CANDIDATE}")
        break()
    endif()
endforeach()

if(ORTOOLS_ROOT_DIR STREQUAL "" AND IS_DIRECTORY "${ORTOOLS_DIR}")
    set(ORTOOLS_ROOT_DIR "${ORTOOLS_DIR}")
endif()

set(ORTOOLS_CONFIG "${ORTOOLS_ROOT_DIR}/lib/cmake/ortools/ortoolsConfig.cmake")

file(MAKE_DIRECTORY "${DEPS_DIR}")

if(NOT EXISTS "${ORTOOLS_CONFIG}")
    set(ARCHIVE_PATH "${DEPS_DIR}/${ORTOOLS_ARCHIVE}")
    message(STATUS "Telechargement OR-Tools: ${ORTOOLS_URL}")
    file(DOWNLOAD
        "${ORTOOLS_URL}"
        "${ARCHIVE_PATH}"
        SHOW_PROGRESS
        STATUS DL_STATUS
        TLS_VERIFY ON
    )

    list(GET DL_STATUS 0 DL_CODE)
    if(NOT DL_CODE EQUAL 0)
        list(GET DL_STATUS 1 DL_MSG)
        message(FATAL_ERROR "Echec telechargement OR-Tools: ${DL_MSG}")
    endif()

    # Extraction de l'archive dans _deps
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xvf "${ARCHIVE_PATH}"
        WORKING_DIRECTORY "${DEPS_DIR}"
        RESULT_VARIABLE EXTRACT_RC
    )
    if(NOT EXTRACT_RC EQUAL 0)
        message(FATAL_ERROR "Echec extraction OR-Tools.")
    endif()

    # Identifie le dossier extrait (or-tools_...)
    file(GLOB EXTRACTED_CANDIDATES LIST_DIRECTORIES true "${DEPS_DIR}/or-tools*")
    set(ORTOOLS_ROOT_DIR "")
    foreach(CANDIDATE IN LISTS EXTRACTED_CANDIDATES)
        if(IS_DIRECTORY "${CANDIDATE}")
            set(ORTOOLS_ROOT_DIR "${CANDIDATE}")
            break()
        endif()
    endforeach()

    if(ORTOOLS_ROOT_DIR STREQUAL "")
        message(FATAL_ERROR "Impossible de trouver le dossier OR-Tools extrait.")
    endif()

    set(ORTOOLS_CONFIG "${ORTOOLS_ROOT_DIR}/lib/cmake/ortools/ortoolsConfig.cmake")
endif()

if(NOT EXISTS "${ORTOOLS_CONFIG}")
    message(FATAL_ERROR "OR-Tools configure file introuvable: ${ORTOOLS_CONFIG}")
endif()

set(ortools_DIR "${ORTOOLS_ROOT_DIR}/lib/cmake/ortools")
list(PREPEND CMAKE_PREFIX_PATH "${ORTOOLS_ROOT_DIR}")
find_package(ortools CONFIG REQUIRED)

# ----------------------------
# Targets
# ----------------------------
add_library(mcts_core
    src/mcts/Node.cpp
    src/mcts/MCTS.cpp
    src/mcts/UCB.cpp
)

target_include_directories(mcts_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(mcts_core PUBLIC OpenMP::OpenMP_CXX)
endif()

target_link_libraries(mcts_core PUBLIC ortools::ortools)

add_executable(mcts_main src/main.cpp)
target_link_libraries(mcts_main PRIVATE mcts_core)