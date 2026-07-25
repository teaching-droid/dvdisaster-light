/*  dvdisaster: Additional error correction for optical media.
 *  Copyright (C) 2026 dvdisaster Light contributors.
 *
 *  This file is part of dvdisaster Light.
 *
 *  dvdisaster is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  dvdisaster is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with dvdisaster. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * A crash-safe, resumable read status map in the GNU ddrescue mapfile format.
 * Positions and sizes are stored in hex BYTES (sector s -> byte s*2048), so the
 * files can be inspected and edited with ddrescuelog. dvdisaster.h must be
 * included before this header (it uses the glib integer types).
 */

#ifndef MAPFILE_H
#define MAPFILE_H

enum { MAP_UNTRIED = 0,   /* '?' - not attempted yet             */
       MAP_GOOD    = 1,   /* '+' - read ok / present in the image */
       MAP_BAD     = 2 }; /* '-' - unreadable, needs a retry      */

typedef struct _MapFile
{  char   *path;          /* mapfile path                         */
   guint8 *status;        /* one byte per sector (MAP_* above)     */
   gint64  sectors;       /* domain size in sectors               */
   int     dirty;         /* status changed since last save       */
   GTimer *flushTimer;    /* throttles periodic saves             */
} MapFile;

MapFile *MapFileOpen(const char *path, gint64 sectors);
void     MapFileMark(MapFile *mf, gint64 sector, int status);
void     MapFileMarkRange(MapFile *mf, gint64 first, gint64 count, int status);
int      MapFileSave(MapFile *mf, gint64 current_pos_sector, char current_status);
void     MapFileFlushMaybe(MapFile *mf, gint64 current_pos_sector, double min_interval);
void     MapFileClose(MapFile *mf);   /* final save, then free */

#endif /* MAPFILE_H */
