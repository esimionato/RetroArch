/*  SSNES - A Super Nintendo Entertainment System (SNES) Emulator frontend for libsnes.
 *  Copyright (C) 2010-2011 - Hans-Kristian Arntzen
 *
 *  Some code herein may be based on code found in BSNES.
 * 
 *  SSNES is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  SSNES is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with SSNES.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "image.h"
#include "file.h"

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "general.h"

#ifdef HAVE_IMLIB

#include <Imlib2.h>
bool texture_image_load(const char *path, struct texture_image *out_img)
{
   Imlib_Image img;
   img = imlib_load_image(path);
   if (!img)
      return false;

   imlib_context_set_image(img);

   out_img->width = imlib_image_get_width();
   out_img->height = imlib_image_get_height();

   size_t size = out_img->width * out_img->height * sizeof(uint32_t);
   out_img->pixels = malloc(size);
   if (!out_img->pixels)
   {
      imlib_free_image();
      return false;
   }

   const uint32_t *read = imlib_image_get_data_for_reading_only();
   // Convert ARGB -> RGBA.
   for (unsigned i = 0; i < size / sizeof(uint32_t); i++)
      out_img->pixels[i] = (read[i] >> 24) | (read[i] << 8);

   imlib_free_image();
   return true;
}

#else

bool texture_image_load(const char *path, struct texture_image *out_img)
{
   // TODO: Check more gracefully.
   if (!strstr(path, ".tga"))
      return false;

   void *raw_buf = NULL;
   ssize_t len = read_file(path, &raw_buf);
   if (len < 0)
      return false;

   uint8_t *buf = raw_buf;

   if (buf[2] != 2) // Uncompressed RGB
   {
      free(buf);
      return false;
   }

   unsigned width = 0;
   unsigned height = 0;

   uint8_t info[6];
   memcpy(info, buf + 12, 6);

   width = info[0] + ((unsigned)info[1] * 256);
   height = info[2] + ((unsigned)info[3] * 256);
   unsigned bits = info[4];

   SSNES_LOG("Loaded TGA: (%ux%u @ %u bpp)\n", width, height, bits);

   unsigned size = width * height * sizeof(uint32_t);
   out_img->pixels = malloc(size);
   out_img->width = width;
   out_img->height = height;
   if (!out_img->pixels)
   {
      free(buf);
      return false;
   }

   const uint8_t *tmp = buf + 18;
   if (bits == 32)
   {
      for (unsigned i = 0; i < width * height; i++)
      {
         uint32_t r = tmp[i * 4 + 2];
         uint32_t g = tmp[i * 4 + 1];
         uint32_t b = tmp[i * 4 + 0];
         uint32_t a = tmp[i * 4 + 3];

         out_img->pixels[i] = (r << 24) | (g << 16) | (b << 8) | a;
      }
   }
   else if (bits == 24)
   {
      for (unsigned i = 0; i < width * height; i++)
      {
         uint32_t r = tmp[i * 3 + 2];
         uint32_t g = tmp[i * 3 + 1];
         uint32_t b = tmp[i * 3 + 0];
         uint32_t a = 0xff;

         out_img->pixels[i] = (r << 24) | (g << 16) | (b << 8) | a;
      }
   }
   else
   {
      free(buf);
      free(out_img->pixels);
      out_img->pixels = NULL;
      return false;
   }

   free(buf);
   return true;
}

#endif
