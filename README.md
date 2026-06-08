# 2D Auto Racing Simulator

## Zamysł projektu

Projekt to symulator wyścigowy 2D, w którym główny nacisk położony jest nie na zręcznościowe sterowanie pojazdem, lecz na rywalizację algorytmiczną. Zamiast manualnej kontroli, gracze tworzą własne "boty" wyścigowe, pisząc kod logiki pojazdu w klasie dziedziczącej po wspólnym interfejsie. Co każdą klatkę symulacji (wewnątrz metody `onTick`), boty otrzymują pełną telemetrię wyścigu i na jej podstawie samodzielnie obliczają wektory przyspieszenia oraz skrętu.

Środowisko fizyczne opiera się na ciągłych modelach matematycznych. Kształt toru jest generowany analitycznie na podstawie linii środkowej zaczytywanej z pliku wektorowego (SVG). Pozwala to na precyzyjne i pozbawione błędów "schodkowania" obliczanie kolizji i granic jezdni. Projekt symuluje również zaawansowane aspekty wyścigowe, takie jak zużycie opon, zmuszając logikę graczy do optymalizacji strategii na torze i wyliczania optymalnych okien zjazdowych do pitstopu w zależności od zachowania rywali.

## Główne komponenty, ich zależności i cele

* **Silnik Fizyczny (Physics Engine)**
    * **Cel:** Symulacja realistycznej kinematyki pojazdów w przestrzeni 2D. Odpowiada za obliczanie przyczepności, poślizgu, degradacji opon oraz precyzyjne wykrywanie kolizji..
    * **Zależności:** Otrzymuje wejście (sterowanie) z Modułu Botów oraz ciągłe granice mapy z Modułu Toru.

* **Moduł Toru (SvgTrackLoader i Track Data)**
    * **Cel:** Przetwarzanie zewnętrznych plików z trasami do postaci matematycznej. Wczytuje ścieżkę wektorową (centerline) z pliku SVG, dokonuje parsingu danych i analitycznie generuje płynne zewnętrzne i wewnętrzne granice toru.
    * **Zależności:** Wymaga lekkiej biblioteki do parsowania XML (np. pugixml lub TinyXML-2). Przekazuje wyliczone wielokąty i wektory normalne do Silnika Fizycznego oraz Silnika Graficznego.

* **Architektura Botów (Bot API & Telemetry)**
    * **Cel:** Zapewnienie bezpiecznego i modułowego środowiska dla kodu graczy. Wykorzystuje polimorfizm (interfejs bazowy) oraz stałe referencje do przekazywania danych telemetrycznych (pozycja, wektory prędkości, stan mieszanki opon), chroniąc przed nieuczciwą modyfikacją stanu gry. Stosuje wzorzec statycznej rejestracji do automatycznego włączania nowych botów do wyścigu podczas kompilacji.
    * **Zależności:** Niezależny zbiór plików, który wchodzi w interakcję jedynie ze ściśle określoną strukturą danych dostarczaną przez rdzeń silnika.

* **Silnik Graficzny (Renderer)**
    * **Cel:** Wizualizacja aktualnego stanu symulacji. Odpowiada za renderowanie wygenerowanego kształtu toru, pojazdów (w oparciu o ich pozycje i rotację z silnika fizycznego) oraz rysowanie interfejsu diagnostycznego.
    * **Zależności:** Wykorzystuje zewnętrzną bibliotekę multimedialną (SDL2). Odczytuje dane w trybie "tylko do odczytu" z pozostałych komponentów po zakończeniu obliczeń logicznych w danej klatce.

```text
    src/
├── main.cpp
├── Core/
│   ├── PhysicsEngine.h / .cpp   (Obliczenia: poślizg, kolizje (toglable dla zderzeń wyścigówek ze sobą), ruch)
│   ├── Renderer.h / .cpp        (Rysowanie: okno, tor, sprite'y wyścigówek)
│   └── TrackLoader.h / .cpp     (Logika czytania SVG i generowania granic)
├── Shared/
│   └── Telemetry.h              (Tylko definicje struktur: CarState, TrackInfo)
├── API/
│   ├── IBot.h                   (Interfejs z czysto wirtualnym on_tick)
│   └── BotRegistry.h / .cpp     (Zarządca, który przechowuje listę skompilowanych botów)
└── Bots/
    ├── TwójBot.cpp              (przykładowe kody kod wyścigówek)
    └── BlagojaBot.cpp           

    compile test_bots:
```

```bash
g++ test_bots.cpp Bots/IgorBot.cpp Bots/BlagojaBot.cpp Bots/TractorBot.cpp Bots/TestBot.cpp -o bot_test
```
trzeba modyfikować zależnie od botów z których chcemy skorzystać

Renderowanie i fizyka:
```bash
g++ main.cpp Core/PhysicsEngine.cpp Core/Renderer.cpp -o racing_sim -lSDL2
```