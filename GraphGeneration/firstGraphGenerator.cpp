#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <sstream>
#include <utility>
#include <set>
#include <cmath>
#include <algorithm>

using namespace std;

class Graph
{
private:
    vector<int> nodeSelector;
    set<pair<int, int>> edgeSet;
    unordered_map<int, unordered_set<int>> indexMap;

    unordered_map<string, long double> getLNParameters(string fileName);

public:
    unordered_map<int, unordered_map<int, int>> adjacentList;
    unordered_map<int, int> degreeMap;
    int numberOfNodes;

    Graph(int n, int d, double beta);
    Graph(int n, int d);
    void addEdge(int source, int target, int capacity);
    void removeEdge(int source, int target);
    void createClique(int m0);
    void associateCapacities(unordered_map<string, long double> parametersDict);

    void exportCsv(const string &outputPath);
};

Graph::Graph(int n, int d, double beta)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> realDistribution(0.0, 1.0);

    this->numberOfNodes = n;

    // Inizializza hashmap dei gradi dei nodi
    for (int i = 0; i < n; i++)
    {
        this->degreeMap[i] = 0;
    }

    unordered_map<int, double> expCache;

    for (int k = 1; k <= n; k++)
    {
        expCache[k] = exp(-beta * k);
    }

    this->createClique(d + 1);

    for (int newSource = d + 1; newSource < n; newSource++)
    {

        // cout << "Processamento nodo " << newSource << endl;

        vector<pair<int, int>> edgeToEliminate;

        for (auto [source, target] : this->edgeSet)
        {
            int sourceDegree = this->degreeMap[source];
            int targetDegree = this->degreeMap[target];

            int maxDegree = max(sourceDegree, targetDegree);

            double q = expCache[maxDegree];

            if (realDistribution(gen) < q)
            {
                edgeToEliminate.push_back({source, target});
            }
        }

        for (auto [source, target] : edgeToEliminate)
        {
            this->removeEdge(source, target);
        }

        unordered_set<int> targets;

        while (targets.size() < d)
        {
            uniform_int_distribution<int> targetDistribution(0, this->nodeSelector.size() - 1);
            int targetIndex = targetDistribution(gen);
            targets.insert(this->nodeSelector[targetIndex]);
        }

        for (int newTarget : targets)
        {
            this->addEdge(newSource, newTarget, 0);
        }
    }

    string fileName = "Data/Network/parameterNetworks.csv";
    unordered_map<string, long double> dictOfParameters = this->getLNParameters(fileName);

    this->associateCapacities(dictOfParameters);
}

Graph::Graph(int n, int d)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> realDistribution(0.0, 1.0);

    this->numberOfNodes = n;

    for (int i = 0; i < n; i++)
    {
        this->degreeMap[i] = 0;
    }

    this->createClique(d + 1);

    for (int newSource = d + 1; newSource < n; newSource++)
    {

        // cout << "Processamento nodo " << newSource << endl;

        unordered_set<int> targets;

        while (targets.size() < d)
        {
            uniform_int_distribution<int> targetDistribution(0, this->nodeSelector.size() - 1);
            int targetIndex = targetDistribution(gen);
            targets.insert(this->nodeSelector[targetIndex]);
        }

        for (int newTarget : targets)
        {
            this->addEdge(newSource, newTarget, 0);
        }
    }

    string fileName = "Data/Network/parameterNetworks.csv";
    unordered_map<string, long double> dictOfParameters = this->getLNParameters(fileName);

    this->associateCapacities(dictOfParameters);
}

void Graph::createClique(int m0)
{
    for (int source = 0; source < m0; source++)
    {
        for (int target = source + 1; target < m0; target++)
        {
            this->addEdge(target, source, 0);
        }
    }
}

void Graph::addEdge(int source, int target, int capacity)
{
    this->adjacentList[source][target] = capacity;
    this->adjacentList[target][source] = capacity;

    this->degreeMap[source] += 1;
    this->degreeMap[target] += 1;

    this->nodeSelector.push_back(source);
    this->indexMap[source].insert(this->nodeSelector.size() - 1);

    this->nodeSelector.push_back(target);
    this->indexMap[target].insert(this->nodeSelector.size() - 1);

    this->edgeSet.insert({source, target});
}

void Graph::removeEdge(int source, int target)
{
    this->adjacentList[source].erase(target);
    this->adjacentList[target].erase(source);

    this->degreeMap[source] -= 1;
    this->degreeMap[target] -= 1;

    // Rimuoviamo l'arco dal set
    this->edgeSet.erase({source, target});

    // Lambda helper per fare Swap & Pop in sicurezza senza rompere indexMap
    auto removeNodeOccurrence = [this](int node)
    {
        if (this->indexMap[node].empty())
            return;

        int nodeIndex = this->indexMap[node].extract(this->indexMap[node].begin()).value();
        int lastIndex = this->nodeSelector.size() - 1;
        int lastNode = this->nodeSelector[lastIndex];

        swap(this->nodeSelector[nodeIndex], this->nodeSelector[lastIndex]);
        this->nodeSelector.pop_back();

        // Aggiorniamo indexMap solo se abbiamo effettivamente spostato un elemento diverso
        if (nodeIndex != lastIndex)
        {
            this->indexMap[lastNode].erase(lastIndex);
            this->indexMap[lastNode].insert(nodeIndex);
        }
    };

    // Eseguiamo il remove separatamente e in sequenza pulita
    removeNodeOccurrence(source);
    removeNodeOccurrence(target);
}

