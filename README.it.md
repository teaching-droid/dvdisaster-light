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

Aggiunto dal fork:

* un **codificatore GPU OpenCL** per RS03 con selezione del dispositivo: `--encoding-device auto|cpu|gpu[:n]|list`. L'impostazione predefinita sceglie la GPU più potente e ripiega silenziosamente sulla CPU se manca un driver OpenCL; funzionano i driver di qualunque produttore.
* un codificatore CPU **AVX2** accanto a quello SSE2 (scelto automaticamente a runtime)
* la codifica parte all'istante e scrive molto meno: i file di correzione non vengono più pre-scritti con settori segnaposto, e la parità viene scritta in grandi blocchi
* una ripartizione dei tempi della pipeline con `--verbose`
* un controllo di parità GPU (`regtest/gpu-parity.bash`) che verifica per ogni dispositivo GPU l'identità bit per bit con i codificatori CPU
* Windows 7 SP1 e successivi restano supportati (verificato automaticamente nella CI)

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

## Prestazioni

Valori misurati su un sistema campione (CPU desktop a 8 core, GPU NVIDIA dedicata recente, archiviazione NVMe); i valori assoluti variano da macchina a macchina:

| Carico di lavoro | dvdisaster Light 0.1.0 (CPU) | attuale (CPU) | attuale (GPU) |
|------------------|------------------------------|---------------|---------------|
| immagine da 42 GB, 32 radici (14,3%) | circa 7 minuti | 56 s | 27 s |
| immagine da 2 GB, 170 radici (200%) | 17 s | 4,2 s | 3,2 s |

Due consigli pratici: scrivere il file di correzione su un **disco diverso** da quello dell'immagine (lettura e scrittura non competono così per lo stesso dispositivo), e lasciare lavorare la selezione automatica del dispositivo; forzarne uno serve solo per i test.

## Riconoscimenti e licenza

Questo fork poggia sul lavoro di **Carsten Gnoerlich**, che ha creato dvdisaster e lo ha mantenuto per molti anni, del **team di sviluppo di dvdisaster** e di **speed47**, il cui fork mantenuto (0.79.10-pl6) è la base diretta di questo codice e la cui suite di regressione rende sostenibile un fork come questo.

dvdisaster Light è software libero sotto la **GNU General Public License v3** (vedi [COPYING](COPYING)); è una versione modificata di dvdisaster e non è associato agli autori originali. Il sorgente completo di ogni uscita è pubblicato qui insieme ai binari.
