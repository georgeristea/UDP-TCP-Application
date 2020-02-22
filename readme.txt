!!! Fisierele .json si .py nu sunt scrise de mine.

1. Informatii despre structurile de date folosite:
--------------------------------------------------

	Pentru a retine informatii despre fiecare client am definit urmatorele tipuri
de date:

** Client ** cu urmatoarele campuri:
------------
a. "connected" - retine daca un client este conectat pe un anumit socket 
sau nu (0 - nu este conectat, 1 - conectat)
b. "socketId" - retine id-ul socketului prin care s-a realizat conexiunea
cu serverul
c. "nrSubs" - retine numarul de topicuri la care este abonat un client
d. "clientId" - retine id-ul cu care se logheaza fiecare client.(unic)
e. "subscriptions" - reprezinta un vector de abonamente, unde fiecare abonament 
este o structura cu tipul <topic>, explicat mai jos.

** topic ** cu urmatoarele campuri:
-----------
a. "topicName" - vector de maxim 50 de caractere ce retine numele topicului.
b. "SF" - reprezinta sf-ul ales de client atunci cand se aboneaza la un topic
c. "nrMsj" - numarul de mesaje stocate de clientul x atunci cand acesta are 
setat sf = 1 si este deconectat
d. "msj" - reprezinta un vector de mesaje (alocat dinamic), unde fiecare compo-
nenta are tipul <udpInfo> (in "msj" retin mesajele ce trebuiesc trimise clientu-
lui atunci cand se reconecteaza).

** udpInfo ** cu urmatoarele campuri:

a. "port" - retin portulul clientului UDP de la care a primit mesaj serverul
b. "ip_addr" - reprezinta adresa IPv4 a serverului
c. "msj" - reprezinta o structura de tipul "udpMsj" ce contine urmatoarele 
campuri:
	- "topic" - reprezinta numele topicului (*obs*, topicName si topic retin 
				acelasi nume. Asa am tratat cazul in care, clientii se aboneaza 
				la topicuri pentru care nu s-au primit inca mesaje de la clientii UDP)
	- "type" - reprezinta tipul de date pentru continutul mesajului primit de la 
				clintii UDP
	- "content" - un vector de maxim 1500 de caractere in care stochez informatia
				  primita de la clientii UDP.

!!! Pentru a retine toti clintii, am alocat initial un vector de "maxClients" 
elemente, pe care il realoc in cazul in care se conecteaza mai multi clienti.
Am ales sa fac asta pentru a nu folosi foarte multi apeluri de realoc.

2. Functii ajutatoare:
----------------------

a. "searchClientByID" - functia cauta un anumit client in vectorul de clienti
si intoarce indexul acestuia, daca exista sau -1, daca clientul nu exista.
b. "searchAndSubscribe" - functia verifica daca un client este abonat la un 
anumit topic.Daca este abonat, atunci doar modifica variabila sf(store&forward)
- daca acest lucru este dorit de client.Daca nu este abonat, atunci se adauga 
topicul in vectorul de abonamente ale clientului.


3. IMPLEMENTARE:
----------------

	Prima data am creat socketii: atat pentru clintii TCP, cat si pentru cei UDP
si am initializat serv_addr cu informatiile despre tipul conexiunii, adresa ip si 
portul pe care are loc.Apoi am setat multimea de descriptori cu id-ul socketului 
TCP initial, id-ul socketului UDP si 0 (pentru stdin, in cazul comenzii "quit").
In client identificatorii sunt: socket-ul pe care are loc conexiunea cu serverul
si stdin.

================================================================================

---   SERVER   ---

	In server am alocat un vector de clienti de dimensiune initiala ("maxClients")
pentru a retine informatiile despre clientii TCP.Apoi, in bucla "while" cu ajutorul 
functiei "select" se itereaza prin toti identificatorii setati ai descriptorilor.
Avem 4 cazuri:

	- daca este setat socket-ul initial("sock_tcp") atunci inseamna ca un nou 
