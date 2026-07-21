# dvdisaster Light

[English](README.md) | [Deutsch](README.de.md) | [日本語](README.ja.md) | **Italiano**

**dvdisaster Light** è un fork snellito, solo a riga di comando, di [dvdisaster](https://dvdisaster.jcea.es), concentrato su un unico obiettivo: **correzione d'errore RS03 per supporti ottici, alla massima velocità che l'hardware consente**.

dvdisaster protegge l'immagine di un disco con dati di parità Reed-Solomon. Se in seguito il supporto si degrada, il danno può essere riparato finché resta inferiore alla parità aggiunta. La protezione agisce a livello di immagine e sopravvive quindi perfino ai danni al filesystem.

## Differenze rispetto a dvdisaster

Mantenuto, compatibile byte per byte:

* il **codec RS03** in entrambe le varianti: file di correzione separati (`-mRS03 -o file`) e immagini aumentate (`-mRS03 -o image`)
* creazione, verifica e riparazione delle immagini; rimozione dei dati di correzione da un'immagine aumentata
* lettura delle immagini da unità fisiche (strategia lineare)
* la suite di test di regressione originale (parti RS03), che fa rispettare la promessa di compatibilità qui sotto

Rimosso:

* i codec RS01 e RS02 (per i supporti protetti con questi, usare il dvdisaster originale; dove i formati coincidono, i due programmi riparano a vicenda i propri dischi)
* l'interfaccia GTK; questo fork è uno strumento a riga di comando (un'interfaccia grafica potrà arrivare in seguito come programma separato)
* la strategia di lettura adattiva

In programma (il numero di versione lo dice: questa è una prima uscita di un piano più ampio):

* un **codificatore RS03 OpenCL** con selezione della GPU e ripiego su CPU
* un percorso CPU AVX2 accanto a quello SSE2 esistente
* Windows 7 SP1 e successivi restano supportati

## Promessa di compatibilità

I file e le immagini aumentate prodotti da dvdisaster Light sono **identici bit per bit** a quelli di dvdisaster 0.79.10-pl6 a parità di input e impostazioni, e i supporti protetti con RS03 da qualunque versione di dvdisaster restano riparabili qui, e viceversa. La suite di regressione ereditata più un confronto byte per byte contro il binario originale su una vera immagine Blu-ray da 41,5 GB difendono questa promessa a ogni modifica.

## Schema delle versioni

`dvdisaster Light 0.1.0 (based on dvdisaster 0.79.10-pl6)`: la versione Light conta le uscite di questo fork; la versione base indica lo stato esatto del progetto originale da cui deriva il codec. I campi di versione nel formato dei file restano legati alla versione base, così le altre versioni di dvdisaster interpretano correttamente i file.

## Compilazione

Linux e simili:

```
./configure && make -j$(nproc)
```

Windows (MSYS2, ambiente MINGW64):

```
pacman -S --needed git diffutils make pkg-config mingw-w64-x86_64-glib2 mingw-w64-x86_64-gcc
./configure && make -j16
```

Test di regressione: `cd regtest && ./runtests.sh` (la directory `/var/tmp/regtest` deve esistere).

## Riconoscimenti e licenza

Questo fork poggia sul lavoro di **Carsten Gnoerlich**, che ha creato dvdisaster e lo ha mantenuto per molti anni, del **team di sviluppo di dvdisaster** e di **speed47**, il cui fork mantenuto (0.79.10-pl6) è la base diretta di questo codice e la cui suite di regressione rende sostenibile un fork come questo.

dvdisaster Light è software libero sotto la **GNU General Public License v3** (vedi [COPYING](COPYING)); è una versione modificata di dvdisaster e non è associato agli autori originali. Il sorgente completo di ogni uscita è pubblicato qui insieme ai binari.
