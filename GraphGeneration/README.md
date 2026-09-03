# Generazione di Grafi Casuali per la Lightning Network

> Questo modulo implementa la generazione di reti sintetiche progettate per simulare la topologia e la distribuzione di liquidità della Lightning Network (LN)[cite: 2]. Vengono proposti due modelli generativi, che differiscono principalmente per:
> - La logica con cui i nuovi nodi scelgono a chi connettersi.
> - La probabilità di chiusura (scomparsa) degli archi nel tempo[cite: 3].

---

## 1. Calcolo dei parametri per la distribuzione delle capacità

Dal punto di vista statistico, le capacità dei canali seguono una distribuzione congiunta mista:
* Una **Log-Normale** per quanto riguarda il corpo principale della distribuzione (i canali medi).
* Una **Power-Law** (legge di potenza) per quanto riguarda la coda (i canali giganti)[cite: 2].

Poiché i dati seguono questa distribuzione mista, è emersa la necessità di identificare con precisione la soglia $x_{min}$ al di sotto della quale i dati seguono la Log-Normale e al di sopra della quale seguono la Power-Law.

Questa problematica è stata risolta mediante il **metodo di Clauset**, che per ogni possibile valore di $x_{min}$ esegue le seguenti operazioni:

1.  **Calcolo dello Stimatore di Massima Verosimiglianza (MLE)** per il parametro $\alpha$, escludendo i dati al di sotto di $x_{min}$, mediante la formula:
    $$\widetilde{\alpha} = 1 + n\left[ \sum_{i=1}^n \ln \frac{x_i}{x_{min} - \frac{1}{2}}\right]^{-1}$$ 
2.  **Calcolo della distanza di Kolmogorov-Smirnov ($D$)** fra i dati rimanenti e la Power-Law teorica:
    $$D = \max_{x \geq x_{min}} \left\lvert S(x) - P(x)\right\rvert$$
    Dove $S(x)$ è la funzione di ripartizione empirica basata sui dati reali e $P(x)$ è la funzione di ripartizione teorica basata sui parametri $\widetilde{\alpha}$ e $x_{min}$.
3.  **Selezione della soglia ottimale:** Dopo aver valutato tutti i candidati, viene scelto l'$x_{min}$ che minimizza la distanza $D$.

Per quanto riguarda i canali che compongono il corpo della distribuzione (ovvero quelli con valore $< x_{min}$), i parametri della distribuzione Log-Normale, definita come $LogNormal(\mu, \sigma^2)$, sono stati ricavati mediante i rispettivi Stimatori di Massima Verosimiglianza:

$$\widetilde{\mu} = \frac{1}{N} \sum_{i=1}^{N} \ln(X_i)$$
$$\widetilde{\sigma}^2 = \frac{1}{N} \sum_{i=1}^{N} (\ln(X_i) - \widetilde{\mu})^2$$

Infine, è stata calcolata $p_{tail}$, ovvero la probabilità che un canale appartenga alla coda della distribuzione (e quindi generato tramite Power-Law):

$$p_{tail} = \frac{|\{X: X \geq x_{min}\}|}{\text{Numero totale dei canali}}$$ 

---

## 2. Primo Modello: Basato sui Gradi (Topologia)

**Intuizione:** Questo modello ipotizza che l'importanza di un nodo sia dettata puramente dalla sua connettività (numero di canali aperti). I nuovi nodi tendono a collegarsi a quelli già molto connessi (hub), mentre i canali tra nodi periferici hanno maggiore probabilità di venire chiusi rispetto a quelli che coinvolgono i grandi hub[cite: 3].

### Generazione della Topologia
Per riprodurre la struttura a rete *scale-free* della LN, viene impiegata una variante dinamica del modello generativo **Barabási–Albert**[cite: 2, 3].

Dato in input un grafo completo iniziale $G_0 = (V_0, E_0)$, ad ogni round $i$, il grafo $G_{i+1}$ viene generato seguendo questi passi:
*   Viene aggiunto l'$(i+1)$-esimo nodo al grafo: 
    $$V_{i+1} = V_i \cup \{v_{i+1}\}$$
*   Il nuovo nodo sceglie $d$ nodi a cui connettersi ($u_1, u_2, \dots, u_d$). Il nodo $u_j$ viene scelto con probabilità proporzionale al suo grado: $\frac{deg_{G_i}(u_j)}{2|E_i|}$.
 Di conseguenza, i nuovi archi vengono aggiunti all'insieme: 
    $$E_{i+1} = E_i \cup \{\{v_{i+1}, u_1\}, \dots, \{v_{i+1}, u_d\}\}$$
*   Ogni arco esistente $(u, v) \in E_i$ ha una probabilità di scomparire inversamente proporzionale ai gradi dei suoi estremi[cite: 3]: 
    $$q_i(u,v) = e^{-\beta \cdot \max{\{deg_{G_i}(u), deg_{G_i}(v)}\}}$$

### Generazione e Associazione delle Capacità
Una volta stabilita la topologia finale del grafo, il programma assegna le capacità ai singoli archi. L'obiettivo è simulare il mondo reale, dove le connessioni tra grandi hub concentrano la maggior quantità di liquidità. Il processo avviene in due fasi:

1.  **Generazione dei valori:** Per ogni arco del grafo, viene estratta una capacità casuale (con probabilità $p_{tail}$ dalla Power-Law, altrimenti dalla Log-Normale). L'intero set di capacità generate viene poi ordinato in modo decrescente.
2.  **Assegnazione per score:** Ad ogni arco $(u, v)$ viene assegnato un punteggio proporzionale alla connettività dei suoi capi: $Score = deg(u) \cdot deg(v)$. Gli archi vengono ordinati in base allo score decrescente. Le capacità più alte precedentemente generate vengono così assegnate ai canali con lo score più elevato.

---

## 3. Secondo Modello: Basato sulle Capacità (Liquidità)

**Intuizione:** A differenza del primo, questo modello assume che l'evoluzione della rete sia guidata dalla *liquidità reale* e non solo dal numero di connessioni. I nodi scelgono i propri partner in base ai fondi totali che questi gestiscono, e i canali scarsamente finanziati hanno una maggiore probabilità di venire chiusi[cite: 3].

### Generazione della Topologia
Anche in questo caso viene utilizzata una variante dinamica del modello **Barabási-Albert** orientata ai pesi (capacità) degli archi[cite: 3].

Dato in input un grafo completo $G_0 = (V_0, E_0, w)$ dove ogni arco presenta una capacità iniziale costante $w(e) = c \quad \forall e \in E_0, c \in \mathbb{N}$[cite: 3], ad ogni round $i$ il grafo $G_{i+1}$ viene aggiornato con le seguenti regole:

*   Vengono aggiunti $N_i \sim Geom(\frac{1}{10})$ nuovi nodi[cite: 3].
*   Ogni nuovo nodo $u$ seleziona $d$ nodi esistenti a cui connettersi[cite: 3]. La probabilità di scegliere un determinato nodo è direttamente proporzionale alla **somma delle capacità degli archi già incidenti** su quel nodo[cite: 3].
*   Per ogni nuovo arco $(u,v)$ generato, viene immediatamente estratta e assegnata una capacità (seguendo il metodo statistico descritto nel primo capitolo)[cite: 3].
*   Ogni arco esistente $e = (u,v)$ scompare con una probabilità basata sul rapporto tra la sua capacità e la capacità totale dei suoi nodi estremi[cite: 3]: 
    $$p(e) = \left[ \frac{w(e)}{w(u) + w(v)}\right]^\beta$$