client vrea sa se conecteze.In server accept conexiunea cu clientul TCP si adaug
noul descriptor in multime.Apoi primesc de la client id-ul unic.Daca clientul 
se afla deja in vectorul de clienti atunci il adaug in vector.Daca exista deja 
inseamna ca am un client care a fost conectat pe un anumit socket si s-a re-
conectat acum, deci trebuie sa modific in informatiile deja existente pentru acel 
client doar noul id al socket-ului si faptul ca s-a reconectat (connected = 1).
* Apoi trebuie sa ii trimit clientului care s-a reconectat toate mesajele pe care
acesta le-a pierdut cat timp a fost deconectat(in cazul in care are setat SF = 1).

	- daca este setat identificatorul socketului UDP inseamna ca primesc in 
server mesaje de la clientii UDP.Informatiile primite le stochez intr-o structura
de tipul "udpMsj".Verific pentru fiecare mesaj tipul de date al continutului
primit(In toate cazurile am folosit functiile ntohl/ntohs pentru a face conversia).
- Daca primesc date de tipul INT/SHORT_REAL fac conversia iar apoi formez numarul
cum este specificat in pdf-ul temei.Pentru tipul FLOAT numarul in formez intr-un
string (pentru a evita cazuri in care un tip de date float/double nu poate retine
o precizie foarte mare x * 10^(-100)) astfel: Daca puterea lui 10 este mai mare 
sau egala cu numarul de cifre ale alipirii dintre partea reala si partea fractio-
nara inseamna ca numarul ese negativ, deci trebuie sa adaug zerouri la inceput,
urmate de '.' si apoi numarul. Atfel pun primele "nr.cifre alipire - puterea lui 10"
cifre, urmate de '.' si apoi restul cifrelor.
* In toate cazurile rezultatul il stochez in variabila "content".Dupa ce formez 
mesajul il trimit tuturor clientilor conectati ce sunt abonati la topicul respectiv.
Daca clinetii sunt deconectati si au sf = 1 atunci pentru fiecare client stochez 
mesajele pana cand acesta se reconecteaza.

	- daca este setat identificatorul 0 inseamna ca am primit o comanda de la 
tastatura(comanda "exit"). In acest caz trimit tuturor clientilor conectati un 
mesaj de exit si astept ca acestia sa inchida conexiunea, dupa care inchid si 
din server. Daca primesc alta comanda invalida, pur si simplu afisez un mesaj 
de eroare. (*Nu am reusit sa tratez cazul cand serverul se inchide brusc: 
CTRL + C / CTRL + Z. In acest caz, daca am clienti conectati la server conexiunea
nu se inchide corespunzator si portul ramane blocat).

	- daca am primit date de la clientii conectati verific prima data daca 
clientul a inchis brusc conexiunea(CTRL + C) si in acest caz il marchez ca si 
deconectat si elimin din multimea de descriptori id-ul socketului pe care a fost 
vechea conexiune. Daca am primit o comanda de subscribe/unsubscribe atunci doar
adaug/elimin topicul "X" din vectorul de abonamente ale clientului. Daca am primit
o comanda invalida, afisez un mesaj de atentionare.

================================================================================

---   CLIENT TCP  ---

In cazul clientilor TCP avem urmatoarele doua cazuri: 

	- daca este setat identificatorul 0 inseamna ca are loc interactiunea cu 
utilizatorul. Daca primesc comanda "exit" atunci pur si simplu opresc conexiunea
cu serverul. Daca primesc o comanda de subscribe/unsubscribe atunci trimit catre
server un mesaj cu informatiile despre topicul de la care/la care vreau sa ma 
dezabonez/abonez si cu tipul subscribtiei (sf) si afisez un mesaj corespunzator(
(un)subscribed <topic>). Daca primesc alta comanda, de exemplu: "subscribe <topic>"
 / "subscribe <topic> (sf >= 2)" / "unsubscribe <topic> <sf>" sau orice alta 
 comanda infara celor valide afisez un mesaj de atentionare si nu se trimite 
 niciun mesaj catre server.
 	- in cel de-al doilea caz inseamna ca am primit un mesaj de la server care 
 poate fi de "exit", caz in care inchid clientul sau poate fi un mesaj primit 
 de server de la clinetii UDP, caz in care il afisez in formatul cerut.
