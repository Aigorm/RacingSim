# 2D Autonomous Racing Simulator

## Zamysl Projektu
Ten projekt to zaawansowany symulator wyscigow samochodowych 2D, zaprojektowany nie jako standardowa gra zrecznosciowa, lecz jako arena algorytmiczna dla autonomicznych botow. Zamiast sterowania klawiatura, uzytkownicy pisza wlasne skrypty, ktore analizuja telemetrie pojazdu w czasie rzeczywistym i podejmuja decyzje o kacie skretu, przyspieszeniu i hamowaniu.

Symulator kladzie ogromny nacisk na fizyke jazdy. Silnik (Physics Engine) implementuje realistyczny model kinematyczny, uwzgledniajac mase pojazdu, sile docisku aerodynamicznego, slizg oraz degradacje opon, zmuszajac tworcow botow do optymalizacji trajektorii i predkosci, a nie tylko podazania "po sznurku".

## Glowne Funkcjonalnosci
* Realistyczna Fizyka: Implementacja oporow toczenia, oporu aerodynamicznego, limitow przyczepnosci bocznej opon oraz zuzycia ogumienia.
* Bezbledny System Sedziowski: Wykorzystanie matematyki wektorowej i detekcji przeciecia odcinkow do weryfikacji okrazen i eliminacji oszustw.
* Telemetryczny Interfejs Botow: Zunifikowane srodowisko, w ktorym kazdy bot operuje na identycznych danych wejsciowych (pozycja wlasna, przeciwnicy, uklad toru).
* Narzedzie TrackCreator: Zautomatyzowany konwerter plikow graficznych SVG na wektorowe trasy wyscigowe gotowe do importu przez silnik.

---

## Struktura Projektu

Projekt zostal podzielony na logiczne moduly zgodnie z zasadami programowania zorientowanego obiektowo:

* API/ - Interfejsy i architektura wtyczek. Zawiera klase bazowa IBot oraz BotRegistry do automatycznej rejestracji sztucznej inteligencji.
* assets/ - Folder docelowy na wygenerowane pliki tekstowe tras (.txt).
* Bots/ - Implementacje konkretnych algorytmow sterujacych (np. LineFollowerBot, IgorBot). To tutaj piszesz swoj kod wyscigowy.
* Core/ - Jadro symulatora. Zawiera PhysicsEngine (obliczenia wektorowe), TrackLoader (parsowanie map) oraz Renderer (obsluga SDL2).
* Shared/ - Struktury danych dzielone miedzy silnikiem a botami (np. Vector2D, Telemetry, ControlOutput).
* TrackCreator/ - Narzedzia pomocnicze, w tym skrypt Pythona do generowania nowych map.

---

## Kompilacja i Uruchomienie

### Wymagania systemowe
* Kompilator C++ (wspierajacy standard C++17 lub nowszy, np. g++).
* Biblioteka graficzna SDL2 (Simple DirectMedia Layer).

### Przykladowa kompilacja (Linux / MinGW)
Aby skompilowac projekt, nalezy zlinkowac wszystkie pliki .cpp oraz biblioteke SDL2. Bedac w glownym folderze projektu, uruchom:

```bash
g++ main.cpp Core/*.cpp Bots/*.cpp -o AutonomousRacing -lSDL2
```

Nastepnie uruchom plik wykonywalny:

```bash
./AutonomousRacing
```

---

## Dodawanie Nowych Torow (TrackCreator)

Projekt wspiera dowolne, niestandardowe tory wyscigowe. Aby stworzyc wlasny tor:
1. Narysuj petle uzywajac narzedzia sciezki (krzywe Beziera) w dowolnym programie wektorowym (np. Inkscape lub https://editor.graphite.art/) i zapisz jako zwykly plik .svg w folderze TrackCreator/ (upewnij się, że twój tor jest złożony z jednej "gładkiej" (bez kontów) krzywej która tworzy pętlę).
2. Upewnij sie, ze masz zainstalowanego Pythona i biblioteke: `pip install svgpathtools`.
3. Uruchom skrypt z poziomu folderu TrackCreator:
```bash
python svg_to_track.py
```
4. Podaj nazwe pliku oraz szerokosci jezdni. Skrypt automatycznie wysrodkuje tor, przeliczy krzywe na wektory i zapisze gotowy plik .txt bezposrednio w folderze assets/.

### Jak zaladowac nowy tor do symulatora?
Otworz plik main.cpp w glownym folderze i znajdz instrukcje odpowiedzialna za ladowanie sciezki. Zmien argument na nazwe swojego nowo wygenerowanego pliku:

```cpp
// Przyklad zmiany toru w main.cpp
TrackLoader loader;
TrackInfo myTrack = loader.loadTrack("assets/moj_nowy_tor.txt"); 
physics.setTrack(myTrack);
```

---

## Tworzenie Wlasnego Bota

Tworzenie nowych zawodnikow jest zautomatyzowane dzieki systemowi makr. Nie musisz modyfikowac petli glownej main.cpp, aby dodac auto!

1. Stworz nowy plik .cpp w folderze Bots/ (np. MyProBot.cpp).
2. Zalacz bazowe naglowki i odziedzicz klase po IBot.
3. Zaimplementuj metode on_tick(), ktora wylicza zachowanie co klatke.
4. Na samym koncu pliku uzyj makra REGISTER_BOT.

Szablon:

```cpp
#include "../API/IBot.h"
#include "../API/BotRegistry.h"

class MyProBot : public IBot {
public:
    void on_tick(const Telemetry& data, ControlOutput& out) override {
        // Tu zaimplementuj swoja logike (np. PID, Pure Pursuit, DP)
        out.throttle = 1.0f;       // Gaz (0.0 do 1.0)
        out.brake = 0.0f;          // Hamulec (0.0 do 1.0)
        out.steeringAngle = 0.0f;  // Skret (-1.0 do 1.0)
    }

    std::string getName() const override {
        return "MyProBot";
    }

    ColorRGB getColor() const override {
        return ColorRGB(0, 255, 0); // Zielony bolid
    }
};

// Ta linijka automatycznie dodaje bota do wyscigu przy kompilacji!
REGISTER_BOT(MyProBot)
```