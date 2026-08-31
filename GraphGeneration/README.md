# Generazione di grafi casuali per la Lightning Network

## Generazione della Topologia (Archi)
Per riprodurre la struttura a rete scale-free (a invarianza di scala) della Lightning Network (LN), viene impiegata una variante del modello generativo **Barabási–Albert** basata sul paper [[1]](./Paper/graphGenerationWithElimination.pdf). Dato in input un grafo completo $G_0 = (V_0, E_0)$, ad ogni round $i$, il grafo $G_{i+1}$ viene generato secondo i seguenti passi:
  * Viene aggiunto l'$(i+1)$-esimo nodo al grafo: $$V_{i+1} = V_i \cup \{v_{i+1}\}$$
  * Il nodo $(i+1)$-esimo sceglie $d$ nodi a cui connettersi $u_1, u_2, \dots, u_d$, dove il nodo $u_j$ viene scelto con probabilità $\frac{deg_{G_i}(u_j)}{2|E_i|}$. Di conseguenza: $$E_{i+1} = E_i \cup \{\{v_{i+1}, u_1\}, \dots, \{v_{i+1}, u_d\}\}$$
  * Ogni arco $(u, v) \in E_i$ scompare con probabilità: $$q_i(u,v) = e^{-\beta \cdot \max{\{deg_{G_i}(u), deg_{G_i}(v)}\}}$$


## Calcolo dei parametri per la distribuzione delle capacità

Le capacità dei canali, come analizzato all'interno di [[2]](../Paper/distributionOfPowerLaw.pdf), seguono una distribuzione congiunta:
  * Una **LogNormal** per quanto riguarda il corpo della distribuzione.
  * Una **PowerLaw** per quanto riguarda la coda.

Poiché le capacità seguono questa distribuzione mista, è emersa la necessità di identificare la soglia $x_{min}$ al di sotto della quale i dati seguono la LogNormal e al di sopra della quale seguono la PowerLaw.

Questa problematica è stata risolta mediante il **metodo di Clauset** mostrato in [[3]](../Paper/powerLawInEmpiricalData.pdf), che per ogni possibile valore di $x_{min}$ esegue le seguenti operazioni:
  - Calcolo dello **Stimatore di Massima Verosimiglianza** (MLE) per $\alpha$, escludendo i dati al di sotto di $x_{min}$, mediante la formula:  $$\widetilde\alpha = 1 + n\left[ \sum_{i=1}^n \ln \frac{x_i}{x_{min} - \frac{1}{2}}\right]^{-1}$$ 
  - Calcolo della **distanza di Kolmogorov-Smirnov** fra i dati rimanenti e la PowerLaw teorica: $$D = \max_{x \geq x_{min}} \left\lvert S(x) - P(x)\right\rvert$$
  Dove $S(x)$ è la funzione di ripartizione empirica basata sui dati reali e $P(x)$ è la funzione di ripartizione teorica basata sui parametri $\widetilde\alpha$ e $x_{min}$.
  - Dopo aver valutato tutti i possibili valori candidati, viene scelto l'$x_{min}$ che minimizza la distanza $D$.

Per quanto riguarda invece le capacità dei canali che compongono il corpo della distribuzione (ovvero quelle con valore $< x_{min}$), i parametri della distribuzione $LogNormal(\mu, \sigma^2)$ sono stati ricavati mediante i rispettivi **Stimatori di Massima Verosimiglianza**:

$$\widetilde{\mu} = \frac{1}{N} \sum_{i=1}^{N} \ln(X_i)$$

$$\widetilde{\sigma}^2 = \frac{1}{N} \sum_{i=1}^{N} (\ln(X_i) - \widetilde{\mu})^2$$

Infine, dopo aver ricavato questi parametri, è stata calcolata $p_{tail}$, ovvero la probabilità che un canale appartenga alla coda della distribuzione (PowerLaw), mediante la formula:
$$ p_{tail} = \frac{|\{X: X \geq x_{min}\}|}{\text{Numero totale dei canali}}$$ 

## Generazione ed associazione delle capacità agli archi

Una volta stabilita la topologia finale del grafo e calcolati i parametri statistici, il programma procede con la generazione e l'assegnazione delle capacità ai singoli archi. L'obiettivo è fare in modo che le connessioni tra nodi altamente connessi (hub) ricevano le capacità maggiori.

Il processo avviene in due fasi principali:
1. **Generazione dei valori:** Per ogni arco presente nel grafo, viene generato un valore di capacità casuale. Con probabilità $p_{tail}$ il valore viene estratto dalla distribuzione PowerLaw, altrimenti viene estratto dalla distribuzione LogNormal. L'insieme di tutte le capacità generate viene poi ordinato in modo decrescente.
2. **Assegnazione per score di grado:** Ad ogni arco $(u, v)$ del grafo viene assegnato uno score calcolato come il prodotto dei gradi dei suoi estremi: $Score = deg(u) \times deg(v)$. Gli archi vengono quindi ordinati in modo decrescente in base a questo score. Infine, le capacità più alte generate al passo precedente vengono assegnate progressivamente agli archi con lo score più elevato.