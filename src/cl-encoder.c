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
 *** OpenCL RS03 encoder.
 ***
 *** The OpenCL runtime is loaded dynamically at run time; the binary
 *** carries no import table entry for it. On machines without any
 *** OpenCL driver the load simply fails and the caller falls back to
 *** the CPU encoders.
 ***
 *** The kernel is a direct transcription of the rotating register
 *** formulation used by the CPU encoders (see rs-encoder.c): each
 *** work item owns one codeword column, keeps the whole nroots byte
 *** register private, walks the ndata layers in order and finally
 *** writes the register out in natural order (the shiftInit choice
 *** guarantees the rotation ends at zero). Using the same bLut tables
 *** and the same update rule makes the result bit-identical to the
 *** CPU encoders by construction.
 ***/

#include "dvdisaster.h"

#include <string.h>

#ifdef SYS_MINGW
  #include <windows.h>
#else
  #include <dlfcn.h>
#endif

/*
 * Minimal OpenCL declarations. Deliberately self contained so that no
 * platform needs OpenCL headers or link libraries at build time.
 */

typedef void* cl_platform_id;
typedef void* cl_device_id;
typedef void* cl_context;
typedef void* cl_command_queue;
typedef void* cl_program;
typedef void* cl_kernel;
typedef void* cl_mem;
typedef gint32 cl_int;
typedef guint32 cl_uint;
typedef guint64 cl_bitfield;

#define CL_SUCCESS                  0
#define CL_DEVICE_TYPE_GPU          (1<<2)
#define CL_PLATFORM_NAME            0x0902
#define CL_DEVICE_NAME              0x102B
#define CL_DEVICE_GLOBAL_MEM_SIZE   0x101F
#define CL_DEVICE_MAX_COMPUTE_UNITS 0x1002
#define CL_DEVICE_VERSION           0x102F
#define CL_MEM_READ_WRITE           (1<<0)
#define CL_MEM_READ_ONLY            (1<<2)
#define CL_MEM_COPY_HOST_PTR        (1<<5)
#define CL_PROGRAM_BUILD_LOG        0x1183
#define CL_BLOCKING                 1
#define CL_NON_BLOCKING             0

#if defined(_WIN32) && !defined(_WIN64)
  #define CLAPI __stdcall
#else
  #define CLAPI
#endif

typedef cl_int  (CLAPI *p_clGetPlatformIDs)(cl_uint, cl_platform_id*, cl_uint*);
typedef cl_int  (CLAPI *p_clGetPlatformInfo)(cl_platform_id, cl_uint, size_t, void*, size_t*);
typedef cl_int  (CLAPI *p_clGetDeviceIDs)(cl_platform_id, cl_bitfield, cl_uint, cl_device_id*, cl_uint*);
typedef cl_int  (CLAPI *p_clGetDeviceInfo)(cl_device_id, cl_uint, size_t, void*, size_t*);
typedef cl_context (CLAPI *p_clCreateContext)(const void*, cl_uint, const cl_device_id*, void*, void*, cl_int*);
typedef cl_command_queue (CLAPI *p_clCreateCommandQueue)(cl_context, cl_device_id, cl_bitfield, cl_int*);
typedef cl_program (CLAPI *p_clCreateProgramWithSource)(cl_context, cl_uint, const char**, const size_t*, cl_int*);
typedef cl_int  (CLAPI *p_clBuildProgram)(cl_program, cl_uint, const cl_device_id*, const char*, void*, void*);
typedef cl_int  (CLAPI *p_clGetProgramBuildInfo)(cl_program, cl_device_id, cl_uint, size_t, void*, size_t*);
typedef cl_kernel (CLAPI *p_clCreateKernel)(cl_program, const char*, cl_int*);
typedef cl_mem  (CLAPI *p_clCreateBuffer)(cl_context, cl_bitfield, size_t, void*, cl_int*);
typedef cl_int  (CLAPI *p_clEnqueueWriteBuffer)(cl_command_queue, cl_mem, cl_uint, size_t, size_t, const void*, cl_uint, const void*, void*);
typedef cl_int  (CLAPI *p_clEnqueueReadBuffer)(cl_command_queue, cl_mem, cl_uint, size_t, size_t, void*, cl_uint, const void*, void*);
typedef cl_int  (CLAPI *p_clSetKernelArg)(cl_kernel, cl_uint, size_t, const void*);
typedef cl_int  (CLAPI *p_clEnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*, cl_uint, const void*, void*);
typedef cl_int  (CLAPI *p_clFinish)(cl_command_queue);
typedef cl_int  (CLAPI *p_clReleaseMemObject)(cl_mem);
typedef cl_int  (CLAPI *p_clReleaseKernel)(cl_kernel);
typedef cl_int  (CLAPI *p_clReleaseProgram)(cl_program);
typedef cl_int  (CLAPI *p_clReleaseCommandQueue)(cl_command_queue);
typedef cl_int  (CLAPI *p_clReleaseContext)(cl_context);

