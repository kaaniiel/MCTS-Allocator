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

if(WIN32)
    # Nom exact de l'archive C++ Windows publiee par OR-Tools pour VS2022 x64.
    set(ORTOOLS_ARCHIVE "or-tools_x64_VisualStudio2022_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.zip" CACHE STRING "OR-Tools archive name" FORCE)
elseif(UNIX AND NOT APPLE)
    # Exemple Ubuntu 22.04 x64 (a ajuster selon ta cible Linux)
    set(ORTOOLS_ARCHIVE "or-tools_amd64_ubuntu-22.04_cpp_${ORTOOLS_VERSION}.${ORTOOLS_BUILD_ID}.tar.gz" CACHE STRING "OR-Tools archive name" FORCE)
else()
    message(FATAL_ERROR "Plateforme non supportee par ce template.")
endif()

set(ORTOOLS_URL "https://github.com/google/or-tools/releases/download/${ORTOOLS_VERSION}/${ORTOOLS_ARCHIVE}" CACHE STRING "OR-Tools download URL" FORCE)

set(DEPS_DIR "${CMAKE_BINARY_DIR}/_deps")
set(ORTOOLS_DIR "${DEPS_DIR}/ortools")
set(ORTOOLS_CONFIG "${ORTOOLS_DIR}/lib/cmake/ortools/ortoolsConfig.cmake")

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

    # Renomme le dossier extrait (or-tools_...) vers _deps/ortools
    file(GLOB EXTRACTED_CANDIDATES LIST_DIRECTORIES true "${DEPS_DIR}/or-tools*")
    list(LENGTH EXTRACTED_CANDIDATES CAND_COUNT)
    if(CAND_COUNT LESS 1)
        message(FATAL_ERROR "Impossible de trouver le dossier OR-Tools extrait.")
    endif()

    list(GET EXTRACTED_CANDIDATES 0 EXTRACTED_DIR)

    if(EXISTS "${ORTOOLS_DIR}")
        file(REMOVE_RECURSE "${ORTOOLS_DIR}")
    endif()
    file(RENAME "${EXTRACTED_DIR}" "${ORTOOLS_DIR}")
endif()

list(PREPEND CMAKE_PREFIX_PATH "${ORTOOLS_DIR}")
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