void Graph::associateCapacities(unordered_map<string, long double> parametersDict)
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> unif(0.0, 1.0);
    lognormal_distribution<double> logNormal(parametersDict["mu"], parametersDict["sigma"]);

    vector<long long> capacities;

    for (int i = 0; i < this->edgeSet.size(); i++)
    {
        double p = unif(gen);
        double cap = 0.0;

        if (p < parametersDict["p_tail"])
        {
            double uNew = unif(gen);
            cap = parametersDict["x_min"] * pow(1.0 - uNew, -1.0 / (parametersDict["alpha"] - 1.0));
        }
        else
        {
            cap = logNormal(gen);
        }

        capacities.push_back(static_cast<long long>(round(cap)));
    }

    sort(capacities.rbegin(), capacities.rend());

    vector<pair<double, pair<int, int>>> scoreEdges;

    for (auto const &[source, target] : this->edgeSet)
    {
        double score = this->degreeMap[source] * this->degreeMap[target];
        scoreEdges.push_back({score, {source, target}});
    }

    sort(scoreEdges.rbegin(), scoreEdges.rend());

    for (int i = 0; i < scoreEdges.size(); i++)
    {
        int source = scoreEdges[i].second.first;
        int target = scoreEdges[i].second.second;

        long long capacity = capacities[i];

        this->adjacentList[source][target] = capacity;
        this->adjacentList[target][source] = capacity;
    }
}

unordered_map<string, long double> Graph::getLNParameters(string fileName)
{
    ifstream file(fileName);

    string headerLine, valueLine;

    getline(file, headerLine);
    getline(file, valueLine);

    vector<string> keys;
    vector<string> values;

    string token;

    stringstream headerStream(headerLine);
    while (getline(headerStream, token, ';'))
    {
        keys.push_back(token);
    }

    stringstream valueStream(valueLine);
    while (getline(valueStream, token, ';'))
    {
        values.push_back(token);
    }

    unordered_map<string, long double> dictOfParameters;

    for (int i = 0; i < values.size(); i++)
    {
        dictOfParameters[keys[i]] = stold(values[i]);
    }

    return dictOfParameters;
}

void Graph::exportCsv(const string &outputPath)
{
    ofstream outfile(outputPath);
    if (!outfile.is_open())
    {
        cerr << "Errore: impossibile aprire o creare il file " << outputPath << endl;
        return;
    }

    outfile << "Source;Target;DegreeSource;Degreetarget;Capacities\n";

    for (auto const &[u, neighbours] : this->adjacentList)
    {
        for (auto const &[v, weight] : neighbours) // Estraggo la coppia (vicino, peso)
        {
            if (u < v)
            {
                outfile << u << ";" << v << ";"
                        << this->degreeMap.at(u) << ";"
                        << this->degreeMap.at(v) << ";"
                        << weight << endl; // Opzionale se in futuro vuoi salvare il peso
            }
        }
    }

    outfile.close();
}

int main(int argc, char *argv[])
{
    // Valori di default
    int n = 100000;
    int d = 6;
    double beta = 0.6;
    string outputPath = "Data/Graphs/grafoBarabasi.csv";

    // Parsing dei parametri non posizionali
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            string arg = argv[i];

            if (arg == "-h" || arg == "--help")
            {
                cout << "Uso: " << argv[0] << " [OPZIONI]\n\n";
                cout << "Opzioni disponibili:\n";
                cout << "  -n <int>        : Numero di nodi totali (default: 10000)\n";
                cout << "  -d <int>        : Archi per nuovo nodo (default: 6)\n";
                cout << "  -beta <double>  : Parametro beta (default: 0.4), se -1 avverrà la generazione secondo il modello classico di BA\n";
                cout << "  -file <string>  : Percorso file CSV (default: Data/Graphs/grafoBarabasi.csv)\n";
                cout << "  -h, --help      : Mostra questo messaggio di aiuto\n";
                return 0;
            }
            else if (arg == "-n" && i + 1 < argc)
            {
                n = stoi(argv[++i]);
            }
            else if (arg == "-d" && i + 1 < argc)
            {
                d = stoi(argv[++i]);
            }
            else if (arg == "-beta" && i + 1 < argc)
            {
                beta = stod(argv[++i]);
            }
            else if (arg == "-file" && i + 1 < argc)
            {
                outputPath = argv[++i];
            }
            else
            {
                cerr << "Opzione o argomento non valido: " << arg << endl;
                cerr << "Usa '" << argv[0] << " --help' per le opzioni disponibili." << endl;
                return 1;
            }
        }
    }
    catch (const exception &e)
    {
        cerr << "Errore nella conversione del valore per una delle opzioni: " << e.what() << endl;
        return 1;
    }

    cout << "Avvio generazione con parametri:\n"
         << " - n: " << n << "\n"
         << " - d: " << d << "\n"
         << " - beta: " << beta << "\n"
         << " - Path: " << outputPath << "\n\n";

    if (beta != -1)
    {
        Graph g(n, d, beta);
        g.exportCsv(outputPath);
    }
    else
    {
        Graph g(n, d);
        g.exportCsv(outputPath);
    }

    return 0;
}