static struct
{  int tried, ok;
   p_clGetPlatformIDs GetPlatformIDs;
   p_clGetPlatformInfo GetPlatformInfo;
   p_clGetDeviceIDs GetDeviceIDs;
   p_clGetDeviceInfo GetDeviceInfo;
   p_clCreateContext CreateContext;
   p_clCreateCommandQueue CreateCommandQueue;
   p_clCreateProgramWithSource CreateProgramWithSource;
   p_clBuildProgram BuildProgram;
   p_clGetProgramBuildInfo GetProgramBuildInfo;
   p_clCreateKernel CreateKernel;
   p_clCreateBuffer CreateBuffer;
   p_clEnqueueWriteBuffer EnqueueWriteBuffer;
   p_clEnqueueReadBuffer EnqueueReadBuffer;
   p_clSetKernelArg SetKernelArg;
   p_clEnqueueNDRangeKernel EnqueueNDRangeKernel;
   p_clFinish Finish;
   p_clReleaseMemObject ReleaseMemObject;
   p_clReleaseKernel ReleaseKernel;
   p_clReleaseProgram ReleaseProgram;
   p_clReleaseCommandQueue ReleaseCommandQueue;
   p_clReleaseContext ReleaseContext;
} cl;

static void* cl_sym(void *lib, const char *name)
{
#ifdef SYS_MINGW
   return (void*)GetProcAddress((HMODULE)lib, name);
#else
   return dlsym(lib, name);
#endif
}

