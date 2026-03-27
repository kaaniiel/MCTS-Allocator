# Spécifie la version minimale de CMake requise
cmake_minimum_required(VERSION 3.14)

# Nom du projet et langage utilisé
project(MCTS_Allocation VERSION 1.0 LANGUAGES CXX)

# Exiger la norme C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 1. RECHERCHE DES LIBRAIRIES DE PARALLÉLISATION ---

# Avec MSVC récent, FindOpenMP peut exiger le runtime LLVM (/openmp:llvm).
if(MSVC)
    set(OpenMP_RUNTIME_MSVC "llvm")
    set(OpenMP_CXX_FLAGS "-openmp:llvm" CACHE STRING "CXX compiler flags for OpenMP parallelization" FORCE)
endif()

# Recherche d'OpenMP
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP trouvé ! Le support multi-thread est activé.")
else()
    message(WARNING "OpenMP n'a pas été trouvé sur ce système.")
endif()

# Recherche de oneTBB (Intel Threading Building Blocks)
# Décommentez ces lignes si vous avez installé TBB (ex: via apt-get install libtbb-dev ou vcpkg)
# find_package(TBB REQUIRED)
# if(TBB_FOUND)
#     message(STATUS "oneTBB trouvé !")
# endif()

# --- 2. CRÉATION DE LA BIBLIOTHÈQUE CENTRALE ---
add_library(mcts_core
    src/mcts/Node.cpp
    # Ajoutez vos autres fichiers sources ici au fur et à mesure
    src/mcts/MCTS.cpp
    src/mcts/UCB.cpp
    
)

target_include_directories(mcts_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- 3. LIAISON DES LIBRAIRIES DE PARALLÉLISATION ---

# Si OpenMP est trouvé, on le lie au coeur de notre programme
if(OpenMP_CXX_FOUND)
    target_link_libraries(mcts_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# Si oneTBB est utilisé, on le lie également
# if(TBB_FOUND)
#     target_link_libraries(mcts_core PUBLIC TBB::tbb)
# endif()

# --- 4. EXÉCUTABLE PRINCIPAL ---
add_executable(mcts_main src/main.cpp)
target_link_libraries(mcts_main PRIVATE mcts_core)

# --- 5. EXÉCUTABLES D'EXPÉRIMENTATION ---
# add_executable(run_scalability experiments/run_scalability.cpp)
# target_link_libraries(run_scalability PRIVATE mcts_core)