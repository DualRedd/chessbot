# Toteutusdokumentti

Projekti toteuttaa määrittelydokumentissa suunnitellut ominaisuudet ja pääsin myös soveltamaan paljon muitakin menetelmiä, jotka eivät tuolloin vielä olleet tuttuja.
Ohjelma koostuu kolmesta pääosasta. Projektin ydin moduuli sisältää bitboardeilla toteutetun shakkilaudan esityksen ja siirtojen generoinnin. Tätä ydintä hyödyntää
käyttöliittymä sekä tekoäly, jotka ovat omat moduulinsa. Lisäksi on yksikkötestejä ja suorituskykytestejä.

## Toteutetut ominaisuudet

### Ydin

Laudan esitys tukee kaikkia shakin siirtoja, eli myös ohestalyöntiä, linnoittautumista ja korotusta. Lauta on toteutettu hyvin tehokkaasti bitboardeja käyttäen.
Siirtojen generointi hyödyntää myös bitboardeja. Käytössä on lisäksi sanottu magic-bitboard menetelmä liukuvien nappuloiden siirtojen generoimisessa.
Siirtojen generaattori tukee myös pelkkien syöntien, hiljaisten siirtojen sekä shakin väistöjen generointia.

### Käyttöliittymä

Käyttöliittymässä voi pelata kehitettyä tekoälyä vastaan, sekä laittaa sen pelaamaan itseään vastaan. Käyttöliittymä mahdollistaa peruslaatuiset konfiguraatioasetukset
tekoälylle, kuten mietintäajan ja syvyyden rajoittamisen, sekä muut tekoälykohtaiset parameterit. Käyttöliittymä huomioi tasapelit siirtojen toiston tai 50-siirron
säännön perusteella sekä muutamat yksinkertaiset tapaukset, joissa materiaalin vähyys merkitsee varmaa tasapeliä.

Pelit tekoälyllä itseään vastaa eivät ole aina kaikista kiinnostavimpia tasaisen tason ja saman strategian vuoksi, joten loppuvaiheessa toteutin vaihtoehdoksi myös
valita pelaajaksi minkä tahansa komentorivillä toimivan uci-standardia käyttävän tekoälyn. Tämä kuitenkin on tuettu tällä hetkellä vain linux-pohjaisilla käyttöjärjestelmillä.

### Shakkitekoäly

Shakkitekoäly on integroitu käyttöliittymään, joten sen konfigurointi sen kautta on helppoa. Tekoälyllä on myös uci-standardia tukeva komentoriviohjelma,
mikä mahdollistaa muun muassa sen testaamisen muita tekoälyjä vastaan. Seuraavaksi ominaisuuksia, joita tekoäly toteuttaa.

#### Alpha-beta karsinta

Minimax-algoritmin ensimmäinen optimisaatio. Toimii pohjana kaikille pelipuun karsintamenetelmille.
Sikäli turvallinen, että se voi karsia vain aidosti huonompia siirtoja. Siis ainakin heuristiikan mukaan huonompia siirtoja.

#### Laudan evaluaatio

Tekoälyn on jotenkin arvioitava pelitilanteen hyvyyttä, sillä pelin pelaaminen loppuun asti on mahdotonta rajallisessa ajassa.
Tekoäly ottaa huomioon nappuloiden arvojet ja sijainnit. Nämä arvot perustuvat yleiseen shakkiteoriaan. Nappuloiden sijaintien
arvo on myös jaettu alku- ja loppupeliin, joten pelin aikana sijaintien pisteytys muuttuu materiaalin huventuessa.

Lisäksi tekoäly arvioi sotilaiden rakennetta. Miinusta tulee esimerkiksi kaksin- tai kolmikertaisista sotilaista samalla pylväällä,
sotilaista ilman naapuria ja sotilaista joiden etenemistä estää vihollisen sotilas ilman että omat sotilaat voivat auttaa.
Plussaa tulee taas sotilaista, jotka ovat päässeet vihollisten sotilaiden ohi eli ovat todennäköisempiä pääsemään laudan päätyyn.

Evaluaatiofunktio toimii inkrementaalisesti, eli jokaisen siirron aikana tehdään vain tarvittavat muutokset evaluaatioon,
eikä koko arvoa lasketa alusta asti. Poikkeuksena sotilaiden evaluaatio ei toimi inkrementaalisesti, sillä päivittämisen logiikka
itsessään olisi melkein yhtä raskas kuin uudelleenlaskenta. Sen sijaan, koska sotilaiden rakenne muuttuu suhteellisen hitaasti,
niiden evaluaatiot tallennetaan pieneen hajautustauluun, jolloin niitä ei tarvitse yhtä usein laskea uudestaan. Testeissä hajautustaulun
osumaprosentti on ollut 95-98 % luokkaa.

