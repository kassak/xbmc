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
#pragma once

#include "JoystickTypes.h"

#include <string>

class IJoystickButtonMap;

/*!
 * \ingroup joysticks
 *
 * \brief Button mapper interface to assign the driver's raw button/hat/axis
 *        elements to physical joystick features.
 *
 * \sa IJoystickButtonMap
 */
class IJoystickButtonMapper
{
public:
  virtual ~IJoystickButtonMapper(void) { }

  /*!
   * \brief The add-on ID of the game controller associated with this button mapper
   *
   * \return The ID of the add-on extending kodi.game.controller
   */
  virtual std::string ControllerID(void) const = 0;

  /*!
   * \brief Handle button motion
   *
   * \param buttonIndex The index of the button as reported by the driver
   * \param bPressed    true for press motion, false for release motion
   */
  virtual bool OnButton(IJoystickButtonMap* buttonMap, unsigned int buttonIndex) = 0;

  /*!
   * \brief Handle hat motion
   *
   * \param hatIndex     The index of the hat as reported by the driver
   * \param cardinalDir  The cardinal direction: up, down, right, left (exclusively)
   */
  virtual bool OnHat(IJoystickButtonMap* buttonMap, unsigned int hatIndex, HatDirection cardinalDir) = 0;

  /*!
   * \brief Handle axis motion
   *
   * If a joystick feature requires multiple axes (analog sticks, accelerometers),
   * they can be buffered for later processing.
   *
   * \param axisIndex   The index of the axis as reported by the driver
   * \param position    The position of the axis in the closed interval [-1.0, 1.0]
   */
  virtual bool OnAxis(IJoystickButtonMap* buttonMap, unsigned int axisIndex) = 0;
};
