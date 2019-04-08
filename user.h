// user.h
//---------
// This file contains user definable compiler directives for aArtisan, modified for aArtisanAutotune

#ifndef USER_H
#define USER_H


// Celsius. 
#define CELSIUS



// PID Channel.
#define PID_CHAN 2 // default physical channel for PID input, selectable by PID CHAN command


// Filter levels.
#define BT_FILTER 50 // filtering level (percent) for BT
#define ET_FILTER 50 // filtering level (percent) for ET






#define BAUD 115200  // serial baud rate (version 3)
#define AMB_FILTER 70 // 70% filtering on ambient sensor readings


// default values for systems without calibration values stored in EEPROM
#define CAL_GAIN 1.00 // you may substitute a known gain adjustment from calibration
#define UV_OFFSET 0 // you may subsitute a known value for uV offset in ADC
#define AMB_OFFSET 0.0 // you may substitute a known value for amb temp offset (Celsius)

// choose one of the following for the PWM time base for heater output on OT1 or OT2
//#define TIME_BASE pwmN4sec  // recommended for Hottop D which has mechanical relay
//#define TIME_BASE pwmN2sec
#define TIME_BASE pwmN1Hz  // recommended for most electric heaters controlled by standard SSR
//#define TIME_BASE pwmN2Hz
//#define TIME_BASE pwmN4Hz
//#define TIME_BASE pwmN8Hz 
//#define TIME_BASE 15 // should result in around 977 Hz (TODO these need testing)
//#define TIME_BASE 7 // approx. 1.95kHz
//#define TIME_BASE 6 // approx. 2.2kHz
//#define TIME_BASE 3 // approx. 3.9kHz



// Thermocouple types
// permissable options:  typeT, typeK, typeJ
#define TC_TYPE1 typeK  // thermocouple on TC1
#define TC_TYPE2 typeK  // thermocouple on TC2
#define TC_TYPE3 typeK  // thermocouple on TC3
#define TC_TYPE4 typeK  // thermocouple on TC4


// *************************************************************************************

#endif
