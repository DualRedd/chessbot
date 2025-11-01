# Määrittelydokumentti
Opinto-ohjelma: tietojenkäsittelytieteen kandidaatti

Projektin tavoitteena on luoda shakkibotti käyttäen minimax-algoritmia ja alpha-beta-karsintaa. Alpha-beta-karsintaa on tavoite tehostaa hyödyntämällä transpositiotaulua (eng. transposition table) ja iteratiivista syvenemistä (eng. iterative deepening). Iteratiivinen syveneminen mahdollistaa samalla helpon tavan rajoittaa botin ajatteluaikaa. Muita optimisaatioita olisi kiinnostavaa myös tutkia, mikäli aikaa riittää. Lautojen arviointiin on luotava heuristiikkafunktio. Tarkoitus ottaa huomioon ainakin pelinappuloiden määrä, laatu ja sijainnit. Tätä voi sitten kehittää pidemmälle ajan riittäessä.

# Tekninen toteutus

Kielenä c++. Vertaisarviointia voin tehdä lisäksi ainakin Python, C# ja Java -kielillä.
Projektin käännösten ja rakennusprosessin hallintaan CMake. Alustavasti kehitys tapahtuu Linux-ympäristössä, mutta myöhemmin voidaan lisätä tuki myös Windowsille.
Yksikkötestaukseen GoogleTest ja mahdollisesti suorituskyvyn testausta tarvittaessa GoogleBenchmark.
Riippuvuuksien hallintaan ei ole tarkoitus käyttää erillistä työkalua CMaken lisäksi.

Käyttöliittymän luontiin ajattelin käyttää SFML-kirjastoa, jolla on nopea toteuttaa yksikertainen interaktiivinen shakkilauta. Tavoitteena on, että soveluksella voisi pelata eri bottiversioita vastaan tai laittaa botit pelaamaan toisiaan vastaan. Tällöin olisi helppo verrata miten eri optimisaatiot vaikuttavat botin tasoon. Graafisen käyttöliittymän lisäksi ohjelmaa voisi ajaa myös komentoriviltä, jos haluaa esim. simuloida useampia pelejä nopeasti.

Laudan esittämiseen käytetään boteilla bitboard-esitystä, joka tehostaa muun muassa siirtojen generointia. Suunnittelin, että botit kommunikoisivat pelin kanssa käyttäen UCI-siirtoformaattia ja Forsyth–Edwards-notaatiota laudalle. UCI mahdollistaa pelin ohjaamisen myös komentoriviltä. Testejä on myös helpompi luoda käyttäen näitä ihmiselle luettavia notaatioita. Pieni ajatus tässä taustalla on, että botteja voisi näin myös testata jotain verkosta löytyviä botteja vastaan. Tämä ei tietenkään ole mielekästä pitkälle kehitettyjä ja tunnettuja botteja vastaan, sillä tässä projektissa ei tulla pääsemään lähelle niiden tasoa.

# Jatkoa

Aloitusluennolla mainitsin kiinnostuneeni neuroverkkojen hyödyntämisestä heuristiikkafunktiona. Neuroverkkoja käytetään nykyäänkin osasssa shakkibotteja. Olen aikaisemmin koodannut MLP-verkon ja vastavirta-algoritmin, joten tämän toteuttaminen mahdollisesti onnistuisi järkevässä ajassa. Koulutusta varten voisi käyttää avoimia kokoelmia korkean tason pelejä (esim. https://database.lichess.org/). Uskon, että jo suhteellisen pienellä verkolla voisi päästä samalle tasolle kuin manuaalisesti luodulla heuristiikalla, joten koulutus olisi mahdollista toteuttaa järkevässä ajassa. Tämä tietenkin laajentaa projektia huomattavasti ja on mahdollista, että aika ei tähän riitä. Mainittakoon tämä siis vain lisätavoitteena.

# Lähteet

#### Yleistä tietoa shakkiboteissa käytetyistä tekniikoista:
- https://www.chessprogramming.org/Main_Page

#### Notaatioista:
- UCI: https://en.wikipedia.org/wiki/Universal_Chess_Interface
- FEN: https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation

#### Projektin riippuvuuksia:
- Cmake: https://cmake.org/
- SMFL: https://www.sfml-dev.org/
- GoogleTest: https://github.com/google/googletest
