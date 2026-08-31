import json
from collections import Counter
from typing import List, Tuple, Dict
import pandas as pd
import numpy as np
import igraph as ig


def generateFile(listEdges: List[List[int]], outputName: str) -> None:
    dataFrame = pd.DataFrame(
        listEdges,
        columns=["Source", "Target", "DegreeSource", "DegreeTarget", "Capacity"]
    )
    dataFrame.to_csv(outputName, index=False, sep=";", encoding="utf-8")

def saveParameter(dictOfParameter: dict, outputName: str) -> None:
    dataFrame = pd.DataFrame([dictOfParameter])
    dataFrame.to_csv(outputName, index=False, sep=";", encoding="utf-8")


def extrapolateInformation(fileName: str) -> List[List[int]]:
    with open(fileName, "r", encoding="utf-8") as file:
        data = json.load(file)

    nodeDict = {node['pub_key']: index for index, node in enumerate(data['nodes'])}
    print(f"Totale nodi nel JSON: {len(data['nodes'])}")

    degreeDict = Counter()
    for edge in data['edges']:
        degreeDict[edge['node1_pub']] += 1
        degreeDict[edge['node2_pub']] += 1

    listEdges = []
    for edge in data['edges']:
        startKey = edge['node1_pub']
        endKey = edge['node2_pub']

        sourceDegree = degreeDict[startKey]
        targetDegree = degreeDict[endKey]
        capacity = edge['capacity']

        source = nodeDict[startKey]
        target = nodeDict[endKey]

        listEdges.append([
            int(source), 
            int(target), 
            int(sourceDegree), 
            int(targetDegree), 
            int(capacity)
        ])

    return listEdges


def computeEstimates(listEdges: List[List[int]]) -> Tuple[float, float]:
    capacitiesList = [edge[4] for edge in listEdges]
    capacities = np.array(capacitiesList, dtype=float)
    capacities = capacities[capacities > 0]

    logCaps = np.log(capacities)

    mu = float(np.mean(logCaps))
    sigma = float(np.std(logCaps, ddof=0))

    return mu, sigma


def analyzeDoubleEdges(listEdges: List[List[int]]) -> bool:
    edgeSet = set()

    for source, target, _, _, _ in listEdges:
        edge = (min(source, target), max(source, target))
        if edge in edgeSet:
            print(f"Doppio arco trovato tra i nodi: {source} e {target}")
            return True
        edgeSet.add(edge)

    return False


def computeAssortativity(listEdges: List[List[int]]) -> Tuple[float, float]:
    df = pd.DataFrame(
        listEdges,
        columns=["Source", "Target", "DegreeSource", "DegreeTarget", "Capacity"]
    )

    degSource = df['DegreeSource'].values
    degTarget = df['DegreeTarget'].values

    kx = np.concatenate([degSource, degTarget])
    ky = np.concatenate([degTarget, degSource])

    meanK = np.mean(kx)
    varianceK = np.var(kx)
    covarianceK = np.mean((kx - meanK) * (ky - meanK))

    rFormula = float(covarianceK / varianceK) if varianceK != 0 else 0.0

    edges = [(row[0], row[1]) for row in listEdges]
    G = ig.Graph(edges=edges, directed=False)
    
    rIgraph = G.assortativity_degree(directed=False)

    return float(rFormula), float(rIgraph)


def computeDiameter(listEdges: List[List[int]]) -> tuple[int, int, int]:
    edges = [(row[0], row[1]) for row in listEdges]
    G = ig.Graph.TupleList(edges, directed=False)

    totalNodes = G.vcount()
    if totalNodes == 0:
        return 0, 0, 0
        
    components = G.connected_components(mode="weak")
    lccG = components.giant()
    diameter = int(lccG.diameter(directed=False))
    return diameter, lccG.vcount(), totalNodes


def computeMedianDegree(listEdges: list[List[int]]) -> float:
    nodeMap = {}
    for source, target, sourceDegree, targetDegree, _ in listEdges:
        if source not in nodeMap:
            nodeMap[source] = sourceDegree
        if target not in nodeMap:
            nodeMap[target] = targetDegree

    degreeList = np.array(list(nodeMap.values()))

    return np.median(degreeList)


def computeNetworkParameters(listEdges: List[List[int]]) -> Dict[str, float]:
    """
    Calcola tutti i parametri necessari per modellare le capacità dei canali LN.
    - Coda (Power-Law): x_min e alpha tramite metodo di Clauset.
    - Corpo (Log-Normal): mu e sigma sui dati < x_min.
    """
    if not listEdges:
        raise ValueError("La lista degli archi è vuota.")

    capacities = np.array([edge[4] for edge in listEdges if edge[4] > 0], dtype=np.float64)
    capacitiesSorted = np.sort(capacities)
    totalN = len(capacitiesSorted)
    
    xminsCandidates = np.unique(capacitiesSorted)
    
    bestD = np.inf
    bestXmin = None
    bestAlpha = None
    bestTailCount = 0

    print(f"Ricerca x_min su {len(xminsCandidates)} candidati unici...")

    for xmin in xminsCandidates:
        startIdx = np.searchsorted(capacitiesSorted, xmin)
        dataTail = capacitiesSorted[startIdx:]
        n = len(dataTail)

        if n < 50:
            break

        logRatios = np.log(dataTail / (xmin - 0.5))
        sumLog = np.sum(logRatios)
        if sumLog == 0.0:
            continue
            
        alpha = 1.0 + (n / sumLog)

        uniqueVals, counts = np.unique(dataTail, return_counts=True)
        
        cdfEmpirical_right = np.cumsum(counts) / n
        cdfEmpirical_left = np.insert(cdfEmpirical_right[:-1], 0, 0.0)
        
        cdfTheoretical = 1.0 - (uniqueVals / xmin) ** (-(alpha - 1.0))

        D_right = np.abs(cdfEmpirical_right - cdfTheoretical)
        D_left = np.abs(cdfEmpirical_left - cdfTheoretical)
        D = max(np.max(D_right), np.max(D_left))

        if D < bestD:
            bestD = D
            bestXmin = xmin
            bestAlpha = alpha
            bestTailCount = n

    bodyData = capacitiesSorted[capacitiesSorted < bestXmin]
    
    if len(bodyData) > 0:
        logCaps = np.log(bodyData)
        mu = float(np.mean(logCaps))
        sigma = float(np.std(logCaps, ddof=0))
    else:
        mu, sigma = 0.0, 0.0 

    p_tail = bestTailCount / totalN

    return {
        "x_min": float(bestXmin),
        "alpha": float(bestAlpha),
        "mu": mu,
        "sigma": sigma,
        "p_tail": float(p_tail)
    }


if __name__ == "__main__":
    jsonPath = "Data/Network/realNetwork.json"
    parameterPath = "Data/Network/parameterNetworks.csv"
    
    listEdges = extrapolateInformation(jsonPath)

    #generateFile(listEdges, "Data/Network/lightningNetworkEdges.csv")

    dictOfValues = computeNetworkParameters(listEdges)

    mu, sigma = computeEstimates(listEdges)
    dictOfValues["mu"] = mu
    dictOfValues["sigma"] = sigma

    # hasDoubles = analyzeDoubleEdges(listEdges)

    rFormula, rIgraph = computeAssortativity(listEdges)
    dictOfValues["assortativity"] = rFormula

    diameter, lccNodes, totalNodes = computeDiameter(listEdges)
    dictOfValues["diameter"] = diameter

    medianDegree = computeMedianDegree(listEdges)
    dictOfValues["medianDegree"] = medianDegree

    saveParameter(dictOfValues, parameterPath)


    