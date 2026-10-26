# Changelog — QGroundControl per il rover MEDEA

Modifiche del fork `IntelligentSystemsLabUTV/qgroundcontrol`, ramo `medea-hmi`, rispetto a
QGroundControl **v5.1.5** (commit upstream `3a67d31`). Il fork è la dashboard (HMI) del rover agricolo
autonomo del progetto MEDEA (Tor Vergata DICII – DAUSY, con CREA-IT).

Guide d'uso: [guida per l'operatore](guida-operatore.md), [guida per il tecnico](guida-tecnico.md).

## Principi seguiti

- **Utenti finali:** agricoltori che non usano ROS. Le prime schermate devono essere semplici e in
  italiano; tutto ciò che è tecnico sta dietro la **modalità avanzata** (modalità tecnico).
- **Nascondere, non cancellare:** le funzioni per droni non sono state rimosse dal codice. Tornano
  visibili scegliendo un altro veicolo in *Impostazioni → Generali → Preferenze veicolo* o attivando la
  modalità avanzata. Così il fork resta vicino a upstream e gli aggiornamenti a nuove versioni di
  QGroundControl generano meno conflitti.
- **Seguire il tipo di veicolo:** dove possibile l'interfaccia si adatta al tipo del veicolo collegato
  (o a quello scelto per la pianificazione), come già fa upstream. Un drone collegato a questa versione
  continua a vedere i propri comandi.
- **Codice di upstream:** stile, Fact System, null-check sui veicoli e Conventional Commits come da
  `AGENTS.md`. Le funzioni nuove hanno test unitari.

---

## 2026-10-26

### Lavoro in campo nella Pianificazione

**Cosa.**

- Nuovo modello di piano e pattern **Lavoro in campo** (`FieldWorkComplexItem`, primo tra i modelli per
  i rover): il rover lavora lungo le file di una coltura, con le voltate in capezzagna.
  - **Campo** disegnato sulla mappa o preso dal rover: *Angolo dal rover* aggiunge come angolo la
    posizione attuale del rover (RTK, più precisa della mappa satellitare), *Ricomincia il campo*.
  - **Operatore:** ambiente (campo aperto / serra), coltura (vigneto, frutteto, orticole in pieno campo,
    mais, orticole in serra, personalizzata), tipo di lavoro (mappatura, rilevamento infestanti,
    diserbo, NDVI, NDRE, irrorazione), velocità in km/h (2,8 di serie, massimo 3), direzione delle file
    (parallele al lato evidenziato in giallo), punto di partenza.
  - **Tecnico** (modalità avanzata, *Coltura e rover*): interfila, larghezza della fascia coltivata, file
    per passata, prima fila dal bordo, capezzagna, raggio di sterzata. La coltura propone i primi quattro
    (valori indicativi da confermare con il CREA).
  - **Percorso:** passate alternate al centro delle file scavalcate; capezzagna come fascia lungo ogni
    lato incontrato dalle file (anche obliquo); voltate a U se le passate distano almeno due raggi,
    altrimenti a goccia, campionate circa ogni metro. Se una voltata esce dal campo, se la fascia
    coltivata non lascia spazio alle ruote o se nel campo non entra nessuna fila, il lavoro non si può
    salvare né caricare e l'editor spiega perché.
  - **Statistiche:** file, passate, carreggiata necessaria, tipo di voltata, lunghezza, tempo stimato,
    area. Sulla mappa le file in verde.
  - Al rover arriva un `MAV_CMD_DO_CHANGE_SPEED` con la velocità di lavoro prima del primo waypoint;
    ambiente, coltura e tipo di lavoro restano nel file `.plan`.
- `TransectStyleComplexItem` (upstream): due metodi virtuali (`_entryMissionItemCount`,
  `_appendEntryMissionItems`) per gli elementi di missione che un pattern mette prima del primo waypoint.
- **Fly View:** con il rover e più di 30 punti si vedono solo la partenza e il punto in corso, non
  centinaia di numeri.
