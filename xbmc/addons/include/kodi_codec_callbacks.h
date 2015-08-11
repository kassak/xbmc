/*
 *      Copyright (C) 2012-2015 Team XBMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifndef __KODI_CODEC_CALLBACKS_H__
#define __KODI_CODEC_CALLBACKS_H__

/*!
 * \brief Callback table for Kodi's codec callbacks
 */

#include "xbmc_codec_types.h"

typedef xbmc_codec_t (*CODECGetCodecByName)(const void* addonData, const char* strCodecName);

typedef struct CB_CODEC
{
  CODECGetCodecByName   GetCodecByName;
} CB_CODECLib;

#endif
