# dvdisaster Light

[English](README.md) | **Deutsch** | [日本語](README.ja.md) | [Italiano](README.it.md)

**dvdisaster Light** ist ein verschlankter, reiner Kommandozeilen-Fork von [dvdisaster](https://dvdisaster.jcea.es) mit genau einem Ziel: **RS03-Fehlerkorrektur für optische Medien, so schnell wie die Hardware es hergibt**.

dvdisaster schützt ein Disc-Abbild mit Reed-Solomon-Paritätsdaten. Baut das Medium später ab, lässt sich der Schaden reparieren, solange er kleiner ist als die hinzugefügte Parität. Der Schutz arbeitet auf Abbild-Ebene und übersteht damit sogar Schäden am Dateisystem.

## Unterschiede zu dvdisaster

Erhalten, Byte für Byte kompatibel:

* der **RS03-Codec** in beiden Varianten: separate Fehlerkorrektur-Dateien (`-mRS03 -o file`) und erweiterte Abbilder (`-mRS03 -o image`)
* Erzeugen, Prüfen und Reparieren von Abbildern; Entfernen der Fehlerkorrektur-Daten aus einem erweiterten Abbild
* Einlesen von Abbildern aus physischen Laufwerken (lineare Strategie)
* die Regressionstests des Originals (RS03-Teile), die das Kompatibilitätsversprechen unten absichern

Entfernt:

* die Codecs RS01 und RS02 (für damit geschützte Medien das Original-dvdisaster verwenden; wo sich die Formate überschneiden, reparieren beide Programme gegenseitig ihre Discs)
* die GTK-Oberfläche; dieser Fork ist ein Kommandozeilen-Werkzeug (eine grafische Oberfläche erscheint später eventuell als eigenständiges Programm)
* die adaptive Lesestrategie

Vom Fork hinzugefügt:

* ein **OpenCL-GPU-Kodierer** für RS03 mit Geräteauswahl: `--encoding-device auto|cpu|gpu[:n]|list`. Die Vorgabe wählt die stärkste GPU und fällt ohne OpenCL-Treiber stillschweigend auf die CPU zurück; Treiber aller Hersteller funktionieren.
* ein **AVX2**-CPU-Kodierer neben dem SSE2-Pfad (wird zur Laufzeit automatisch gewählt)
* die Kodierung startet sofort und schreibt deutlich weniger: Fehlerkorrektur-Dateien werden nicht mehr mit Platzhalter-Sektoren vorbeschrieben, und die Parität wird in großen Blöcken geschrieben
* eine Zeitaufschlüsselung der Pipeline unter `--verbose`
* ein GPU-Paritätstest (`regtest/gpu-parity.bash`), der für jedes GPU-Gerät bitgleiche Ausgabe mit den CPU-Kodierern nachweist
* Windows 7 SP1 und neuer bleiben unterstützt (automatisch in der CI geprüft)
* **Härtung des Lesevorgangs für beschädigte Discs**: Rückwärtslesen (`-R`), eine absturzsichere, fortsetzbare Statuskarte im GNU-ddrescue-Format (`--mapfile`, sodass `ddrescuelog` damit arbeitet), phasenweise Wiederherstellung in beide Richtungen (`--retry`), ein Zeitlimit pro Lesevorgang für sterbende Discs (`--read-timeout`) und eine vollständige Wiederherstellung in einem Befehl, die Lesen und ecc-Auffüllen wiederholt, bis das Abbild vollständig ist (`--rescue`). Siehe [RECOVERY.md](RECOVERY.md).

## Kompatibilitätsversprechen

Dateien und erweiterte Abbilder aus dvdisaster Light sind bei gleichen Eingaben und Einstellungen **bit-identisch** mit denen von dvdisaster 0.79.10-pl6, und mit RS03 geschützte Medien jeder dvdisaster-Version bleiben hier reparierbar, und umgekehrt. Die geerbten Regressionstests plus ein byte-genauer Vergleich gegen das Original auf einem echten 41,5-GB-Blu-ray-Abbild sichern dieses Versprechen bei jeder Änderung.

## Versionsschema

`dvdisaster Light 0.3.0 (based on dvdisaster 0.79.10-pl6)`: die Light-Version zählt die Ausgaben dieses Forks; die Basis-Version benennt den exakten Original-Stand, von dem der Codec abstammt. Die Versionsfelder im Dateiformat bleiben an die Basis-Version gebunden, damit andere dvdisaster-Versionen die Dateien korrekt einordnen.

## Bauen

Linux und Verwandte:

```
./configure && make -j$(nproc)
```

Windows (MSYS2, MINGW64-Umgebung):

```
pacman -S --needed git diffutils make pkg-config mingw-w64-x86_64-glib2 mingw-w64-x86_64-gcc
./configure && make -j16
```

Regressionstests: `cd regtest && ./runtests.sh` (das Verzeichnis `/var/tmp/regtest` muss existieren).

## Leistung

Zahlen von einem Beispielsystem (Desktop-CPU mit 8 Kernen, aktuelle dedizierte NVIDIA-GPU, NVMe-Speicher); absolute Werte fallen auf anderen Rechnern anders aus:

| Aufgabe | dvdisaster Light 0.1.0 (CPU) | aktuell (CPU) | aktuell (GPU) |
|---------|------------------------------|---------------|---------------|
| 42-GB-Abbild, 32 Roots (14,3 %) | rund 7 Minuten | 56 s | 27 s |
| 2-GB-Abbild, 170 Roots (200 %) | 17 s | 4,2 s | 3,2 s |

Zwei praktische Hinweise: Die Fehlerkorrektur-Datei auf ein **anderes Laufwerk** schreiben als das mit dem Abbild (Lesen und Schreiben konkurrieren dann nicht um dasselbe Gerät), und die automatische Geräteauswahl arbeiten lassen; ein Gerät zu erzwingen ist nur zum Testen nötig.

## Dank und Lizenz

Dieser Fork steht auf der Arbeit von **Carsten Gnoerlich**, der dvdisaster geschaffen und viele Jahre gepflegt hat, dem **dvdisaster-Entwicklerteam** und **speed47**, dessen gepflegter Fork (0.79.10-pl6) die direkte Grundlage dieses Codes ist und dessen Regressionstests einen Fork wie diesen überhaupt erst wartbar machen.

dvdisaster Light ist freie Software unter der **GNU General Public License v3** (siehe [COPYING](COPYING)); es ist eine veränderte Fassung von dvdisaster und steht in keiner Verbindung zu den ursprünglichen Autoren. Der vollständige Quelltext jeder Ausgabe wird hier zusammen mit den Binärdateien veröffentlicht.