- **Test:** `FieldWorkComplexItemTest` (file e voltate a goccia e a U, velocità prima del primo punto,
  capezzagna troppo stretta o troppo larga, fascia troppo larga, preset delle colture, lato obliquo,
  direzione e partenza, salvataggio e caricamento).

**Perché.** Dopo la chiamata con il CREA il percorso nel campo lo calcola la dashboard (sul rover non
c'è un planner): l'operatore deve indicare il campo e la coltura, non i singoli punti. Il Survey di
upstream non conosce file, capezzagna e raggio di sterzata, e le sue passate a zig-zag avrebbero fatto
girare il rover sopra la coltura.

### Schermata Guida del rover: stati, FERMA, avanzamento e avvisi

**Cosa.**

- **Stati del rover** (FirmwarePlugin MEDEA): le modalità diventano gli stati macchina delle regole
  operative del CREA, con nomi pensati per l'operatore: *Pronto*, *Al lavoro*, *In pausa*, *Rientro*,
  *Guida manuale*, *Arresto di emergenza* (prima *Fermo*, *Lavoro*, *Pausa*, *Manuale*, *Emergenza*) e
  due nuovi, non selezionabili: *Avvio in corso* (custom_mode 7) e *Guasto* (8).
- **Barra in alto:** lo stato del rover è una pillola colorata (grigio, verde, blu, giallo, viola,
  arancione, rosso) al posto di *Pronto/Motori accesi* + modalità; in modalità operatore la modalità non
  è più ripetuta accanto. L'indicatore GPS diventa **indicatore di posizione** a parole: *RTK fisso*,
  *RTK float*, *GPS senza RTK*, *Nessuna posizione* (`RoverPositionIndicator.qml`).
- **Pulsanti** (`RoverJobControls.qml`): **FERMA** (rosso, il più grande) al posto di *Pausa* e *Stop*:
  ferma il rover e spegne i motori, il lavoro resta sospeso. Da *In pausa*: **Riprendi**, **Torna alla
  base** e **Termina lavoro** (con conferma; è lo STOP delle regole operative, chiude il lavoro). In
  *Avvio in corso* e *Guida manuale* compare solo una spiegazione; in *Guasto* e *Arresto di emergenza*
  nessun pulsante.
- **Riquadro del lavoro** (`RoverJobPanel.qml`, in alto a sinistra): percentuale e barra di
  avanzamento, punto in corso (*Punto 3 di 12*), distanza e tempo che mancano, velocità in km/h,
  pulsante **Registro eventi** (i messaggi del rover con l'ora). Il registro si apre dentro il riquadro e
  non in una finestra modale, così **FERMA** resta raggiungibile mentre il rover si muove.
- **Avvisi** (`RoverStatusBanner.qml`, al centro): *Arresto di emergenza* e *Guasto* (con il motivo)
  finché durano; *Lavoro completato* e *Lavoro non completato* (con il motivo) finché l'operatore non
  li chiude o non carica un altro lavoro.
- Nuovo `MedeaRoverJobStatus` (C++, `QML_ELEMENT`, un'istanza nella Fly View esposta in
  `globals.roverJobStatusFlyView`): stato macchina, colore, avanzamento sul lavoro caricato (dalla
  posizione di partenza al punto di `MISSION_CURRENT`), tempo rimanente con la velocità media
  dell'ultimo minuto, esito del lavoro da `MISSION_CURRENT.mission_state`.
- **Modalità operatore:** nascosto il pulsante rosso di registrazione video della Fly View.
- **Velocità di crociera** predefinita per le statistiche: 0,78 m/s (2,8 km/h, la velocità delle
  lavorazioni in campo), minimo 0,1 m/s.
- **Test:** nuovo `MedeaRoverJobStatusTest` (stati, distanza percorsa lungo il lavoro);
  `MedeaRoverFirmwarePluginTest` aggiornato per gli 8 stati.

**Perché.** Dopo la chiamata con S. Figorilli (CREA) la dashboard segue gli stati macchina e gli stati
della missione delle sue regole operative, e la gestione del lavoro passa alla dashboard (piano in
`docs/piano-dashboard.md` del repository `medea_hmi`):

- l'operatore deve capire a colpo d'occhio cosa fa il rover: prima c'erano due indicazioni (stato e
  modalità) con parole tecniche, ora un solo stato colorato;
- un solo modo di fermare il rover, che non fa mai perdere il lavoro: con *Pausa* e *Stop* affiancati
  era facile chiudere il lavoro per sbaglio;
- l'arresto di emergenza è il fungo elettrico: la dashboard lo mostra e spiega cosa fare, ma non lo
  comanda; il pulsante rosso rotondo della registrazione si poteva scambiare per un arresto;
- avanzamento in percentuale e lavoro completato/non completato sono richiesti esplicitamente dalle
  regole operative;
- satelliti e HDOP non dicono niente a un agricoltore; "RTK fisso" sì.

---

## 2026-10-20

### Pulsanti del rover nella schermata Guida (FirmwarePlugin MEDEA)

**Cosa.**

- Nuovo **FirmwarePlugin del rover MEDEA** (`src/FirmwarePlugin/MedeaRover/`): lo usano i rover e le
  barche MAVLink con autopilota generico, cioè il rover presentato dal bridge Kafka ↔ MAVLink
  (`src/medea_bridge` nel repository `medea_hmi`). Gli altri veicoli generici restano al plugin di base.
  - Modalità riconosciute dal numero (custom_mode 1-6) e mostrate con nomi tradotti: *Fermo*,
    *Lavoro*, *Pausa*, *Rientro*, *Manuale*, *Emergenza*.
  - *Avvia* invia `MAV_CMD_MISSION_START` (o `MAV_CMD_DO_PAUSE_CONTINUE` 1 se il rover è in pausa),
    *Pausa* `MAV_CMD_DO_PAUSE_CONTINUE` 0, *Rientro* `MAV_CMD_NAV_RETURN_TO_LAUNCH`. Il rover si arma da
    solo quando parte.
  - I comandi sono inviati senza il messaggio generico "comando negato": il motivo di un rifiuto arriva
    dal rover come messaggio di errore e compare da solo in alto.
  - Nella barra in alto restano solo GPS e batteria.
- `FirmwarePluginManager`: la classe di firmware "generica" compare una sola volta anche se una factory
  ne gestisce una parte.
- **Fly View:** nuovo pannello `RoverJobControls.qml` in basso al centro, con pulsanti grandi che
  compaiono secondo lo stato del rover. Ci sono *Avvia lavoro* e *Torna alla partenza*, che chiedono
  conferma, poi *Pausa*, *Riprendi* e *Stop*, che agiscono subito.
- **Comandi generici nascosti:** per questo rover `GuidedActionsController.roverJobUI` spegne le azioni
  guidate di QGroundControl e le loro richieste automatiche (*Avvia missione*, *Continua missione*,
  arma/disarma, *Vai qui*). Sono nascosti anche il pulsante *Rientra*, la barra degli strumenti a
  sinistra e il menu *Azioni* (tranne in modalità avanzata) e la finestra *Piano completato*.
- **Plan View:** dopo un caricamento riuscito sul rover il programma passa da solo alla schermata Guida.
- **Barra in alto, modalità operatore:** con un veicolo collegato, toccando lo stato o la modalità non
  si apre più nessun pannello. Per i rover armati lo stato dice *Motori accesi* invece di *In volo*;
  "Flying" ora è tradotto *In volo*, prima era *In movimento*.
- **Test:** `MedeaRoverFirmwarePluginTest` verifica quali veicoli ricevono il plugin e come vengono
  mappate le modalità.
- **Guide:** descrivono il collegamento con il bridge, i pulsanti del rover e le prove con il rover finto,
  con immagini nuove (`operatore-guida.jpg`, `operatore-pulsanti.jpg`).

**Perché.** Nella prima prova con il bridge si vedeva quanto era scomodo il percorso dell'operatore:

- QGroundControl trattava il rover come veicolo generico e i suoi pulsanti davano *Guided mode not
  supported by Vehicle*;
- per partire servivano più passi (armare dal pannello di stato, poi *Avvia missione*);
- toccando la Q, lo stato e la modalità si aprivano tre pannelli diversi, e il menu delle modalità
  era vuoto.

Ora il lavoro si avvia con un pulsante e una conferma, e si vedono solo i comandi utili in quel momento.
Il resto non è cancellato: torna con la modalità avanzata o con un altro tipo di veicolo.

---

## 2026-10-19

### Modalità operatore e modalità tecnico

**Cosa.** La *Advanced Mode* di upstream è ora **spenta a ogni avvio** (non viene salvata). Senza
modalità avanzata l'operatore vede:

- nel menu principale solo **Guida**, **Pianifica** e **Chiudi** (Analizza, Configura e Impostazioni
  sono nascosti);
- gli indicatori della barra in alto (GPS, batteria, modalità, collegamento...) senza la parte
  "espandi" che porta alle impostazioni;
- nella Fly View, il pannello della camera senza l'ingranaggio delle impostazioni (griglia,
  adattamento, ripristino e formattazione della memoria della camera);
