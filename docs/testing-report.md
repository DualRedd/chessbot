# Testausdokumetti

Yksikkötestauksen kattavuusraportti on nähtävissä [Codecov:ssa](https://app.codecov.io/gh/DualRedd/chessbot).
Testit voi suorittaa projektin rakennettuaan seuraavalla komennolla
```bash
ctest --output-on-failure
```

Kaikki dokumentissa mainitut suorituskykymittausten tulokset on mitattu pöytäkoneella Intel Core i7-12700K prosessorilla (3.6 GHz).

## Siirtojen generointi, tekeminen ja peruuttaminen

Siirtojen generoinnin sekä siirtojen tekemisen ja perumisen suorituskykyä ja oikeellisuutta on testattu [perft](https://www.chessprogramming.org/Perft)-testillä.
Testissä käydään läpi pelipuuta tietystä asemasta lähtien tietylle syvyydelle, tehden kaikki mahdolliset siirrot rekursiivisesti jokaisessa solmussa.
Testin aikana lasketaan käytyjen solmujen määrä. Tätä voidaan verrata tunnettuihin arvoihin. Testiasetelmat kattavat myös kaikki erikoissiirrot.

Suorituskykyä voidaan mitata samalla testillä, kun jaetaan käytyjen solmujen määrä kuluneella ajalla. Testissä on mahdollista nopeuttaa solmujen laskemista ohittamalla siirtojen
tekemisen lehtisolmuihin saakka, sillä niissä ei enää ole tarpeen generoida lisää siirtoja. Erityisesti suorituskykytestauksessa kuitenkin teen siirrot viimeiselle
solmulle asti, sillä se mittaa tarkemmin myös siirtojen tekemisen ja peruuttamisen suhteellista tehokkuutta siirtojen generointiin nähden oikeassa käyttötarkoituksessa
minimax algoritmissa.

**Perft-testauksen tulokset:** 47 miljoonaa solmua sekunnissa.

Lisäksi on varmistettu apufunktioiden oikeellisuus. Näiden testaus nojaa oletukseen, että laillisten siirtojen generoinnin on testattu olevan oikein.
Siirtojen generaattori mahdollistaa generoida vain syönnit tai hiljaiset siirrot, joten on testattu, että näiden yhdistelmä vastaa kaikkia laillisia siirtoja.
Eräs apufunktio mahdollistaa testata, onko mikä tahansa siirto laillinen tietyssä laudan asetelmassa. Tämä on testattu käymällä läpi kaikki mahdolliset
lailliset ja laittomat siirrot testiasetelmissa ja varmistamalla tuloksen vastaavan generoituja laillisia siirtoja.
Samankaltaisella idealla on varmistettu, että funktio, joka testaa antaisiko siirto shakin, toimii oikein. Eli vertaamalla sitä olemassaolevan shakkitilanteen
tunnistavaan funktioon, jonka perft-testaus on osoittanut toimivaksi.


## Inkrementaaliset päivitykset

Monet tekniikat hyödyntävät inkrementaalisia päivityksiä, sillä arvoja ei ole tarpeen laskea täysin alusta solmujen välillä.
Näitä ovat Zobrist-hajautusarvot, laudan evaluaatio ja pelinappuloiden sijainnit. Perft-testaus kattaa hyvin laajasti pelilaudan inkrementaaliset muutokset.
Zobrist- ja evaluaatio-arvoja on testattu monipuolisissa lähtöasetelmissa käyden läpi kaikki siirrot ja varmistaen, että inkrementaalisen päivityksen
tuottamama arvo vastaa täysin nollasta rakennettua. Laudan evaluaation tapauksessa on vielä testattu, että evaluaatio on symmetrinen, eli antaa saman
arvon jos kaikkien nappuloiden värit vaihdettaisiin päittäin, sekä että alkutilanteen evaluaatio on nolla.

## Tekoälyn testaus

Tekoälylle on haastavampaa luoda kattavia yksikkötestejä. Nyt on testattu, että se löytää varman voiton 1-6 siirron syvyydellä normaaleissa tilanteissa.
Tiettyjä hyvin haastavia voittoja tekoäly ei välttämättä löydä nopeasti, sillä on esimerkiksi mahdollista luoda tilanteita, jotka vaativat lyhyellä tähtäimellä
materiaalin menettämistä suhteellisen paljon. Tekoäly hyödyntää erilaisia karsintatekniikoita siirroille, joten voi mennä
kauankin, ennen kuin tekoäly edes tutkii tällaista siirtosarjaa, sillä muut näyttävät alkuun paremmilta.

Yksinkertaisissa loppupeleissä tekoäly voi löytää shakkimatin hyvinkin nopeasti pitkältä.
Alla esimerkiksi eräs tulos [tästä](https://lichess.org/analysis/8/2k5/p7/5N2/4K2P/8/8/8_w_-_-_0_1?color=white) laudan tilasta.
Tekoäly (versio 0.3.2) löysi neljässä sekunnissa shakkimatin 22 puolisiirron syvyydeltä.

```
info depth 21 seldepth 34 score mate 11 nodes 14570383 nps 10368922 time 4070
```

### Siirtojen järjestäminen

Tekoälyllä voidaan tutkia samat tilat tietylle syvyydelle ilman aikarajaa, ja laskea keskiarvo käydyistä solmuista. Tällä tavalla voidaan verrata
siirtojen järjestämiseen tehtyjä muutoksia. Tätä testatessa on tarpeen ottaa pois käytöstä kaikki heuristiset karsintamenetelmät, sillä tällöin
siirtojärjestys voi myös vaikuttaa tietyissä tapauksissa lopputulokseen.

### Elo-testaus

Yksikkotestejä paremmin tekoälyn kehitystä pystyy mittamaan pelaamalla suuria määriä pelejä aikaisempaa versiota vastaan, tai jotain toista tekoälyä vastaan.
Elo-luokitus kuvaa suhteellista eroa pelaajien tasossa. Esimerkiksi 100 pisteen ero tarkoittaa, että paremman pelaajan pistemäärä on 64%.
400 pisteen ero tarkoittaa jo 90% pistemäärää. Pistemäärä shakissa lasketaan voitoista ja tasapeleistä. Voitto on yhden pisteen arvoinen ja tasapeli puoli pistettä.
Prosenttiluku saadaan jakamalla maksimipisteillä.

Tekoälyjen testaamiseen toisiaan vastaan käytin [cutechess](https://github.com/cutechess/cutechess) komentorivityökalua, jolla voi ajaa turnauksia tekoälyjen kesken.
Tätä varten tekoälyn on tuettava uci-standardia komentorivillä. Lisäksi generoin [Lichess:n](https://database.lichess.org/) avoimesta pelidatasta aloitusasetelmia,
käyttäen pelejä, joissa molempien pelaajien rating oli yli 2000. Näitä käyttämällä testit on monipuolisempia. Varmistin myös, että tekoälyllä on 50 voittoprosentti
itseään vastaan aloitusasetelmilla, eli ne eivät anna toiselle etua.

Testasin tekoälyn uusinta versiota paikallisesti ladattuja Robocide- ja Baislicka- tekoälyjä vastaan, joiden Elo-luokitus on testattu [Computer Chess Rating Lists](https://computerchess.org.uk/ccrl/4040/) -sivustolla. Niiden taso on noin 2150-2200. Alla testitulokset turnauksesta, jossa jokainen pari pelasi 1000 peliä ja mietintäaikaa jokaisella siirrolla oli 200ms.

| #  | Player          | Rating  | Points  | Played | (%) |
|----|-----------------|---------|---------|--------|-----|
| 1  | Minimax0.3.2    | 2220.0  | 1102.5  | 2000   | 55  |
| 2  | Baislicka       | 2197.5  | 1003.0  | 2000   | 50  |
| 3  | Robocide        | 2172.0  |  894.5  | 2000   | 45  |

Yllättävästi Baislicka sijoittui ylemmäksi kuin Robocide, toisin kuin CCRL-sivustolla. Suoriutumisessa voi tietysti olla eroja erilaisia vastustajia vastaan pelatessa.

Testasin myös eri kehitysversioita tekoälystä toisiaan vastaan. 220 peliä per pari 100ms mietintäajalla.

| #  | Player          | Rating  | Points  | Played | (%) | Features / Changes in Version                                     |
|----|-----------------|---------|---------|--------|-----|-------------------------------------------------------------------|
| 1  | Minimax0.3.1    | 2160.0  | 1247.5  | 1540   | 81  | Null Move Pruning                                                 |
| 2  | Minimax0.3.0    | 2160.0  | 1247.5  | 1540   | 81  | Delta and futility pruning                                        |
| 3  | Minimax0.2.1    | 2057.5  | 1091.5  | 1540   | 71  | Check extensions and Late Move Reductions                         |
| 4  | Minimax0.2.0    | 1969.2  | 944.5   | 1540   | 61  | Better evaluation function (endgame and pawn structure)           |
| 5  | Minimax0.1.3    | 1696.1  | 469.0   | 1540   | 30  | History heuristic for move ordering and SEE bug fixes             |
| 6  | Minimax0.1.2    | 1692.2  | 462.5   | 1540   | 30  | Killer Heuristic for move ordering and better draw awareness      |
| 7  | Minimax0.1.1    | 1623.8  | 351.0   | 1540   | 23  | Principal Variation Search and move ordering bug fixes            |
| 8  | Minimax0.1.0    | 1620.9  | 346.5   | 1540   | 22  | Iterative deepening, tranposition table, quiescence search        |

### Suorituskyvystä

Tuorein versio tekoälystä tutkii noin 5 miljoonaa solmua sekunnissa, riippuen myös hieman laudan tilanteesta.
Tämä on hidastunut alun 10+ miljoonasta, kun on lisätty heuristiikkoja ja vaativampi evaluaatiofunktio.

