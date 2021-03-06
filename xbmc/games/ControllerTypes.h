/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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

#include <memory>
#include <vector>

namespace GAME
{
  class CGameController;
  typedef std::shared_ptr<CGameController> GameControllerPtr;
  typedef std::vector<GameControllerPtr>   GameControllerVector;

  /*!
   * \brief Types of features used in the game controller abstraction
   */
  enum FeatureType
  {
    FEATURE_UNKNOWN,
    FEATURE_BUTTON,
    FEATURE_ANALOG_STICK,
    FEATURE_ACCELEROMETER,
    FEATURE_KEY,
    FEATURE_RELATIVE_POINTER,
    FEATURE_ABSOLUTE_POINTER,
  };

  enum ButtonType
  {
    BUTTON_UNKNOWN,
    BUTTON_DIGITAL,
    BUTTON_ANALOG,
  };

  enum FeatureGeometryType
  {
    GEOMETRY_NONE,
    GEOMETRY_RECTANGLE,
    GEOMETRY_CIRCLE,
  };
}
