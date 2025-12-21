# Käyttöohje

Projektin kääntämiseen ohjeet löytyvät [README:stä](https://github.com/DualRedd/chessbot/blob/main/README.md).

## Graafinen käyttöliittymä

Graafisen käyttöliittymän    voi käynnistää projektin käännettyään komennolla
```bash
./chess_gui
```
tai Windows:illa myös ajamalla sen tiedostojenhallinnan kautta.

Käyttöliittymässä on oikealla sivupaaneeli, jossa voi konfiguroida pelilautaa ja pelaajia.
Napeista voi aloittaa uuden pelin, kääntää laudan tai peruuttaa siirron.
Valikoista voi valita pelaajat ja konfiguroida niiden asetuksia.
Muutetut konfiguraatiot otetaan käyttöön vasta, kun "uusi peli"-nappia on painettu. 

Minimax-pelaaja on tässä projektissa toteutettu tekoäly. Human-pelaajaa kontrolloidaan käyttöliittymässä
raahamalla nappuloita tai klikkaamalla ruutuja peräkkäin. Vain linuxilla tällä hetkellä tuettu
"UCI engine"-pelaaja mahdollistaa yhdistää minkä tahansa uci-standardia komentorivillä tukevan tekoälyn
käyttöliittymään. Sille on konfiguroitava komento, jolla se voidaan käynnistää komentoriviltä.

## Tekoälyn UCI komentorivikäyttöliittymä

Tekoälyn voi käynnistää projektin käännettyään komentorivillä komennolla
```bash
./minimax_cli
```

Se tukee kommunikointia [UCI-standardilla](https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf).
Tässä eräs esimerkkikommunikaatio:
```
> uci
< id name MyMinimax
< id author Haapiainen
< uciok
> ucinewgame
> position startpos
> go movetime 10
< info depth 1
< info depth 1 seldepth 1 score cp 37 nodes 1 nps inf time 0 pv b1c3
< info depth 2
< info depth 2 seldepth 2 score cp 0 nodes 22 nps inf time 0 pv b1c3 b8c6
< info depth 3
< info depth 3 seldepth 3 score cp 48 nodes 127 nps inf time 0 pv d2d4 b8c6 c1e3
< info depth 4
< info depth 4 seldepth 5 score cp 0 nodes 918 nps 2584000 time 1 pv d2d4 d7d5 c1e3 c8f5
< info depth 5
< info depth 5 seldepth 5 score cp 37 nodes 1602 nps 2288500 time 2 pv d2d4 d7d5 c1e3 c8f5 g1f3
< info depth 6
< info depth 6 seldepth 6 score cp 16 nodes 6500 nps 2799166 time 6 pv g1f3 e7e6 e2e4 f8d6 e4e5 d6c5
< info depth 7
< bestmove g1f3
```

## Testien ajaminen

Testit voi ajaa seuraavalla komennolla:
```bash
ctest --output-on-failure
```

Ne voi suoraan myös ajaa GoogleTest:n omalla käyttöliittymällä:
```bash
./tests/unit_tests
```

## Suorituskykytestien ajaminen

[Perft](https://www.chessprogramming.org/Perft)-testit voi ajaa tällä komennolla:
```bash
./benchmarks/perft
```

Ja puun karsinnan testit tällä:
```bash
./benchmarks/pruning
```
