"""Affichage interactif des résultats d'expériences.

Utilise PySimpleGUI + matplotlib pour ouvrir une fenêtre graphique permettant
de sélectionner dynamiquement les paramètres/mesures à afficher.
"""

import json
import glob
import os
from pathlib import Path
import pandas as pd
import PySimpleGUI as sg
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


def find_latest_results(pattern="results/experiments_*.json"):
	files = glob.glob(pattern)
	if not files:
		return None
	return max(files, key=os.path.getmtime)


def load_experiments(path=None):
	if path is None:
		path = find_latest_results()
	if path is None:
		raise FileNotFoundError("Aucun fichier de résultats trouvé dans results/")
	with open(path, "r") as f:
		data = json.load(f)

	rows = []
	finals = []
	for exp in data.get("experiments", []):
		params = exp.get("parameters", {})
		mcts = exp.get("results", {}).get("mcts", {})
		for tr in mcts.get("tries", []):
			try_index = tr.get("tryIndex")
			try_duration_us = tr.get("tryDurationUs", tr.get("tryDurationMs"))
			final = {
				**params,
				"tryIndex": try_index,
				"finalScore": tr.get("finalScore"),
				"tryDurationUs": try_duration_us,
				"tryDuration": tr.get("tryDuration"),
			}
			finals.append(final)
			for step in tr.get("steps", []):
				step_time_us = step.get("stepTimeUs", step.get("stepTimeMs"))
				cumulative_time_us = step.get("cumulativeTimeUs", step.get("cumulativeTimeMs"))
				row = {
					**params,
					"tryIndex": try_index,
					"tryDurationUs": try_duration_us,
					"tryDuration": tr.get("tryDuration"),
					"stepTimeUs": step_time_us,
					"cumulativeTimeUs": cumulative_time_us,
				}
				row.update(step)
				rows.append(row)

	df_steps = pd.DataFrame(rows)
	df_finals = pd.DataFrame(finals)
	return df_steps, df_finals, path


def draw_figure(canvas, figure):
	if canvas.children:
		for child in canvas.winfo_children():
			child.destroy()
	figure_canvas_agg = FigureCanvasTkAgg(figure, master=canvas)
	figure_canvas_agg.draw()
	figure_canvas_agg.get_tk_widget().pack(side="top", fill="both", expand=1)
	return figure_canvas_agg


def make_window(df_steps, df_finals, results_path):
	param_keys = [k for k in df_steps.columns if k not in ("currentBudget", "percentBudgetUsed", "score", "allocation", "stepTimeUs", "stepTimeMs", "cumulativeTimeUs", "cumulativeTimeMs", "tryDurationUs", "tryDurationMs", "tryDuration") and k != "tryIndex"]
	numeric_cols = [c for c in df_steps.select_dtypes(include="number").columns.tolist() if c != "tryIndex"]
	if "finalScore" in df_finals.columns:
		numeric_cols.append("finalScore")
	for metric in ("stepTimeUs", "stepTimeMs", "cumulativeTimeUs", "cumulativeTimeMs", "tryDurationUs", "tryDurationMs"):
		if metric in df_finals.columns or metric in df_steps.columns:
			numeric_cols.append(metric)

	# Build left column with filters
	left_col = []
	for k in param_keys:
		vals = sorted(df_steps[k].dropna().unique().tolist())
		left_col.append([sg.Text(k)])
		left_col.append([sg.Listbox(values=[str(v) for v in vals], select_mode=sg.SELECT_MODE_MULTIPLE, size=(20, 4), key=f"-FILT-{k}-")])

	left_col += [
		[sg.Text("Mesures")],
		[sg.Listbox(values=numeric_cols, select_mode=sg.SELECT_MODE_MULTIPLE, size=(20, 6), key="-METRICS-")],
		[sg.Text("Grouper par")],
		[sg.Combo(["none"] + param_keys, default_value="none", key="-GROUP-")],
		[sg.Text("Type de graphique")],
		[sg.Combo(["line", "box", "bar"], default_value="line", key="-PLOT-")],
		[sg.Button("Mettre à jour", key="-UPDATE-")],
		[sg.Button("Sauvegarder figure", key="-SAVE-")],
		[sg.Text(f"Fichier: {results_path}", size=(40, 2))]
	]

	# Right column for plot
	plot_col = [[sg.Canvas(key="-CANVAS-", size=(640, 480))]]

	layout = [[sg.Column(left_col), sg.VSeperator(), sg.Column(plot_col)]]

	window = sg.Window("Visualiseur d'expériences", layout, finalize=True, resizable=True)
	return window


