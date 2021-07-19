# Multi-CPU-scheduler-simulator di Filippo Iacobelli
Come base del progetto ho utlizzato il codice fornito dal professore. Il primo passo è stato creare uno scheduler con un numero prefissato (>1) di CPU.
Successivamente ho esteso lo scheduler ad un numero arbitrario di CPU dichiarato dall'utente al momento del avvio dello scheduler.
Nel frattempo ho definito un maggior numero di stampe in modo da monitorare l'output del processo e verificare eventuali errori.
Ho deciso infine di inserire un secondo algoritmo di scheduling che sia preemptive ma basato sulla priorità. 
Per evitare starvation dei processi ho fatto in modo di inserire un sistema di ageing all'interno dello scheduler che dimunuisce la priorità del processi ogni due quanti.
Per cambiare algoritmo di scheduling da priority a round robin è sufficiente commentare e decommentare una porzione di codice.
Ovviamente per poter effettuare queste modifiche ho dovuto aggiungere alcuni campi nelle strutture dati già fornite dal professore.
