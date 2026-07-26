# Recovering a damaged disc with dvdisaster Light

dvdisaster Light is not only an error-correction tool. Its reader is built to pull data
off scratched, aging or marginal optical media, with or without an error correction file.
This guide covers the reading and recovery options and a workflow that gets the most off a
failing disc.

All of these options are **off by default**: a plain `-r` read behaves exactly as before.
They only change behaviour when you ask for them, so scripts are unaffected.

For an illustrated walkthrough of *how* each feature works, see
[HOW_IT_WORKS.md](HOW_IT_WORKS.md) (also in [Deutsch](HOW_IT_WORKS.de.md),
[日本語](HOW_IT_WORKS.ja.md), [Italiano](HOW_IT_WORKS.it.md)).

## The reading options

| Option | What it does |
|--------|--------------|
| `-r, --read` | read the medium into an image (`-i image.iso`, default `medium.iso`) |
| `-d, --device dev` | which drive to read from (e.g. `-d G:` on Windows, `-d /dev/sr0` on Linux) |
| `-R, --reverse` | read the medium from the last sector to the first |
| `--retry` | phased recovery: after the first pass, alternate both directions over the still-missing sectors until no more can be read |
| `--rescue` | full recovery in one command: loop reading (with `--retry`) and filling from ecc until the image is complete (needs an ecc file, `-e`) |
| `--mapfile file` | keep a crash-safe, resumable status map in GNU ddrescue format |
| `--read-timeout n` | give up on any read that takes longer than `n` seconds and move on (for dying discs) |
| `-j, --jump n` | after a read error, skip `n` sectors forward before trying again (default 16) |
| `--read-attempts n-m` | read a defective sector between `n` and `m` times before giving up |
| `--read-medium n` | read the whole medium up to `n` times |
| `--internal-rereads n` | how many times the **drive** may retry internally before reporting an error (lower = fail faster on bad sectors) |
| `--fill-unreadable n` | fill unreadable sectors in the image with byte value `n` |

## A recovery workflow

1. **First pass, with a map.** Read fast, recording what worked:
   ```
   dvdisaster -r -i disc.iso --mapfile disc.map -d G:
   ```
   If the disc reads clean, you are done. If not, `disc.iso` now has the good sectors and
   `disc.map` records exactly which sectors are still missing.

2. **Recover the rest from both sides.** Re-run over only the missing sectors, attacking
   each defect from both directions until nothing more comes back:
   ```
   dvdisaster -r -i disc.iso --mapfile disc.map --retry -d G:
   ```

3. **If the drive stalls on bad areas** (a dying disc can spend minutes on one sector),
   cap the wait so it keeps moving and comes back later:
   ```
   dvdisaster -r -i disc.iso --mapfile disc.map --retry --read-timeout 15 -d G:
   ```

4. **Use error correction if you have it.** If you made an ecc file while the disc was
   healthy, fill in everything the parity can reconstruct, then re-read only what is still
   missing:
   ```
   dvdisaster -f -i disc.iso -e disc.ecc
   dvdisaster -r -i disc.iso --mapfile disc.map --retry -d G:
   ```

5. **Try another drive.** See "Multiple drives" below.

## Reverse reading (`-R`)

A forward read overshoots a defect and loses the sectors just after it; a reverse read
recovers them by approaching from the other side. Some drives also track or error-correct
better in one direction. `--retry` already alternates both directions, so you rarely need
`-R` on its own, but it is there for a manual reverse pass. Reverse is a single-sector
recovery pass, so it is slower than the bulk forward read by design.

## Phased recovery (`--retry`)

`--retry` runs the initial pass, then keeps alternating a reverse pass and a forward pass
over the sectors that are still missing, stopping when a whole round recovers nothing.
Alternating directions trims each defect from both edges. If a `--mapfile` is present it
drives which sectors are attempted (already-good sectors are skipped). This turns the old
"read, then read again, then read the other way" by hand into one command.

## Full automatic recovery (`--rescue`)

If you have an ecc file, `--rescue` does the whole loop for you in one command: read what the
drive can (with both-direction `--retry`), fill what the RS03 parity can reconstruct, re-read
the sectors the parity could not fix, and repeat until the image is complete or nothing more
can be recovered:

```
dvdisaster --rescue -i disc.iso -e disc.ecc --mapfile disc.map -d G:
```

Each round only re-attempts the sectors still missing, and the fill reconstructs whatever the
drive cannot read at all (as long as the remaining damage is within the parity you added). It
stops when the image is whole, when a whole round makes no progress, or after a safety limit
of rounds. Without an ecc file it simply does the read-and-retry part and reports what remains.

`--rescue` is the automated form of steps 1 to 4 above; run the steps by hand if you want more
control over each pass.

## Giving up on slow sectors (`--read-timeout`)

By default a read command may take a long time (the driver default is minutes) because the
drive retries internally. On a dying disc that means one bad sector can stall the whole job
for minutes. `--read-timeout n` caps each read at `n` seconds; a read that exceeds it is
treated as a failure, the sector is marked, and reading moves on. Combine it with `--retry`
so those skipped sectors are attempted again on later passes, when the drive may have
better luck. The timeout applies only to bulk sector reads, not to control commands.

## The status map and `ddrescuelog`

`--mapfile` writes the standard **GNU ddrescue mapfile format** (positions and sizes in
hex bytes, one `+`/`-`/`?` status per region). That means ddrescue's own `ddrescuelog`
tool works on dvdisaster Light maps unmodified, for example:

```
ddrescuelog -t disc.map      # summary: how much is rescued / bad / not tried
ddrescuelog -l- disc.map     # list the byte ranges still unreadable
ddrescuelog -l+ disc.map     # list the ranges already recovered
ddrescuelog -D disc.map      # succeeds only if the rescue is complete
```

You can also change the status of ranges, restrict work to a domain, and create maps from
a list with `ddrescuelog`; see its manual. The map is written safely (to a temporary file,
flushed, then renamed), so it survives a crash or power loss mid-read and you can resume by
re-running the same command.

## Multiple drives ("best of N")

A scratch that one drive cannot read often reads fine in another. Point a second (or third)
drive at the **same image and the same mapfile**:

```
dvdisaster -r -i disc.iso --mapfile disc.map -d G:      # first drive
dvdisaster -r -i disc.iso --mapfile disc.map -d H:      # second drive: only the gaps
```

Because the image and map record what is already recovered, each drive only re-reads the
sectors the previous drives could not, so you accumulate the best result across all of them.
Add `--retry` (and `--read-timeout`) to each run for the stubborn sectors. If you prefer to
keep separate images per drive, `--merge-images a,b` copies the good sectors of image `b`
into image `a`.

## Notes

* Reverse and retry passes read one sector at a time; they are recovery passes, slower than
  the bulk forward read on purpose.
* None of this decrypts protected media. dvdisaster Light reads raw data sectors, so it is
  for your own data discs or already-unprotected content.
* For the underlying error-correction workflow (creating and using ecc files), see the
  main documentation and `dvdisaster --help`.

## Credits

The status map and the recovery approach here follow [**GNU ddrescue**](https://www.gnu.org/software/ddrescue/)
by Antonio Diaz Diaz. `--mapfile` uses ddrescue's mapfile format, so its `ddrescuelog` tool
works on these maps unchanged, and the phased both-direction recovery follows ddrescue's copy /
trim / scrape approach. No ddrescue code is used here; the algorithms are reimplemented.