def update_plot(window, df_steps, df_finals):
	# Gather filters
	df = df_steps.copy()
	param_keys = [k for k in df.columns if k not in ("currentBudget", "percentBudgetUsed", "score", "allocation", "stepTimeUs", "stepTimeMs", "cumulativeTimeUs", "cumulativeTimeMs", "tryDurationUs", "tryDurationMs", "tryDuration") and k != "tryIndex"]
	for k in param_keys:
		sel = window[f"-FILT-{k}-"].get()
		if sel:
			# values stored as strings in listbox
			# try to cast back to numeric if original dtype numeric
			try:
				dtype = df[k].dtype
				if pd.api.types.is_numeric_dtype(dtype):
					sel_cast = [float(s) for s in sel]
				else:
					sel_cast = sel
			except Exception:
				sel_cast = sel
			df = df[df[k].isin(sel_cast)]

	metrics = window["-METRICS-"].get()
	if not metrics:
		metrics = ["score"] if "score" in df.columns else df.select_dtypes(include="number").columns.tolist()
	group_by = window["-GROUP-"].get()
	plot_type = window["-PLOT-"].get()

	fig, ax = plt.subplots(figsize=(6.4, 4.8))

	if plot_type == "line":
		x_col = "percentBudgetUsed" if "percentBudgetUsed" in df.columns else "currentBudget"
		for metric in metrics:
			if group_by and group_by != "none":
				for name, group in df.groupby(group_by):
					pivot = group.groupby(x_col)[metric].mean()
					ax.plot(pivot.index, pivot.values, label=f"{metric} / {group_by}={name}")
			else:
				pivot = df.groupby(x_col)[metrics[0]].mean()
				ax.plot(pivot.index, pivot.values, label=metrics[0])
		ax.set_xlabel(x_col)
		ax.set_ylabel(", ".join(metrics))
		ax.legend()

	elif plot_type == "box":
		x_col = group_by if group_by and group_by != "none" else "tryIndex"
		data = []
		labels = []
		for name, group in df.groupby(x_col):
			data.append(group[metrics[0]].dropna().values)
			labels.append(str(name))
		ax.boxplot(data, labels=labels)
		ax.set_ylabel(metrics[0])

	elif plot_type == "bar":
		if group_by and group_by != "none":
			mean = df.groupby(group_by)[metrics[0]].mean()
			ax.bar(mean.index.astype(str), mean.values)
			ax.set_ylabel(metrics[0])
		else:
			mean = df[metrics[0]].mean()
			ax.bar([metrics[0]], [mean])
			ax.set_ylabel(metrics[0])

	fig.tight_layout()

	canvas_elem = window["-CANVAS-"]
	canvas = canvas_elem.TKCanvas
	draw_figure(canvas, fig)
	plt.close(fig)


def main(path=None):
	try:
		df_steps, df_finals, results_path = load_experiments(path)
	except Exception as e:
		sg.popup_error("Erreur lors du chargement des résultats:", str(e))
		return

	window = make_window(df_steps, df_finals, results_path)

	# Initial plot
	update_plot(window, df_steps, df_finals)

	while True:
		event, values = window.read()
		if event in (sg.WIN_CLOSED, "Exit"):
			break
		if event == "-UPDATE-":
			update_plot(window, df_steps, df_finals)
		if event == "-SAVE-":
			save_path = sg.popup_get_file("Enregistrer l'image", save_as=True, file_types=(("PNG Files", "*.png"),))
			if save_path:
				# regenerate figure and save
				# simple approach: re-create and save using same plotting routine
				fig, ax = plt.subplots(figsize=(6.4, 4.8))
				# reuse update_plot logic lightly: plot mean score vs percentBudgetUsed
				x_col = "percentBudgetUsed" if "percentBudgetUsed" in df_steps.columns else "currentBudget"
				metric = values.get("-METRICS-")
				metric = metric[0] if metric else ("score" if "score" in df_steps.columns else df_steps.select_dtypes(include="number").columns[0])
				pivot = df_steps.groupby(x_col)[metric].mean()
				ax.plot(pivot.index, pivot.values)
				fig.tight_layout()
				fig.savefig(save_path)
				sg.popup("Figure sauvegardée:", save_path)

	window.close()


if __name__ == "__main__":
	import sys
	path = sys.argv[1] if len(sys.argv) > 1 else None
	main(path)