- nel pannello Remote ID (se il veicolo lo supporta), gli stati non aprono più le impostazioni;
- nella Plan View: niente scelta firmware/tipo di veicolo, niente sezioni *Predefiniti*, *Punti di
  raccolta* (rally), *Trasforma* e *Impostazioni iniziali della camera*, niente livello *Punti di raccolta*
  nel selettore dei livelli;
- nessuna finestra di domande al primo avvio (veicolo e unità di misura sono già impostati).

La modalità avanzata si attiva dal menu principale **tenendo premuto** sulla scritta della versione
(su desktop anche Shift+clic) e confermando; il messaggio di conferma è stato riscritto per il
personale tecnico. In modalità avanzata compare la scritta rossa "Modalità avanzata" e torna tutto.
Il gesto funziona in ogni build: in upstream stava sulla riga della data, che compare solo nelle build
giornaliere, quindi nelle build di rilascio la modalità avanzata non era raggiungibile.

**Perché.** L'operatore non deve arrivare per caso a impostazioni che possono rendere il rover
inutilizzabile. Si riusa il meccanismo esistente di upstream invece di crearne uno nuovo, e la
pressione prolungata evita attivazioni accidentali (prima su mobile bastava un tocco).

**Dove.** `QGCCorePlugin` (`_showAdvancedUI`, `showAdvancedUIMessage`, `showInitialSetup*`),
`Toolbar/SelectViewDropdown.qml`, `QmlControls/ToolIndicatorPage.qml`, `PlanView/PlanTreeView.qml`
(`rowHeightProvider`), `PlanView/PlanEditLayers.qml`, `PlanView/PlanInfoEditor.qml`,
`FlightMap/Widgets/PhotoVideoControl.qml`, `Toolbar/RemoteIDIndicatorPage.qml`.

