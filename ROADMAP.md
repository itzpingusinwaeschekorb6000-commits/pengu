# Pengu – Roadmap

Pengu ist ein Habit-Tracker als Spiel, nach dem Vorbild der iOS-App „Monke – Habit Game“.
Statt eines Affen auf einer Tropeninsel lebt hier ein **Brillenpinguin** (African Penguin,
*Spheniscus demersus*) auf einer südafrikanischen Insel wie Boulders Beach: Granitfelsen,
Milkwood-Bäume, Proteas, Strelitzien, Holzstege. Die Währung sind **Sardinen** statt Bananen.

## Entscheidung: Technik

**C++17 + Qt 6 Quick (QML)**, gebaut mit `qmake6`/`make`, installiert als Arch-Paket.

- Läuft nativ auf dem Laptop (Omarchy/Hyprland, Wayland), keine Browser-Runtime nötig.
- QML ist stark bei animierten 2D-Oberflächen (Insel, Pinguin, Sheets, Swipe-Gesten);
  C++ hält die Logik testbar (Qt Test, offscreen).
- Folgt dem Omarchy-Theme (Akzentfarbe, Fensterrundung) und der Systemsprache (Deutsch).

## Architektur (auf Erweiterbarkeit ausgelegt)

| Schicht | Datei | Aufgabe |
|---|---|---|
| Daten | `src/data/items.json` | Shop-Katalog: Items, Varianten, Preise, Kategorien, Sprites. Neues Item = neuer JSON-Eintrag + SVG, kein Code. |
| Daten | `src/data/templates.json` | Habit-Vorlagen nach Ziel gruppiert. |
| Logik | `src/catalog.*` | Lädt Katalog + Vorlagen, Übersetzung der Namen (en/de). |
| Logik | `src/profile.*` | XP, Level-Kurve, Sardinen, Einstellungen. |
| Logik | `src/habits.*` | Habits, Tagesplan, Erledigt/Übersprungen, Belohnungen, Streaks, Statistik, Heatmap. |
| Logik | `src/island.*` | Insel-Raster, Terrain, platzierte Objekte, Inventar, Kauf, Wegfindung (BFS). |
| Logik | `src/reminders.*` | Desktop-Benachrichtigungen (Morgen, Abend, pro Habit). |
| Logik | `src/game.*` | Verdrahtet alles, speichert atomar (`~/.local/share/pengu/pengu.json`) mit Schema-Version + Migration. |
| Darstellung | `src/tilemap.*` | Zeichnet das isometrische Raster (C++ `QQuickPaintedItem`). |
| UI | `src/qml/*.qml` | Insel, HUD, Shop, Bau-Modus, Habits, Statistik, Einstellungen. |

Jedes Modul serialisiert sich selbst (`toJson`/`fromJson`). Ein neues Feature bekommt ein
eigenes Modul + eigenen Schlüssel im Speicherstand, ohne bestehende Module anzufassen.

## Schritte

- [x] **0. Analyse** – Video (6 min) Frame für Frame ausgewertet, App-Store-Recherche.
- [x] **1. Gerüst** – qmake-Projekt, Build/Test/Install-Skripte, Theme + Übersetzer.
- [x] **2. Datenmodell & Speicherung** – Katalog, Profil (Level-Kurve wie Monke: Lvl 4 = 44/70 XP bei 134 XP), atomares Speichern, Schutz vor kaputten Dateien.
- [x] **3. Habit-Engine** – Planung nach Wochentagen, Erledigt/Übersprungen/Rückgängig, Belohnung nach Schwierigkeit, Habit-Streaks, perfekte Tage, Statistik, Heatmap, strenge Streaks, Wochenstart.
- [x] **4. Insel** – isometrisches Raster, Wasser mit Flachwasser-Saum, Objekte mit Tiefensortierung, Lichtschein, Kamera (Ziehen, Zoom), Pinguin mit Wegfindung, Watscheln, Sitzen am Nest.
- [x] **5. Shop & Inventar** – Suche, Sortierung, Filter (Kategorie, „nur leistbar“), Varianten-Seiten, „Besitzt N“.
- [x] **6. Bau-Modus** – Kategorien-Leiste, Platzieren per Klick, Verschieben per Ziehen, Spiegeln, Entfernen (zurück ins Inventar), Terrain-Kacheln erweitern die Insel.
- [x] **7. Habits-UI** – To-Dos/Erledigt/Übersprungen, Gruppen, „Heute nicht geplant“, Wischen für Erledigt/Überspringen bzw. Bearbeiten/Löschen, Vorlagen, Formular mit „Hilf mir“-Tipps, Vorschau, Tagesnavigation.
- [x] **8. Statistik & Einstellungen** – Level-Balken, Streaks, Heatmap; Benachrichtigungen, Präferenzen, User-ID, Zurücksetzen.
- [x] **9. Härtung** – eigener Code-Angriff (Grenzfälle, kaputte Speicherstände, Doppelbuchungen, Zeitreisen), Tests, Übersetzungs-Check, Offscreen-Screenshots.
- [x] **10. Installation** – Arch-Paket, Starter-Eintrag, Icon.

## Bewusst nicht drin (Monke hat es im Video nicht gezeigt) – künftige Features

- Fokus-Sessions (Timer, die Sardinen geben)
- Freunde: weitere Tiere (z. B. Kaptölpel, Klippschliefer), die zufällig freigeschaltet werden
- Tag/Nacht-Zyklus und Wetter
- Mehr Land / mehrere Inseln
- Konto-Sync, Discord, Widgets
- Pinguin-Kosmetik (Mützen, Schals)

Neue Tiere passen in `island.*` als weitere „Bewohner“ neben dem Pinguin; Fokus-Sessions
werden ein eigenes Modul, das wie `habits` über `Profile::reward()` Sardinen vergibt.
