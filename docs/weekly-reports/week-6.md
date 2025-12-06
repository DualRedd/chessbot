# Viikkoraportti 5

Alkuviikosta tuli kirjoitettua siirtojen generointi bitboardeilla uudestaan.
Tajusin, että oliorakenne tosiaan toi hieman hitautta generointiin.
Tämä ei kuitenkaan ollut pääsyy, sillä halusin myös mahdollistaa tehokkaan generoinnin
syönneille, hiljaisille siirroille ja shakin väistöille.
Pseudo-laillisten siirtojen suodattamisessa laillisiksi siirroiksi siirrryin myös
tehokkaampaan tapaan, joka ei tee ja peru koko siirtoa, vaan tarkistaa suoraan tarvittavat ehdot.
Ja koska shakkitilanteissa nyt generoidaan vain väistöt, ehdot siirron laillisuudelle ovat yksinkertaisempia.
Samalla tiivistin siirtojen pakkauksen 32-bittisestä 16-bittiseksi.

Aikaisempi toteutus kävi [perft-testauksessa](https://www.chessprogramming.org/Perft) läpi 20 miljoonaa solmua sekunnissa.
Uudempi 45+ miljoonaa solmua sekunnissa. Isompi muutos on kuitenkin quiescence-haussa, jossa voidaan nyt generoida vain syönnit.
On myönnettävä, että tutkitun syvyyden kannalta lineaariset nopeutukset eivät ole yhtä merkitseviä.
Satuin kuitenkin innostumaan tästä osa-alueesta. Laudan esitys tarjoaa nyt kuitenkin uusia apufunktioita,
esimerksi sidottujen nappuloiden käsittelyyn, joita tarvitsee tietyissä algoritmeissa,
kuten [SEE](https://www.chessprogramming.org/Static_Exchange_Evaluation).

Testasin nostaa hiljaisissa siirroissa upseerien siirrot ennen muita, mistä mainitsit keskustelussamme.
Se vaikutti merkittävästi etenkin quiescence-haussa karsittujen solmujen määrään.
Uskoisin, että SEE:n lisäämisellä olisi tätäkin suurempi vaikutus,
sillä tällöin voitaisiin jättää tutkimatta syönnit, jotka ovat häviäviä materiaalin kannalta.
Sitä voi hyödyntää yleisestikin siirtojen järjestämisessä.

Keskustelun pohjalta korjasin monia pieniä virheitä minimax-toteutuksessa.
Iteratiivisessä syvenemisessä parhaan siirron järjestäminen juuressa toimi sittenkin oikein, joten "ilmainen" nopeutus
jäi siltä osin saamatta. Juuresta kuitenkin puuttui beta-cutoffin tarkistus. Varmistin, että aikakatkaisu toimii oikein.
Nyt ennen kuin alisolmun arvoa käytetään tarkistetaan lopetusehto, ja palataan heti ylös sen toteutuessa.
Tosin vielä voisi varmaan paremmin tallentaa saatua tietoa transpositiotauluun, sen sijaan että vain hylätään koko tulos.
Mietin, että jos osa alisolmuista on tutkittu kokonaan, kannattaisiko transpositiotauluun tallentaa paras löydetty siirto
ja sen evaluaatio alarajana.
Pohdin myös, että näiden osittaisten tulosten ei pitäisi täyttää transpositiotaulua,
sillä niitä olisi vain yksi kappale joka syvyydellä.

Siirryin nyt täysin fail-softiin, eli palautusarvot voivat olla alle alphan tai yli betan.
Tällä oli pieni (alle 5%) vaikutus puun karsintaan.
Aspiraatioikkunan ei pitäisi vaatia fail-softia, sillä uudelleenlaskennan tarpeen voi tunnistaa
siitä, onko arvo tasan alpha tai tasan beta. Tajusin tätä kirjoittaessani, että minulla on vielä tuo sama ehto käytössä,
joten sen voisi nyt muuttaa tiukaksi epäyhtälöksi.
Quiescence-haussa en enää käytä transpositiotaulua siirtojen järjestämiseen, sillä sen pois jättäminen nopeutti
hakua noin 20%. Quiescence-haku myös huomioi nyt shakkitilanteet, eli jatkaa silloin tutkimista.

Viikon aikana aloitin testaamaan suorituskykyä ja keskustelumme pohjalta myös puun karsintaa.
Käyn siis samat positiot läpi tietyllä syvyydellä, ja mittaan käytyjen solmujen määrää.
En ole vielä ihan varma miten hakuhistoriaa kannattaisi ottaa mukaan testissä,
joten nyt tekoäly aloittaa sijainnin tutkimisen nollasta.
Voisin ehkä pakottaa aina saman siirtosarjan, jolloin transpositiotaulun vaikutusta
saisi mitattua paremmin usean peräkkäisen haun aikana.

Viikon lopuksi tuloksina alpha-beta haun nopeus koneellani on reilu miljoona solmua sekunnissa ja quiescence-haun noin 5-7 miljoonaa solmua sekunnissa. Tekoäly pääsee nyt lähes poikkeuksetta vähintään 9 siirron syvyydelle viidessä sekunnissa.
Aspiraatioikkuna testeissä toimii parhaiten, kun se on 20-25 pisteen suuruinen suuntaansa. Quiescence-haku varmaankin tekee heuristiikan arvoista stabiileja, jonka vuoksi pieni ikkuna toimii.

En saanut vielä kirjoittetua testaus- ja toteutusdokumentteja, joten se siirtyy ensi viikolle.
Testit alkavat ainakin valmistua. Vielä voisin testata yksikkötesteissä ainakin shakkimatin löytämistä tekoälyllä.
Yritän saada myös testattua tekoälyä verkkossa olevaa tai paikallista tekoälyä vastaan
ja saada tuloksen sen ELO-vahvuudelle.
Pitää katsoa mitä parannuksia ennen demoa ehtii vielä toteuttamaan. Null-window-search olisi kyllä kiinnostava.

Viikon työaika: 28h
