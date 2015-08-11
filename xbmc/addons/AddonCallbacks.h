#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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

#include "addons/include/kodi_addon_callbacks.h"
#include "addons/include/kodi_codec_callbacks.h"
#include "addons/include/kodi_game_callbacks.h"
#include "addons/include/kodi_guilib_callbacks.h"
#include "addons/include/kodi_peripheral_callbacks.h"
#include "addons/include/kodi_pvr_callbacks.h"

typedef CB_AddOnLib* (*XBMCAddOnLib_RegisterMe)(void *addonData);
typedef void (*XBMCAddOnLib_UnRegisterMe)(void *addonData, CB_AddOnLib *cbTable);
typedef CB_CODECLib* (*XBMCCODECLib_RegisterMe)(void *addonData);
typedef void (*XBMCCODECLib_UnRegisterMe)(void *addonData, CB_CODECLib *cbTable);
typedef CB_GUILib* (*XBMCGUILib_RegisterMe)(void *addonData);
typedef void (*XBMCGUILib_UnRegisterMe)(void *addonData, CB_GUILib *cbTable);
typedef CB_PeripheralLib* (*XBMCPeripheralLib_RegisterMe)(void *addonData);
typedef void (*XBMCPeripheralLib_UnRegisterMe)(void *addonData, CB_PeripheralLib *cbTable);
typedef CB_PVRLib* (*XBMCPVRLib_RegisterMe)(void *addonData);
typedef void (*XBMCPVRLib_UnRegisterMe)(void *addonData, CB_PVRLib *cbTable);
typedef CB_GameLib* (*XBMCGameLib_RegisterMe)(void *addonData);
typedef void (*XBMCGameLib_UnRegisterMe)(void *addonData, CB_GameLib *cbTable);

typedef struct AddonCB
{
  const char                *libBasePath;                  ///> Never, never change this!!!
  void                      *addonData;
  XBMCAddOnLib_RegisterMe    AddOnLib_RegisterMe;
  XBMCAddOnLib_UnRegisterMe  AddOnLib_UnRegisterMe;
  XBMCCODECLib_RegisterMe    CODECLib_RegisterMe;
  XBMCCODECLib_UnRegisterMe  CODECLib_UnRegisterMe;
  XBMCGUILib_RegisterMe      GUILib_RegisterMe;
  XBMCGUILib_UnRegisterMe    GUILib_UnRegisterMe;
  XBMCPeripheralLib_RegisterMe   PeripheralLib_RegisterMe;
  XBMCPeripheralLib_UnRegisterMe PeripheralLib_UnRegisterMe;
  XBMCPVRLib_RegisterMe      PVRLib_RegisterMe;
  XBMCPVRLib_UnRegisterMe    PVRLib_UnRegisterMe;
  XBMCGameLib_RegisterMe     GameLib_RegisterMe;
  XBMCGameLib_UnRegisterMe   GameLib_UnRegisterMe;
} AddonCB;


namespace ADDON
{

class CAddon;
class CAddonCallbacksAddon;
class CAddonCallbacksCodec;
class CAddonCallbacksGUI;
class CAddonCallbacksPeripheral;
class CAddonCallbacksPVR;
class CAddonCallbacksGame;

class CAddonCallbacks
{
public:
  CAddonCallbacks(CAddon* addon);
  ~CAddonCallbacks();
  AddonCB *GetCallbacks() { return m_callbacks; }

  static CB_AddOnLib* AddOnLib_RegisterMe(void *addonData);
  static void AddOnLib_UnRegisterMe(void *addonData, CB_AddOnLib *cbTable);
  static CB_CODECLib* CODECLib_RegisterMe(void *addonData);
  static void CODECLib_UnRegisterMe(void *addonData, CB_CODECLib *cbTable);
  static CB_GUILib* GUILib_RegisterMe(void *addonData);
  static void GUILib_UnRegisterMe(void *addonData, CB_GUILib *cbTable);
  static CB_PeripheralLib* PeripheralLib_RegisterMe(void *addonData);
  static void PeripheralLib_UnRegisterMe(void *addonData, CB_PeripheralLib *cbTable);
  static CB_PVRLib* PVRLib_RegisterMe(void *addonData);
  static void PVRLib_UnRegisterMe(void *addonData, CB_PVRLib *cbTable);
  static CB_GameLib* GameLib_RegisterMe(void *addonData);
  static void GameLib_UnRegisterMe(void *addonData, CB_GameLib *cbTable);

  CAddonCallbacksAddon *GetHelperAddon() { return m_helperAddon; }
  CAddonCallbacksCodec *GetHelperCodec() { return m_helperCODEC; }
  CAddonCallbacksGUI *GetHelperGUI() { return m_helperGUI; }
  CAddonCallbacksPeripheral *GetHelperPeripheral() { return m_helperPeripheral; }
  CAddonCallbacksPVR *GetHelperPVR() { return m_helperPVR; }
  CAddonCallbacksGame *GetHelperGame() { return m_helperGame; }

private:
  AddonCB             *m_callbacks;
  CAddon              *m_addon;
  CAddonCallbacksAddon *m_helperAddon;
  CAddonCallbacksCodec *m_helperCODEC;
  CAddonCallbacksGUI   *m_helperGUI;
  CAddonCallbacksPeripheral *m_helperPeripheral;
  CAddonCallbacksPVR   *m_helperPVR;
  CAddonCallbacksGame  *m_helperGame;
};

}; /* namespace ADDON */
