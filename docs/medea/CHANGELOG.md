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
