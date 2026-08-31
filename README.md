# Lightning Network Generator

> Un framework per la **generazione sintetica**, l'**analisi di verosimiglianza** e il **benchmarking dei protocolli di routing** sulla Lightning Network.

---

## Panoramica del Progetto

Il progetto si propone tre obiettivi principali:
1. **Generazione di Grafi Randomici Empirici:** Creare grafi che approssimino il più possibile sia la topologia della rete reale che la sua distribuzione di liquidità: [$\rightarrow$](GraphGeneration/README.md)
   * **Topologia:** Sfrutta una variante del modello *Barabási-Albert* per simulare la struttura *scale-free* della rete.
   * **Capacità dei Canali:** Sfrutta un modello statistico Ibrido (Log-Normale + Power-Law)
2. **Analisi di Verosimiglianza:** Valutare e confrontare metricamente le reti generate rispetto ai dati reali della LN.
3. **Workbench di Routing:** Fornire un ambiente di test (*workbench*) per simulare e analizzare l'efficienza dei diversi algoritmi di routing sul grafo generato tenendo conto della liquidità effettiva dei canali.

---

### Analisi della Rete
Valutazione comparativa continua tra i grafi sintetici prodotti e la topologia reale della Lightning Network tramite metriche chiave (distribuzione dei gradi, assortatività, diametro).

---
