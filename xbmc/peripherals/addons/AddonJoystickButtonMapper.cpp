/*
 *      Copyright (C) 2014 Team XBMC
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

#include "AddonJoystickButtonMapper.h"
#include "addons/include/kodi_peripheral_utils.hpp"
#include "peripherals/Peripherals.h"

using namespace PERIPHERALS;

CAddonJoystickButtonMapper::CAddonJoystickButtonMapper(CPeripheral* device, const std::string& strDeviceId)
  : m_device(device),
    m_addon(g_peripherals.GetAddon(device)),
    m_strDeviceId(strDeviceId)
{
}

bool CAddonJoystickButtonMapper::Load(void)
{
  return m_addon.get() != NULL;
}

bool CAddonJoystickButtonMapper::MapButton(unsigned int featureIndex, const CJoystickDriverPrimitive& primitive)
{
  bool retVal(false);

  if (m_addon)
  {
    switch (primitive.Type())
    {
      case DriverPrimitiveTypeButton:
      {
        ADDON::DriverButton driverButton(featureIndex, primitive.Index());
        retVal = m_addon->MapJoystickFeature(m_device, m_strDeviceId, &driverButton);
        break;
      }
      case DriverPrimitiveTypeHatDirection:
      {
        ADDON::DriverHat driverHat(featureIndex, primitive.Index(), ToHatDirection(primitive.HatDir()));
        retVal = m_addon->MapJoystickFeature(m_device, m_strDeviceId, &driverHat);
        break;
      }
      case DriverPrimitiveTypeSemiAxis:
      {
        ADDON::DriverSemiAxis driverSemiAxis(featureIndex, primitive.Index(), ToSemiAxisDirection(primitive.SemiAxisDir()));
        retVal = m_addon->MapJoystickFeature(m_device, m_strDeviceId, &driverSemiAxis);
        break;
      }
      default:
        break;
    }
  }

  return retVal;
}

bool CAddonJoystickButtonMapper::MapAnalogStick(unsigned int featureIndex,
                                                int horizIndex, bool horizInverted,
                                                int vertIndex,  bool vertInverted)
{
  bool retVal(false);

  if (m_addon)
  {
    ADDON::DriverAnalogStick driverAnalogStick(featureIndex, horizIndex, horizInverted,
                                                             vertIndex,  vertInverted);

    retVal = retVal = m_addon->MapJoystickFeature(m_device, m_strDeviceId, &driverAnalogStick);
  }

  return retVal;
}

bool CAddonJoystickButtonMapper::MapAccelerometer(unsigned int featureIndex,
                                                  int xIndex, bool xInverted,
                                                  int yIndex, bool yInverted,
                                                  int zIndex, bool zInverted)
{
  bool retVal(false);

  if (m_addon)
  {
    ADDON::DriverAccelerometer driverAccelerometer(featureIndex, xIndex, xInverted,
                                                                 yIndex, yInverted,
                                                                 zIndex, zInverted);

    retVal = retVal = m_addon->MapJoystickFeature(m_device, m_strDeviceId, &driverAccelerometer);
  }

  return retVal;
}

JOYSTICK_DRIVER_HAT_DIRECTION CAddonJoystickButtonMapper::ToHatDirection(HatDirection dir)
{
  switch (dir)
  {
    case HatDirectionLeft:   return JOYSTICK_DRIVER_HAT_LEFT;
    case HatDirectionRight:  return JOYSTICK_DRIVER_HAT_RIGHT;
    case HatDirectionUp:     return JOYSTICK_DRIVER_HAT_UP;
    case HatDirectionDown:   return JOYSTICK_DRIVER_HAT_DOWN;
    default:                 break;
  }
  return JOYSTICK_DRIVER_HAT_UNKNOWN;
}

JOYSTICK_DRIVER_SEMIAXIS_DIRECTION CAddonJoystickButtonMapper::ToSemiAxisDirection(SemiAxisDirection dir)
{
  switch (dir)
  {
    case SemiAxisDirectionNegative: return JOYSTICK_DRIVER_SEMIAXIS_DIRECTION_NEGATIVE;
    case SemiAxisDirectionPositive: return JOYSTICK_DRIVER_SEMIAXIS_DIRECTION_POSITIVE;
    default:                        break;
  }
  return JOYSTICK_DRIVER_SEMIAXIS_DIRECTION_UNKNOWN;
}
