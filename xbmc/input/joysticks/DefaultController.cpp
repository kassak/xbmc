/*
 *      Copyright (C) 2014-2015 Team XBMC
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

#include "DefaultController.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/joysticks/JoystickTranslator.h"
#include "input/Key.h"
#include "threads/SingleLock.h"
#include "ApplicationMessenger.h"

#include <algorithm>

#define DEFAULT_GAME_CONTROLLER   "game.controller.default"
#define HOLD_TIMEOUT_MS           500
#define REPEAT_TIMEOUT_MS         50
#define ANALOG_DIGITAL_THRESHOLD  0.5f

#ifndef ABS
#define ABS(x)  ((x) >= 0 ? (x) : (-x))
#endif

#ifndef MAX
#define MAX(x, y)  ((x) >= (y) ? (x) : (y))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)  (sizeof(x) / sizeof((x)[0]))
#endif

CDefaultController::CDefaultController(void) :
  m_holdTimer(this),
  m_lastButtonPress(0)
{
}

std::string CDefaultController::ControllerID(void) const
{
  return DEFAULT_GAME_CONTROLLER;
}

InputType CDefaultController::GetInputType(const std::string& feature) const
{
  unsigned int buttonKeyId = GetButtonKeyID(feature);
  if (buttonKeyId != 0)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(buttonKeyId, 0)));
    if (action.GetID() > 0 && action.IsAnalog())
      return INPUT_TYPE_ANALOG;
  }

  return INPUT_TYPE_DIGITAL;
}

bool CDefaultController::OnButtonPress(const std::string& feature, bool bPressed)
{
  unsigned int buttonKeyId = GetButtonKeyID(feature);
  if (buttonKeyId != 0)
  {
    if (bPressed)
    {
      CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(buttonKeyId, 0)));
      if (action.GetID() > 0)
        ProcessButtonPress(action);
    }
    else
    {
      ProcessButtonRelease(buttonKeyId);
    }
  }

  return true;
}

bool CDefaultController::OnButtonMotion(const std::string& feature, float magnitude)
{
  unsigned int buttonKeyId = GetButtonKeyID(feature);
  if (buttonKeyId != 0)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(buttonKeyId, 0)));
    if (action.GetID() > 0)
    {
      CAction actionWithAmount(action.GetID(), magnitude, 0.0f, action.GetName());
      CApplicationMessenger::Get().SendAction(actionWithAmount);
    }
  }

  return true;
}

bool CDefaultController::OnAnalogStickMotion(const std::string& feature, float x, float y)
{
  unsigned int buttonKeyId  = GetButtonKeyID(feature, x, y);

  float magnitude = MAX(ABS(x), ABS(y));

  unsigned int buttonRightId = GetButtonKeyID(feature,  1.0f,  0.0f);
  unsigned int buttonUpId    = GetButtonKeyID(feature,  0.0f,  1.0f);
  unsigned int buttonLeftId  = GetButtonKeyID(feature, -1.0f,  0.0f);
  unsigned int buttonDownId  = GetButtonKeyID(feature,  0.0f, -1.0f);

  unsigned int buttonKeyIds[] = {buttonRightId, buttonUpId, buttonLeftId, buttonDownId};

  for (unsigned int i = 0; i < ARRAY_SIZE(buttonKeyIds); i++)
  {
    if (buttonKeyIds[i] == 0)
      continue;

    CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(buttonKeyId, 0)));
    if (action.GetID() > 0)
    {
      if (action.IsAnalog())
      {
        if (buttonKeyId == buttonKeyIds[i])
        {
          CAction actionWithAmount(action.GetID(), magnitude, 0.0f, action.GetName());
          CApplicationMessenger::Get().SendAction(actionWithAmount);
        }
      }
      else
      {
        if (buttonKeyId == buttonKeyIds[i])
        {
          if (magnitude >= ANALOG_DIGITAL_THRESHOLD)
            ProcessButtonPress(action);
          else if (magnitude < ANALOG_DIGITAL_THRESHOLD)
            ProcessButtonRelease(buttonKeyId);
        }
        else
        {
          ProcessButtonRelease(buttonKeyIds[i]);
        }
      }
    }
  }

  return true;
}

bool CDefaultController::OnAccelerometerMotion(const std::string& feature, float x, float y, float z)
{
  return false; // TODO
}

void CDefaultController::OnTimeout(void)
{
  CSingleLock lock(m_digitalMutex);

  const unsigned int holdTimeMs = (unsigned int)m_holdTimer.GetTotalElapsedMilliseconds();

  if (m_lastButtonPress != 0 && holdTimeMs >= HOLD_TIMEOUT_MS)
  {
    CAction action(CButtonTranslator::GetInstance().GetAction(g_windowManager.GetActiveWindowID(), CKey(m_lastButtonPress, holdTimeMs)));
    if (action.GetID() > 0)
      CApplicationMessenger::Get().SendAction(action);
  }
}

void CDefaultController::ProcessButtonPress(const CAction& action)
{
  if (std::find(m_pressedButtons.begin(), m_pressedButtons.end(), action.GetButtonCode()) == m_pressedButtons.end())
  {
    ClearHoldTimer();

    CApplicationMessenger::Get().SendAction(action);

    CSingleLock lock(m_digitalMutex);

    m_pressedButtons.push_back(action.GetButtonCode());
    StartHoldTimer(action.GetButtonCode());
  }
}

void CDefaultController::ProcessButtonRelease(unsigned int buttonKeyId)
{
  std::vector<unsigned int>::iterator it = std::find(m_pressedButtons.begin(), m_pressedButtons.end(), buttonKeyId);
  if (it != m_pressedButtons.end())
  {
    m_pressedButtons.erase(it);

    if (buttonKeyId == m_lastButtonPress || m_pressedButtons.empty())
      ClearHoldTimer();
  }
}

void CDefaultController::StartHoldTimer(unsigned int buttonKeyId)
{
  m_holdTimer.Start(REPEAT_TIMEOUT_MS, true);
  m_lastButtonPress = buttonKeyId;
}

void CDefaultController::ClearHoldTimer(void)
{
  m_holdTimer.Stop(true);
  m_lastButtonPress = 0;
}

unsigned int CDefaultController::GetButtonKeyID(const std::string& feature,
                                                float x /* = 0.0f */,
                                                float y /* = 0.0f */,
                                                float z /* = 0.0f */)
{
  if      (feature == "a")             return KEY_BUTTON_A;
  else if (feature == "b")             return KEY_BUTTON_B;
  else if (feature == "x")             return KEY_BUTTON_X;
  else if (feature == "y")             return KEY_BUTTON_Y;
  else if (feature == "start")         return KEY_BUTTON_START;
  else if (feature == "back")          return KEY_BUTTON_BACK;
  else if (feature == "guide")         return KEY_BUTTON_GUIDE;
  else if (feature == "leftbumper")    return KEY_BUTTON_LEFT_SHOULDER;
  else if (feature == "rightbumper")   return KEY_BUTTON_RIGHT_SHOULDER;
  else if (feature == "leftthumb")     return KEY_BUTTON_LEFT_THUMB_BUTTON;
  else if (feature == "rightthumb")    return KEY_BUTTON_RIGHT_THUMB_BUTTON;
  else if (feature == "up")            return KEY_BUTTON_DPAD_UP;
  else if (feature == "down")          return KEY_BUTTON_DPAD_DOWN;
  else if (feature == "right")         return KEY_BUTTON_DPAD_RIGHT;
  else if (feature == "left")          return KEY_BUTTON_DPAD_LEFT;
  else if (feature == "lefttrigger")   return KEY_BUTTON_LEFT_TRIGGER;
  else if (feature == "righttrigger")  return KEY_BUTTON_RIGHT_TRIGGER;
  else if (feature == "leftstick")
  {
    if      (y >= x && y >  -x)          return KEY_BUTTON_LEFT_THUMB_STICK_UP;
    else if (y <  x && y >= -x)          return KEY_BUTTON_LEFT_THUMB_STICK_RIGHT;
    else if (y <= x && y <  -x)          return KEY_BUTTON_LEFT_THUMB_STICK_DOWN;
    else if (y >  x && y <= -x)          return KEY_BUTTON_LEFT_THUMB_STICK_LEFT;
  }
  else if (feature == "rightstick")
  {
    if      (y >= x && y >  -x)          return KEY_BUTTON_RIGHT_THUMB_STICK_UP;
    else if (y <  x && y >= -x)          return KEY_BUTTON_RIGHT_THUMB_STICK_RIGHT;
    else if (y <= x && y <  -x)          return KEY_BUTTON_RIGHT_THUMB_STICK_DOWN;
    else if (y >  x && y <= -x)          return KEY_BUTTON_RIGHT_THUMB_STICK_LEFT;
  }
  else if (feature == "accelerometer") return 0; // TODO

  return 0;
}
