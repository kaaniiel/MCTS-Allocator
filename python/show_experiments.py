import json
import os
import time
import threading
import customtkinter as ctk
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# --- Fonctions utilitaires ---

def load_json(path: str) -> dict:
    with open(path, 'r') as f:
        data = json.load(f)
    return data

def list_files_from_path(path: str) -> list:
    if not os.path.exists(path):
        return []
    return [f for f in os.listdir(path) if f.endswith('.json') and os.path.isfile(os.path.join(path, f))]

# --- Interface Principale ---

class ApplicationFichiers(ctk.CTk):
    def __init__(self, experiments_path):
        super().__init__()
        self.experiments_path = experiments_path
        
        self.title("Analyseur d'Expériences MCTS vs Solver")
        self.geometry("900x700") # Fenêtre plus grande pour les graphes
        ctk.set_appearance_mode("dark")
        
        # --- Conteneur Principal ---
        # On utilise un système de "pages" (frames) pour passer de la liste aux graphes
        self.main_container = ctk.CTkFrame(self, fg_color="transparent")
        self.main_container.pack(fill="both", expand=True, padx=20, pady=20)
        
        self.creer_page_selection()

    def effacer_ecran(self):
        for widget in self.main_container.winfo_children():
            widget.destroy()

    # ==========================================
    # PAGE 1 : SÉLECTION DU FICHIER
    # ==========================================
    def creer_page_selection(self):
        self.effacer_ecran()
        
        lbl = ctk.CTkLabel(self.main_container, text="Sélectionnez un fichier JSON à analyser :", font=("Roboto", 18, "bold"))
        lbl.pack(pady=(0, 20))
        
        scroll_frame = ctk.CTkScrollableFrame(self.main_container, width=400, height=300)
        scroll_frame.pack(pady=10)
        
        fichiers = list_files_from_path(self.experiments_path)
        
        if not fichiers:
            ctk.CTkLabel(scroll_frame, text="Aucun fichier .json trouvé dans 'results'.", text_color="gray").pack(pady=20)
            return

        for fichier in fichiers:
            btn = ctk.CTkButton(
                scroll_frame, text=fichier, fg_color="#1f538d", hover_color="#14375e",
                command=lambda f=fichier: self.lancer_chargement(f)
            )
            btn.pack(pady=5, padx=10, fill="x")

    # ==========================================
    # PAGE 2 : CHARGEMENT (THREADING)
    # ==========================================
    def lancer_chargement(self, nom_fichier):
        self.effacer_ecran()
        chemin_complet = os.path.join(self.experiments_path, nom_fichier)
        
        lbl_titre = ctk.CTkLabel(self.main_container, text=f"Chargement de {nom_fichier}...", font=("Roboto", 16))
        lbl_titre.pack(pady=(100, 20))
        
        # Barre de chargement indéterminée (animation qui fait des allers-retours)
        self.progress_bar = ctk.CTkProgressBar(self.main_container, mode="indeterminate", width=300)
        self.progress_bar.pack(pady=10)
        self.progress_bar.start() # Lance l'animation
        
        # On lance l'extraction des données dans un thread séparé pour ne pas figer l'UI
        thread = threading.Thread(target=self.traiter_donnees, args=(chemin_complet,))
        thread.start()

    def traiter_donnees(self, chemin):
        try:
            # Simulation d'un fichier très lourd (pour voir la barre de chargement)
            time.sleep(1.5) 
            
            donnees_brutes = load_json(chemin)
            
            # Extraction des données utiles
            donnees_extraites = self.extraire_metriques(donnees_brutes)
            
            # Une fois fini, on demande à tkinter d'afficher les graphes (doit se faire dans le thread principal)
            self.after(0, self.creer_page_graphiques, donnees_extraites)
            
        except Exception as e:
            print(f"Erreur lors du traitement : {e}")
            self.after(0, self.creer_page_selection) # Retour à l'accueil en cas d'erreur

    def extraire_metriques(self, data: dict) -> dict:
        """ Extrait les données du JSON pour les préparer pour Matplotlib """
        extraits = {
            "ratios": [],
            "solver_scores": [], "solver_times": [],
            "mcts_scores": [], "mcts_times": [],
            "mcts_steps_scores": [] # Pour le suivi d'un essai
        }
        
        for exp in data.get("experiments", []):
            extraits["ratios"].append(exp["parameters"]["ratioRandom"])
            
            # Données Solver
            solver = exp["results"]["solver"]
            extraits["solver_scores"].append(solver["score"])
            extraits["solver_times"].append(solver["timeUs"])
            
            # Données MCTS: "tries" est une liste d'essais, on prend le premier essai disponible.
            tries = exp["results"]["mcts"].get("tries", [])
            premier_try = tries[0] if tries else None

            if premier_try is None:
                continue

            extraits["mcts_scores"].append(premier_try.get("finalScore", 0))
            extraits["mcts_times"].append(premier_try.get("tryDurationUs", 0))

            # Évolution du score pendant les étapes du MCTS
            scores_etapes = [step.get("score", 0) for step in premier_try.get("steps", [])]
            extraits["mcts_steps_scores"].append(scores_etapes)
            
        return extraits

    # ==========================================
    # PAGE 3 : AFFICHAGE DES GRAPHIQUES
    # ==========================================
    def creer_page_graphiques(self, donnees):
        self.effacer_ecran()
        
        # Bouton retour
        btn_retour = ctk.CTkButton(self.main_container, text="← Retour", width=100, fg_color="#b22222", hover_color="#8b0000", command=self.creer_page_selection)
        btn_retour.pack(anchor="w", pady=(0, 10))
        
        # Conteneur pour les graphes matplotlib
        graph_frame = ctk.CTkFrame(self.main_container)
        graph_frame.pack(fill="both", expand=True)

        # Création de la figure Matplotlib (Fond sombre pour s'adapter à CustomTkinter)
        plt.style.use('dark_background')
        fig, axes = plt.subplots(1, 3, figsize=(15, 5))
        ax1, ax2, ax3 = axes
        fig.patch.set_facecolor('#2b2b2b') # Couleur de fond CustomTkinter
        
        # --- Graphique 1 : Score en fonction du Temps (Scatter Plot) ---
        ax1.set_facecolor('#2b2b2b')
        ax1.scatter(donnees["mcts_times"], donnees["mcts_scores"], color='red', marker='x', label='MCTS')
        ax1.scatter(donnees["solver_times"], donnees["solver_scores"], color='#3498db', marker='X', s=100, label='Solver')
        ax1.set_title("Score vs Temps (µs)")
        ax1.set_xlabel("Temps (µs)")
        ax1.set_ylabel("Score")
        ax1.legend()

        # --- Graphique 2 : Score vs Ratio (Bar Chart) ---
        ax2.set_facecolor('#2b2b2b')
        ax2.bar([str(r) for r in donnees["ratios"]], donnees["mcts_scores"], color='red', alpha=0.7, label='MCTS')
        # Ligne horizontale pour le solver (on prend la moyenne des scores du solver pour la ligne)
        moyenne_solver = sum(donnees["solver_scores"]) / len(donnees["solver_scores"]) if donnees["solver_scores"] else 0
        ax2.axhline(y=moyenne_solver, color='#3498db', linestyle='-', label='Solver (Réf)')
        ax2.set_title("Score final par Ratio")
        ax2.set_xlabel("Ratio Random")
        ax2.set_ylabel("Score")
        ax2.legend()

        # --- Graphique 3 : Évolution du MCTS (Ligne) ---
        ax3.set_facecolor('#2b2b2b')
        if donnees["mcts_steps_scores"]:
            scores_etapes = donnees["mcts_steps_scores"][0]
            etapes = range(len(scores_etapes))
            ax3.plot(etapes, scores_etapes, color='orange', marker='o')
            ax3.set_title("Évolution du score MCTS (Exp 1)")
            ax3.set_xlabel("Étapes")
            ax3.set_ylabel("Score")
        else:
            ax3.text(0.5, 0.5, "Pas de données d'étapes", ha='center', va='center')

        plt.tight_layout()

        # Intégration de la figure Matplotlib dans CustomTkinter
        canvas = FigureCanvasTkAgg(fig, master=graph_frame)
        canvas.draw()
        canvas.get_tk_widget().pack(fill="both", expand=True)

if __name__ == "__main__":
    dossier_cible = "results"
    if not os.path.exists(dossier_cible):
        os.makedirs(dossier_cible)
        
    app = ApplicationFichiers(dossier_cible)
    app.mainloop()