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
#ifndef __KODI_PVR_CALLBACKS_H__
#define __KODI_PVR_CALLBACKS_H__

/*!
 * \brief Callback table for Kodi's pvr callbacks
 */

#include "addons/include/xbmc_pvr_types.h"

typedef void (*PVRTransferEpgEntry)(void *userData, const ADDON_HANDLE handle, const EPG_TAG *epgentry);
typedef void (*PVRTransferChannelEntry)(void *userData, const ADDON_HANDLE handle, const PVR_CHANNEL *chan);
typedef void (*PVRTransferTimerEntry)(void *userData, const ADDON_HANDLE handle, const PVR_TIMER *timer);
typedef void (*PVRTransferRecordingEntry)(void *userData, const ADDON_HANDLE handle, const PVR_RECORDING *recording);
typedef void (*PVRAddMenuHook)(void *addonData, PVR_MENUHOOK *hook);
typedef void (*PVRRecording)(void *addonData, const char *Name, const char *FileName, bool On);
typedef void (*PVRTriggerChannelUpdate)(void *addonData);
typedef void (*PVRTriggerTimerUpdate)(void *addonData);
typedef void (*PVRTriggerRecordingUpdate)(void *addonData);
typedef void (*PVRTriggerChannelGroupsUpdate)(void *addonData);
typedef void (*PVRTriggerEpgUpdate)(void *addonData, unsigned int iChannelUid);

typedef void (*PVRTransferChannelGroup)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP *group);
typedef void (*PVRTransferChannelGroupMember)(void *addonData, const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER *member);

typedef void (*PVRFreeDemuxPacket)(void *addonData, DemuxPacket* pPacket);
typedef DemuxPacket* (*PVRAllocateDemuxPacket)(void *addonData, int iDataSize);

typedef struct CB_PVRLib
{
  PVRTransferEpgEntry           TransferEpgEntry;
  PVRTransferChannelEntry       TransferChannelEntry;
  PVRTransferTimerEntry         TransferTimerEntry;
  PVRTransferRecordingEntry     TransferRecordingEntry;
  PVRAddMenuHook                AddMenuHook;
  PVRRecording                  Recording;
  PVRTriggerChannelUpdate       TriggerChannelUpdate;
  PVRTriggerTimerUpdate         TriggerTimerUpdate;
  PVRTriggerRecordingUpdate     TriggerRecordingUpdate;
  PVRTriggerChannelGroupsUpdate TriggerChannelGroupsUpdate;
  PVRTriggerEpgUpdate           TriggerEpgUpdate;
  PVRFreeDemuxPacket            FreeDemuxPacket;
  PVRAllocateDemuxPacket        AllocateDemuxPacket;
  PVRTransferChannelGroup       TransferChannelGroup;
  PVRTransferChannelGroupMember TransferChannelGroupMember;

} CB_PVRLib;

#endif