#### Transpositiotaulu

Kun minimax laskee tietyn solmun arvon tietyllä syvyydellä, tulos tallennetaan transpositiotauluun, jotta sitä voidaan hyödyntää
uudestaan, kun tullaan samaan laudan tilanteeseen. Tällöin toisella kertaa parhaimmassa tapauksessa solmua ei tarvitse tutkia
uudestaan, sillä tranpositiotaulun perusteella tiedetään se esimerkiksi liian huonoksi.

Alpha-beta karsinnan tehokkuus perustuu siirtojen järjestämiseen. Transpositiotaulu auttaa järjestämisessä, sillä vaikka tallennettu arvo
ei annan suoraan vastausta solmun arvolle, tallennettu paras siirto voidaan testata ensimmäisenä. Koska se on todennäköisesti edelleen hyvä,
alpha-beta karsinta voi toimia tehokkaammin seuraavilla testattavilla siirroilla.

Transpositiotaulu on siis hajautustalu. Avaimena toimii 64-bittinen Zobrist hajautusarvo. Hajautusarvo lasketaan nappuloiden sijaintien,
siirtovuoron ja muiden erikoissiirtoihin liittyvän tilan perusteella. Sitä voidaan päivittää tehokkaasti inkrementaalisesti siirtojen yhteydessä.
Itse asiassa sotilaiden hajautustaulun avain toimii samoin, mutta sisältää tietoa vain sotilaiden sijainnista.

#### Iteratiivinen syveneminen

Sen sijaan, että etsitään suoraan lukitulle syvyydelle, lisätään etsintäsyvyyttä yksi kerrallaan. Ehkä aluksi järjenvastaiselta kuulostava
menetelmä vaikuttaisi tutkivan vain enemmän solmuja. Iteratiivinen syveneminen kuitenkin täyttää nopeammin transpositiotaulua alustavilla tuloksilla,
jolloin jokaisella seuraavalla iteraatiolla siirtojen järjestäminen on tarkempaa, ja alpha-beta karsinta tehostuu.

Tämä myös mahdollistaa helpon tavan toteuttaa aikakatkaisun tekoälyn mietinnälle. Kun lopetussignaali tulee, voidaan nykyinen iteraatio keskeyttää.
Se ei ole ongelma, sillä aikaisemman iteraation tulos on jo saatavilla. Yksinkertaisinta on vain heittää osittaisen haun tulos hukkaan,
mutta itseasiassa tuloksia voi tietyin ehdoin myös hyödyntää. Tässä on oltava varovainen, sillä löysin tekoälystäni pariin otteeseen
virheen keskeytetyn haun käsittelyssä. Nyt toteutuksen pitäisi olla oikein, mutta jokatapauksessa vaikutus tekoälyn vahvuuteen oli lopulta melko pieni.

#### Aspiraatioikkuna

Tämä oli alkuun käytössä ennen PVS-hakua, jonka kanssa se on hankalampi yhdistää. Ideana on rajata alpha-beta ikkunan kokoa iteratiivisen syvenemisen
aikaisempien tulosten perusteella, ja näin tehostaa karsintaa. Mikäli tulos onkin ikkunan ulkopuolella, on haku tehtävä uudestaan täydellä tai
kasvetetulla ikkunalla.

#### Quiescence haku

Hiljaisuus-haun tarkoitus on välttää arvioimasta taktisesti jännittyneitä tilanteita heuristiikan perusteella. Sen sijaan minimax-tyylillä jatketaan
puun tutkimista, mutta vain testaten syöntejä sekä shakin väistöjä. Ideana on päätyä hiljaiseen tilaan, jossa ei ole akuuttia taktista uhkaa.
Tämä estää horizon-efektin, jossa tekoäly lopettaa haun juuri ennen suurta menetystä.

Tämä haku voi helposti viedä yli kymmenkertaisen määrän solmuja normaalin alpha-beta rutiinin verrattuna. Alkuun näin olikin, mutta
monien menetelmien avulla tätä on saatu rajoitettua, ja se on nyt suhteellisen samalla tasolla alpha-betan kanssa.
Näitä menetelmiä esimerksi Static Exchange Evaluation (SEE) ja Delta Pruning.

#### Static Exhange Evaluation

