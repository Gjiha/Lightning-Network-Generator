# Lightning Network Generator

> Un framework per la **generazione sintetica** e l'**analisi di verosimiglianza*

---

## Panoramica del Progetto

Il progetto si propone due obiettivi principali:
1. **Generazione di Grafi Randomici Empirici:** Creare grafi che approssimino il più possibile sia la topologia della rete reale che la sua distribuzione di liquidità: [$\rightarrow$](GraphGeneration/README.md)
   * **Topologia:** Sfrutta una variante del modello *Barabási-Albert* per simulare la struttura *scale-free* della rete.
   * **Capacità dei Canali:** Sfrutta un modello statistico Ibrido (Log-Normale + Power-Law)
2. **Analisi di Verosimiglianza:** Valutare e confrontare metricamente le reti generate rispetto ai dati reali della LN.

