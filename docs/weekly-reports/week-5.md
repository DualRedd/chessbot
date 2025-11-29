# Viikkoraportti 5

Viikon aluksi viimeistelin käyttöliittymän. Tarvittavat konfigurointiasetukset pelille ja pelaajille
ovat sivupalkissa, ja esimerkiksi tekälyjen asetukset luodaan dynaamisesti, joten uusia asetuksia
on helppo lisätä myöhemmin. Lisäksi käyttöliittymä tunnistaa nyt kaikki lopputilanteet, mukaanlukien
tasapelit toistuvan aseman tai 50-siirron säännön perusteella.

Pääsin taas jatkamaan tekoälyn kehittämistä.
Toteutin transpositiotaulun avoimen hajautuksen menetelmällä,
käyttäen lineaarista kokeilua (eng. linear probing). Korvausstrategiana on korvata matalien hakujen
tulokset syvemmillä, sekä vanhojen hakuiteraatioiden tulokset uusilla.
Toteutin myös iteratiivisen syvenemisen aspiraatio-ikkunalla sekä quiescence-haun.
Haun kestoa voi nyt siis rajoittaa ajallisesti. Siirtojen järjestämisessä otetaan tietenkin huomioon
transpositiotaulun tallentamat parhaat siirrot, jotta iteratiivisesta syvenemisestä on hyötyä.

Tekoäly pystyy nyt pelaamaan paremmin itseään vastaan, sillä iteratiivinen syveneminen tuo hieman
satunnaisuutta siirtoihin, jolloin jokainen peli ei enää etene täysin samoin.
Tekoäly huomioi nyt myös tasapelit evaluaatiossa, joten se ei yhtä usein enää jää toistamaan siirtoja.
Toisaalta transpositiotaulu tuo mukanaan pienen ongelman, sillä tallennettu paras arvo/siirto voi
todellisuudessa johtaa tasapeliin nykyisessä tilanteessa, vaikka aiemmin näin ei käynyt.
Tämä on ilmeisesti yleinen ongelma shakkiboteissa. Tämä on kuitenkin suhteellisen harvinaista, ja ilmeisesti
suurempi ongelma vasta kun puolisiirtolaskuri alkaa lähestyä sataa, jolloin lähes kaikki transpositiotaulun
arvot voivat olla todellisuudessa tasapelejä.

Tekoäly pystyy nyt omalla koneellani laskemaan muutamasssa sekunnissa noin 8 siirron syvyydelle alkupelissä
ja lähemmäs 12 siirron syvyydelle loppupelissä. Pelejä katsoessa huomaa, että välillä tekoäly ei näytä
tietävän miten peliä tulisi edistää, jos mitään selkeää voittoa ei ole näkyvissä lähitulevaisuudessa.
Esimerkiksi voittavassa loppupelissä välillä kestää hieman ennen kuin sotilaita aletaan siirtämään
eteenpäin, ja välillä alkupelissa siirtoja tuhlataan nappuloiden kehittämisen sijaan. Uskoisin, että
syynä on nykyinen yksinkertainen evaluaatiofunktio.

Tekoälyn kehittämisessä ajattelin siis seuraavaksi keskittyä hieman evaluaatiofunktion parantamiseen.
Ensin kuitenkin ajattelin lisätä testikattavuutta sekä toteuttaa suorituskyvyn mittaamista.
Myös jotain metriikoita esim. transpositiotauluun liittyen olisi hyvä kerätä.
Voisin luoda myös tavan testata omaa tekoälyä tunnettuja tekoälyjä vastaan.
Tavoitteena olisi siis luoda tapoja, joilla voin konkreettisesti alkaa mittaamaan
tekoälyn tasoa ja kehitystä. Ensi viikolla yritän saada testaus- ja toteutusdokumentit ajan tasalle.

Viikon työaika: 18h