Nopea tapa arvioida onko syönti voittava. Simuloi vain tarvittavat muutokset, eikä tee jokaista siirtoa. Ideana on simuloida, kuinka pelaajat
syövät nappuloita samalla ruudulla, eli aina vuorotellen syödään vähiten arvokkaalla nappulalla.

Tätä käytän kahdessa paikassa. Quiescence haussa kaikki häviävät syönnit jätetään tutkimitta. Siirtojen järjestämisessä syönnit jaetaan
voittaviin ja häviäviin. Voittavat syönnit ovat alkupäässä ja häviävät vasti hiljaisten siirtojen jälkeen.

#### Principal Variation Search (PVS)

Principal variation tarkoittaa sitä siirtosarjaa, jonka tekoäly uskoo olevan paras pelitilanteessa.
Se siis sisältää arvioidut omat ja vastustajan parhaat vastaukset joka vuorolla.
PVS-haun ideana on tutkia ensimmäiseksi järjestetty siirto (paras siirto, eli PV siirto) täydellä alpha-beta ikkunalla
ja kaikki muut siirrot nollaikkunalla. Nollaikkunalla yritetään todistaa, että siirto on huonompi kuin kuin ensimmäinen.
Jos siirto osoittuukin paremmaksi, on se tutkittava uudestaan (ja siitä tulee uusi PV siirto).
Menetelmä vaatiikin hyvää siirtojen järjestystä.

#### MVV-LVA

Yksinkertainen tapa järjestää syöntejä. Yritetään syödä mahdollisimman arvokas nappula mahdollisimman arvottomalla.

#### Killer Heuristic

Menetelmä siirtojen järjestämiseen. Pidetään taulussa kirjaa muutamasta hyvästä hiljaisesta siirrosta, jotka saman
haun aikana aiheuttivat beta-cutoffin samalla syvyydellä. Nämä voidaan järjestää ennen muita hiljaisia siirtoja.

#### History Heuristic

Toinen menetelmä hiljaisten siirtojen järjestämiseen. Osittain samankaltainen kuin Killer heuristic, mutta historia pysyy
eri hakujen välillä. Voidaan ajatella kuvaavan pelin strategista suuntaa.

#### Check Extensions

Jos siirto tuottaa shakin, kasvatetaan haun syvyyttä yhdellä. Yksinkertainen ja toimiva. Itse hyödynsin tässä jälleen lisäksi SEE-funktiota,
ja kasvatan syvyyttä yhdellä vain, jos siirto ei menetä materiaalia. Tästä aletaan päästä menetelmiin, jotka muokkaavat puun rakennetta
huomattavasti. Kiinnitetään siis enemmän huomioita mahdollisesti taktisesti tärkeisiin siirtoihin.

#### Late Move Reductions

Perustuu hyvään siirtojen järjestykseen. Ensimmäiset 3-4 siirtoa tutkitaan täydellä syvyydellä. Muiden siirtojen syvyyttä lasketaan
jonkin lukitun arvon tai edistyneemmän kaavan perusteella. Jälleen voi vaatia uudelleenhaun, jos oletus siirron huonoudesta ei pädekään.
Yhdessä PVS haun kanssa voi tarkoittaa, että joskus voidaan joutua tehdä kaksi uudelleenhakua.

#### Futility and Delta Pruning

Kaksi nimeä samalle idealle. Delta pruning on käytössä quiescence haussa ja futility pruning alpha-beta päärutiinissa.
Jos solmun evaluaatio on hyvin paljon alle alphan, voidaan tietyin ehdoin ja marginaalein jättää tutkimatta siirtoja tai koko solmu,
sillä on epätodennäköistä, että arvo enää nousisi yli alphan.

#### Null Move Pruning

Kiinnostava menetelmä. Jos solmun evaluaatio on yli betan, voidaan tehdä tyhjä siirto ja antaa vuoro vastustajalle.
Tämän jälkeen tutkintasyvyyttä lasketaan huomattavasti ja katsotaan nollaikkunalla, pysyyko arvo yli betan.
Haku on hyvin halpa. Ideana on, että jos tilanne on niin hyvä, että tekemättä siirtoa se pysyy hyvänä, voidaan olettaa,
että tekemällä siirron se myös pysyy hyvänä. On kuitenkin olemassa harvinaisia tilanteita, joissa olisi
parasta olla tekemättä mitään. Nämä ovat yleisempiä tietyissä loppupeleissä, joten menetelmän voi kytkeä tällöin pois käytöstä.
