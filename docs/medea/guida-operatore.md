# Guida per l'operatore

Questa guida spiega come usare la dashboard del rover agricolo MEDEA per preparare un lavoro,
avviarlo e seguirlo. Non servono conoscenze di informatica o di ROS.

> **Bozza.** La guida descrive la versione in sviluppo (ottobre 2026). Il collegamento con il rover
> passa da un programma ponte (bridge) provato finora con un rover simulato: alcune schermate e alcuni
> nomi cambieranno. Le immagini vengono dal rover simulato.

## Prima di tutto: la sicurezza

- **La dashboard non è l'arresto di emergenza.** Per fermare subito il rover usa il pulsante di
  arresto di emergenza del rover (o del radiocomando), come indicato dal CREA.
- Prima di avviare un lavoro controlla che nell'area non ci siano persone o animali, che il rover sia
  in vista e che la batteria sia carica.

## Avviare il programma

Apri **QGroundControl** dall'icona sul desktop o sul tablet. Si apre la schermata **Guida**, con la
mappa. Quando il rover è acceso e collegato alla rete, il programma si collega da solo.

## La schermata Guida

![Schermata Guida con il rover collegato e un lavoro caricato](img/operatore-guida.jpg)

*Nell'immagine il rover è simulato e il video a barre colorate è un'immagine di prova.*

1. **Pulsante Q** (in alto a sinistra): apre il menu.
2. **Stato del rover**, accanto alla Q:
   - *Disconnesso*: il rover non è collegato;
   - *Pronto*: il rover è collegato, con i motori spenti;
   - *Motori accesi*: il rover lavora, è in pausa o sta tornando.

   Più a destra c'è la **modalità**: *Fermo*, *Lavoro*, *Pausa*, *Rientro*, *Manuale* (guidato con il
   radiocomando) o *Emergenza*. Stato e modalità servono solo da leggere: toccarli non apre niente.
3. **Indicatori** (in alto a destra): GPS (numero di satelliti e precisione) e **batteria** in
   percentuale. Tocca un indicatore per vedere i dettagli.
4. **Mappa**: la freccia rossa è il rover; la linea arancione con i numeri è il lavoro caricato sul
   rover; il numero verde è il punto verso cui il rover sta andando; la linea rossa è il tragitto già
   fatto; la casetta verde è il punto di partenza.
5. **Video** della camera (in basso a sinistra). Tocca il riquadro per scambiare video e mappa.
   *IN ATTESA DEL VIDEO* vuol dire che la camera non sta inviando immagini.
6. **Valori** (in basso a destra): velocità, distanza dal punto di partenza, tempo di lavoro, distanza
   percorsa; accanto c'è la bussola.
