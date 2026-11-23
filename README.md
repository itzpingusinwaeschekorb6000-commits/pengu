# Pengu

![Pengu: the penguin island](docs/island.png)

Ein Habit-Tracker als kleines Spiel, nach dem Vorbild der iOS-App „Monke – Habit Game“ –
aber mit einem **Brillenpinguin** (African Penguin) auf einer südafrikanischen Insel statt
eines Affen. Jede erledigte Gewohnheit bringt **Sardinen** und XP; mit Sardinen kaufst du im
Shop Milkwood-Bäume, Proteas, Granitfelsen, Strandhütten, einen Leuchtturm und mehr und baust
damit deine Insel aus.

## Funktionen

- **Insel:** isometrische Insel im Meer, der Pinguin watschelt herum und ruht an seinem Nest.
  Ziehen verschiebt die Kamera, Mausrad/Pinch zoomt.
- **Habits:** To-dos / Erledigt / Übersprungen, Gruppen, „Heute nicht geplant“, Tage pro Woche,
  Schwierigkeit (2–35 Sardinen), 2-Minuten-Version mit „Hilf mir“-Tipps, tägliche Erinnerung,
  Vorlagen nach Zielen, frühere Tage nachtragen.
  Karte nach rechts ziehen → Erledigt/Überspringen, nach links → Bearbeiten/Löschen,
  Rechtsklick → Menü.
- **Shop:** Suche, Sortierung, Filter (Kategorie, „nur leistbar“), Varianten-Seiten.
- **Bau-Modus:** Gekauftes aus der Leiste wählen und platzieren, verschieben, spiegeln,
  entfernen. Terrain-Platten färben Felder um oder erweitern die Insel ins Wasser.
- **Statistik:** Level, perfekte Serie, Gesamtzahlen, Heatmap des letzten halben Jahres.
- **Einstellungen:** Desktop-Benachrichtigungen (morgens, abends, pro Habit),
  strenge Serien, Wochenstart, XP & Sardinen zurücksetzen.

Pengu folgt der Omarchy-Akzentfarbe, der Fensterrundung und der Systemsprache (Deutsch/Englisch).
Der Spielstand liegt in `~/.local/share/pengu/pengu.json` und wird atomar geschrieben.

## Tastenkürzel

| Taste | Aktion |
|---|---|
| `H` / `Leertaste` | Gewohnheiten |
| `N` | Neue Gewohnheit |
| `←` / `→` | Vorheriger / nächster Tag |
| `S` | Shop |
| `B` | Bau-Modus |
| `+` / `-` / `0` | Zoom rein / raus / zurück |
| `Esc` | Zurück |
| `?` | Alle Tastenkürzel |
| `Q` | Beenden |

## Entwickeln

```sh
bin/build      # baut build/pengu
bin/test       # Qt-Test-Suite (offscreen)
bin/install    # baut das Arch-Paket und installiert es (App-Starter: Super + Space)
bin/shot save.json out.png 1120 780 'window.go("shop")'   # Screenshot offscreen
build/pengu /tmp/test.json   # mit einem Test-Spielstand starten
```

Neue Shop-Items: Eintrag in `src/data/items.json` + SVG in `src/sprites/`. Neue Vorlagen:
`src/data/templates.json`. Die Roadmap und geplante Features stehen in [ROADMAP.md](ROADMAP.md).

## Handy (Android) und Updates

Laptop und Handy bauen aus **demselben Code**. Die wenigen Unterschiede:

| Was | Wo |
|---|---|
| Speicherort, Sprache, Ecken | `#ifdef Q_OS_ANDROID` in `src/game.cpp`, `src/translator.cpp`, `src/theme.cpp` |
| Erinnerungen | `src/reminders.cpp` (gleiche Regeln) + `android/src/app/pengu/island/*.java` |
| Updates installieren | `src/updater.cpp` (Android: System-Installer, Linux: `pkexec pacman -U`) |
| App-Name, Icon, Berechtigungen | `android/AndroidManifest.xml`, `android/res/` |
| Layout | gleiche QML; `window.mobile` und `SafeArea` für Handy-Details |

### Wie ein Update ankommt

1. Code ändern, committen, `git push`.
2. GitHub Actions (`.github/workflows/release.yml`) testet, baut `pengu.apk` und
   `pengu-x86_64.pkg.tar.zst` und veröffentlicht beide als Release `v0.2.<Commit-Anzahl>`.
3. Pengu prüft beim Start, alle 6 Stunden und beim Zurückkehren in die App die Datei
   `version.json` des neuesten Releases. Ist sie neuer, erscheint **„Update verfügbar – Jetzt updaten“**.
4. Handy: Pengu lädt die APK, Android fragt „Aktualisieren?“ → fertig, Spielstand bleibt.
   Laptop: Pengu lädt das Paket, fragt nach dem Passwort, dann „Neu starten“.

Die Versionsnummer ist `VERSION` + Anzahl der Commits, auf beiden Plattformen gleich.
Das Repo, von dem Updates kommen, steht in `REPO`.

### Android lokal bauen

```sh
bin/android-setup   # einmalig: Qt für Android, SDK/NDK nach toolchain/ (mehrere GB, von git ignoriert)
bin/android-key     # einmalig: Signatur-Schlüssel in ~/.config/pengu-signing
bin/android-build   # → build/android/pengu.apk
toolchain/android-sdk/platform-tools/adb install -r build/android/pengu.apk   # per USB aufs Handy
```

**Der Signatur-Schlüssel (`~/.config/pengu-signing`) ist wichtig:** Android installiert ein
Update nur, wenn es mit demselben Schlüssel signiert ist. Sichere den Ordner. GitHub bekommt
ihn als Secret (`bin/android-key --secrets` zeigt die Werte für `ANDROID_KEYSTORE_BASE64`
und `ANDROID_KEYSTORE_PASSWORD`).

### Pengu aufs Handy (für dich und Freunde)

1. Auf dem Handy die Release-Seite öffnen: https://github.com/itzpingusinwaeschekorb6000-commits/pengu/releases/latest
2. `pengu.apk` herunterladen und öffnen. Android fragt einmal, ob der Browser Apps
   installieren darf → erlauben.
3. Ab dann kommen Updates über den Knopf in der App. Beim ersten Update fragt Android,
   ob **Pengu** Apps installieren darf → erlauben.
