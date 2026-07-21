/*  dvdisaster: Additional error correction for optical media.
 *  Copyright (C) 2004-2017 Carsten Gnoerlich.
 *  Copyright (C) 2019-2021 The dvdisaster development team.
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

#include "dvdisaster.h"

/***
 *** Reed-Solomon encoding using AVX2 intrinsics
 ***
 *** Same algorithm as the SSE2 encoder, but the coefficient row is
 *** XORed in 32 byte blocks. The parity stride (nroots_aligned) is a
 *** multiple of 16, not necessarily 32, so the row is processed as
 *** whole 32 byte blocks plus at most one trailing 16 byte block;
 *** everything past the stride belongs to the next codeword column
 *** and must not be touched. All loads are unaligned: the parity
 *** base is only guaranteed 16 byte alignment, and the bLut rows
 *** carry no alignment guarantee at all.
 ***
 *** The CPU capability probe (ProbeAVX2) deliberately lives in
 *** rs-encoder.c: this file is compiled with -mavx2, so the compiler
 *** may emit VEX encoded instructions anywhere in it, which would
 *** fault on a pre AVX machine before the probe could answer.
 ***/

#ifdef HAVE_AVX2

#include <immintrin.h>

void encode_next_layer_avx2(ReedSolomonTables *rt, unsigned char *data, unsigned char *parity, guint64 layer_size, int shift)
{  gint32 *gf_index_of  = rt->gfTables->indexOf;
   gint32 *enc_alpha_to = rt->gfTables->encAlphaTo;
   gint32 *rs_gpoly     = rt->gpoly;
   int nroots           = rt->nroots;
   int nroots_aligned   = (nroots+15)&~15;
   int blocks32         = nroots_aligned>>5;
   int tail16           = nroots_aligned&16;
   guint64 i;
   int j;

   for(i=0; i<layer_size; i++)
   {  int feedback    = gf_index_of[data[i] ^ parity[shift]];

      if(feedback != GF_ALPHA0) /* non-zero feedback term */
      {	 guint8 *par_idx = parity;
	 guint8 *e_lut = rt->bLut[feedback] + nroots - shift - 1;

	 for(j=blocks32; j; j--)
	 {  __m256i par = _mm256_loadu_si256((__m256i*)par_idx);
	    __m256i lut = _mm256_loadu_si256((__m256i*)e_lut);
	    _mm256_storeu_si256((__m256i*)par_idx, _mm256_xor_si256(par, lut));
	    par_idx += 32;
	    e_lut += 32;
	 }

	 if(tail16)
	 {  __m128i par = _mm_loadu_si128((__m128i*)par_idx);
	    __m128i lut = _mm_loadu_si128((__m128i*)e_lut);
	    _mm_storeu_si128((__m128i*)par_idx, _mm_xor_si128(par, lut));
	 }

	 parity[shift] = enc_alpha_to[feedback + rs_gpoly[0]];
      }
      else  /* zero feedback term */
	parity[shift] = 0;

      parity += nroots_aligned;
   }
}
#else /* don't have AVX2 */
/* Stub function to keep the linker happy.
 * Should never be executed.
 */

void encode_next_layer_avx2(ReedSolomonTables *rt, unsigned char *data, unsigned char *parity, guint64 layer_size, int shift)
{
   Stop("Mega borkage - encode_next_layer_avx2() stub called.\n");
}
#endif /* HAVE_AVX2 */
