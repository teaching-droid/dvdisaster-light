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

/*** src type: no GUI code ***/

/***
 *** A read status map in the GNU ddrescue mapfile format.
 ***
 *** The map is a parallel, crash-safe record of which sectors have been read.
 *** It complements (does not replace) the image + dead sector markers: writing
 *** the map never changes an image byte, so a --mapfile read is bit-identical
 *** to one without. Its value is a compact status file that is fsync'd, can be
 *** inspected/edited with ddrescuelog, and reloads across sessions so a later
 *** retry pass can be pointed at just the still-bad sectors.
 ***
 *** Format: comment lines start with '#'; the first non-comment line is the
 *** status line "current_pos current_status current_pass"; the remaining lines
 *** are blocks "pos size status", all positions/sizes hex bytes (sector*2048).
 *** Status chars: '+' finished, '-' bad-sector, '?' non-tried (a read failure
 *** written by ddrescue as '*' or '/' is loaded as bad/needs-retry).
 ***/

#include "dvdisaster.h"
#include "mapfile.h"

#if defined(_WIN32) || defined(__MINGW32__) || defined(__MINGW64__)
 #include <io.h>
 #define MAP_FSYNC(fp) _commit(_fileno(fp))
#else
 #include <unistd.h>
 #define MAP_FSYNC(fp) fsync(fileno(fp))
#endif

#define MAP_SECTOR_BYTES 2048

/*
 * Map a ddrescue status char to our internal code (and back).
 */

static int char_to_status(char c)
{  switch(c)
   {  case '+': return MAP_GOOD;
      case '?': return MAP_UNTRIED;
      default:  return MAP_BAD;   /* '-', '*', '/', 'F' ... all mean "retry" */
   }
}

static char status_to_char(int s)
{  switch(s)
   {  case MAP_GOOD: return '+';
      case MAP_BAD:  return '-';
      default:       return '?';
   }
}

/*
 * Load an existing mapfile into the status array. Silently starts from an
 * all-untried map if the file is absent or unreadable.
 */

static void map_load(MapFile *mf)
{  FILE *fp = fopen(mf->path, "r");
   char line[256];
   int status_line_seen = FALSE;

   if(!fp) return;

   while(fgets(line, sizeof(line), fp))
   {  char *p = line;
      long long pos, size;
      char st;

      while(*p == ' ' || *p == '\t') p++;
      if(*p == '#' || *p == '\n' || *p == '\r' || *p == 0)
	 continue;

      /* The first non-comment line is the status line, not a block. */

      if(!status_line_seen)
      {  status_line_seen = TRUE;
	 continue;
      }

      /* Block line: "pos size status" with 0x-prefixed hex numbers. */

      if(sscanf(p, "%lli %lli %c", &pos, &size, &st) == 3 && pos >= 0 && size > 0)
      {  gint64 first = pos / MAP_SECTOR_BYTES;
	 gint64 last  = (pos + size) / MAP_SECTOR_BYTES;   /* exclusive */
	 gint64 k;
	 int code = char_to_status(st);

	 for(k = first; k < last && k < mf->sectors; k++)
	    if(k >= 0)
	       mf->status[k] = code;
      }
   }

   fclose(fp);
}

/*
 * Open (and load if present) a mapfile for a medium of the given sector count.
 */

MapFile *MapFileOpen(const char *path, gint64 sectors)
{  MapFile *mf = g_malloc0(sizeof(MapFile));

   mf->path       = g_strdup(path);
   mf->sectors    = sectors;
   mf->status     = g_malloc0(sectors);   /* all MAP_UNTRIED (0) */
   mf->flushTimer = g_timer_new();

   map_load(mf);
   return mf;
}

/*
 * Query the recorded status of a sector.
 */

int MapFileStatus(MapFile *mf, gint64 sector)
{  if(!mf || sector < 0 || sector >= mf->sectors)
      return MAP_UNTRIED;
   return mf->status[sector];
}

/*
 * Record the status of a single sector.
 */

void MapFileMark(MapFile *mf, gint64 sector, int status)
{  if(!mf || sector < 0 || sector >= mf->sectors)
      return;

   if(mf->status[sector] != status)
   {  mf->status[sector] = status;
      mf->dirty = TRUE;
   }
}

/*
 * Record the status of a run of sectors.
 */

void MapFileMarkRange(MapFile *mf, gint64 first, gint64 count, int status)
{  gint64 i;

   if(!mf) return;
   for(i = 0; i < count; i++)
      MapFileMark(mf, first + i, status);
}

/*
 * Write the map out atomically: build in "<path>.new", flush it to stable
 * storage, then replace the old file. Consecutive same-status sectors are
 * coalesced into one block line.
 */

int MapFileSave(MapFile *mf, gint64 current_pos_sector, char current_status)
{  char *tmp;
   FILE *fp;
   gint64 s;

   if(!mf) return FALSE;

   tmp = g_strdup_printf("%s.new", mf->path);
   fp  = fopen(tmp, "w");
   if(!fp)
   {  g_free(tmp);
      return FALSE;
   }

   fprintf(fp, "# Mapfile. Created by dvdisaster Light.\n");
   fprintf(fp, "# current_pos  current_status  current_pass\n");
   fprintf(fp, "0x%08" PRIx64 "  %c  1\n",
	   (guint64)current_pos_sector * MAP_SECTOR_BYTES, current_status);
   fprintf(fp, "#      pos          size  status\n");

   s = 0;
   while(s < mf->sectors)
   {  int st = mf->status[s];
      gint64 run = 1;

      while(s + run < mf->sectors && mf->status[s + run] == st)
	 run++;

      fprintf(fp, "0x%08" PRIx64 "  0x%08" PRIx64 "  %c\n",
	      (guint64)s * MAP_SECTOR_BYTES,
	      (guint64)run * MAP_SECTOR_BYTES,
	      status_to_char(st));
      s += run;
   }

   fflush(fp);
   MAP_FSYNC(fp);
   fclose(fp);

   /* rename() will not replace an existing file on Windows, so remove first.
      The brief window is acceptable: a crash there leaves "<path>.new" intact. */

   remove(mf->path);
   if(rename(tmp, mf->path) != 0)
   {  g_free(tmp);
      return FALSE;
   }

   g_free(tmp);
   mf->dirty = FALSE;
   return TRUE;
}

/*
 * Save the map if it changed and enough time has passed since the last save.
 * Rides the reader's progress tick so it costs nothing on a clean, fast read.
 */

void MapFileFlushMaybe(MapFile *mf, gint64 current_pos_sector, double min_interval)
{  if(!mf || !mf->dirty)
      return;
   if(g_timer_elapsed(mf->flushTimer, NULL) < min_interval)
      return;

   MapFileSave(mf, current_pos_sector, '?');
   g_timer_start(mf->flushTimer);
}

/*
 * Final save and free.
 */

void MapFileClose(MapFile *mf)
{  if(!mf) return;

   MapFileSave(mf, mf->sectors, '+');

   if(mf->flushTimer) g_timer_destroy(mf->flushTimer);
   g_free(mf->status);
   g_free(mf->path);
   g_free(mf);
}
