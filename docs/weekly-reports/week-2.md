# Viikkoraportti 2

Tällä viikkolla olen toteuttanut perusominaisuudet graafiseen käyttöliittymään.
Siirtojen generointi on valmis (mukaanlukien tornitus ohestalyönti ja sotilaan korotus).
Kaikki siirrot voi pelata käyttöliittymällä.
Lauta on toteutettu bitboard-esityksenä, joten siirtojen generointi pitäisi olla jo suhteellisen tehokasta.
Myöhemmin tarkoitus optimoida ainakin tornien ja lähettien hyökkäyksien generointia magic-bitboardeilla,
sillä nyt käytössä on vain yksinkertainen silmukka.

Viikon aikana olen oppinut ja lukenut lisää shakkimoottoreissa käytettyihin menetelmiin liittyen.
Aluksi aloin toteuttamaan laudan esitysta ja siirtojen generointia käyttöliittymää varten vähemmän tehokkaalla ja luettavammalla tavalla,
mutta tajusin onneksi nopeasti, että voisin aloittaa suoraan bitboard-esityksen toteuttamisen.
Tätä esitystä voi nyt suoraan hyödyntää myös tekoälyillä siirtojen generointiin.

Olen alustavasti aloittanut testaamaan siirtojen generointia ja niiden tekemistä/peruuttamista.
Ne vaikuttavat toimivan oikein. Aloitin myös toteuttamaan testikattavuusraporttien luontia.
C++:n kanssa pieni ärsytys näyttäisi olevan haarautumiskattavuuden mittaaminen, sillä gcov luo haarautumisdataa
monille riveille, joissa ei ole oikeasti testattavaa haaraa (esim. rivit jotka heittävät jonkun virheen).
Vähän olen tätä ehtinyt tutkia, ja ilmeisesti lcov:n luomista tracefile-tekstitiedostoista datan voisi poistaa manuaalisesti riveiltä,
joilta ei löydy vaikka if- tai while- lausetta.

Kysymys: Aikataulussa toisella viikolla on kohta "Projektin testikattavuus seurattavissa" - tarkoittaako tämä,
että raportin pitäisi olla julkisesti nähtävissä?

Ensi viikolla suunnittelin aluksi lisääväni testikattavuutta laudan bitboard-esitykselle. Käyttöliittymä on nyt perusominaisuuksiltaan valmis,
joten tarkoitus on lähteä toteuttamaan alustavaa versiota minimax-algoritmista ja heuristiikasta.
Toteutan todennäköisesti alpha-beta-karsinnan suoraan.
Kiinnostavaa olisi myös verrata eri versioita tekoälystä toisiinsa kehityksen aikana, joten pohdinnassa on miten eri versioiden luominen olisi järkevintä.
Yksinkertaisimmillaan toimisi koodin monistaminen aina uutta versiota luodessa, mutta konfigurointiasetukset voisivat myös toimia.
On kuitenkin joitain optimointeja, joiden päälle ja pois kytkemisen mahdollistaminen heikentäisi suorituskykyä jo itsessään.

Viikon työaika: 22h
