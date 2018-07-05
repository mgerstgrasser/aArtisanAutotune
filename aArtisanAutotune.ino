#include "user.h"
#include <Wire.h>
#include <thermocouple.h> // type K, type J, and type T thermocouple support
#include <cADC.h> // MCP3424
//#include <mcEEPROM.h> // EEPROM calibration data 
#include <PWM16.h> // for SSR output


#include <PID_AutoTune_v0.h> // the Autotuner

#define DP 3 

#ifdef CELSIUS // only affects startup conditions -- UNITS command determines runtime units
boolean Cscale = true;
#else
boolean Cscale = false;
#endif

#define IO3 3

int levelOT1, levelOT2, levelIO3;  // parameters to control output levels
float AT; // ambient temp
float T[NC];  // final output values referenced to physical channels 0-3
tcBase * tcp[4];
TC_TYPE1 tc1;
TC_TYPE2 tc2;
TC_TYPE3 tc3;
TC_TYPE4 tc4;

uint8_t pid_chan = PID_CHAN;

cADC adc( A_ADC ); // MCP3424
ambSensor amb( A_AMB ); // MCP9800
PWM16 ssr;  // object for SSR output on OT1, OT2
filterRC fT[NC]; // filter for logged ET, BT

double Input, Output;


//mcEEPROM eeprom;
//calBlock caldata;



float convertUnits ( float t ) {
  if ( Cscale ) return F_TO_C( t );
  else return t;
}

void get_samples() // this function talks to the amb sensor and ADC via I2C
{
  int32_t v;
  tcBase * tc;
  float tempF;
  //  int32_t itemp;

  uint16_t dly = amb.getConvTime(); // use delay based on slowest conversion
  uint16_t dADC = adc.getConvTime();
  dly = dly > dADC ? dly : dADC;

  for ( uint8_t jj = 0; jj < NC; jj++ ) { // one-shot conversions on both chips
    uint8_t k = jj; // map logical channels to physical ADC channels
    // jj = logical channel; k = physical channel
    if ( k > 0 ) {
      --k;
      tc = tcp[k]; // each channel may have its own TC type
      adc.nextConversion( k ); // start ADC conversion on physical channel k
      amb.nextConversion(); // start ambient sensor conversion
      delay( dly ); // give the chips time to perform the conversions
      amb.readSensor(); // retrieve value from ambient temp register
      v = adc.readuV(); // retrieve microvolt sample from MCP3424
      tempF = tc->Temp_F( 0.001 * v, amb.getAmbF() ); // convert uV to Celsius
      // filter on direct ADC readings, not computed temperatures
      v = fT[k].doFilter( v << 10 );  // multiply by 1024 to create some resolution for filter
      v >>= 10;
      AT = amb.getAmbF();
      T[k] = tc->Temp_F( 0.001 * v, AT ); // convert uV to Fahrenheit;
    }
  }
};


void do_tuning(float output_step, int control_type, float look_back, float noise_band) {

  long lastSerialOutput = millis();

  boolean tuning = true;
  double kp, ki, kd;
  Input = convertUnits( T[pid_chan] );
  Output = levelOT1;

  Serial.print("Starting tuning with parameters:  outputStep = ");
  Serial.print(output_step);
  Serial.print("; noiseBand = ");
  Serial.print(noise_band);
  Serial.print("; lookBack = ");
  Serial.print(look_back);
  Serial.print("; controlType = ");
  Serial.println(control_type);

  PID_ATune aTune(&Input, &Output);
  aTune.SetControlType(control_type);
  aTune.SetNoiseBand(noise_band);
  aTune.SetOutputStep(output_step);
  aTune.SetLookbackSec(look_back);

  while (tuning) {
    // get temperature
    get_samples();
    Input = convertUnits( T[pid_chan] );

    // output debug to serial
    if (millis() > lastSerialOutput + 1000) {

      Serial.print("TUNING! Temperatures: ");
      Serial.print( convertUnits( AT ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[0] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[1] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[2] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[3] ), DP );
      Serial.print(", Levels: ");
      Serial.print( levelOT1 );
      Serial.print(",");
      Serial.print( levelOT2 );
      Serial.print(",");
      Serial.print( levelIO3);
      Serial.println(",");

      status_string();
      
      lastSerialOutput = millis();
    }


    byte val = (aTune.Runtime());
    if (val != 0)
    {
      tuning = false;
    }
    if (!tuning)
    { // when tuning algorithm is done, output values to serial.
      kp = aTune.GetKp();
      ki = aTune.GetKi();
      kd = aTune.GetKd();
      Serial.print("TUNING DONE!! kp = ");
      Serial.print(kp, 8);
      Serial.print("; ki = ");
      Serial.print(ki, 8);
      Serial.print("; kd = ");
      Serial.println(kd, 8);
    }

    levelOT1 = Output;
    ssr.Out( levelOT1, levelOT2 );

  }
}

