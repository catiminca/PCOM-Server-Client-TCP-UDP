Minca Ecaterina - Ioana 324CAb

Pentru implemetarea temei, am structurat codul in 2 fisiere, unul pentru server
si unul pentru client, iar cel de common preluat din laborator pentru a avea
functiile de send_all si recv_all. M-am folosit de structurile:
-udp, in care retin un mesaj primit de la un client UDP, care va contine topic-ul,
tipul de date si continutul efectiv
-packet, in care voi retine practic formatul unui mesaj care va fi trimis atunci
cand clientul va fi pornit, avand mesajul in sine pe care trebuie sa-l primeasca
si dimensiunea acestuia
-un enum pentru a retine tipurile posibile de mesaje: identify(la conectare),
subscribe_with_sf(am primit si sf = 1 la actiunea de subscribe), subscribe_witout_sf
(sf = 0), unsubscribe si message(cand primesc mesaje udp). De fiecare data cand trimit
niste date prima oara voi trimite o variabila de tipul packet_type pentru a specifica
ce tip de comanda va urma si apoi o lungime pentru a trimite ulterior datele de
lungimea buna. De fiecare data cand primesc date, voi verifica daca tipul comenzii
este cel potrivit actiunii pe care urmeaza sa o execut.

Server:
M-am folosit de map-uri in felul urmator:
-clients_messages este pentru a retine pentru un id, mesajele acestuia
-fd_addr, in care se va retine corespondenta id, adresa clientului
-clients_fd, cheia este id si valoarea este fd-ul sau
-fd_clients, cheia este un fd si valoarea este id-ul asociat acestuia; Am retinut
in acesta valoarea -1 in cazul in care clientul a mai fost conectat, dar s-a deconectat
-all_topics, care retine corespondenta dintre numele unui topic si un alt map, in
care cheia este id-ul clientului si valoarea este sf-ul.

In functie de fd-ul, trebuie realizata o actiune.
-stdin_case
    ->se poate primi doar exit, caz in care se inchid toti socket-ii si se
        termina programul
-udp_case
    ->in acest caz trebuie semnalati toti clientii care sunt
        abonati la topicul primit ca mesaj
    -> daca clientul este conectat, i se trimite mesajul
    -> daca acesta este deconectat, dar abonat la topic, i se va salva mesajul
        in lista sa de mesaje
-tcp_case_listen
    -> care accepta conexiuni de la clienti tcp

-cazul in care se va citi de la stdin in client o comanda de subscribe/unsubscribe
si aceasta va fi procesata corespunzator, modificand in map - ul pentru topicuri
in functie de comanda(inserare nou id si sf sau stergere id)

Subscriber:
-udp_handler_msg
    -> afiseaza datele primite de la server in functie de tipul lor
-stdin_case
    -> daca primeste exit, trebuie inchis client-ul si fd-ul sau
    -> altfel se va citi un buffer si in functie de comanda se va
        trimite cerere de abonare/dezabonare de la un topic la server

Atat pentru server cat si pentru client m-am folosit de laborator.