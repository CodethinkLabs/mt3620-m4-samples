/* Copyright (c) Codethink Ltd. All rights reserved.
   Licensed under the MIT License. */

#ifndef JOYSTICK_H_
#define JOYSTICK_H_

#include <stdbool.h>
#include <stdint.h>

#include "lib/ADC.h"

typedef struct Joystick Joystick;

/// <summary>Used for the calibration finite state machine and as a direction for Joystick_Calibrate,
/// Indicates the calibration state for the centered joystick position.</summary>
#define JOYSTICK_CENTER 0
/// <summary>Used for the calibration finite state machine and as a direction for Joystick_Calibrate.
/// Indicates the calibration state for the maximum extent in the y direction.</summary>
#define JOYSTICK_Y_MAX  1
/// <summary>Used for the calibration finite state machine and as a direction for Joystick_Calibrate.
/// Indicates the calibration state for the minimum extent in the y direction.</summary>
#define JOYSTICK_Y_MIN  2
/// <summary>Used for the calibration finite state machine and as a direction for Joystick_Calibrate.
/// Indicates the calibration state for the maximum extent in the x direction.</summary>
#define JOYSTICK_X_MAX  3
/// <summary>Used for the calibration finite state machine and as a direction for Joystick_Calibrate.
/// Indicates the calibration state for the minimum extent in the x direction.</summary>
#define JOYSTICK_X_MIN  4

/// <summary>Returned during calibration if the x/y min and max values are equal or less
/// than/greater than the equivalent center position values.</summary>
#define ERROR_JOYSTICK_CAL               (ERROR_SPECIFIC - 1)

/// <summary>Returned during calibration if the direction parameter passed to Joystick_Cal doesn't
/// match any of the declared directions.</summary>
#define ERROR_JOYSTICK_NOT_A_DIRECTION   (ERROR_SPECIFIC - 2)

/// <summary>A data structure for containing joystick x/y data.</summary>
typedef struct {
	int32_t x;
	int32_t y;
} Joystick_XY;

/// <summary>
/// <para>This function opens and initializes a Joystick struct then returns a pointer to it.</para>
/// </summary>
/// <param name="data">The memory address of the ADC data.</param>
/// <param name="numChannels">The number of channels enabled on the ADC.</param>
/// <param name="channelX">The channel number on the ADC that the joystick V_x is connected to.</param>
/// <param name="channelY">The channel number on the ADC that the joystick V_y is connected to.</param>
/// <returns>A pointer to the initialized struct.</returns>
Joystick *Joystick_Open(ADC_Data *data, uint32_t numChannels, uint16_t channelX, uint16_t channelY);

/// <summary>
/// <para>This function zeroes out all values in the provided Joystick struct.</para>
/// </summary>
/// <param name="handle">The memory address of the Joystick struct to be cleared.</param>
void Joystick_Close(Joystick *handle);

/// <summary>
/// <para>This function returns the uncalibrated values of the joystick's V_x and V_y.</para>
/// </summary>
/// <param name="handle">The memory address of the joystick to return raw data for.</param>
/// <returns>The raw joystick V_x and V_y values.</returns>
Joystick_XY Joystick_GetRawXY(const Joystick *handle);

/// <summary>
/// <para>This function is used to collect the minimum and maximum values for x and y,
/// as well as the center positions for x and y.</para>
/// </summary>
/// <param name="handle">The memory address of the joystick to get calibration data for.</param>
/// <param name="direction">Input used to indicate which position the joystick is in.</param>
/// <returns>A status that indicates if the joystick calibration was successful.</returns>
int32_t Joystick_Calibrate(Joystick *handle, uint8_t direction);

/// <summary>
/// <para>This function returns calibrated values for V_x and V_y as a positive/negative percentage
/// relative to the center position and min/max values.</para>
/// </summary>
/// <param name="handle">The memory address of the joystick to get data for.</param>
/// <returns>The calibrated V_x and V_y values.</returns>
Joystick_XY Joystick_GetXY(const Joystick *handle);

#endif // #ifndef JOYSTICK_H_
