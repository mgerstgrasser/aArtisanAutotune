# aArtisanAutotune
A PID Autotuning sketch for aArtisan.

aArtisan / aArtisanQ_PID [1] is an Arduino sketch for home coffee roasting machines. Among its functionality is PID control of the roasting temperature. Tuning the PID control parameters can be tedious. This sketch is a mix of parts of the aArtisan sketch and a PID Autotuning library aimed at automating this process.

**Requirements:** All the libraries required for aArtisan, plus the following PID Autotune library:
https://github.com/jhagberg/Arduino-PID-AutoTune-Library

**Compatibility:** This library works with Arduino UNO and TC4 or TC4+ boards. The hardware interface is taken directly from aArtisan. The sketch allows control of OT1, OT2 and IO3 using PWM mode (PAC mode / ZCD is not supported at this time).

**How it works:** The PID Autotuning algorithm works by starting with the roaster already in "equilibrium", i.e. with the heater output constant and the temperature stable somewhere in the range that will be of interest in practice. The algorithm then successively increases then decreases the heater output above and below this steady state level, and measures the system's response. From this some reasonable PID parameters can be computed. Multiple algorithms are available that might give slightly different results.

**Usage:**
* Edit the user.h in the aArtisanAutotune sketch. Not all aArtisan settings apply to the autotuning sketch, but a few do. The two most important ones are at the beginning of the file:
  * PID_CHAN sets the (physical) PID channel.
  * BT_FILTER and ET_FILTER settings in the aArtisan / aArtisanQ_PId user.h file are often overwritten by Artisan. Since you won't be using Artisan for the tuning, I recommend putting the Artisan filter settings into user.h for this sketch.
  * Adjust the remaining settings if needed. Note that this autotuning sketch will always operate in PWM mode; PAC mode from aArtisanQ_PID is not currently supported.
* Compile and flash onto your Arduino.
* Connect in Arduino Serial Monitor.
* You should first get the roaster to some steady state that is reflective of what will be happening during roasts. That is, pre-heat it to somewhere in the usual roast temperature range and wait for the temperature to settle. Use "OT1 xxx", "OT2 xxx" and "IO3 xxx" commands to turn on fan (if needed) and adjust heater level to achieve desired temperature. Let it settle for a minute or two. Keep an eye on temperatures to avoid overheating. Don't set the heater too high. 50% might be a good starting point.
* Use "TUNE xxx yyy zzz n" to start the tuning algorithm, where xxx gives the size of the output step, yyy gives the noise band, zzz gives the lookback, and n selects the tuning algorithm. See below for more details. For instance, TUNE 20 1.5 30 1
* The sketch will output status to the serial console periodically, in format Ambient Temperature, TC Temperatures 1-4, Levels OT1, OT2, IO3. Check this to make sure the temperatures stay safe; if your roaster is overheating unplug it immediately and restart with a smaller output step / higher fan duty.
* You can also send a character lowercase 'x' on the serial interface to stop the tuning, and set OT1 to 0.
* When the tuning is done the computed PID parameters will be displayed on the serial console. Note this down for future use.
* Note that when tuning completes, OT1 will remain on at the previous level, so keep watching temperatures until you turn the heater off manually.
* Reflash your aArtisan sketch and use the computed parameters in Artisan.

**The TUNE command:**

TUNE xxx yyy zzz n

xxx gives how far to step the output. Choose this based on your real-world usage. If your roast usually is between 60 and 80 percent heater output, pre-heat the heater to 70 percent output, and use 10 percent output step.

yyy gives the noise band, i.e. how much temperature readings will fluctuate due to random noise. This will likely be a few degrees or so.

zzz gives how far the algorithm should look back for peak detection. Try 30 seconds as a start and see if it works.

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

E.g. TUNE 20 1.5 30 1

**After you're done**

Any of the above algorithms should give you a starting point for PID tuning parameters. If you still have problems:
* Try a different tuning algorithm, e.g. no-overshoot.
* If you have irregular swings in temperature or heater output, this might be due to noise in the temperature reading. Try increasing the filtering level e.g. to 90% or 95%.







[1] https://github.com/greencardigan/TC4-shield/tree/master/applications/Artisan/aArtisan_PID/trunk/src/aArtisanQ_PID
