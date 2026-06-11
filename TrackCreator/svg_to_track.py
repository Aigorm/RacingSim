import os
from svgpathtools import svg2paths

def main():
    print("=== Konwerter Torow: SVG -> TXT (Auto-Centrowanie) ===")
    
    # Parametry Twojego okna w SDL2
    WINDOW_WIDTH = 1280.0
    WINDOW_HEIGHT = 720.0
    
    # 1. Pobieranie danych od użytkownika
    filename = input("Podaj nazwe pliku SVG (bez rozszerzenia): ").strip()
    if filename.lower().endswith('.svg'):
        filename = filename[:-4]
        
    svg_file = f"{filename}.svg"
    txt_file = f"{filename}.txt"

    if not os.path.exists(svg_file):
        print(f"\n[BLAD] Plik '{svg_file}' nie istnieje w tym folderze!")
        return

    try:
        outer_width = float(input("Podaj szerokosc toru na wewnatrz (w pikselach): "))
        inner_width = float(input("Podaj szerokosc toru do zewnatrz (w pikselach): "))
    except ValueError:
        print("\n[BLAD] Szerokosc musi byc liczba!")
        return

    # 2. Wczytywanie pliku SVG
    try:
        paths, attributes = svg2paths(svg_file)
    except Exception as e:
        print(f"\n[BLAD] Nie udalo sie sparsowac pliku SVG: {e}")
        return

    if not paths:
        print("\n[BLAD] Nie znaleziono zadnych sciezek w pliku SVG.")
        return

    track_path = paths[0]
    
    # 3. Obliczanie punktów 
    path_length = track_path.length()
    density = 10.0 # 1 punkt na każde 10 pikseli
    num_points = max(int(path_length / density), 10)

    print(f"\nGenerowanie {num_points} punktow...")

    # Zbieramy surowe punkty do listy
    raw_points = []
    for i in range(num_points):
        t = i / float(num_points)
        raw_points.append(track_path.point(t))

    # 4. Matematyczne centrowanie (Magia)
    # Znajdujemy skrajne wartości X i Y
    min_x = min(p.real for p in raw_points)
    max_x = max(p.real for p in raw_points)
    min_y = min(p.imag for p in raw_points)
    max_y = max(p.imag for p in raw_points)
    
    # Środek samego toru
    track_center_x = (min_x + max_x) / 2.0
    track_center_y = (min_y + max_y) / 2.0
    
    # Środek naszego okna w SDL2
    target_center_x = WINDOW_WIDTH / 2.0
    target_center_y = WINDOW_HEIGHT / 2.0
    
    # O ile musimy przesunąć każdy punkt, żeby wyrównać środki
    offset_x = target_center_x - track_center_x
    offset_y = target_center_y - track_center_y

    # 5. Zapis do pliku TXT z nałożonym przesunięciem
    try:
        with open(txt_file, 'w') as f:
            f.write(f"{outer_width} {inner_width}\n")
            
            for p in raw_points:
                # Przesuwamy punkt i zapisujemy
                final_x = p.real + offset_x
                final_y = p.imag + offset_y
                f.write(f"{final_x:.2f} {final_y:.2f}\n")
                
        print(f"[SUKCES] Plik zapisany poprawnie jako: {txt_file} (Tor zostal wycentrowany!)")
        
    except Exception as e:
        print(f"\n[BLAD] Wystapil problem podczas zapisu: {e}")

if __name__ == "__main__":
    main()