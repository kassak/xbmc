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
#ifndef __KODI_ADDON_CALLBACKS_H__
#define __KODI_ADDON_CALLBACKS_H__

/*!
 * \brief Callback table for Kodi's add-on callbacks
 */

#include "libXBMC_addon.h"

typedef void (*AddOnLogCallback)(void *addonData, const ADDON::addon_log_t loglevel, const char *msg);
typedef void (*AddOnQueueNotification)(void *addonData, const ADDON::queue_msg_t type, const char *msg);
typedef bool (*AddOnWakeOnLan)(const char* mac);
typedef bool (*AddOnGetSetting)(void *addonData, const char *settingName, void *settingValue);
typedef char* (*AddOnUnknownToUTF8)(const char *sourceDest);
typedef char* (*AddOnGetLocalizedString)(const void* addonData, long dwCode);
typedef char* (*AddOnGetDVDMenuLanguage)(const void* addonData);
typedef void (*AddOnFreeString)(const void* addonData, char* str);

typedef void* (*AddOnOpenFile)(const void* addonData, const char* strFileName, unsigned int flags);
typedef void* (*AddOnOpenFileForWrite)(const void* addonData, const char* strFileName, bool bOverWrite);
typedef ssize_t (*AddOnReadFile)(const void* addonData, void* file, void* lpBuf, size_t uiBufSize);
typedef bool (*AddOnReadFileString)(const void* addonData, void* file, char *szLine, int iLineLength);
typedef ssize_t (*AddOnWriteFile)(const void* addonData, void* file, const void* lpBuf, size_t uiBufSize);
typedef void (*AddOnFlushFile)(const void* addonData, void* file);
typedef int64_t (*AddOnSeekFile)(const void* addonData, void* file, int64_t iFilePosition, int iWhence);
typedef int (*AddOnTruncateFile)(const void* addonData, void* file, int64_t iSize);
typedef int64_t (*AddOnGetFilePosition)(const void* addonData, void* file);
typedef int64_t (*AddOnGetFileLength)(const void* addonData, void* file);
typedef void (*AddOnCloseFile)(const void* addonData, void* file);
typedef int (*AddOnGetFileChunkSize)(const void* addonData, void* file);
typedef bool (*AddOnFileExists)(const void* addonData, const char *strFileName, bool bUseCache);
typedef int (*AddOnStatFile)(const void* addonData, const char *strFileName, struct __stat64* buffer);
typedef bool (*AddOnDeleteFile)(const void* addonData, const char *strFileName);
typedef bool (*AddOnCanOpenDirectory)(const void* addonData, const char* strURL);
typedef bool (*AddOnCreateDirectory)(const void* addonData, const char *strPath);
typedef bool (*AddOnDirectoryExists)(const void* addonData, const char *strPath);
typedef bool (*AddOnRemoveDirectory)(const void* addonData, const char *strPath);

typedef struct CB_AddOn
{
  AddOnLogCallback        Log;
  AddOnQueueNotification  QueueNotification;
  AddOnWakeOnLan          WakeOnLan;
  AddOnGetSetting         GetSetting;
  AddOnUnknownToUTF8      UnknownToUTF8;
  AddOnGetLocalizedString GetLocalizedString;
  AddOnGetDVDMenuLanguage GetDVDMenuLanguage;
  AddOnFreeString         FreeString;

  AddOnOpenFile           OpenFile;
  AddOnOpenFileForWrite   OpenFileForWrite;
  AddOnReadFile           ReadFile;
  AddOnReadFileString     ReadFileString;
  AddOnWriteFile          WriteFile;
  AddOnFlushFile          FlushFile;
  AddOnSeekFile           SeekFile;
  AddOnTruncateFile       TruncateFile;
  AddOnGetFilePosition    GetFilePosition;
  AddOnGetFileLength      GetFileLength;
  AddOnCloseFile          CloseFile;
  AddOnGetFileChunkSize   GetFileChunkSize;
  AddOnFileExists         FileExists;
  AddOnStatFile           StatFile;
  AddOnDeleteFile         DeleteFile;
  AddOnCanOpenDirectory   CanOpenDirectory;
  AddOnCreateDirectory    CreateDirectory;
  AddOnDirectoryExists    DirectoryExists;
  AddOnRemoveDirectory    RemoveDirectory;
} CB_AddOnLib;

#endif
