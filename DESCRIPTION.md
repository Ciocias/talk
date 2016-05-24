# Talk!

## Index

+ 1 [Overview](#1-overview)
  + 1.1 [Specifications](#1-1-specifications)
  + 1.2 [Project](#1-2-project)
  + 1.3 [Server usage](#1-3-server-usage)
  + 1.4 [Client usage](#1-4-client-usage)
+ 2 [Internal architecture](#2-internal-architecture)
  + 2.1 [Server Implementation](#2-1-server-implementation)
    + 2.1.1 [Main session](#2-1-1-main-session)
    + 2.1.2 [Message type](#2-1-2-message-type)
    + 2.1.3 [Broker](#2-1-3-broker)
    + 2.1.4 [User thread](#2-1-4-user-thread)
  + 2.2 [Client Implementation](#2-2-client-implementation)


# 1 Overview

Nel seguente paragrafo vengono mostrate le specifiche seguite per la realizzazione del progetto e le modalità di installazione e di lancio dell'applicazione, rimandando al *README* del progetto. Infine viene illustrato in grandi linee l'utilizzo del server e più in dettaglio quello del client.

## 1.1 Specifications

L'applicazione **Talk!** offre un servizio di chat via internet gestito tramite architettura client/server. Una volta effettuata la connessione al servizio è possibile visualizzare una lista degli altri utenti connessi e disponibili per iniziare una nuova conversazione. Durante la sessione i due client collegati tra loro risulteranno indisponibili per ulteriori connessioni ed ovviamente permetteranno ai relativi utenti di scambiarsi messaggi. Una volta che uno dei due client decida di terminare la sessione entrambi non saranno più in grado di scambiarsi messaggi, ma torneranno nuovamente disponibili finchè essi resteranno connessi al server.

## 1.2 Project

Il progetto è disponibile tramite repository *git* e mantenuto su [GitHub]( https://github.com/barbosa23/talk ). Una volta scaricato il pacchetto per installare ed eseguire l'applicazione è sufficiente attenersi alle istruzioni presenti nel file *README*.

## 1.3 Server usage

Una volta lanciato il server esso si mette in ascolto sulla porta specificata come parametro e resta in attesa di nuove connessioni ( in quantità limitata ) da parte dei client. Quando quest'ultime vengono accettate l'applicazione si preoccupa di mantere traccia dell'utente e dei suoi dati. Il server è inoltre capace di reagire al segnale SIGINT garantendo uno shutdown corretto.

## 1.4 Client usage

Quando il client viene lanciato, come prima cosa invia al server un messaggio di **join** con il nickname scelto. Una volta ricevuto il messaggio il server verifica che il nickname non sia già in uso, se lo è il programma viene interrotto ed è possibile tentare di ricollegarsi con un altro pseudonimo, al contrario se il **join** ha successo il server crea un nuovo utente. Ora l'utente entra nello stato *disponibile* e può digitare una serie di comandi dallo standard input ed inviarli al server. La lista dei comandi è la seguente:

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

Una volta che l'utente inizia una sessione di chat, sia se iniziata mediante il comando **/talk** sia se su invito, varia il suo stato in *occupato* e da quel momento in poi i messaggi che esso invierà ( eccezion fatta per i comandi ) saranno recapitati all'altro utente. Vale a dire che in questo momento stiamo effettivamente scambiando messaggi con il nostro interlocutore. Per poter tornare *disponibile* è necessario chiudere la sessione e questo può avvenire quando uno dei due utenti collegati digita il comando **/close**. In caso che l'utente impegnato in una conversazione usi direttamente il comando **/quit** sarà il client a preoccuparsi di chiudere correttamte la sessione prima di uscire. Non appena l'utente torna *disponibile* può effettuare nuovamente il comando **/talk** o essere invitato ad una nuova conversazione, altrimenti può semplicemente chiudere l'applicazione.

**[Index](#index)**

# 2 Internal architecture

A questo punto dovremmo avere una panoramica abbastanza chiara del funzionamento generale dell'applicazione, quindi scendiamo più in dettaglio analizzando l'implementazione del server e del client.

## 2.1 Server Implementation

Prima di analizzare la struttura interna del server e le scelte progettuali fatte illustriamo lo svoglimento di una sessione del server.

### 2.1.1 Main session

Appena il server viene lanciato la prima operazione svolta è quella di inizializzare alcune strutture dati quali un semaforo per gestire le operazioni sugli utenti ed una coda che andrà a contenere i messaggi ricevuti dal server prima di essere elaborati (**waiting_queue**). Non appena superata questa fase preliminare viene lanciato in parallelo il [**broker**](#2-1-3-broker), elemento fondamentale per tale tipo di architettura. Una volta lanciata correttamente questa sub-routine siamo in grado di passare il controllo al main-loop, il vero cuore del server. A questo punto vengono create e settate le strutture dati necessarie per gestire i socket di comunicazione dell'applicazione, dopodichè si entrerà in un ciclo continuo dove il server è bloccato in attesa di connessioni. Nel momento in cui un client richiede di instaurare una connessione, il server tenta di creare un nuovo utente con i dati ricevuti (nickname ed indirizzo ip), se questo task non solleva alcun problema il server manterrà tutte le informazioni ed i metadati del client in questione, tra cui l'essenziale riferimento al suo thread di gestione personale ( **user_handler** ) che si occupa di gestire le interazioni dell'utente con il server. Dopo la conclusione di queste operazioni il programma torna in attesa di ulteriori connessioni, a questo punto possiamo analizzare il thread user_handler. Questa routine per prima cosa si aspetta un messaggio di tipo join da parte del client, così da poter verificare se è possibile effettuare una nuova registrazione. Se in questa operazione non si verificano problemi il server risponde con un messaggio di benvenuto e viene lanciata un'ulteriore sub-routine chiamata user_receiver, che analizzeremo a breve. Da questo momento in poi lo user_handler si preoccupa di ricevere i messaggi dell'utente associato, analizzarli, e reagire di conseguenza. In altre parole se esso riceve semplici messaggi si preoccupa di inoltrarli al ricevente, altrimenti in presenza di messaggi di comando vengono risolte le richieste dell'utente. Il thread user_receiver invece ha il compito di controllare che nella coda di messaggi in ingresso dell'utente siano presenti nuovi messaggi, se così è esso lo preleva e invia il contenuto al suo client collegato cosi da poterlo visualizzare, al contrario se la suddetta coda è momentaneamente vuota il thread è bloccato sull'operazione di prelievo. Questo è il quadro generale del funionamento del server, mentre per quando riguarda la chiusura dello stesso per il momento l'applicazione è reattiva al segnale di SIGINT, vale a dire che quando riceve tale segnale, prima di terminare come dovrebbe normalmente accadere, cattura l'evento e si preoccupa di liberare correttamente la memoria allocata in precedenza.

### 2.1.2 Message type

Quando una stringa viene letta su un socket viene analizzata e, se non corrisponde a un comando viene impacchettata nella seguente struttura
 ```
typedef struct _tlk_message_s {
  int id;               //message id
  tlk_user_t *sender;   //sender id
  tlk_user_t *receiver; //receiver id
  char *content;        //message string
} tlk_message_t;
```
Da ora in poi per comodità quando parleremo di messaggio faremo riferimento alla struttura appena mostrata. Il messaggio appena creato ora necessita di essere recapitato al giusto destinatario, a tale scopo viene inserito in una coda di messaggi in attesa di essere inviati, chiameremo tale coda **waiting_queue**.

### 2.1.3 Broker

A questo punto è necessario che i messaggi pendenti siano smistati verso il giusto utente, ognuno dei quali ha associato una coda per i messaggi ricevuti. Per indirizzare tali messaggi nelle giuste code di ricezione il server lancia un nuovo thread chiamato **broker_routine**. La sua mansione è molto semplice, se c'è almeno un messaggio nella **waiting_queue** lo preleva e lo inserisce nella coda del giusto destinatario. Uno scheletro della routine del broker potrebbe essere

```
/* Be careful to add error handling for production code */

while (1)
{
  tlk_message_t *msg = NULL;
  unsigned int i;

  /* Check for messages in the waiting queue */
  tlk_queue_dequeue(waiting_queue, (void **) &msg);

  /* Mutual exclusion for operation with users */
  tlk_sem_wait(&users_mutex);

  for (i = 0; i < current_users; i++)
  {
    if (users_list[i] -> id == (msg -> receiver) -> id)
    {
      /* Sort message in correct queue */
      tlk_queue_enqueue(users_list[i] -> queue, (void **) &msg);
    }
  }
  tlk_sem_post(&users_mutex);
}
```

### 2.1.4 User thread

All'interno del server, per ogni nuova connessione, viene creato un thread **user_handler**. Se tale thread riesce nella registrazione del nuovo utente, si dedica alla gestione delle interazioni di quest'ultimo con il server, quali comandi e sessioni di chat.

## 2.2 Client Implementation

La prima operazione svolta dal client è quella di creare ed inizializzare il descrittore del socket verso il server per poter effettuare la connessione. Una volta stabiliti i canali di comunicazione tra i due processi il client spedisce un messaggio di **join** al server specificando il suo nickname, e si mette in attesa di una risposta. Se essa equivale a un errore l'applicazione termina, questo può essere dovuto a problemi di comunicazione e di scambio di messaggi o più semplicemente perchè il nickname scelto è già in uso. Diversamente nel caso di risposta positiva siamo pronti per creare e lanciare due thread, **sender** e **receiver**, i quali si occuperanno del compito più importante del client ossia quello dello scambio di messaggi. Le mansioni più in dettaglio sono:

  + **sender routine** <br/>
    Appena lanciato il thread si blocca in attesa di un messaggio sullo standard input, quindi una volta premuto invio il buffer contenente il messaggio viene inviato al server. Viene verificato il corretto invio del messaggio ed inoltre se esso corrisponde al comando **/quit** che indica l'intento dell'utente di voler chiudere il client, in questo caso si fa terminare il thread e si comunica anche al **receiver** la volontà di terminare. Al contrario se siamo in prensenza di un tipo di messaggio differente, il thread si riposiziona in attesa di un nuovo input.

  + **receiver routine** <br/>
    La prima operazione eseguita è la selezione di un descrittore di lettura, dopodichè il thread si blocca in attesa di un messaggio. All'arrivo di un messaggio se tale operazione non è andata a buon fine per problemi relativi al socket l'applicazione termina, allo stesso modo se il messaggio ricevuto correttamente è un messaggio che impone la chiusura del client da parte del server. Se non si verifica nessuna delle situazioni precendenti il messaggio ricevuto è stampato sullo standard output del processo, il quale infine si riposiziona in ricezione di nuovi messaggi.

Una volta che i nostri due thread sono destinati a spirare, ossia quando si richiede di chiudere il client, su di essi viene eseguito un join dal modulo che li ha creati precedentemente il quale si occupa anche di chiudere il socket di comunicazione con il server. A questo punto il main dell'applicazione può terminare con successo. Osserviamo come il client si occupi semplicemente di scambiare in maniera ottimale dei messaggi con il server senza bisogno di mantenere alcuna struttura dati, o sull'utente o sulle sessioni di chat.


**[Index](#index)**
