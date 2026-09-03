#!/bin/bash

CSV_GRAPH="../Data/Graphs/firstModel.csv"

for beta in -1 0.3 0.5 0.7 0.9; do
    echo "--- Avvio simulazioni per beta = $beta ---"
    
    for n in $(seq 2000 2000 20000); do
        echo "Generazione grafo con n=$n, beta=$beta"
        
        ./GraphGeneration/graphGenerator -n $n -d 6 -beta $beta -file $CSV_GRAPH
        
        python3 DataAnalyzer/analizeGraph.py $n $beta
    done
done

echo "Tutte le simulazioni sono completate!"