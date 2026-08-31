import pandas as pd
import igraph as ig
import numpy as np
import matplotlib.pyplot as plt
import argparse
import os

def plotDegreePowerLaw(fileName: str, outputFile: str) -> None:
    """
    Genera e salva il grafico log-log della distribuzione dei gradi.
    """
    df = pd.read_csv(fileName, sep=";")
    df.columns = df.columns.str.strip()

    edges = list(zip(df['Source'], df['Target']))
    G = ig.Graph.TupleList(edges, directed=False)

    degreeSequence = G.degree()
    degreeCounts = pd.Series(degreeSequence).value_counts().sort_index()

    plt.figure(figsize=(8, 6))
    plt.loglog(degreeCounts.index, degreeCounts.values, marker='o', linestyle='none', color='royalblue', alpha=0.7)

    plt.title(f"Distribuzione dei Gradi - Power Law ({G.vcount():,} nodi)".replace(',', '.'))
    plt.xlabel("Grado (k)")
    plt.ylabel("Numero di nodi con grado k")
    plt.grid(True, which="both", ls="--", alpha=0.5)

    plt.savefig(outputFile, dpi=300, bbox_inches='tight')
    plt.close()

    return

def plotCapacityPowerLaw(fileName: str, outputFile: str) -> None:
    """
    Genera e salva il grafico log-log della distribuzione delle capacità dei canali
    usando la CCDF (Complementary Cumulative Distribution Function).
    """
    df = pd.read_csv(fileName, sep=";")
    df.columns = df.columns.str.strip()

    # Gestione flessibile del nome colonna ('Capacity' o 'Capacities')
    colName = 'Capacity' if 'Capacity' in df.columns else 'Capacities'
    
    # Estraiamo i valori in un array numpy per ottimizzare l'ordinamento
    capacities = df[colName][df[colName] > 0].values

    if len(capacities) == 0:
        print("Nessuna capacità valida trovata nel file.")
        return

    # 1. Ordinamento crescente delle capacità
    capacities_sorted = np.sort(capacities)
    n = len(capacities_sorted)

    # 2. Calcolo della CCDF P(X >= C)
    # Crea un array decrescente da 1.0 (100%) fino a 1/n (l'ultimo elemento)
    ccdf = np.arange(n, 0, -1) / n

    # 3. Creazione del plot
    plt.figure(figsize=(8, 6))
    
    # Usiamo una linea continua invece dei punti sparsi
    plt.loglog(capacities_sorted, ccdf, marker='', linestyle='-', color='darkorange', linewidth=2)

    plt.title(f"CCDF delle Capacità dei Canali ({n:,} canali)".replace(',', '.'))
    plt.xlabel("Capacità C (satoshi)")
    plt.ylabel("Probabilità $P(Capacità \geq C)$")
    plt.grid(True, which="both", ls="--", alpha=0.5)

    plt.savefig(outputFile, dpi=300, bbox_inches='tight')
    plt.close()

    return


def plotGraph(fileName: str, outputFile: str) -> None:
    """
    Genera e salva la rappresentazione visuale del grafo sfruttando l'impaginazione rapida di igraph.
    """
    df = pd.read_csv(fileName, sep=";")
    df.columns = df.columns.str.strip()

    edges = list(zip(df['Source'], df['Target']))
    G = ig.Graph.TupleList(edges, directed=False)

    # Layout Fruchterman-Reingold velocizzato in C
    layout = G.layout_fruchterman_reingold()
    
    # Normalizzazione dimensioni nodi in base al grado
    degrees = G.degree()
    maxDegree = max(degrees) if degrees else 1
    nodeSizes = [5 + (d / maxDegree) * 15 for d in degrees]

    fig, ax = plt.subplots(figsize=(10, 10))
    
    ig.plot(
        G,
        target=ax,
        layout=layout,
        vertex_size=nodeSizes,
        vertex_color="crimson",
        edge_color="grey",
        edge_width=0.5
    )

    
    plt.title("Visualizzazione Rete")
    plt.axis('off')

    plt.savefig(outputFile, dpi=300, bbox_inches='tight')
    plt.close()

    return 

def plotMetricsEvolution(resultsFile: str, outputDir: str) -> None:
    """
    Genera e salva i grafici per Assortatività, Diametro e Grado Mediano
    al variare del numero di nodi (n), differenziando le curve per il valore di beta.
    """
    # Carica i dati aggregati salvati da analizeGraph.py
    df = pd.read_csv(resultsFile, sep=";")
    
    # Assicuriamoci che la cartella di output esista
    os.makedirs(outputDir, exist_ok=True)
    
    # Dizionario delle metriche salvate nello script: {nome_colonna: (Titolo, Etichetta Y)}
    metrics = {
        "assortativity": ("Evoluzione dell'Assortatività", "Assortativity Coefficient"),
        "diameter": ("Evoluzione del Diametro", "Diameter"),
        "median degree": ("Evoluzione del Grado Mediano", "Median Degree")
    }
    
    # Ordiniamo per 'n' per garantire che le linee dei grafici siano continue
    df = df.sort_values(by="n")
    
    # Estraiamo i valori unici di beta presenti nel file e li ordiniamo
    betas = sorted(df['beta'].unique())
    
    for metric_col, (title, ylabel) in metrics.items():
        if metric_col not in df.columns:
            print(f"Colonna '{metric_col}' non trovata nel DataFrame. Salto...")
            continue
            
        plt.figure(figsize=(10, 6))
        
        # Plottiamo una linea separata per ogni valore di beta
        for beta in betas:
            subset = df[df['beta'] == beta]
            plt.plot(
                subset['n'], 
                subset[metric_col], 
                marker='o', 
                markersize=4, 
                linestyle='-', 
                label=f'$\\beta = {beta}$'
            )
            
        # Formattazione per avvicinarsi allo stile del paper
        plt.title(title)
        plt.xlabel("Number of nodes")
        plt.ylabel(ylabel)
        plt.legend(title="Parametro $\\beta$")
        plt.grid(True, linestyle="--", alpha=0.6)
        
        # Costruiamo il path finale e salviamo la figura
        safe_name = metric_col.replace(' ', '_')
        outputFile = os.path.join(outputDir, f"{safe_name}_evolution.png")
        plt.savefig(outputFile, dpi=300, bbox_inches='tight')
        plt.close()
        
    print(f"I grafici sono stati generati con successo nella cartella: {outputDir}")
    return

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("type", type=str, help="Tipologia di plot")
    parser.add_argument("fileName", type=str, help="File di input")
    args = parser.parse_args()


    if args.type == "powerLaw": 
        plotDegreePowerLaw(args.fileName, "Data/Plots/degreePowerlaw.png")
        plotCapacityPowerLaw(args.fileName, "Data/Plots/capacityPowerlaw.png")
    elif args.type == "degree":
        plotDegreePowerLaw(args.fileName, "Data/Plots/degreePowerlaw.png")
    elif args.type == "capacity":
        plotCapacityPowerLaw(args.fileName, "Data/Plots/capacityPowerlaw.png")
    elif args.type == "graph":
        plotGraph(args.fileName, "Data/Plots/graphVista.png")
    elif args.type == "LN":
        plotDegreePowerLaw("Data/Network/lightningNetworkEdges.csv", "Data/Plots/realDegreePowerlaw.png")
        plotCapacityPowerLaw("Data/Network/lightningNetworkEdges.csv", "Data/Plots/realCapacityPowerlaw.png")
    elif args.type == "metrics":
        plotMetricsEvolution("Data/Graphs/allData.csv", "Data/Plots/Metrics")