static int cl_load(void)
{  void *lib = NULL;

   if(cl.tried) return cl.ok;
   cl.tried = 1;

#ifdef SYS_MINGW
   lib = (void*)LoadLibraryA("OpenCL.dll");
#else
 #ifdef SYS_DARWIN
   lib = dlopen("/System/Library/Frameworks/OpenCL.framework/OpenCL", RTLD_NOW);
 #else
   lib = dlopen("libOpenCL.so.1", RTLD_NOW);
   if(!lib) lib = dlopen("libOpenCL.so", RTLD_NOW);
 #endif
#endif

   if(!lib)
   {  Verbose("[OpenCL: runtime library not found]\n");
      return 0;
   }

#define LOAD(name) \
   if(!(cl.name = (p_cl##name)cl_sym(lib, "cl" #name))) \
   {  Verbose("[OpenCL: symbol cl" #name " missing]\n"); return 0; }

   LOAD(GetPlatformIDs); LOAD(GetPlatformInfo);
   LOAD(GetDeviceIDs); LOAD(GetDeviceInfo);
   LOAD(CreateContext); LOAD(CreateCommandQueue);
   LOAD(CreateProgramWithSource); LOAD(BuildProgram); LOAD(GetProgramBuildInfo);
   LOAD(CreateKernel); LOAD(CreateBuffer);
   LOAD(EnqueueWriteBuffer); LOAD(EnqueueReadBuffer);
   LOAD(SetKernelArg); LOAD(EnqueueNDRangeKernel); LOAD(Finish);
   LOAD(ReleaseMemObject); LOAD(ReleaseKernel); LOAD(ReleaseProgram);
   LOAD(ReleaseCommandQueue); LOAD(ReleaseContext);
#undef LOAD

   cl.ok = 1;
   return 1;
}

/*
 * GPU device enumeration. Devices are numbered across all platforms
 * in enumeration order; the numbering matches --device list output.
 */

#define CL_MAX_DEVICES 16

static int cl_enumerate(cl_device_id *devices, char names[][128], int max)
{  cl_platform_id platforms[8];
   cl_uint n_platforms = 0;
   int count = 0;
   int p;

   if(!cl_load()) return 0;

   if(cl.GetPlatformIDs(8, platforms, &n_platforms) != CL_SUCCESS)
      return 0;

   for(p=0; p<(int)n_platforms; p++)
   {  cl_device_id devs[CL_MAX_DEVICES];
      cl_uint n_devs = 0;
      int d;

      if(cl.GetDeviceIDs(platforms[p], CL_DEVICE_TYPE_GPU, CL_MAX_DEVICES, devs, &n_devs) != CL_SUCCESS)
	 continue;

      for(d=0; d<(int)n_devs && count<max; d++)
      {  devices[count] = devs[d];
	 if(names)
	 {  char dname[96] = "", version[32] = "";
	    guint64 mem = 0;

	    cl.GetDeviceInfo(devs[d], CL_DEVICE_NAME, sizeof(dname), dname, NULL);
	    cl.GetDeviceInfo(devs[d], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(mem), &mem, NULL);
	    cl.GetDeviceInfo(devs[d], CL_DEVICE_VERSION, sizeof(version), version, NULL);
	    g_snprintf(names[count], 128, "%s (%" PRId64 " MB, %s)",
		       dname, (gint64)(mem/(1024*1024)), version);
	 }
	 count++;
      }
   }

   return count;
}

void CLListDevices(void)
{  cl_device_id devices[CL_MAX_DEVICES];
   char names[CL_MAX_DEVICES][128];
   int n, i;

   n = cl_enumerate(devices, names, CL_MAX_DEVICES);

   if(!n)
   {  PrintCLI(_("No OpenCL GPU devices found; only --device cpu is available.\n"));
      return;
   }

   PrintCLI(_("Available encoding devices:\n"));
   PrintCLI("  cpu    : %s\n", _("the processor (always available)"));
   for(i=0; i<n; i++)
      PrintCLI("  gpu:%-2d : %s\n", i, names[i]);
}

/*
 * The encoder kernel. One work item per codeword column; private
 * rotating register; same tables and update rule as the CPU encoders.
 */

static const char *cl_encoder_source =
"__kernel void rs03_encode(__global const uchar *layers,\n"
"                          __global uchar *parity,\n"
"                          __global const uchar *lut,\n"
"                          __global const uchar *logtab,\n"
"                          __global const uchar *aloggp0,\n"
"                          int ndata, int nroots, int lutstride,\n"
"                          int shiftinit, ulong columns)\n"
"{  ulong j = get_global_id(0);\n"
"   uchar reg[176];\n"
"   int t, p, sh;\n"
"\n"
"   if(j >= columns) return;\n"
"\n"
"   for(p=0; p<nroots; p++) reg[p] = 0;\n"
"   sh = shiftinit;\n"
"\n"
"   for(t=0; t<ndata; t++)\n"
"   {  uchar fb = layers[(ulong)t*columns + j] ^ reg[sh];\n"
"\n"
"      if(fb)\n"
"      {  int f = logtab[fb];\n"
"         __global const uchar *row = lut + (ulong)f*lutstride + (nroots-1-sh);\n"
"\n"
"         for(p=0; p<nroots; p++)\n"
"            reg[p] ^= row[p];\n"
"         reg[sh] = aloggp0[f];\n"
"      }\n"
"      else reg[sh] = 0;\n"
"\n"
"      sh++; if(sh >= nroots) sh = 0;\n"
"   }\n"
"\n"
"   for(p=0; p<nroots; p++)\n"
"      parity[(ulong)p*columns + j] = reg[p];\n"
"}\n";

struct _CLEncoder
{  cl_context context;
   cl_command_queue queue;
   cl_program program;
   cl_kernel kernel;
   cl_mem layersBuf, parityBuf, lutBuf, logBuf, alogBuf;
   int ndata, nroots, lutStride, shiftInit;
   guint64 maxColumns;
   char deviceName[128];
};

char* CLEncoderDeviceName(CLEncoder *enc)
{  return enc->deviceName;
}

static void cl_encoder_release(CLEncoder *enc)
{  if(enc->layersBuf) cl.ReleaseMemObject(enc->layersBuf);
   if(enc->parityBuf) cl.ReleaseMemObject(enc->parityBuf);
   if(enc->lutBuf)    cl.ReleaseMemObject(enc->lutBuf);
   if(enc->logBuf)    cl.ReleaseMemObject(enc->logBuf);
   if(enc->alogBuf)   cl.ReleaseMemObject(enc->alogBuf);
   if(enc->kernel)    cl.ReleaseKernel(enc->kernel);
   if(enc->program)   cl.ReleaseProgram(enc->program);
   if(enc->queue)     cl.ReleaseCommandQueue(enc->queue);
   if(enc->context)   cl.ReleaseContext(enc->context);
   g_free(enc);
}

void CLEncoderFree(CLEncoder *enc)
{  if(enc) cl_encoder_release(enc);
}

/*
 * Initialize the GPU encoder for the given code parameters.
 * deviceIndex -1 selects the strongest GPU (most compute units).
 * Returns NULL with an explanation in reason[] when unavailable.
 */

CLEncoder* CLEncoderInit(ReedSolomonTables *rt, int ndata, guint64 maxColumns,
			 int deviceIndex, char *reason, int reasonLen)
{  cl_device_id devices[CL_MAX_DEVICES];
   char names[CL_MAX_DEVICES][128];
   cl_device_id dev;
   CLEncoder *enc;
   unsigned char *flat;
   unsigned char table[256];
   cl_int err = CL_SUCCESS;
   int n, i, pick;
   int nroots = rt->nroots;

   reason[0] = 0;

   n = cl_enumerate(devices, names, CL_MAX_DEVICES);
   if(!n)
   {  g_snprintf(reason, reasonLen, "%s", "no OpenCL GPU device available");
      return NULL;
   }

   if(deviceIndex >= 0)
   {  if(deviceIndex >= n)
      {  g_snprintf(reason, reasonLen, "gpu:%d does not exist (%d device%s found)",
		    deviceIndex, n, n==1 ? "" : "s");
	 return NULL;
      }
      pick = deviceIndex;
   }
   else  /* pick the device with the most compute units */
   {  cl_uint best = 0;

      pick = 0;
      for(i=0; i<n; i++)
      {  cl_uint units = 0;
	 cl.GetDeviceInfo(devices[i], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(units), &units, NULL);
	 if(units > best)
	 {  best = units;
	    pick = i;
	 }
      }
   }
   dev = devices[pick];

   enc = g_malloc0(sizeof(CLEncoder));
   enc->ndata = ndata;
   enc->nroots = nroots;
   enc->lutStride = 2*nroots;
   enc->shiftInit = rt->shiftInit;
   enc->maxColumns = maxColumns;
   g_snprintf(enc->deviceName, sizeof(enc->deviceName), "gpu:%d %s", pick, names[pick]);

   enc->context = cl.CreateContext(NULL, 1, &dev, NULL, NULL, &err);
   if(!enc->context || err != CL_SUCCESS)
   {  g_snprintf(reason, reasonLen, "context creation failed (CL error %d)", err);
      cl_encoder_release(enc);
      return NULL;
   }

   enc->queue = cl.CreateCommandQueue(enc->context, dev, 0, &err);
   if(!enc->queue || err != CL_SUCCESS)
   {  g_snprintf(reason, reasonLen, "queue creation failed (CL error %d)", err);
      cl_encoder_release(enc);
      return NULL;
   }

   enc->program = cl.CreateProgramWithSource(enc->context, 1, &cl_encoder_source, NULL, &err);
   if(!enc->program || err != CL_SUCCESS)
   {  g_snprintf(reason, reasonLen, "program creation failed (CL error %d)", err);
      cl_encoder_release(enc);
      return NULL;
   }

   err = cl.BuildProgram(enc->program, 1, &dev, NULL, NULL, NULL);
   if(err != CL_SUCCESS)
   {  char log[1024] = "";
      cl.GetProgramBuildInfo(enc->program, dev, CL_PROGRAM_BUILD_LOG, sizeof(log)-1, log, NULL);
      Verbose("[OpenCL build log: %s]\n", log);
      g_snprintf(reason, reasonLen, "kernel build failed (CL error %d)", err);
      cl_encoder_release(enc);
      return NULL;
   }

   enc->kernel = cl.CreateKernel(enc->program, "rs03_encode", &err);
   if(!enc->kernel || err != CL_SUCCESS)
   {  g_snprintf(reason, reasonLen, "kernel creation failed (CL error %d)", err);
      cl_encoder_release(enc);
      return NULL;
   }

   /* Constant tables: the flattened bLut rows, the log table and the
      per feedback alpha^(f + gpoly[0]) table. */

   flat = g_malloc(256 * enc->lutStride);
   for(i=0; i<256; i++)
      memcpy(flat + i*enc->lutStride, rt->bLut[i], enc->lutStride);
   enc->lutBuf = cl.CreateBuffer(enc->context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
				 256 * enc->lutStride, flat, &err);
   g_free(flat);
   if(err != CL_SUCCESS) goto buffer_failed;

   for(i=0; i<256; i++)
      table[i] = (unsigned char)rt->gfTables->indexOf[i];
   enc->logBuf = cl.CreateBuffer(enc->context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
				 256, table, &err);
   if(err != CL_SUCCESS) goto buffer_failed;

   for(i=0; i<255; i++)
      table[i] = (unsigned char)rt->gfTables->encAlphaTo[i + rt->gpoly[0]];
   table[255] = 0;
   enc->alogBuf = cl.CreateBuffer(enc->context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
				  256, table, &err);
   if(err != CL_SUCCESS) goto buffer_failed;

   enc->layersBuf = cl.CreateBuffer(enc->context, CL_MEM_READ_ONLY,
				    (size_t)ndata * maxColumns, NULL, &err);
   if(err != CL_SUCCESS) goto buffer_failed;

   enc->parityBuf = cl.CreateBuffer(enc->context, CL_MEM_READ_WRITE,
				    (size_t)nroots * maxColumns, NULL, &err);
   if(err != CL_SUCCESS) goto buffer_failed;

   return enc;

buffer_failed:
   g_snprintf(reason, reasonLen, "buffer allocation failed (CL error %d)", err);
   cl_encoder_release(enc);
   return NULL;
}

/*
 * Encode one chunk: upload the ndata layer buffers, run the kernel.
 * The parity stays resident on the device until CLEncoderDownload().
 */

int CLEncoderEncode(CLEncoder *enc, unsigned char **layers, guint64 columns)
{  size_t global, local = 256;
   cl_int err;
   int t;

   for(t=0; t<enc->ndata; t++)
   {  err = cl.EnqueueWriteBuffer(enc->queue, enc->layersBuf, CL_NON_BLOCKING,
				  (size_t)t*columns, columns, layers[t], 0, NULL, NULL);
      if(err != CL_SUCCESS)
      {  Verbose("[OpenCL: layer upload failed (%d)]\n", err);
	 return FALSE;
      }
   }

   err  = cl.SetKernelArg(enc->kernel, 0, sizeof(cl_mem), &enc->layersBuf);
   err |= cl.SetKernelArg(enc->kernel, 1, sizeof(cl_mem), &enc->parityBuf);
   err |= cl.SetKernelArg(enc->kernel, 2, sizeof(cl_mem), &enc->lutBuf);
   err |= cl.SetKernelArg(enc->kernel, 3, sizeof(cl_mem), &enc->logBuf);
   err |= cl.SetKernelArg(enc->kernel, 4, sizeof(cl_mem), &enc->alogBuf);
   err |= cl.SetKernelArg(enc->kernel, 5, sizeof(cl_int), &enc->ndata);
   err |= cl.SetKernelArg(enc->kernel, 6, sizeof(cl_int), &enc->nroots);
   err |= cl.SetKernelArg(enc->kernel, 7, sizeof(cl_int), &enc->lutStride);
   err |= cl.SetKernelArg(enc->kernel, 8, sizeof(cl_int), &enc->shiftInit);
   err |= cl.SetKernelArg(enc->kernel, 9, sizeof(guint64), &columns);
   if(err != CL_SUCCESS)
   {  Verbose("[OpenCL: kernel argument setup failed (%d)]\n", err);
      return FALSE;
   }

   global = ((columns + local - 1) / local) * local;
   err = cl.EnqueueNDRangeKernel(enc->queue, enc->kernel, 1, NULL, &global, &local, 0, NULL, NULL);
   if(err != CL_SUCCESS)
   {  Verbose("[OpenCL: kernel launch failed (%d)]\n", err);
      return FALSE;
   }

   err = cl.Finish(enc->queue);
   if(err != CL_SUCCESS)
   {  Verbose("[OpenCL: kernel execution failed (%d)]\n", err);
      return FALSE;
   }

   return TRUE;
}

/*
 * Download the parity planes straight into the output slices
 * (slice[k][j] layout matches parity plane k byte j exactly).
 */

int CLEncoderDownload(CLEncoder *enc, unsigned char **slices, guint64 columns)
{  cl_int err;
   int k;

   for(k=0; k<enc->nroots; k++)
   {  err = cl.EnqueueReadBuffer(enc->queue, enc->parityBuf, CL_NON_BLOCKING,
				 (size_t)k*columns, columns, slices[k], 0, NULL, NULL);
      if(err != CL_SUCCESS)
      {  Verbose("[OpenCL: parity download failed (%d)]\n", err);
	 return FALSE;
      }
   }

   err = cl.Finish(enc->queue);
   if(err != CL_SUCCESS)
   {  Verbose("[OpenCL: parity download failed at finish (%d)]\n", err);
      return FALSE;
   }

   return TRUE;
}
