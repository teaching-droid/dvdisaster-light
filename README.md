# dvdisaster Light

**English** | [Deutsch](README.de.md) | [日本語](README.ja.md) | [Italiano](README.it.md)

**dvdisaster Light** is a stripped-down, CLI-only fork of [dvdisaster](https://dvdisaster.jcea.es) focused on one thing: **RS03 error correction for optical media, as fast as the hardware allows**.

dvdisaster protects a disc image with Reed-Solomon parity. If the medium later degrades, the damage can be repaired as long as it is smaller than the parity you added. The protection works at the image level, so it survives even filesystem damage.

## What is different from dvdisaster

Kept, byte-for-byte compatible:

* the **RS03 codec**, both flavours: separate error correction files (`-mRS03 -o file`) and augmented images (`-mRS03 -o image`)
* creating, verifying and fixing images; stripping the ecc data from an augmented image
* reading images from physical drives (linear strategy)
* the upstream regression test suite (RS03 parts), which enforces the compatibility promise below

Removed:

* the RS01 and RS02 codecs (for media protected with those, use original dvdisaster; both projects repair each other's discs where formats overlap)
* the GTK user interface; this fork is a command line tool (a graphical front end may ship later as a separate program)
* the adaptive reading strategy

Added by the fork:

* an **OpenCL GPU encoder** for RS03 with device selection: `--encoding-device auto|cpu|gpu[:n]|list`. The default picks the strongest GPU and silently falls back to the CPU when no OpenCL driver is present; any vendor's driver works.
* an **AVX2** CPU encoder next to the SSE2 one (chosen automatically at run time)
* encoding starts instantly and writes far less: ecc files are no longer pre-written with placeholder sectors, and parity is written in large batched runs
* a pipeline timing breakdown under `--verbose`
* a GPU parity gate (`regtest/gpu-parity.bash`) that verifies every GPU device produces bit-identical output to the CPU encoders
* Windows 7 SP1 and newer remain supported (audited automatically in CI)
* **reader hardening for damaged discs**: reverse reading (`-R`), a crash-safe resumable status map in GNU ddrescue format (`--mapfile`, so `ddrescuelog` works on it), phased both-direction recovery (`--retry`), a per-read give-up for dying discs (`--read-timeout`), and one-command full recovery that loops reading and ecc-filling until the image is whole (`--rescue`). See [RECOVERY.md](RECOVERY.md).

## Compatibility promise

Files and augmented images produced by dvdisaster Light are **bit-identical** to those produced by dvdisaster 0.79.10-pl6 for the same input and settings, and media protected with RS03 by any dvdisaster version remain repairable here, and vice versa. The inherited regression suite plus a byte-exact comparison against the original binary on a real 41.5 GB Blu-ray image guard this promise for every change.

## Version scheme

`dvdisaster Light 0.2.0 (based on dvdisaster 0.79.10-pl6)`: the Light version counts this fork's own releases; the base version names the exact upstream state the codec derives from. The on-disk format version fields stay tied to the base version so other dvdisaster versions interpret the files correctly.

## Building

Linux and similar:

```
./configure && make -j$(nproc)
```

Windows (MSYS2, MINGW64 environment):

```
pacman -S --needed git diffutils make pkg-config mingw-w64-x86_64-glib2 mingw-w64-x86_64-gcc
./configure && make -j16
```

Run the regression tests with `cd regtest && ./runtests.sh` (needs `/var/tmp/regtest` to exist).

## Performance

Numbers from one sample system (a desktop 8 core CPU, a current discrete NVIDIA GPU, NVMe storage); absolute values will differ on other machines:

| Workload | dvdisaster Light 0.1.0 (CPU) | current (CPU) | current (GPU) |
|----------|------------------------------|---------------|---------------|
| 42 GB image, 32 roots (14.3%) | about 7 minutes | 56 s | 27 s |
| 2 GB image, 170 roots (200%) | 17 s | 4.2 s | 3.2 s |

Two practical tips: write the error correction file to a **different drive** than the one holding the image (reading and writing then do not compete for the same device), and let the automatic device selection do its work; forcing a device is only needed for testing.

## Credits and license

This fork stands on the work of **Carsten Gnoerlich**, who created dvdisaster and maintained it for many years, **the dvdisaster development team**, and **speed47**, whose maintained fork (0.79.10-pl6) is the direct base of this code and whose regression suite makes a fork like this maintainable at all.

dvdisaster Light is free software under the **GNU General Public License v3** (see [COPYING](COPYING)); it is a modified version of dvdisaster and not associated with the original authors. Complete source for every release is published here alongside any binaries.
