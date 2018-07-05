# aArtisanAutotune
A PID Autotuning sketch for aArtisan.

aArtisan [1] is an Arduino sketch for home coffee roasting machines. Among its functionality is PID control of the roasting temperature. Tuning the PID control parameters can be tedious. This sketch is a mix of parts of the aArtisan sketch and a PID Autotuning library aimed at automating this process.

**Requirements:** All the libraries required for aArtisan, plus the following PID Autotune library:
https://github.com/t0mpr1c3/Arduino-PID-AutoTune-Library

**Compatibility:** This library works with Arduino UNO and TC4 or CR3 boards. The hardware interface is taken directly from aArtisan. The sketch allows control of OT1, OT2 and IO3. Note that the sketch does NOT have AC fan / ZCD support. Thermocouple channels 3 and 4 seem to give wrong readings, so only channels 1 and 2 are supported for autotuning.

**How it works:** The PID Autotuning algorithm works by starting with the roaster already in "equilibrium", i.e. with the heater output constant and the temperature stable somewhere in the range that will be of interest in practice. The algorithm then successively increases then decreases the heater output above and below this steady state level, and measures the system's response. From this some reasonable PID parameters can be computed. Multiple algorithms are available that might give slightly different results.

**Usage:**
* Copy the user.h from your aArtisan sketch into the directory with the aArtisanAutotune sketch. Not all settings from this file will be applicable to this sketch, but some (e.g. PID_CHAN) do matter.
* Compile and flash onto your Arduino.
* Connect in Arduino Serial Monitor.
* You must first get the roaster to some steady state that is reflective of what will be happening during roasts, i.e. pre-heat it to somewhere in the usual roast temperature range and wait for the temperature to settle. Use "OT1 xxx", "OT2 xxx" and "IO3 xxx" commands to turn on fan (if needed) and adjust heater level to achieve desired equilibrium temperature. Note that you must use _exactly_ one whitespace and three digits, e.g. "OT1 050", not "OT1 50".
* Use "TUNE xxxxx yyyyy zzzzz n" to start the tuning algorithm, where xxxxx gives the size of the output step, yyyyy gives the noise band, zzzzz gives the lookback, and n selects the tuning algorithm. See below for more details. Again you must use the exact number of digits for each parameter (e.g. "12.34", "00012", "12345" are all okay, but "123" is not.) Example: TUNE 10.00 5.000 30.00 1
* The sketch will output status to the serial console periodically, in format Ambient Temperature, TC Temperatures 1-4, Levels OT1, OT2, IO3. TC channels 3 & 4 might have wrong readings due to a bug. Check this to make sure the temperatures stay safe; if your roaster is overheating unplug it immediately and restart with a smaller output step / higher fan duty.
* When the tuning is done the computed PID parameters will be displayed on the serial console. Note this down for future use.
* Reflash your aArtisan sketch and use the computed parameters in Artisan.

**The TUNE command:**

TUNE xxxxx yyyyy zzzzz n

xxxxx gives how far to step the output. Choose this based on your real-world usage. If your roast usually is between 60 and 80 percent heater output, pre-heat the heater to 70 percent output, and use 10 percent output step.

yyyyy gives the noise band, i.e. how much temperature readings will fluctuate due to random noise. This will likely be a few degrees or so.

zzzzz gives how far the algorithm should look back for peak detection. Try 30 seconds as a start and see if it works.
n lets you select among different tuning algorithms. Ziegler Nichols PID (1) worked for me, the full list is:

   * ZIEGLER_NICHOLS_PI = 0,	
   * ZIEGLER_NICHOLS_PID = 1,
   * TYREUS_LUYBEN_PI = 2,
   * TYREUS_LUYBEN_PID = 3,
   * CIANCONE_MARLIN_PI = 4,
   * CIANCONE_MARLIN_PID = 5,
   * AMIGOF_PI = 6,
   * PESSEN_INTEGRAL_PID = 7,
   * SOME_OVERSHOOT_PID = 8,
   * NO_OVERSHOOT_PID = 9

E.g. TUNE 10.00 5.000 30.00 1





**Known issues:** TC channels 3 & 4 seem to give wrong readings.


[1] https://github.com/greencardigan/TC4-shield/tree/master/applications/Artisan/aArtisan
