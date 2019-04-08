#include "user.h"
#include <Wire.h>
#include <thermocouple.h> // type K, type J, and type T thermocouple support
#include <cADC.h> // MCP3424
//#include <mcEEPROM.h> // EEPROM calibration data 
#include <PWM16.h> // for SSR output


#include <PID_AutoTune_v0.h> // the Autotuner
// The t0mpr1c3 Autotune library currently does not seem to compile on recent Arduino IDE versions. Use this one instead:
// https://github.com/jhagberg/Arduino-PID-AutoTune-Library

// #define DEBUG

#define NC 4 

#define DP 2 

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

uint32_t last_output = 0;
uint32_t last_sample_time = 0;
uint8_t last_sample_channel = 0;


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

  if ( millis() >= last_sample_time + dly){
    last_sample_channel++;
    int k = last_sample_channel;
    last_sample_channel = last_sample_channel % NC;
    #ifdef DEBUG
    Serial.print("Reading ADC channel ");
    Serial.println(k);
    #endif
    if ( k > 0 ) {
      --k;
      #ifdef DEBUG
      Serial.print("Reading ADC channel ");
      Serial.println(k);
      #endif
      tc = tcp[k]; // each channel may have its own TC type
//        delay( dly ); // give the chips time to perform the conversions
      amb.readSensor(); // retrieve value from ambient temp register
      v = adc.readuV(); // retrieve microvolt sample from MCP3424
      tempF = tc->Temp_F( 0.001 * v, amb.getAmbF() ); // convert uV to Celsius
      // filter on direct ADC readings, not computed temperatures
      v = fT[k].doFilter( v << 10 );  // multiply by 1024 to create some resolution for filter
      v >>= 10;
      AT = amb.getAmbF();
      T[k] = tc->Temp_F( 0.001 * v, AT ); // convert uV to Fahrenheit;
      last_sample_time = millis();
      #ifdef DEBUG
      Serial.print("Temperature is ");
      Serial.println(convertUnits(tc->Temp_F( 0.001 * v, AT )));
      #endif
      adc.nextConversion( (k+1)%NC ); // start ADC conversion on physical channel k
      amb.nextConversion(); // start ambient sensor conversion
    }
  }
}




void do_tuning(float output_step, int control_type, float look_back, float noise_band) {

  last_output = millis();

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
    if (millis() > last_output + 1000) {

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
      Serial.println( levelIO3);
//      Serial.println(",");

      //status_string();
      
      last_output = millis();
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
      Serial.print("Tuning parameters were:  outputStep = ");
      Serial.print(output_step);
      Serial.print("; noiseBand = ");
      Serial.print(noise_band);
      Serial.print("; lookBack = ");
      Serial.print(look_back);
      Serial.print("; controlType = ");
      Serial.println(control_type);
//      levelOT1 = 0;
//      ssr.Out( levelOT1, levelOT2 );
      delay(5000);
    }

    levelOT1 = Output;
    ssr.Out( levelOT1, levelOT2 );

    while ( Serial.available() ){
      delay(1);
      if (Serial.read() == 'x') {
        tuning = false;
        Serial.println("Received \'x\' key, tuning stopped.");
        levelOT1 = 0;
        ssr.Out( levelOT1, levelOT2 );
      }
    }

  }
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

  // Command parser
  if ( Serial.available()){
    char command[80];
    char par1[80];
    char par2[80];
    char par3[80];
    char par4[80];
    int index = 0;
    while ( Serial.available()){
      char nextchar = Serial.read();
      if(nextchar == '\n')
      {
        break;
      }
      else
      {
        command[index] = nextchar;
        index++;
        command[index] = '\0'; // Keep the string NULL terminated
      }
      delay(1);
    }

    char * tok, com;
    com = strtok (command," ,;:");
    
    float params[5] = {0,0,0,0,0};
    tok = strtok (NULL, " ,;:");
    int i = 0;
    while (tok != NULL && i < 5)
    {
      params[i] = atof(tok);
      tok = strtok (NULL, " ,;:");

      i++;
    }

      
    // Do stuff
    if ( (command[0] == 'o' || command[0] == 'O') && (command[1] == 't' || command[1] == 'T') && command[2] == '1' ){
        levelOT1 = params[0];
        ssr.Out( levelOT1, levelOT2 );
        Serial.print("OK : OT1 set to "); Serial.println(params[0]);
    }
    else if ( (command[0] == 'o' || command[0] == 'O') && (command[1] == 't' || command[1] == 'T') && command[2] == '2' ) {
        levelOT2 = params[0];
        ssr.Out( levelOT1, levelOT2 );
        Serial.print("OK : OT2 set to "); Serial.println(params[0]);
    }
    else if ( (command[0] == 'i' || command[0] == 'I') && (command[1] == 'o' || command[1] == 'O') && command[2] == '3' ) {
        levelIO3 = params[0];
        float pow = 2.55 * levelIO3;
        analogWrite( IO3, round( pow ) );
        Serial.print("OK : IO3 set to "); Serial.println(params[0]);
    }
    
    else if ( (command[0] == 't' || command[0] == 'T') && (command[1] == 'u' || command[1] == 'U') && (command[2] == 'n' || command[2] == 'N') && (command[3] == 'e' || command[3] == 'E') ) {
  
        float output_step = params[0];
        float noise_band = params[1];
        float look_back = params[2];
        int control_type = params[3];
        Serial.print("OK : Start tuning with parameters: Output step = "); Serial.print(params[0]);
        Serial.print(" Noise band = "); Serial.print(params[1]);
        Serial.print(" Look back = "); Serial.print(params[2]);
        Serial.print(" Tuning algorithm= "); Serial.println((int) params[3]);
        do_tuning(output_step, control_type, look_back, noise_band);
    }
    else{
        Serial.print("Sorry, I'm unable to parse that: ");
        Serial.println(command);
    }
  }

  // Periodically write out current temperatures and heater/fan levels.
  if ( millis() >= last_output + 1000){
    last_output = millis();
    status_string();
  }

}





void setup() {
  analogWrite( IO3, 0 ); // set fan off on startup
  
  delay(100);
  Wire.begin();
  Serial.begin(BAUD);


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
  
  //delay(1000);
}
