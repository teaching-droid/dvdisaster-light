# dvdisaster Light

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

Planned (see the version number: this is an early release of a larger plan):

* an **OpenCL RS03 encoder** with a GPU device selector and CPU fallback
* an AVX2 CPU path next to the existing SSE2 one
* Windows 7 SP1 and newer remain supported

## Compatibility promise

Files and augmented images produced by dvdisaster Light are **bit-identical** to those produced by dvdisaster 0.79.10-pl6 for the same input and settings, and media protected with RS03 by any dvdisaster version remain repairable here, and vice versa. The inherited regression suite plus a byte-exact comparison against the original binary on a real 41.5 GB Blu-ray image guard this promise for every change.

## Version scheme

`dvdisaster Light 0.1.0 (based on dvdisaster 0.79.10-pl6)`: the Light version counts this fork's own releases; the base version names the exact upstream state the codec derives from. The on-disk format version fields stay tied to the base version so other dvdisaster versions interpret the files correctly.

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

## Credits and license

This fork stands on the work of **Carsten Gnoerlich**, who created dvdisaster and maintained it for many years, **the dvdisaster development team**, and **speed47**, whose maintained fork (0.79.10-pl6) is the direct base of this code and whose regression suite makes a fork like this maintainable at all.

dvdisaster Light is free software under the **GNU General Public License v3** (see [COPYING](COPYING)); it is a modified version of dvdisaster and not associated with the original authors. Complete source for every release is published here alongside any binaries.
