/*******************************************************************************
  Copyright(c) 2011 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#ifndef INDIBASETYPES_H
#define INDIBASETYPES_H

/*! INDI property type */
typedef enum
{
    INDI_NUMBER, /*!< INumberVectorProperty. */
    INDI_SWITCH, /*!< ISwitchVectorProperty. */
    INDI_TEXT,   /*!< ITextVectorProperty. */
    INDI_LIGHT,  /*!< ILightVectorProperty. */
    INDI_BLOB,   /*!< IBLOBVectorProperty. */
    INDI_UNKNOWN
} INDI_PROPERTY_TYPE;

/*! INDI Equatorial Axis type */
typedef enum
{
    AXIS_RA,     /*!< Right Ascension Axis. */
    AXIS_DE      /*!< Declination Axis. */
} INDI_EQ_AXIS;

/*! INDI Horizontal Axis type */
typedef enum
{
    AXIS_AZ,     /*!< Azimuth Axis. */
    AXIS_ALT     /*!< Altitude Axis. */
} INDI_HO_AXIS;

/*! North/South Direction */
typedef enum
{
    DIRECTION_NORTH,
    DIRECTION_SOUTH
} INDI_DIR_NS;

/*! West/East Direction */
typedef enum
{
    DIRECTION_WEST,
    DIRECTION_EAST
} INDI_DIR_WE;

/*! INDI Error Types */
typedef enum
{
    INDI_DEVICE_NOT_FOUND   =-1,
    INDI_PROPERTY_INVALID   =-2,
    INDI_PROPERTY_DUPLICATED=-3,
    INDI_DISPATCH_ERROR     =-4
} INDI_ERROR_TYPE;

#endif // INDIBASETYPES_H
