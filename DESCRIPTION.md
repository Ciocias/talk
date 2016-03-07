# Talk!

## Index

+ 1 [Overview](#1-overview)
  + 1.1 [Specifications](#1-1-specifications)
  + 1.2 [Project](#1-2-project)
  + 1.3 [Server usage](#1-3-server-usage)
  + 1.4 [Client usage](#1-4-client-usage)
+ 2 [Internal architecture](#2-internal-architecture)
  + 2.1 [Server Implementation](#2-1-server-implementation)
  + 2.2 [Client Implementation](#2-2-client-implementation)


# 1 Overview

Nel seguente paragrafo vengono mostrate le specifiche seguite per la realizzazione del progetto e le modalità di installazione e di lancio dell'applicazione, rimandando al *README* del progetto. Infine viene illustrato in grandi linee il l'utilizzo del server e più in dettaglio quello del client.

## 1.1 Specifications

L'applicazione Talk! offre un servizio di chat via internet gestito tramite server. Gli utenti, una volta effettuata la connessione al servizio, sono in grado di visualizzare una lista degli utenti conessi e disponibili per iniziare una nuova conversazione e iniziare una nuova sessione di chat. Durante la sessione i due client collegati tra loro permetteranno ai relativi utenti di scambiarsi messaggi, e saranno indisponibili ad altre conversazioni. Una volta che uno dei due client decide di terminare la sessione essi non saranno più in grado di scambiare messaggi, ma torneranno disponibili per una nuova sessione finchè saranno connessi al server.

## 1.2 Project

Il progetto è disponibile tramite repository *git* e mantenuto su [GitHub]( https://github.com/barbosa23/talk ). Una volta scaricato il pacchetto per installare ed eseguire l'applicazione è sufficiente seguire le istruzioni presenti sul file *README*.

## 1.3 Server usage

Il programma **tlk_server** rappresenta il server della nostra applicazione. Una volta lanciato esso si mette in ascolto sulla porta specificata come parametro e resta in attesa di nuove connessioni ( in quantità limitata ) da parte dei client. Quando quest'ultime vengono accettate l'applicazione si preoccupa di mantere traccia dell'utente e dei suoi dati. Il server è inoltre capace ( sono in ambiente linux ) di reagire al segnale Ctrl+c garantendo uno shutdown corretto.

## 1.4 Client usage

Il programma **tlk_client** una volta avviato lancia al server un messaggio di **join** con il nickname scelto. Una volta ricevuto il messaggio il server verifica che il nickname non sia già in uso, se lo è il programma viene interrotto ( momentaneo ) ed è possibile tentare di ricollegarsi con un altro pseudonimo. Se il join ha successo il server crea un nuovo utente ed instaura una connessione con il server e client, ora l'utente entra nello stato di *disponibile* ed è defenitivamente connesso e può digitare una serie di comandi ed inviarli al server. La lista dei comandi è la seguente:

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

Una volta che l'utente inizia una sessione di chat ( sia se iniziata da lui con il comando **/talk**, sia se su invito ), varia il suo stato in *occupato* e da quel momento in poi i messaggi che esso invierà ( eccetto per i comandi ) saranno recapitati all'altro utente appartenente alla medesima sessione. Per poter tornare *disponibile* è necessario chiudere la sessione, e questo può avvenire quando uno dei due utenti collegati digita il comando **/close**. In caso che l'utente impegnato in una conversazione usi direttamente il comando **/quit** il client si preoccupa di chiudere correttamte la sessione prima di uscire. Una volta che l'utente torna *disponibile* può effettuare nuovamente il comando **/talk** o essere invitato ad una conversazione, altrimenti può semplicemnte chiudere correttamte l'applicazione con l'apposito comando.

**[Index](#index)**

# 2 Internal architecture
A questo punto dovremmo avere una panoramica abbastanza chiara del funzionamento generale dell'applicazione, quindi scendiamo più in dettaglio analizzando l'implementazione del server e del client.

## 2.1 Server Implementation


## 2.2 Client Implementation

La prima operazione svolta dal client è quella di creare, inizzializzare il descrittore del socket verso il server per poter effettuare una connessione. Una volta stabiliti i canali di comunicazione tra i due processi il client spedisce un messaggio di **join** al server specificando il suo nickname, e si mette in attesa di una risposta. Se essa equivale a un errore l'applicazione termina, questo può essere dovuto da problemi di comunicazione e di scambio di messaggi o più semplicemnte perchè il nickname scelto è già in uso. Nel caso in di risposta positiva siamo pronti per creare e lanciare due thread, **sender** e **receiver**, i quali si occuperanno del compito più importante del client ossia quello dello scambio di messaggi. Le mansioni più in dettaglio sono:

  + **sender routine** <br/>
    Appena lanciato il thread si blocca in attesa di un messaggio sullo standard input, quindi una volta premuto invio il buffer contenente il messaggio viene inviato al server. Viene verificato il corretto invio del messaggio ed inoltre se esso corrisponde al comando **/quit** che indica l'intento dell'utente di voler chiudere il client, in questo caso si fa terminare il thread e si comunica anche al **receiver** la volontà di terminare. Al contrario se siamo in prensenza di un tipo di messaggio differente, il thread si riposiziona in attesa di un nuovo input.

  + **receiver routine** <br/>
    La prima operazione eseguita è la selezione di un descrittore di lettura, dopodichè il thread si blocca in in recezione in attesa di un messaggio. All'arrivo di un messaggio se tale operazione non è andata a buon fine per problemi relativi al socket l'applicazione termina, allo stesso modo se il messaggio ircevuto correttamente è un messaggio che impone la chiusura del client da parte del server. Se non si verifica nessuna delle situazioni precendenti il messaggio ricevuto è stampato sullo standard output del processo, il quale infine si riposiziona in recezione di nuovi messaggi.

Una volta che i nostri due thread sono destinati a morire su di essi viene eseguito un join dal modulo che li ha creati precedentemente il quale si occupa anche di chiudere il socket di comunicazione con il server. A questo punto il main dell'applicazione può terminare con successo. Dopo questa trattazione possiamo osservare come il client si occupi semplicemente di scambiare in maniera ottimale i dei messaggi con il server senza bidogno di mantenere alcuna struttura dati, o sull'utente o sulle sessioni di chat.


**[Index](#index)**
