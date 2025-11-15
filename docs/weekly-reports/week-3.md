# Viikkoraportti 3

Tällä viikolla toteutin lisää testejä laudan bitboard-esitykselle ja loin tekoäly-pelaajien
luomiseen järjestelmän, joka mahdollistaa konfigurointiasetukset. Toteutin myös ensimmäisen version
minimax-tekoälystä alpha-beta karsinnalla käyttäen negamax implementaatiota. Tähän liittyen
oli myös luotava evaluaatio laudalle. Tämä toteutin lisäämällä laudan toteutuksen
päälle uuden kerroksen, joka dynaamisesti laskee siirtojen aiheuttamat muutokset evaluaatioon.

Evaluaatio ottaa nyt huomioon vain nappuloiden sijainnin ja määrät. Shakkimattia ja tasapeliä
ei vielä huomioida. Käyttöliittymäkään ei vielä tunnista pelin loppumista,
joten tarkoitus olisi nämä toteuttaa ensi viikon aikana.

Testasin näin aluksi järjestää siirtoja priorisoimalla kaappaavia siirtoja ja sotilaan korotuksia.
Selkeästi siirtojen järjestyksellä on suuri merkitys, tämäkin nopeutus oli noin kymmenkertainen.
Minimax pystyy tällä hetkellä alle sekunnissa laskemaan viiden siirron syvyydelle.
En ole paljon shakkia harjoitellut, joten jo nyt alan häviämään tekoälylle. Selkeästi se osaa rankaista
yksittäisiä huonoja siirtoja.

Testikattavuusraportti on nyt nähtävissä Codecov-palvelussa. Haarautumakattavuuden ongelmat selvisivät
lopulta helposti, kun siirryin käyttämään gcovr-työkalua gcov+lcov yhdistelmän sijaan. Se tarjoaa flagin
```--exclude-throw-branches```, ja näyttäisi että käytännössä kaikki haarat, joita ei voinut testata
olivat näitä virhehaaroja.

Ensi viikolla suunnitelmana on toteuttaa käyttöliittymä valmiiksi siten, että pelin voi pelata loppuun asti,
sekä mahdollisesti toteuttaa tekoälyn parametrien konfigurointi käyttöliittymän kautta. Tekoälyllä myös on
otettava huomioon pelin loppuminen evaluaatiossa. Seuraavia optimisaatioita joita ajattelin toteuttaa, ovat
transpositiotaulu, iteratiivinen syveneminen sekä siirtojen järjestäminen näihin liittyen ja muilla tekniikoilla,
kuten [MVV-LVA](https://www.chessprogramming.org/MVV-LVA). Myös laudan evaluaatiota on tarpeen kehittää.

Tarkoitus on mahdollistaa näiden eri optimisaatioiden konfigurointi, ja luoda myös komentorivikäyttöliittymä,
jolla voi simuloida suuremman määrää pelejä eri tekoälyillä ja siten mitata optimisaatioiden vaikutuksia.
Tuloksista saisi hyviä kuvia esimerkiksi testausdokumenttiin.

Viikon aikana tuli vastaan [Efficiently updatable neural network (NNUE)](https://en.wikipedia.org/wiki/Efficiently_updatable_neural_network).
Menetelmä ilmeisesti mahdollistaa dynaamiset muutokset laudan evaluaatioon neuroverkoilla.
Pitää tutustua tähän vielä tarkemmin. En tiedä olisiko toteustuskelpoinen ja miten paljon eroaa normaalista neuroverkosta.
Tekemistä kyllä riittää jo muiden optimisaatioiden kanssa.

Viikon työaika: 16h
