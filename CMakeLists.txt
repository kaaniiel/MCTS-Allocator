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

# ----------------------------
# Gurobi (optionnel, pour testLp)
# ----------------------------
set(GUROBI_HOME "" CACHE PATH "Path to the Gurobi installation root")

if(NOT GUROBI_HOME AND DEFINED ENV{GUROBI_HOME})
    set(GUROBI_HOME "$ENV{GUROBI_HOME}")
endif()

if(NOT GUROBI_HOME AND DEFINED ENV{GUROBU_HOME})
    message(FATAL_ERROR "Environment variable GUROBU_HOME was found, but the correct variable name is GUROBI_HOME.")
endif()

if(GUROBI_HOME)
    set(GUROBI_HOME_INPUT "${GUROBI_HOME}")
    if(NOT IS_ABSOLUTE "${GUROBI_HOME}")
        set(_GUROBI_HOME_CANDIDATES
            "${GUROBI_HOME}"
            "${CMAKE_SOURCE_DIR}/${GUROBI_HOME}"
            "$ENV{HOME}/${GUROBI_HOME}"
            "/opt/${GUROBI_HOME}"
        )

        foreach(_GUROBI_HOME_CANDIDATE IN LISTS _GUROBI_HOME_CANDIDATES)
            if(EXISTS "${_GUROBI_HOME_CANDIDATE}")
                set(GUROBI_HOME "${_GUROBI_HOME_CANDIDATE}")
                break()
            endif()
        endforeach()
    endif()

    if(EXISTS "${GUROBI_HOME}")
        get_filename_component(GUROBI_HOME "${GUROBI_HOME}" ABSOLUTE)
        set(GUROBI_HOME "${GUROBI_HOME}" CACHE PATH "Path to the Gurobi installation root" FORCE)
    endif()

    find_path(GUROBI_INCLUDE_DIR
        NAMES gurobi_c++.h
        PATHS "${GUROBI_HOME}/include"
        NO_DEFAULT_PATH
    )

    file(GLOB GUROBI_ALL_LIBS
        "${GUROBI_HOME}/lib/gurobi*.lib"
        "${GUROBI_HOME}/lib/gurobi*.a"
        "${GUROBI_HOME}/lib/gurobi*.so"
        "${GUROBI_HOME}/lib/gurobi*.dylib"
        "${GUROBI_HOME}/lib/libgurobi*.lib"
        "${GUROBI_HOME}/lib/libgurobi*.a"
        "${GUROBI_HOME}/lib/libgurobi*.so"
        "${GUROBI_HOME}/lib/libgurobi*.dylib"
    )

    set(GUROBI_CORE_LIBS "")
    foreach(GUROBI_LIB IN LISTS GUROBI_ALL_LIBS)
        string(FIND "${GUROBI_LIB}" "gurobi_c++" GUROBI_CXX_INDEX)
        if(GUROBI_CXX_INDEX EQUAL -1)
            list(APPEND GUROBI_CORE_LIBS "${GUROBI_LIB}")
        endif()
    endforeach()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        file(GLOB GUROBI_CXX_LIBS
            "${GUROBI_HOME}/lib/gurobi_c++mdd*.lib"
            "${GUROBI_HOME}/lib/gurobi_c++mtd*.lib"
            "${GUROBI_HOME}/lib/gurobi_c++mdd*.a"
            "${GUROBI_HOME}/lib/gurobi_c++mtd*.a"
        )
    else()
        file(GLOB GUROBI_CXX_LIBS
            "${GUROBI_HOME}/lib/gurobi_c++md*.lib"
            "${GUROBI_HOME}/lib/gurobi_c++mt*.lib"
            "${GUROBI_HOME}/lib/gurobi_c++md*.a"
            "${GUROBI_HOME}/lib/gurobi_c++mt*.a"
        )
    endif()

    if(NOT GUROBI_CXX_LIBS)
        file(GLOB GUROBI_CXX_LIBS
            "${GUROBI_HOME}/lib/*gurobi_c++*.lib"
            "${GUROBI_HOME}/lib/*gurobi_c++*.a"
            "${GUROBI_HOME}/lib/*gurobi_c++*.so"
            "${GUROBI_HOME}/lib/*gurobi_c++*.dylib"
        )
    endif()

    list(SORT GUROBI_CXX_LIBS)
    if(GUROBI_CXX_LIBS)
        list(GET GUROBI_CXX_LIBS 0 GUROBI_CXX_LIBRARY)
    endif()

    if(NOT GUROBI_INCLUDE_DIR OR NOT GUROBI_CXX_LIBRARY OR NOT GUROBI_CORE_LIBS)
        message(FATAL_ERROR
            "Gurobi was not found under GUROBI_HOME='${GUROBI_HOME_INPUT}' (resolved to '${GUROBI_HOME}'). Expected include/gurobi_c++.h and library files in lib/."
        )
    endif()
    message(STATUS "Gurobi configured from ${GUROBI_HOME}")
else()
    message(STATUS "Gurobi not configured. Set GUROBI_HOME to build testLp.")
endif()



add_executable(mcts_main src/main.cpp)
target_link_libraries(mcts_main PRIVATE mcts_core)

add_executable(mcts_experiments src/experiments.cpp)
target_link_libraries(mcts_experiments PRIVATE mcts_core)

if(GUROBI_HOME)
    target_include_directories(mcts_main PRIVATE "${GUROBI_INCLUDE_DIR}")
    target_link_libraries(mcts_main PRIVATE ${GUROBI_CXX_LIBRARY} ${GUROBI_CORE_LIBS})
    target_include_directories(mcts_experiments PRIVATE "${GUROBI_INCLUDE_DIR}")
    target_link_libraries(mcts_experiments PRIVATE ${GUROBI_CXX_LIBRARY} ${GUROBI_CORE_LIBS})
endif()

# add executable for testLp
add_executable(testLp src/testLp.cpp)
target_link_libraries(testLp PRIVATE mcts_core)

if(GUROBI_HOME)
    target_include_directories(testLp PRIVATE "${GUROBI_INCLUDE_DIR}")
    target_link_libraries(testLp PRIVATE ${GUROBI_CXX_LIBRARY} ${GUROBI_CORE_LIBS})
endif()