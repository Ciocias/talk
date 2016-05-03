# Talk!

## Index

+ 1 [Overview](#1-overview)
  + 1.1 [Specifications](#1-1-specifications)
  + 1.2 [Project](#1-2-project)
  + 1.3 [Server usage](#1-3-server-usage)
  + 1.4 [Client usage](#1-4-client-usage)
+ 2 [Internal architecture](#2-internal-architecture)
  + 2.1 [Server Implementation](#2-1-server-implementation)
    + 2.1.0 [Main](#2-1-0-main)
    + 2.1.1 [Message type](#2-1-1-message-type)
    + 2.1.2 [Broker](#2-1-2-broker)
    + 2.1.3 [User thread](#2-1-3-user-thread)
  + 2.2 [Client Implementation](#2-2-client-implementation)


# 1 Overview

Nel seguente paragrafo vengono mostrate le specifiche seguite per la realizzazione del progetto e le modalità di installazione e di lancio dell'applicazione, rimandando al *README* del progetto. Infine viene illustrato in grandi linee il l'utilizzo del server e più in dettaglio quello del client.

## 1.1 Specifications

L'applicazione Talk! offre un servizio di chat via internet gestito tramite server. Gli utenti, una volta effettuata la connessione al servizio, sono in grado di visualizzare una lista degli utenti conessi e disponibili per iniziare una nuova conversazione. Durante la sessione i due client collegati tra loro risulteranno indisponibili per ulteriori sessioni ed ovviamente permetteranno ai relativi utenti di scambiarsi messaggi. Una volta che uno dei due client decide di terminare la sessione entrambi non saranno più in grado di scambiare messaggi reciprocamente, ma torneranno disponibili per una nuova chat finchè essi resteranno connessi al server.

## 1.2 Project

Il progetto è disponibile tramite repository *git* e mantenuto su [GitHub]( https://github.com/barbosa23/talk ). Una volta scaricato il pacchetto per installare ed eseguire l'applicazione è sufficiente attenersi alle istruzioni presenti nel file *README*.

## 1.3 Server usage

Il programma **tlk_server** rappresenta il server della nostra applicazione. Una volta lanciato esso si mette in ascolto sulla porta specificata come parametro e resta in attesa di nuove connessioni ( in quantità limitata ) da parte dei client. Quando quest'ultime vengono accettate l'applicazione si preoccupa di mantere traccia dell'utente e dei suoi dati. Il server è inoltre capace ( solo in ambiente linux ) di reagire al segnale Ctrl+c garantendo uno shutdown corretto.

## 1.4 Client usage

Il programma **tlk_client** una volta avviato lancia al server un messaggio di **join** con il nickname scelto. Una volta ricevuto il messaggio il server verifica che il nickname non sia già in uso, se lo è il programma viene interrotto ( momentaneo ) ed è possibile tentare di ricollegarsi con un altro pseudonimo, al contrario se il **join** ha successo il server crea un nuovo utente. Ora l'utente entra nello stato di *disponibile* ed è defenitivamente connesso e può digitare una serie di comandi dalo standard di input ed inviarli al server. La lista dei comandi è la seguente:

  + **/help**
    Visualizza una guida per il client
  + **/list**
    Visualizza la lista degli utenti connessi al server
  + **/talk < nickname >**
    Se il nickname specificato è corretto, inizializza una sessione con quell'utente
  + **/close**
    Termina la sessione di chat
  + **/quit**
    Scollega il client e termina l'applicazione

Una volta che l'utente inizia una sessione di chat ( sia se iniziata da esso mediante il comando **/talk**, sia se su invito ), varia il suo stato in *occupato*, da quel momento in poi i messaggi che esso invierà ( eccezion fatta per i comandi ) saranno recapitati all'altro utente, vale a dire che in questo momento stiamo affettivamente scambiando messaggi con il nostro interlocutore. Per poter tornare *disponibile* è necessario chiudere la sessione, e questo può avvenire quando uno dei due utenti collegati digita il comando **/close**. In caso che l'utente impegnato in una conversazione usi direttamente il comando **/quit** sarà il client a preoccuparsi di chiudere correttamte la sessione prima di uscire. Una volta che l'utente torna *disponibile* può effettuare nuovamente il comando **/talk** o essere invitato ad una nuova conversazione, altrimenti può semplicemnte chiudere correttamte l'applicazione con l'apposito comando.

**[Index](#index)**

# 2 Internal architecture
A questo punto dovremmo avere una panoramica abbastanza chiara del funzionamento generale dell'applicazione, quindi scendiamo più in dettaglio analizzando l'implementazione del server e del client.

## 2.1 Server Implementation

Per poter analizzare la struttura interna del server è necessario conoscere alcune scelte di progetto, quali struttura di alcuni tipi di dato, o tecniche di gestione dei messagi e degli utenti. Chiariti tali dettagli si illusta il lo svoglimento di una sessione del server nel paragrafo [Main](#2-1-0-main).

### 2.1.1 Message type

Quando una stringa viene letta su un socket essa viene inizialmente analizzata, se non corrisponde a un comando questa stringa viene impacchetatta in una struttura quale la seguente
 ```
typedef struct _tlk_message_s {
  int id;               //message id
  tlk_user_t *sender;   //sender id
  tlk_user_t *receiver; //receiver id
  char *content;        //message string
} tlk_message_t;
```
Da ora in poi per comodità quando parleremo di messaggio faremo riferimento alla struttura appena mostrata. Il messaggio appena creato ora necessita di essere recapitato al giusto destinatario, a tale scopo viene inserito in una coda di messaggi in attesa di essere inviati, chiameremo tale coda **waiting_queue**.

### 2.1.2 Broker

A questo punto è necessario che i messaggi pendenti siano preparati alla spedizione verso il giusto utente. Dobbiamo sapere che ogni per ogni utente connesso al server esiste una coda per i messaggi ricevuti dal medesimo utente. Per gestire il compito di indirizzare i messaggi pendenti nelle giuste code di recezione il server lancia un nuovo thread chiamato **broker_routine**. La sua mansione è molto semplice, se c'è almeno un messaggio nella **waiting_queue** lo preleva e lo inserisce nella coda di messaggi del giusto destinatario. Uno scheletro della routine del broker potrebbe essere

```
while (1){
  tlk_message_t *msg = NULL;
  unsigned int i;

  /* Check for messages in the waiting queue */
  tlk_queue_dequeue(waiting_queue, (void **) &msg);

  /* Mutual exclucion for operation with users */
  tlk_sem_wait(&users_mutex);

  for (i = 0; i < current_users; i++){
    if (users_list[i] -> id == (msg -> receiver) -> id){
      /* Sort message in correct queue */
      ret = tlk_queue_enqueue(users_list[i] -> queue, (void **) &msg);
    }
  }
  ret = tlk_sem_post(&users_mutex);
}

```

### 2.1.3 User thread

Quando il server riceve una connessione crea un thread chiamato **user_handler** il quale si occuperà di registrare il client che tenta di collegarsi ed in caso ciò avvenga diventarà il thread associato all'utente registrato con successo.



### 2.1.0 Main
Appena il server viene lanciato la prima operazione svolta è quella di inizializzazione dello stesso, ossia si controlla se la porta inserita è corretta, inoltre vengono creati un semaforo per gestire le operazione sugli utenti cosi da gestire le race condition ed una coda che andrà a contenere i messaggi ricevuti dal server prima di essere elaborati. Non appena superata questa fase iniziale viene creato un nuovo thread che prende il nome di broker, elemento fondamentale per tale tipo di architettetura. Il suo compito è quello di prelevare dalla coda di attesa citata precedentemente i messaggi in arrivo e collocarli nella coda dei di lettura dell'utente destinatario. Una volta lanciato correttamente tale sub-routine siamo ingrado di passare il controllo al main-loop, il vero cuore del server. In questa fase vengono create e settate le strutture dati necessarie per gestire il socket di comunicazione dell'applicazione, dopodichè si entrerà in un ciclo continuo, dove il server si blocca in attesa di accettare una nuova connessione. Nel momento in cui si riceve una nuova connessione il server tenta di creare un nuovo utente, se questo task non solleva alcun problema al termine di questa iterazione avremmo un nuovo utente registarto per il quale il server manterrà tutte le sue informazioni e i suoi metadati, tra cui molto importante il riferimento ad un thread personale ( user_handler ) di ogni user che è quello che si occupa di gestire il nuovo utente all'interno del server. Dopo l'accetazzione di una connessione e dopo aver svolto le manzioni citate in precedenza il server torna in attesa di ulteriori tentativi di connessione, a questo punto possiamo analizzare il thread user_handler. Questa routine per prima cosa si aspetta un messaggio di tipo join da parte del client, così da poter verificare se è possibile registrare il nuovo client. Se in questa operazione non si verificano problemi il server risponde con un messaggio di benvento e viene lanciata un'ulteriore sub-routine chaiamta user_receiver, completamente gestita dallo user_handler, che analizzeremo a breve. Da questo momento in poi lo user_handler si preoccupa di ricevere i messaggi dallo user associato, analizzarli, e reagire di conseguenza, in altre parole se esso riceve semplici messaggi si preoccupa di inoltrarli al ricevente, altrimenti in presenza di messaggi di comando vengono risolte le richieste dell'utente. Il thread user_receiver invece ha il compito di controllare che nella coda di messagi in ingresso dell'utente siano presenti nuovi messagi, se così è esso lo preleva e invia il contenuto al suo client collegato cosi da poterlo visualizzare, al contrario se la suddetta coda è momentaneamente vuota il thread è bloccato su l'operazione di prelievo. Questo è il quadro generale del funionamento del server, mentre per quando rigurda la chiusura dello stesso per il momento è possibile ottenere una chiusura corretta e pulita solo in ambiente Unix, poichè l'applicazione è reattiva al segnale di SIGINT, vale a dire che quando si genera tale segnale l'applicazione prima di terminare come dovrebbe normalemnte accadere, cattura l'evento e si preoccupa di deallocare tutte le strutture dati mantenute prima di terminare.

## 2.2 Client Implementation

La prima operazione svolta dal client è quella di creare ed inizzializzare il descrittore del socket verso il server per poter effettuare la connessione. Una volta stabiliti i canali di comunicazione tra i due processi il client spedisce un messaggio di **join** al server specificando il suo nickname, e si mette in attesa di una risposta. Se essa equivale a un errore l'applicazione termina, questo può essere dovuto a problemi di comunicazione e di scambio di messaggi o più semplicemnte perchè il nickname scelto è già in uso. Diversamente nel caso di risposta positiva siamo pronti per creare e lanciare due thread, **sender** e **receiver**, i quali si occuperanno del compito più importante del client ossia quello dello scambio di messaggi. Le mansioni più in dettaglio sono:

  + **sender routine** <br/>
    Appena lanciato il thread si blocca in attesa di un messaggio sullo standard input, quindi una volta premuto invio il buffer contenente il messaggio viene inviato al server. Viene verificato il corretto invio del messaggio ed inoltre se esso corrisponde al comando **/quit** che indica l'intento dell'utente di voler chiudere il client, in questo caso si fa terminare il thread e si comunica anche al **receiver** la volontà di terminare. Al contrario se siamo in prensenza di un tipo di messaggio differente, il thread si riposiziona in attesa di un nuovo input.

  + **receiver routine** <br/>
    La prima operazione eseguita è la selezione di un descrittore di lettura, dopodichè il thread si blocca in recezione in attesa di un messaggio. All'arrivo di un messaggio se tale operazione non è andata a buon fine per problemi relativi al socket l'applicazione termina, allo stesso modo se il messaggio ricevuto correttamente è un messaggio che impone la chiusura del client da parte del server. Se non si verifica nessuna delle situazioni precendenti il messaggio ricevuto è stampato sullo standard output del processo, il quale infine si riposiziona in recezione di nuovi messaggi.

Una volta che i nostri due thread sono destinati a spirare, ossia quando si richiede di chiudere il client, su di essi viene eseguito un join dal modulo che li ha creati precedentemente il quale si occupa anche di chiudere il socket di comunicazione con il server. A questo punto il main dell'applicazione può terminare con successo. Dopo questa trattazione possiamo osservare come il client si occupi semplicemente di scambiare in maniera ottimale i dei messaggi con il server senza bidogno di mantenere alcuna struttura dati, o sull'utente o sulle sessioni di chat.


**[Index](#index)**
