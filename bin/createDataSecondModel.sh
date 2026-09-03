#!/bin/bash

CSV_GRAPH="Data/Graphs/secondModel.csv"

for beta in 1 2 2.5 3; do
    echo "--- Avvio simulazioni per beta = $beta ---"
    
    for n in $(seq 100 100 2000); do
        echo "Generazione grafo con n=$n, beta=$beta"
        
        ./bin/secondGraphGenerator -n $n -d 6 -beta $beta -file $CSV_GRAPH
        
        python3 DataAnalyzer/analizeGraph.py $n $beta
    done
done

echo "Tutte le simulazioni sono completate!"