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

struct PairHash
{
    template <class T1, class T2>
    size_t operator()(const pair<T1, T2> &p) const
    {
        auto h1 = hash<T1>{}(p.first);
        auto h2 = hash<T2>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};

class Graph
{
private:
    vector<long> nodeSelector;
    unordered_map<pair<int, int>, long, PairHash> edgeSelector;
    unordered_map<string, long double> dictOfParameters;

    mt19937 gen;

    unordered_map<string, long double> getLNParameters(string fileName);

    pair<int, int> makeEdge(int u, int v)
    {
        return {min(u, v), max(u, v)};
    }

public:
    unordered_map<int, unordered_map<int, long>> adjacentList;
    int numberOfNodes;
    unordered_map<int, int> degreeMap;

    Graph(int n, int d, double beta);
    void addEdge(int source, int target, long capacity);
    void removeEdge(int source, int target);
    void createClique(int m0);
    long generateCapacity();

    vector<int> targetSelector(int d);

    void exportCsv(const string &outputpath);
};

Graph::Graph(int n, int d, double beta) : gen(random_device{}())
{
    geometric_distribution<int> newNodeGenerator(0.10);
    uniform_real_distribution<double> eliminateGenerator(0.0, 1.0);

    this->dictOfParameters = this->getLNParameters("Data/Network/parameterNetworks.csv");

    this->createClique(10);
    this->numberOfNodes = 10;

    for (int i = 0; i < n; i++)
    {
        int newNodes = newNodeGenerator(this->gen);
        int totalNodes = newNodes + this->numberOfNodes;

        while (this->numberOfNodes < totalNodes)
        {
            int source = this->numberOfNodes;
            this->numberOfNodes++;

            vector<int> newTargets = this->targetSelector(d);

            for (int target : newTargets)
            {
                if (source != target)
                {
                    long newCapacity = this->generateCapacity();
                    this->addEdge(source, target, newCapacity);
                }
            }
        }

        vector<pair<int, int>> edgesToRemove;
        for (const auto &[edge, capacity] : this->edgeSelector)
        {
            int source = edge.first;
            int target = edge.second;

            long sumWeights = this->nodeSelector[source] + this->nodeSelector[target];
            if (sumWeights == 0)
                continue;

            double probToEspirate = pow(static_cast<double>(capacity) / sumWeights, beta);

            double q = eliminateGenerator(this->gen);

            if (q < probToEspirate)
            {
                edgesToRemove.push_back({source, target});
            }
        }

        for (const auto &edge : edgesToRemove)
        {
            this->removeEdge(edge.first, edge.second);
        }
    }
}

void Graph::removeEdge(int source, int target)
{
    pair<int, int> edgeKey = makeEdge(source, target);
    long capacity = this->adjacentList[source][target];

    this->nodeSelector[source] -= capacity;
    if (this->nodeSelector[source] < 0)
    {
        this->nodeSelector[source] = 0;
    }

    this->nodeSelector[target] -= capacity;
    if (this->nodeSelector[target] < 0)
    {
        this->nodeSelector[target] = 0;
    }

    this->adjacentList[source].erase(target);
    this->adjacentList[target].erase(source);

    this->edgeSelector.erase(edgeKey);

    this->degreeMap[source] -= 1;
    this->degreeMap[target] -= 1;
}

vector<int> Graph::targetSelector(int d)
{
    vector<int> targets;

    vector<long> nodeSelectorPartial;
    nodeSelectorPartial.reserve(this->nodeSelector.size() + 1);
    nodeSelectorPartial.push_back(0);

    long totalSum = 0;
    for (long value : this->nodeSelector)
    {
        totalSum += value;
        nodeSelectorPartial.push_back(totalSum);
    }

    uniform_int_distribution<long> targetDistribution(0, totalSum - 1);

    for (int i = 0; i < d; i++)
    {
        long randomValue = targetDistribution(gen);

        auto it = upper_bound(nodeSelectorPartial.begin(), nodeSelectorPartial.end(), randomValue);

        int targetNode = distance(nodeSelectorPartial.begin(), it) - 1;

        targets.push_back(targetNode);
    }

    return targets;
}

void Graph::createClique(int m0)
{
    long capacity = 10000000;
    for (int source = 0; source < m0; source++)
    {
        for (int target = source + 1; target < m0; target++)
        {
            this->addEdge(source, target, capacity);
        }
    }
}

void Graph::addEdge(int source, int target, long capacity)
{
    this->adjacentList[source][target] = capacity;
    this->adjacentList[target][source] = capacity;

    this->edgeSelector[makeEdge(source, target)] = capacity;

    int maxNode = max(source, target);

    if (maxNode >= this->nodeSelector.size())
    {
        this->nodeSelector.resize(maxNode + 1, 0);
    }

    this->nodeSelector[source] += capacity;
    this->nodeSelector[target] += capacity;

    if (this->degreeMap.find(source) != this->degreeMap.end())
    {
        this->degreeMap[source]++;
    }
    else
    {
        this->degreeMap[source] = 1;
    }
    if (this->degreeMap.find(target) != this->degreeMap.end())
    {
        this->degreeMap[target]++;
    }
    else
    {
        this->degreeMap[target] = 1;
    }
}

long Graph::generateCapacity()
{
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> unif(0.0, 1.0);
    lognormal_distribution<double> logNormal(this->dictOfParameters["mu"], this->dictOfParameters["sigma"]);

    vector<long long> capacities;

    double p = unif(gen);
    double cap = 0.0;

    if (p < this->dictOfParameters["p_tail"])
    {
        double uNew = unif(gen);
        cap = this->dictOfParameters["x_min"] * pow(1.0 - uNew, -1.0 / (this->dictOfParameters["alpha"] - 1.0));
    }
    else
    {
        cap = logNormal(gen);
    }

    return static_cast<long>(round(cap));
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

int main()
{
    Graph g(2000, 7, 2);
    g.exportCsv("Data/Graphs/provaSecondo.csv");
    return 0;
}