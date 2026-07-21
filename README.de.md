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

Geplant (die Versionsnummer sagt es: dies ist eine frühe Ausgabe eines größeren Plans):

* ein **OpenCL-RS03-Kodierer** mit GPU-Auswahl und CPU-Rückfallebene
* ein AVX2-CPU-Pfad neben dem vorhandenen SSE2-Pfad
* Windows 7 SP1 und neuer bleiben unterstützt

## Kompatibilitätsversprechen

Dateien und erweiterte Abbilder aus dvdisaster Light sind bei gleichen Eingaben und Einstellungen **bit-identisch** mit denen von dvdisaster 0.79.10-pl6, und mit RS03 geschützte Medien jeder dvdisaster-Version bleiben hier reparierbar, und umgekehrt. Die geerbten Regressionstests plus ein byte-genauer Vergleich gegen das Original auf einem echten 41,5-GB-Blu-ray-Abbild sichern dieses Versprechen bei jeder Änderung.

## Versionsschema

`dvdisaster Light 0.1.0 (based on dvdisaster 0.79.10-pl6)`: die Light-Version zählt die Ausgaben dieses Forks; die Basis-Version benennt den exakten Original-Stand, von dem der Codec abstammt. Die Versionsfelder im Dateiformat bleiben an die Basis-Version gebunden, damit andere dvdisaster-Versionen die Dateien korrekt einordnen.

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

## Dank und Lizenz

Dieser Fork steht auf der Arbeit von **Carsten Gnoerlich**, der dvdisaster geschaffen und viele Jahre gepflegt hat, dem **dvdisaster-Entwicklerteam** und **speed47**, dessen gepflegter Fork (0.79.10-pl6) die direkte Grundlage dieses Codes ist und dessen Regressionstests einen Fork wie diesen überhaupt erst wartbar machen.

dvdisaster Light ist freie Software unter der **GNU General Public License v3** (siehe [COPYING](COPYING)); es ist eine veränderte Fassung von dvdisaster und steht in keiner Verbindung zu den ursprünglichen Autoren. Der vollständige Quelltext jeder Ausgabe wird hier zusammen mit den Binärdateien veröffentlicht.