**Nota.** Su tablet, prima un tocco sulla versione apriva la modalità avanzata e la pressione prolungata
mostrava le aree touch (debug). Ora la pressione prolungata apre la modalità avanzata; le aree touch
restano disponibili con Ctrl+clic su desktop.

### Nuovi tipi di lavoro: Percorso e Perimetro

**Cosa.** Due nuovi modelli di piano e pattern di missione, disponibili **solo per i veicoli di terra**
(per i droni l'elenco resta quello di upstream):

- **Percorso** (`PathComplexItem`): il rover segue una linea disegnata sulla mappa, tracciata a
  clic o caricata da KML/SHP. Opzioni: *Andata e ritorno* (alla fine torna indietro lungo lo stesso
  percorso), *Ripetizioni*.
- **Perimetro** (`PerimeterComplexItem`): il rover gira lungo il bordo di un'area disegnata sulla
  mappa. Opzioni: *Distanza dal bordo* (il giro viene spostato verso l'interno), *Giri*,
  *Senso opposto*, *Cambia punto di partenza*. Se la distanza dal bordo è troppo grande per l'area,
  l'editor lo segnala e il piano non si può salvare né caricare sul veicolo.

Entrambi generano solo waypoint (`MAV_CMD_NAV_WAYPOINT`): niente camera, niente decollo, niente
estensioni di virata. Insieme ai Waypoint semplici e al Survey ("copertura campo") coprono i lavori
noti oggi: seguire un percorso, controllare il perimetro, seguire dei waypoint.

**Perché.** I pattern di upstream sono pensati per i droni (fotogrammetria, scansione di strutture).
Le classi nuove derivano da `TransectStyleComplexItem`, come Survey e Corridor Scan, quindi
riusano salvataggio nei file `.plan`, statistiche, visualizzazione sulla mappa e caricamento sul
veicolo. Per aggiungere altri tipi di lavoro basta copiarne una.

**Dove.** `src/MissionManager/{Path,Perimeter}{ComplexItem,PlanCreator}.{h,cc}`,
`{Path,Perimeter}.SettingsGroup.json`, `src/PlanView/{Path,Perimeter}{Editor,MapVisual}.qml`,
registrazione in `QGCCorePlugin::complexMissionItemNames/planCreators/createComplexMissionItem`.
Test: `test/MissionManager/{Path,Perimeter}ComplexItemTest`.

Nella visualizzazione comune dei pattern (`TransectStyleMapVisuals.qml`) è stata aggiunta l'opzione
`showTransectPairArrows`: le frecce sulla seconda e sulla penultima passata presuppongono passate di
due punti (Survey) e, su un percorso fatto di una sola passata con molti punti, comparivano lontano
dal tracciato. Percorso e Perimetro le disattivano; per gli altri pattern non cambia nulla.

### Video sempre attivo

**Cosa.** Il video non si può più disattivare:

- la sorgente predefinita è **"Stream video UDP h.264"** sulla porta 5600, quindi il riquadro video
  compare nella Fly View senza configurare nulla;
- dall'elenco delle sorgenti è stata tolta la voce "Stream video disattivato": nelle impostazioni si
  sceglie solo il profilo (UDP h.264/h.265, RTSP, TCP, MPEG-TS, camere USB). Una configurazione salvata
  con il video disabilitato torna alla sorgente predefinita;
- le opzioni *Stream video attivo* e quella che fermava lo stream quando il veicolo viene disarmato
  sono nascoste e forzate (stream sempre attivo, mai fermato al disarmo).

**Perché.** Il video delle camere del rover (RGB e termica) fa parte della dashboard; l'operatore
non deve doverlo attivare e non deve poterlo spegnere per sbaglio. Il tecnico sceglie solo il profilo.

**Dove.** `src/Settings/VideoSettings.cc` (`_setDefaults`, elenco sorgenti) e
`QGCCorePlugin::adjustSettingMetaData`. La sorgente predefinita sta in `VideoSettings` e non nel core
plugin, così un file di override delle impostazioni (vedi la guida tecnica) può ancora cambiarla.

**Verificato** con stream di prova UDP (`gst-launch-1.0`) e RTSP (`mediamtx`). Con il decoder
automatico uno stream H.264 in profilo *high* non veniva mostrato, in profilo *baseline* sì: chiedere
che le camere inviino H.264 baseline o main.

### Traduzioni

- Tradotti in italiano tutti i testi nuovi (Percorso, Perimetro, messaggio della modalità avanzata) e
  due impostazioni video presenti nel codice di v5.1.5 ma assenti dal catalogo
  (*Latenza jitter RTP*, *Riconnessione automatica se si perde lo stream*).

### Test e strumenti

- Nuovi test unitari: `PathComplexItemTest` e `PerimeterComplexItemTest` (generazione dei waypoint,
  andata e ritorno, ripetizioni e giri, distanza dal bordo anche troppo grande, punto di partenza e
  senso, salvataggio e caricamento).
- Test di interfaccia di upstream (`test/QmlUITests`) adattati: girano con la configurazione di serie
  (veicolo offline PX4 multirotore, scala 100%, nessun veicolo preferito, modalità avanzata), perché
  l'interfaccia da drone che verificano esiste ancora; il test del cambio lingua usa l'italiano, dato
  che il cinese non è più compilato; il test delle impostazioni video si aspetta l'opzione "disabilita
  al disarmo" nascosta. Nuovo test `TopLevelViewsTest::_testOperatorModeHidesTechnicianViews`.
- Il controllo ortografico `typos` (solo inglese) esclude `docs/medea/`, scritto in italiano, come già
  le altre documentazioni non inglesi; l'hook usa `--force-exclude` perché l'esclusione valga anche per
  i file passati da pre-commit.

---

## 2026-10-19 — commit precedenti

### Rover come veicolo predefinito (`d1711d1ba`)

**Cosa.**

- Veicolo preferito e veicolo per la pianificazione offline: **Rover** con firmware MAVLink generico.
- Velocità di crociera per le stime di durata della missione: 1 m/s (era 15 m/s, valore da aereo).
- Connessione automatica a Pixhawk, radio SiK e LibrePilot disattivata; restano UDP (usata dal futuro
  bridge Kafka → MAVLink) e RTK GPS.
- Con "Rover" come veicolo preferito sono nascoste le pagine *ADSB Server* e *Remote ID* e le
  impostazioni di altitudine guidata, raggio di loiter, transizione VTOL, decollo obbligatorio e
  atterraggi multipli.
- Per i veicoli di terra: niente pulsanti Decollo/Atterra nella Fly View; griglia della telemetria con
  velocità, distanza da casa, tempo e distanza percorsa (senza altitudine e velocità verticale); nella
  Plan View le statistiche della missione al posto del profilo del terreno; i modelli Survey e Corridor
  Scan non inseriscono più il decollo.

**Perché.** Il veicolo è un rover di terra: le funzioni per droni confondono l'operatore. Sono
nascoste e non rimosse (vedi i principi).

### Unità europee e scala dell'interfaccia (`456382808`)

**Cosa.** Metri, km/h, ettari, °C e grammi indipendentemente dalla lingua del sistema; nomi delle
unità di velocità traducibili; scala dell'interfaccia al 115%; mappa centrata sull'Italia al primo avvio.

**Perché.** Utenti italiani, spesso su tablet o PC all'aperto: valori leggibili senza configurare nulla.

### Lingue (`3c90c9a27`)

**Cosa.** Menu delle lingue ridotto a Sistema, English, Français, Italiano, Español; tolta la voce di
debug della pseudo-localizzazione.

**Perché.** Solo le lingue utili al progetto (tradotte o da tradurre). Il rumeno non esiste in
QGroundControl e andrebbe tradotto da zero.

### Traduzione italiana (`d8d614e10`, `28e7f737e`)

**Cosa.** Traduzione a mano di tutte le parti visibili agli utenti: interfaccia principale,
impostazioni, comandi di missione, valori di telemetria, collegamenti, mappe. Le parole chiave di
ricerca delle impostazioni hanno l'italiano e l'inglese. Restano in inglese la configurazione dei
veicoli PX4/ArduPilot, la sezione Analizza e i nomi propri (RTK, GPS, Pixhawk...).

**Perché.** I cataloghi italiani di upstream erano vuoti (solo testo inglese sincronizzato da Crowdin).

**Attenzione.** Il bot Crowdin di upstream modifica spesso i file `.ts`: aspettarsi conflitti quando si
aggiorna a una nuova versione di QGroundControl.

### Nota sul fork nel README (`a06d379b8`)

Indica la versione di upstream da cui parte il fork.
