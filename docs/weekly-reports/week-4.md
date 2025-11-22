# Viikkoraportti 4

Tällä viikolla oli odotettua vähemmän aikaa projektin työstämiseen. Toteutin käyttöliittymään sivupalkin,
jossa voi konfiguroida pelaajat, peruuttaa siirtoja ja aloittaa uuden pelin. Tätä tehdessä harmikseni
satuin löytämään TGUI-kirjastosta use-after-free -bugin, jonka karttamiseen meni ylimääräistä aikaa.

Minimax algoritmi huomioi nyt myös shakit ja mattitilanteet. Myös käyttöliittymä on melkein valmis tämän suhteen,
eli pystyy pian huomioimaan pelin loppumisen sekä tasapelit toistuvan aseman ja 50-siirron säännön perusteella.
Toteutin myös Zobrist arvojen laskennan, mutta en ole vielä testannut sitä. Vähän "näkymätöntä" työaikaa
meni myös tekoälyjen rajapinnan kehittämiseen, jotta kommunikointi toisella
säikeellä laskevan tekoälyn kanssa onnistuisi ongelmitta.

Käyttöliittymä on nyt siis lähes valmis, joten voin seuraavaksi keskittyä taas enemmän tekoälyn kehittämiseen.
Ensi viikolla jatkan oikeastaan samojen asioiden parissa mitä mainitsin viime viikolla. Transpositiotaulu on
ainakin nopea toteuttaa nyt kun Zobrist arvot ovat käytettävissä.

Viikon työaika: 11h
