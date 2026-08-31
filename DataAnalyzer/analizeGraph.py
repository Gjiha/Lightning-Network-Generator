import numpy as np
import pandas as pd
import igraph as ig
import argparse
import os

def createGraphFromCsv(fileName: str):
    df = pd.read_csv(fileName, sep=";")
    df.columns = df.columns.str.strip()

    edges = list(zip(df['Source'], df['Target']))
    G = ig.Graph.TupleList(edges, directed=False)

    return G

def computeAssortativity(graph) -> float:
    rIgraph = graph.assortativity_degree(directed=False)

    return float(rIgraph)


def computeDiameter(graph, useLcc: bool = True) -> tuple[int, int, int]:
    totalNodes = graph.vcount()
    if totalNodes == 0:
        return 0, 0, 0

    if not graph.is_connected(mode="weak"):
        if useLcc:
            components = graph.connected_components(mode="weak")
            lccG = components.giant()
            diameter = int(lccG.diameter(directed=False))
            return diameter, lccG.vcount(), totalNodes
        else:
            raise ValueError("Il grafo non è connesso e useLcc è impostato a False.")
    else:
        diameter = int(graph.diameter(directed=False))
        return diameter, totalNodes, totalNodes


def computeMedianDegree(graph) -> float:    
    if graph.vcount() == 0:
        return 0.0

    return float(np.median(graph.degree()))



def saveParameter(dictOfParameter: dict, outputName: str) -> None:
    dataFrame = pd.DataFrame([dictOfParameter])
    dataFrame.to_csv(outputName, mode="a", index=False, sep=";", encoding="utf-8", header=not os.path.isfile(outputName))
    return



# if __name__ == "__main__":
#     csvPath = "Data/Graphs/dataGraph.csv"

#     bckPath = "Data/Graphs/allData.csv"

#     parser = argparse.ArgumentParser()
#     parser.add_argument("n", type=int, help="Numero nodi")
#     parser.add_argument("beta", type=float, help="Parametro beta")
#     args = parser.parse_args()


    graph = createGraphFromCsv(csvPath)

    assortativity = computeAssortativity(graph)

    diamSim, lccSim, totSim = computeDiameter(graph)

    degreeMedianCsv = computeMedianDegree(graph)

    dictOfParameter = {
        "n" : args.n,
        "beta": args.beta,
        "assortativity" : assortativity,
        "median degree" : degreeMedianCsv,
        "diameter" : diamSim,
    }

csvPath = "Data/Graphs/provaSecondo.csv"

graph = createGraphFromCsv(csvPath)

assortativity = computeAssortativity(graph)

diamSim, lccSim, totSim = computeDiameter(graph)

degreeMedianCsv = computeMedianDegree(graph)

print(f"assortativity: {assortativity}")
print(f"diam: {diamSim}")
print(f"degreeMedian: {degreeMedianCsv}")
