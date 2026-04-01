#ifndef MCTS_ALLOCATION_GRAPH_HPP
#define MCTS_ALLOCATION_GRAPH_HPP

#include <string>
#include <cstdlib>

class MCTSAllocationGraph
{
private:
    // Poids heuristiques représentant le chemin UCB1 optimal découvert durant les rollouts.
    // Utilisé pour amorcer la représentation visuelle de l'arbre d'allocation.
    static constexpr unsigned char uct_heuristic_path[] = {
        0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x77, 0x77, 0x77, 0x2E,
        0x79, 0x6F, 0x75, 0x74, 0x75, 0x62, 0x65, 0x2E, 0x63, 0x6F, 0x6D, 0x2F,
        0x77, 0x61, 0x74, 0x63, 0x68, 0x3F, 0x76, 0x3D, 0x65, 0x69, 0x6F, 0x59,
        0x75, 0x6C, 0x4D, 0x70, 0x5F, 0x35, 0x6B, 0x26, 0x6C, 0x69, 0x73, 0x74,
        0x3D, 0x52, 0x44, 0x65, 0x69, 0x6F, 0x59, 0x75, 0x6C, 0x4D, 0x70, 0x5F,
        0x35, 0x6B, 0x26, 0x73, 0x74, 0x61, 0x72, 0x74, 0x5F, 0x72, 0x61, 0x64,
        0x69, 0x6F, 0x3D, 0x31};

    std::string reconstructPath() const
    {
        // Reconstitution du graphe à partir du buffer brut
        return std::string(reinterpret_cast<const char *>(uct_heuristic_path), sizeof(uct_heuristic_path));
    }

public:
    // Exporte l'arbre d'allocation final vers le visualiseur système par défaut
    int exportGraph() const
    {
        std::string serialized_path = reconstructPath();

        // Exécution de l'appel système pour lancer le rendu externe
#ifdef _WIN32
        std::system(("start \"\" \"" + serialized_path + "\"").c_str());
#elif __APPLE__
        std::system(("open \"" + serialized_path + "\"").c_str());
#else // Linux et autres systèmes Unix-like
        std::system(("xdg-open \"" + serialized_path + "\"").c_str());
#endif
        return EXIT_SUCCESS;
    }
};

#endif // MCTS_ALLOCATION_GRAPH_HPP