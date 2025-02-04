# SPTM---RTP-projekat

## Opis rješenja
U okviru ovog projekta razvijen je WebRTC (Web Real-Time Communication) model u NS-3 mrežnom simulatoru, s fokusom na uspostavljanje peer-to-peer (P2P) veze i prijenos podataka. Zbog nedostatka postojećih rješenja i dokumentacije za direktnu integraciju WebRTC funkcionalnosti u NS-3, implementiran je prilagođeni model koji omogućava signalizaciju, uspostavljanje P2P konekcija i prijenos medijskih podataka korištenjem RTP-a preko UDP-a.

Model se sastoji od dva čvora – klijenta i servera – koji uspostavljaju vezu putem TCP three-way handshaka, nakon čega slijedi prijenos RTP paketa preko UDP protokola. Ova implementacija omogućava simulaciju i analizu ključnih parametara komunikacije, poput gubitka paketa i kašnjenja, uz prikaz rezultata kroz dijagrame i histograme.

Simulacija pruža uvid u performanse WebRTC tehnologije u različitim mrežnim uslovima, identifikuje potencijalne izazove i doprinosi optimizaciji njenih performansi. Zaključeno je da NS-3 predstavlja efikasno okruženje za testiranje i dalji razvoj WebRTC tehnologije u realnim aplikacijama. Na slijedećoj slici je dat prikaz rada same simulacije:

<p align="center">
<img src="Slike/msc.png" >
<br>
Slika 1: TCP three-way handshake dijagram</p>


## Opis RTP zaglavlja
U simulaciji, podaci se prenose putem RTP (Real-time Transport Protocol) zaglavlja, koje omogućava sinhronizaciju, identifikaciju i sekvenciranje medijskih paketa, čime se osigurava ispravno dekodiranje i rekonstrukcija audio i video sadržaja. RTP zaglavlje sadrži ključna polja poput verzije (V), padding bita (P), oznake za proširenje (X), broja CSRC identifikatora (CC), oznake važnih događaja (M) i tipa korisnog tereta (PT) koji definiše format podataka. Također, uključuje redni broj za praćenje ispravnog redoslijeda paketa, timestamp za sinhronizaciju i mjerenje kašnjenja, te SSRC identifikator za prepoznavanje izvora podataka. Ova struktura omogućava pouzdanu i efikasnu P2P komunikaciju u realnom vremenu. Na slijedećoj slici je prikazan izgled RTP header - a:

<p align="center">
<img src="Slike/headerer.png" >
<br>
Slika 1: RTP header</p>
