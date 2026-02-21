# CrystalVST: Architettura, Flusso Audio e Sincronizzazione BPM

Questo documento spiega in dettaglio come *CrystalVST* elabora il suono, dal momento in cui l'audio entra nel plugin fino al momento in cui esce dai tuoi monitor. È pensato per aiutarti a capire esattamente cosa fa ogni parametro e come assicurarti che tutto rimanga "a tempo" (glitch-free e BPM-synced).

---

## 1. Il Cuore del Sistema: Il "Circular Buffer"

Invece di processare l'audio in tempo reale perdendo i frammenti passati, CrystalVST registra costantemente tutto l'audio in ingresso all'interno di un **Circular Buffer (Buffer Circolare)** lungo 2 secondi.
Immagina un nastro magnetico chiuso a cerchio che gira continuamente: le testine di registrazione scrivono il nuovo audio sovrascrivendo quello più vecchio di 2 secondi. 
Questo permette al motore granulare di pescare suoni dal passato e riportarli nel presente.

### Input Source (Sorgente)
Prima di entrare nel nastro, puoi scegliere chi sta suonando:
- **LIVE**: L'audio in ingresso dalla traccia di Ableton.
- **CHORD**: Un generatore interno di accordi psichedelici a 6 voci.

---

## 2. Il Motore Granulare: Nascita, Vita e Morte di un Grano

Un "Grano" è un piccolissimo frammento di audio prelevato dal Circular Buffer. CrystalVST gestisce fino a 80 grani contemporaneamente.

### A. Quando nasce un grano? (Sincronizzazione BPM)
Il motore interroga costantemente Ableton per conoscere i BPM attuali (es. 120 BPM = 120 quarti al minuto).
- **DENSITY (Grains/Beat)**: Definisce *quanti grani* nascono in un singolo battito (un quarto). 
  - Se Density è `4`, nascerà esattamente 1 grano ogni sedicesimo (1/16).
  - La spaziatura è calcolata **al singolo sample audio**, garantendo una sincronizzazione ritmica chirurgica e a prova di bomba.

### B. Dove pesca l'audio? (Time-Travel)
Appena un grano nasce, sceglie un punto casuale nel Circular Buffer da cui iniziare a leggere (tra pochi millisecondi fa e 1.5 secondi fa). Abbiamo inserito un sistema "Anti-Glitch" che impedisce al grano di leggere nel punto esatto in cui il sistema sta scrivendo, evitando fastidiosi "click" digitali.

### C. Quanto vive un grano?
- **LIFE MIN / MAX (Beats)**: Il grano sceglie una durata casuale compresa tra questi due valori.
  - Anche qui, la durata è calcolata nei BPM del progetto. Un grano lungo `0.5` durerà esattamente un ottavo (1/8).

### D. Cosa gli succede durante la vita?
Durante la sua esistenza, il frammento audio subisce diverse alterazioni:
1.  **PITCH MIN / MAX (Oct)**: L'audio viene riprodotto a velocità diversa (Pitch-shifting) per raggiungere le ottave selezionate (da -4 a +4).
2.  **REVERSE**: C'è una percentuale di probabilità che il nastro venga letto al contrario.
3.  **LOOP CYCLE**: Se attivato, il grano invece di riprodurre il frammento dritto, intrappola una piccolissima porzione (es. 1/16) e la ripete a ciclo continuo per tutta la durata della sua "vita". Abbiamo inserito un micro-crossfade matematico per evitare i click ai bordi del loop.
4.  **GRN FILT (Filtro Dinamico)**: Se attivato, al grano viene applicato un filtro risonante che "spazza" le frequenze autonomamente dall'inizio alla fine della sua vita.
5.  **PAN SPEED**: Il suono del grano non è statico. Nasce in un punto stereo casuale e, a seconda del Pan Speed, "viaggia" da sinistra a destra (o viceversa), rimbalzando invisibilmente se tocca i bordi estremi.
6.  **DELAY MAX / %**: Alcuni grani, invece di suonare subito appena nati, vengono messi "in pausa" e attendono una divisione ritmica esatta (es. un ottavo o un quarto) prima di esplodere, creando poliritmie.

### E. Morte: L'Inviluppo
Per non creare click quando il grano inizia a suonare o quando muore, l'audio viene avvolto da un volume che sale e scende.
- **ATTACK / DECAY (ms)**: Questi parametri regolano questo "ammorbidimento". *Nota bene*: al momento sono in millisecondi (ms) per avere un feeling più fluido o percussivo indipendente dal metronomo. Non alterano il ritmo di inizio del grano (che è sempre a tempo), ma ne cambiano solo il transiente.

---

## 3. Effetti Master

Dopo che tutti i grani hanno suonato, il loro suono viene miscelato e mandato in un modulo effetti finale (applicato su tutto il livello).

1.  **HPF FREQ (Filtro Passa Alto)**: Taglia le frequenze basse fangose in modo precisissimo per evitare che l'accumulo di grani gravi impasti il mix.
2.  **PHASER & REVERB**: Effetti ambientali "Psychedelic" sotterranei. I parametri cambiano da soli piccolissimamente ogni blocco audio per dare la sensazione di uno spazio fluido e vivo. Nessun parametro rimane statico.

## 4. Conclusione sull'Audit BPM

L'architettura attuale è **Estremamente Sincronizzata**. 
Density, Life, Loop e Delay si basano tutti su misurazioni in `samplesPerBeat`, che vengono ricalcolate dinamicamente adattandosi ai BPM dell'host (Live). 

Se sperimenti colpi "fuori tempo", molto probabilmente sono generati da:
1.  Grani con **PITCH pesantemente alterato** (soprattutto ottave basse come -2 o -3) che deformano percettivamente il transiente.
2.  **DELAY MAX**: Che aggiunge divisioni ritmiche matematicamente corrette, ma poliritmiche (es. delay a terzine), che potrebbero suonare ostiche su beat in 4/4 dritti.

Se vuoi che ATTACK e DECAY siano bloccati ai BPM (es. fading di 1/32) invece che in millisecondi, è un fix fattibile per la prossima versione, ma rimuoverebbe un po' di "croccantezza" ai grani molto brevi.
