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
#pragma once

#include "input/joysticks/IJoystickDriverHandler.h"

namespace JOYSTICK
{
  class IJoystickButtonMap;
  class IJoystickInputHandler;
}

namespace PERIPHERALS
{
  class CPeripheral;

  class CAddonJoystickInputHandling : public JOYSTICK::IJoystickDriverHandler
  {
  public:
    CAddonJoystickInputHandling(CPeripheral* peripheral, JOYSTICK::IJoystickInputHandler* handler);

    virtual ~CAddonJoystickInputHandling(void);

    // implementation of IJoystickDriverHandler
    virtual bool OnButtonMotion(unsigned int buttonIndex, bool bPressed);
    virtual bool OnHatMotion(unsigned int hatIndex, JOYSTICK::HAT_STATE state);
    virtual bool OnAxisMotion(unsigned int axisIndex, float position);
    virtual void ProcessAxisMotions(void);

  private:
    JOYSTICK::IJoystickDriverHandler* m_driverHandler;
    JOYSTICK::IJoystickButtonMap*     m_buttonMap;
  };
}