String serial_to_string() {
  String inData = "";
  while (Serial.available() > 0)
  {
    char received = Serial.read();
    inData += received;
    if (received == '\n') break;

  }
  return inData;
}

void status_string() {

      Serial.print("Temperatures: ");
      Serial.print( convertUnits( AT ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[0] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[1] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[2] ), DP );
      Serial.print(",");
      Serial.print( convertUnits( T[3] ), DP );
      Serial.print(", Levels: ");
      Serial.print( levelOT1 );
      Serial.print(",");
      Serial.print( levelOT2 );
      Serial.print(",");
      Serial.println( levelIO3 );

}


void check_serial() {
  String command = serial_to_string();
  if (command.startsWith("OT1") && command.substring(4).toFloat()) {
      levelOT1 = command.substring(4).toInt();
      ssr.Out( levelOT1, levelOT2 );
      Serial.println("OK");
  }
  else if (command.startsWith("OT2") && command.substring(4).toFloat()) {
      levelOT2 = command.substring(4).toInt();
      ssr.Out( levelOT1, levelOT2 );
      Serial.println("OK");
  }
  else if (command.startsWith("IO3") && command.substring(4).toFloat()) {
      levelIO3 = command.substring(4).toInt();
      float pow = 2.55 * levelIO3;
      analogWrite( IO3, round( pow ) );
      Serial.println("OK");
  }
  else if (command.startsWith("TUNE ") && command.substring(5, 10).toFloat() && command.substring(11, 16).toFloat() && command.substring(17).toInt()) {
      float output_step = command.substring(5, 10).toFloat();
      float noise_band = command.substring(11, 16).toFloat();
      float look_back = command.substring(17, 22).toFloat();
      int control_type = command.substring(23).toInt();
      do_tuning(output_step, control_type, look_back, noise_band);
  }
  else {
      Serial.println("Sorry, I'm unable to parse that.");
  }
  status_string();
}

void setup() {
  delay(100);
  Wire.begin();
  Serial.begin(BAUD);


  analogWrite( IO3, 0 ); // set fan off on startup
  amb.init( AMB_FILTER );  // initialize ambient temp filtering

  adc.setCal( CAL_GAIN, UV_OFFSET );
  amb.setOffset( AMB_OFFSET );

  // initialize filters on all channels
  fT[0].init( ET_FILTER ); // digital filtering on ET
  fT[1].init( BT_FILTER ); // digital filtering on BT
  fT[2].init( ET_FILTER);
  fT[3].init( ET_FILTER);

  // set up output variables
  ssr.Setup( TIME_BASE );
  levelOT1 = levelOT2 = levelIO3 = 0;

  // assign thermocouple types
  tcp[0] = &tc1;
  tcp[1] = &tc2;
  tcp[2] = &tc3;
  tcp[3] = &tc4;

}

void loop() {
  get_samples();
  check_serial();
  
  delay(1000);
}