7. **Pulsanti del rover** (in basso al centro): compaiono solo quelli utili in quel momento, vedi
   [Eseguire il lavoro](#eseguire-il-lavoro).
8. **Pulsante rosso** (in alto a destra): registra il video sul computer; *Foto* ne salva
   un'immagine.

## Il menu

![Menu dell'operatore](img/operatore-menu.jpg)

Tocca la **Q** in alto a sinistra:

- **Guida**: la schermata per seguire il rover.
- **Pianifica**: per preparare un lavoro.
- **Chiudi**: chiude il programma (se c'è un lavoro non salvato chiede conferma).

Le impostazioni sono riservate al tecnico e non compaiono nel menu.

## Preparare un lavoro

Apri il menu e scegli **Pianifica**.

### 1. Indica il punto di partenza

Il pannello a destra dice *Clicca sulla mappa per impostare la posizione*: tocca la mappa dove si
trova il rover (di solito vicino alla freccia rossa). Compare la casetta verde, che puoi trascinare.

### 2. Scegli il tipo di lavoro

![Modelli di piano disponibili per il rover](img/operatore-modelli.jpg)

Sotto *Modelli di piano* scegli:

| Tipo di lavoro | Quando usarlo |
| --- | --- |
| **Percorso** | Il rover segue una strada, un filare o una linea che disegni tu. |
| **Perimetro** | Il rover gira lungo il bordo di un campo o di un'area. |
| **Survey** | Il rover copre tutta un'area con passate parallele (in futuro si chiamerà diversamente). |
| **Nessun modello** | Piano vuoto: aggiungi tu i punti da raggiungere, uno per uno. |

*Scansione corridoio* è pensata per i droni: per ora non serve per il rover.

Percorso, Perimetro e Survey aggiungono alla fine **Ritorno alla partenza**: finito il lavoro, il
rover torna al punto di partenza. Con *Nessun modello* lo aggiungi tu con il pulsante **Rientra**
della barra a sinistra.

### 3a. Percorso

![Un percorso con andata e ritorno](img/operatore-percorso.jpg)

1. Scegli **Percorso**. In alto compaiono gli *Strumenti polilinea*.
2. Premi **Traccia** e tocca sulla mappa i punti del percorso, nell'ordine in cui il rover deve
   passarci. Quando hai finito premi **Fine tracciamento**.
   In alternativa **Base** crea un percorso di esempio da modificare, e **Carica KML/SHP...** carica un
   percorso preparato con un altro programma.
3. Per correggere il percorso:
   - trascina i pallini bianchi per spostare i punti;
   - tocca il **+** a metà di un tratto per aggiungere un punto;
   - tocca un punto e scegli *Rimuovi vertice* per toglierlo.
4. Nel pannello a destra puoi scegliere:
   - **Andata e ritorno**: arrivato in fondo, il rover torna indietro lungo lo stesso percorso;
   - **Ripetizioni**: quante volte ripetere il percorso.

   Sotto *Statistiche* trovi la lunghezza del percorso e la distanza totale da percorrere.

### 3b. Perimetro

![Un perimetro con distanza dal bordo di 20 m](img/operatore-perimetro.jpg)

1. Scegli **Perimetro**. In alto compaiono gli *Strumenti poligono*.
2. Disegna l'area:
   - **Base** crea un rettangolo: trascina gli angoli sui confini del campo;
   - **Traccia**: tocca gli angoli del campo uno dopo l'altro, poi **Fine tracciamento**;
   - **Carica KML/SHP...** carica un confine preparato con un altro programma.
3. Nel pannello a destra puoi scegliere:
   - **Distanza dal bordo**: quanti metri più all'interno del confine deve passare il rover (per
     esempio per stare lontano da una recinzione). Il giro del rover è la linea sottile dentro l'area;
   - **Giri**: quante volte girare intorno all'area;
   - **Senso opposto**: gira nell'altro verso;
   - **Cambia punto di partenza**: sposta l'angolo da cui il rover comincia il giro.

Se la distanza dal bordo è troppo grande per l'area compare un avviso: riducila, altrimenti il lavoro
non si può salvare né caricare sul rover.

### 3c. Punti a mano (waypoint)

1. Scegli **Nessun modello**.
2. Premi **Waypoint** nella barra a sinistra e tocca sulla mappa i punti da raggiungere, in ordine.
   Ogni tocco aggiunge un punto numerato. Premi di nuovo **Waypoint** per smettere di aggiungere punti.

### 3d. Survey (copertura di un'area)

Disegna l'area come per il perimetro. Il rover la percorre con passate parallele: **Passo** è la
distanza tra una passata e l'altra, **Angolo** è la direzione delle passate. Questa schermata ha
ancora molte opzioni da drone e verrà semplificata.

### 4. Salva il lavoro (facoltativo)

**Salva** (in alto) salva il lavoro in un file, per riusarlo un altro giorno; **Apri** lo riapre.
**Cancella** svuota il piano (chiede conferma).

### 5. Carica il lavoro sul rover

Premi **Carica sul veicolo** (in alto). Il pulsante è evidenziato quando il lavoro sullo schermo è
diverso da quello sul rover. Appena il rover ha ricevuto il lavoro, il programma passa da solo alla
schermata **Guida**, da dove lo avvii.

Se compare *Il piano ha elementi incompleti*, un elemento non è finito: nell'elenco a destra è
segnato con un punto interrogativo rosso. Di solito manca da disegnare il percorso o l'area.

## Eseguire il lavoro

![I pulsanti del rover: da fermo, durante il lavoro e in pausa](img/operatore-pulsanti.jpg)

1. Dopo **Carica sul veicolo** sei già nella schermata **Guida** (altrimenti: menu → **Guida**).
   Controlla sulla mappa che la linea arancione sia il lavoro giusto.
2. Premi **Avvia lavoro** e conferma con **Sì**. Il rover si mette in moto da solo: lo stato diventa
   *Motori accesi* e la modalità *Lavoro*. Segui il lavoro sulla mappa e nel video.
3. Durante il lavoro i pulsanti in basso sono:
   - **Pausa**: il rover si ferma e aspetta (modalità *Pausa*). **Riprendi** lo fa ripartire da dove si
     era fermato;
   - **Torna alla partenza** (chiede conferma): il rover interrompe il lavoro, torna al punto di
     partenza (modalità *Rientro*) e si ferma;
   - **Stop**: il rover si ferma subito, spegne i motori e il lavoro finisce. *Avvia lavoro* lo rifà
     dall'inizio.
4. Finito il lavoro il rover torna alla partenza e si ferma: modalità *Fermo*, stato *Pronto*. Il lavoro
   resta sul rover: per ripeterlo basta premere di nuovo **Avvia lavoro**.

Se il rover non può eseguire un comando, in alto compare un riquadro giallo con il motivo, per esempio
*Avvia lavoro: nessun lavoro caricato*. Toccalo per chiuderlo.

Pausa e Stop agiscono subito, senza conferma, perché fermano il rover. **Stop non è l'arresto di
emergenza**: in caso di pericolo usa il pulsante di emergenza sul rover.

## Problemi comuni

| Cosa vedi | Cosa fare |
| --- | --- |
| *Disconnesso - Clicca per connetterti manualmente* | Il rover non è collegato. Controlla che sia acceso e che il computer o il tablet sia sulla rete del rover. Se il problema resta, chiama il tecnico. |
| *IN ATTESA DEL VIDEO* | La camera non invia immagini. Controlla che sia accesa; se il problema resta, chiama il tecnico. |
| *Il piano ha elementi incompleti* | Finisci di disegnare l'elemento segnato con il punto interrogativo rosso. |
| *Nessun lavoro sul rover* (sopra i pulsanti) o *nessun lavoro caricato* | Prepara il lavoro e caricalo: *Pianifica* → **Carica sul veicolo**. |
| Riquadro giallo con un messaggio del rover | Leggi il motivo: il rover non ha eseguito il comando. Toccalo per chiuderlo. |
| *il rover non ha risposto* o *Il rover non è collegato* | Controlla che il rover sia acceso e in rete, poi riprova. Se il problema resta, chiama il tecnico. |
| Avviso sulla distanza dal bordo (Perimetro) | Riduci la *Distanza dal bordo*. |
| La batteria diventa rossa | Fai rientrare il rover e ricaricalo. |
| Serve cambiare un'impostazione (video, collegamento, lingua...) | Chiama il tecnico: le impostazioni sono nascoste di proposito. |
