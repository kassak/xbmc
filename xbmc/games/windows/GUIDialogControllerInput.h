#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "games/GameTypes.h"
#include "guilib/GUIDialog.h"
#include "input/joysticks/IJoystickButtonMapper.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <map>
#include <string>
#include <vector>

class CGUIButtonControl;
class CGUIFocusPlane;

class CGUIDialogControllerInput : public CGUIDialog,
                                  protected CThread,
                                  public IJoystickButtonMapper
{
public:
  CGUIDialogControllerInput(void);
  virtual ~CGUIDialogControllerInput(void) { }

  // implementation of CGUIControl
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);

  void DoModal(const GAME::GameControllerPtr& controller, CGUIFocusPlane* focusControl);

  // Implementation of IJoystickButtonMapper
  virtual std::string ControllerID(void) const;
  virtual bool MapPrimitive(IJoystickButtonMap* buttonMap, const CJoystickDriverPrimitive& primitive);

protected:
  // implementation of CThread
  virtual void Process(void);

  // implementation of CGUIWindow
  virtual void OnInitWindow(void);
  virtual void OnDeinitWindow(int nextWindowID);

  void OnFocus(int iFocusedControl);
  bool OnClick(int iSelectedControl);

private:
  void PromptForInput(unsigned int buttonIndex);
  void CancelPrompt(void);
  bool IsPrompting(void) const { return m_controller && m_promptIndex >= 0; }

  int GetFocusedControl(int iControl);
  void SetFocusedControl(int iControl, int iFocusedControl);

  bool SetupButtons(const GAME::GameControllerPtr& controller, CGUIFocusPlane* focusControl);
  void CleanupButtons(void);

  CGUIButtonControl* GetButtonTemplate(void);
  CGUIButtonControl* MakeButton(const std::string& strLabel, unsigned int id, CGUIButtonControl* pButtonTemplate);

  GAME::GameControllerPtr m_controller;
  CGUIFocusPlane*         m_focusControl;

  std::map<GAME::GameControllerPtr, unsigned int> m_lastControlIds; // controller add-on ID -> last selected control ID
  int                                             m_promptIndex; // Index of feature being prompted for input
  CEvent                                          m_inputEvent;
};
