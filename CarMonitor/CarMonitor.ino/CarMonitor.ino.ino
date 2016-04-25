#include <genieArduino.h>
#include <Wire.h>
#include <math.h>
#include <EEPROM.h>
// #include <Time.h>


// Configuration
int targetCoolantTemp = 180; // this is the target for the fans to turn on.
int targetChokeTemp = 120;// set a tempurature that you want the choke to be turned off at.
int fuelGaugeOhms = 80;


// analog pins

const byte AFRatioPin = A7; // pin for LSU 4.9 O2 sensor controller linear output
const byte coolantTempPin = A8; // pin for coolant temp
const byte oilPressPin = A9; // pin for oil pressure
const byte fuelLevelPin = A10; // pin for fuel level
const byte iatTempPin = A11;  // Air intake temperature
const byte fuelPressPin = A12; // Fuel Line Pressure Pin
const byte battVoltagePin = A13; // pin for battery voltage
const byte MiscAnalogInput = A14;  // Placeholder for another analog input. Not used as of now, but circuit is there on board.
const byte intakePressPin = A15; // pin for intake manifold pressure (vac or boost)

// analog input setup
const float aRef = 5.0; // analog reference for board (Volts)
const float regVoltage = 5.0; // instrument unit voltage regulator output (Volts)

//digital Pins

const byte oilWarninglight = 18; // pin for showing if oil pressure has dropped below 11 psi
const byte wireSDAPin = 20; // I2C SDA
const byte wireSCLPin = 21; // I2C SCL

// Fan status Bool
int switch1Val;
bool fanStatus1 = 0;
bool fanStatus2 = 0;

// Steinhart–Hart equation parameters for coolant temp sender
// http://en.wikipedia.org/wiki/Thermistor
const float SHparamA = 1.869336e-3;
const float SHparamB = 2.723037e-4;
const float SHparamC = 2.833889e-7;

unsigned long previousMillis = 0; // for sensor refresh interval

// engine info
byte engineCylinders = 8; // for future use

int displacement = 302; // (units of cu in) for MAFR calculations
int refreshInterval = 750; // milliseconds between sensor updates


//configuration for the Tachometer variables
byte engineCycles = 4; // for tach calculation :Future use
const int tachSensorPin = 19;
const int sensorInterrupt = 4;
const int timeoutValue = 5;
volatile unsigned long lastPulseTime;
volatile unsigned long interval = 0;
volatile int timeoutCounter;
int rpm;
int rpmlast = 3000;
int rpm_interval = 3000;
const int fanOut1 = 6; // need to move from pin 4 to pin 6, Screen needs this for reset, will move it on next test board to pin 6.
const int fanOut2 = 5;
const int eChoke = 22;  // Pin assignment for Choke engagment if you have an electronic choke.

Genie genie;
#define RESETLINE 2  // Change this if you are not using an Arduino Adaptor Shield Version 2 (see code below)

void setup() {
  Serial.begin(200000);  // Serial0 @ 200000 (200K) Baud
  genie.Begin(Serial);   // Use Serial0 for talking to the Genie Library, and to the 4D Systems display

  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events
  // Reset the Display (change D4 to D2 if you have original 4D Arduino Adaptor)
  // THIS IS IMPORTANT AND CAN PREVENT OUT OF SYNC ISSUES, SLOW SPEED RESPONSE ETC
  // If NOT using a 4D Arduino Adaptor, digitalWrites must be reversed as Display Reset is Active Low, and
  // the 4D Arduino Adaptors invert this signal so must be Active High.
  pinMode(RESETLINE, OUTPUT);  // Set D4 on Arduino to Output (4D Arduino Adaptor V2 - Display Reset)
  digitalWrite(RESETLINE, 1);  // Reset the Display via D4
  delay(100);
  digitalWrite(RESETLINE, 0);  // unReset the Display via D4

  delay (3500); //let the display start up after the reset (This is important)

  // Set the brightness/Contrast of the Display - (Not needed but illustrates how)
  // Most Displays, 1 = Display ON, 0 = Display OFF. See below for exceptions and for DIABLO16 displays.
  // For uLCD-43, uLCD-220RD, uLCD-70DT, and uLCD-35DT, use 0-15 for Brightness Control, where 0 = Display OFF, though to 15 = Max Brightness ON.
  genie.WriteContrast(15);

  //Write a string to the Display to show the version of the library used
  genie.WriteStr(0, GENIE_VERSION);

  Serial.begin(9600); // initialize serial communication at 9600 bits per second:
  // Fan Pin Setup
  pinMode(fanOut1, OUTPUT);
  pinMode(fanOut2, OUTPUT);
  pinMode(eChoke, OUTPUT);
  digitalWrite(fanOut1, LOW);
  digitalWrite(fanOut2, LOW);
  // Tach Setup
  pinMode(tachSensorPin, INPUT);
  attachInterrupt(sensorInterrupt, sensorIsr, RISING);
  lastPulseTime = micros();
  timeoutCounter = 0;


}





// the loop routine runs over and over again forever:
void loop() {

  static long waitPeriod = millis(); // Time now

  static int gaugeOilp = getOilpressure();
  static int gaugeEngineTemp = getCoolantTemp();
  static int gaugeIntakeTemp = getIATtemp();
  static int gaugeIntakePress = getIntakePress();
  static int gaugeVoltage = getBattVoltage();
  static int gaugeRPM = getRPM();
  static int gaugeAFratio = getAFRatio();
  static int gaugeFuelPress = getFuelPress();
  static int gaugeFuelLevel = getFuelLevel();

  fanControl(gaugeEngineTemp); // controls fan

  // write the values to the 4DS Screen.
  // Gauge name, Gauge number, Gauge Value
  genie.WriteObject(GENIE_OBJ_COOL_GAUGE, 0, gaugeAFratio); //GENIE_OBJ_ANGULAR_METER
  genie.WriteObject(GENIE_OBJ_COOL_GAUGE, 1, gaugeRPM);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 0, gaugeIntakeTemp);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 1, gaugeEngineTemp);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 2, gaugeVoltage);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 3, gaugeFuelLevel);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 4, gaugeIntakePress);
  genie.WriteObject(GENIE_OBJ_ANGULAR_METER, 5, gaugeOilp);

  genie.ReadObject(GENIE_OBJ_4DBUTTON, 0);

}
void myGenieEventHandler(void)
{
  genieFrame Event;
  genie.DequeueEvent(&Event); // Remove the next queued event from the buffer, and process it below


  if (genie.EventIs(&Event, GENIE_REPORT_OBJ, GENIE_OBJ_4DBUTTON, 0))
  { //if the event is a REPORT_OBJ from 4DBUTTON3
    switch1Val = genie.GetEventData(&Event);  //extract the MSB and LSB values and pass them to rockersw_val

  }

}
int getCoolantTemp()
{
  // Using AC Delco/GM 2 wire temp sensor.
  float array[44] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 4.7k ohm R1 and R2 is your ACdelco Temp Sensor
    2.429165  , 15 ,
    2.137986  , 20 ,
    1.864576  , 25 ,
    1.613833  , 30 ,
    1.38795   , 35 ,
    1.188159  , 40,
    1.013571  , 45,
    0.8626761 , 50,
    0.734253  , 55,
    0.6246509 , 60,
    0.5314699 , 65,
    0.4536661 , 70,
    0.3876349 , 75,
    0.3317441 , 80,
    0.286803  , 85,
    0.2446477 , 90,
    0.2110208 , 95,
    0.1824518 , 100,
    0.1582331 , 105,
    0.1376963 , 110,
    0.1201279 , 115,
    0.1050845 , 120,
  };

  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(coolantTempPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float voltage = val / 1023.0 * aRef;
  float tempr;
  if (voltage >= array[0])
  {
    tempr = array[1];
  }
  else if (voltage <= array[42])
  {
    tempr = array[43];
  }
  else
  {
    for (int i = 1; i < 44; i++)
    {
      int ii = 2 * i;
      if (voltage <= array[ii] && voltage >= array[ii + 2])
      {
        tempr = array[ii + 1] + ((array[ii + 3] - array[ii + 1]) * (voltage - array[ii]) / (array[ii + 2] - array[ii]));
        break;
      }
    }
  }
  int temprF = tempr * 1.8 + 32;
  return temprF;
}
int getOilpressure()
{
  // using VDO Oil Pressure guage, 100PSI
  float array[12] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 330 ohm R1 and R2 is your VDO Oil Pressure sensor

    2.105263 , 0.1,
    1.944444 , 14.5,
    1.764706 , 29,
    1.5625 , 43.5,
    1.071429 , 72.5,
    0.4141189 , 100,
  };
  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(oilPressPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float voltage = val / 1023.0 * aRef;

  float oilp;
  if (voltage >= array[0])
  {
    oilp = array[1];
  }
  else if (voltage <= array[10])
  {
    oilp = array[11];
  }
  else
  {
    for (int i = 1; i < 12; i++)
    {
      int ii = 2 * i;
      if (voltage <= array[ii] && voltage >= array[ii + 2])
      {
        oilp = array[ii + 1] + ((array[ii + 3] - array[ii + 1]) * (voltage - array[ii]) / (array[ii + 2] - array[ii]));
        break;
      }
    }
  }
  int oilpres = oilp;
  return oilpres;
}
int getIATtemp()
{

  float array[30] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 10k ohm R1 and R2 is your ACdelco Temp Sensor

    2.403407 , 0,
    2.085058 , 5,
    1.789109 , 10,
    1.526089 , 15,
    1.284462 , 20,
    1.079046 , 25,
    0.9033183 , 30,
    0.7547971 , 35,
    0.6305165 , 40,
    0.5265277 , 45,
    0.4400365 , 50,
    0.3686551 , 55,
    0.3095685 , 60,
    0.2611127 , 65,
    0.2207991 , 70,


  };

  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(iatTempPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float voltage = val / 1023.0 * aRef;



  float temprIAT;
  if (voltage >= array[0])
  {
    temprIAT = array[1];
  }
  else if (voltage <= array[28])
  {
    temprIAT = array[29];
  }
  else
  {
    for (int i = 1; i < 44; i++)
    {
      int ii = 2 * i;
      if (voltage <= array[ii] && voltage >= array[ii + 2])
      {
        temprIAT = array[ii + 1] + ((array[ii + 3] - array[ii + 1]) * (voltage - array[ii]) / (array[ii + 2] - array[ii]));
        break;
      }
    }
  }
  int temprFIAT = temprIAT * 1.8 + 32;
  return temprFIAT;
}
float getIntakePress()
{
  // will need to test with this sensor and moddify from there.
  // http://www.14point7.com/products/boost-vac-sensor
  // take 10 readings and sum them
  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(intakePressPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float volts = val / 1023.0 * aRef;
  float pressure = 0;
  if (volts <= 1.0)
  {
    pressure = 29.0 * volts - 29.0; // vac
  }
  else
  {
    pressure = 14.5 * volts - 14.5; // boost
  }
  return pressure;
}
float getAFRatio()
{
  float currentAFRatio = getLambda() * 14.7; // convert to Air/Fuel Mass Ratio (Air/Gasoline)
  int currentAFRatioInt = int(currentAFRatio * 10 + 0.5);
  byte lastDigit = currentAFRatioInt % 10;
  currentAFRatioInt /= 10;
  return currentAFRatioInt;
}
float getFuelPress()
{
  // using VDO  Pressure guage, 100PSI
  float array[12] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 330 ohm R1 and R2 is your VDO Oil Pressure sensor

    2.105263 , 0.1,
    1.944444 , 14.5,
    1.764706 , 29,
    1.5625 , 43.5,
    1.071429 , 72.5,
    0.4141189 , 100,
  };


  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(fuelPressPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float voltage = val / 1023.0 * aRef;
  float fuelp;
  if (voltage >= array[0])
  {
    fuelp = array[1];
  }
  else if (voltage <= array[10])
  {
    fuelp = array[11];
  }
  else
  {
    for (int i = 1; i < 12; i++)
    {
      int ii = 2 * i;
      if (voltage <= array[ii] && voltage >= array[ii + 2])
      {
        fuelp = array[ii + 1] + ((array[ii + 3] - array[ii + 1]) * (voltage - array[ii]) / (array[ii + 2] - array[ii]));
        break;
      }
    }
  }
  int fuelpres = fuelp;
  return fuelpres;
}

float getBattVoltage()
{
  // reads input voltage
  // returns battery voltage in Volts
  // resolution: 0.0176 V = 17.6 mV
  // Voltage divider maps 18 V to 5 V
  float R1 = 100000.0; // value of R1 in voltage divider (ohms)
  float R2 = 9900.0; // value of R2 in voltage divider (ohms)
  // take 10 readings and sum them
  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(battVoltagePin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float Vout = (val / 1023.0) * aRef; // convert 10-bit value to Voltage
  float Vin = Vout * (R1 + R2) / R2; // solve for input Voltage
  return Vin; // return calculated input Voltage
}
float getFuelLevel()
{
  // using Factory 80 ohm Fuel Sender(can be adjusted for others.
  float array[12] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 330 ohm R1 and R2 is your VDO Oil Pressure sensor

    2.105263 , 0.1,
    1.944444 , 14.5,
    1.764706 , 29,
    1.5625 , 43.5,
    1.071429 , 72.5,
    0.4141189 , 100,
  };
  int val = 0;
  for (int i = 1; i <= 30; i++)
  {
    val += analogRead(fuelLevelPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float voltage = val / 1023.0 * aRef;

  float level;
  if (voltage >= array[0])
  {
    level = array[1];
  }
  else if (voltage <= array[10])
  {
    level = array[11];
  }
  else
  {
    for (int i = 1; i < 12; i++)
    {
      int ii = 2 * i;
      if (voltage <= array[ii] && voltage >= array[ii + 2])
      {
        level = array[ii + 1] + ((array[ii + 3] - array[ii + 1]) * (voltage - array[ii]) / (array[ii + 2] - array[ii]));
        break;
      }
    }
  }

  level = constrain(level, 0, 100); // constrain level to between 0 and 100 (inclusive)
  return level; // return fuel level in %
}
float getLambda()
{
  // reads analog output of LSU 4.9 O2 sensor driver (0-5 V)
  // 0 V = 0.68 lambda
  // 5 V = 1.36 lambda
  // returns Lambda value (dimensionless)
  // resolution: 0.000665 = 0.01 A/F
  // take 10 readings and sum them
  int val = 0;
  for (int i = 1; i <= 10; i++)
  {
    val += analogRead(AFRatioPin);
  }
  val += 5; // allows proper rounding due to using integer math
  val /= 10; // get average value
  float lambda = (map(val, 0, 1023, 680, 1360)) / 1000.0; // calculate lambda value avoiding limitations of map()
  return lambda;
}
int getRPM()
{
  // checks accumulated tach signal pulses and calculates engine speed
  // returns engine speed in RPM
  // Resolution: 30000 * engineCycles / refreshInterval / engineCylinders RPM (for default values = 20 RPM)
  if (timeoutCounter != 0)
  {
    --timeoutCounter;

    //This calculation will need to be calibrated for each application
    rpm = 60e6 / (float)interval; // ORIGINAL



  }

  //remove erroneous results
  if (rpm > rpmlast + 500) {
    rpm = rpmlast;
  }

  rpmlast = rpm;
  rpm = constrain(rpm, 0, 8000);

  return rpm;

}
void sensorIsr()
{
  unsigned long now = micros();
  interval = now - lastPulseTime;
  lastPulseTime = now;
  timeoutCounter = timeoutValue;
}
void fanControl(float a)
{

  if (switch1Val == 0)
  {
    if (a >= targetCoolantTemp)
    {
      digitalWrite(fanOut1, HIGH); // statment to turn on fan Digital output.
      digitalWrite(fanOut2, HIGH);              // set pin high
      fanStatus1 = true;
      fanStatus2 = true;
    }
    else if (a <= targetCoolantTemp)
    {
      //Statement to turn fan digital output off.
      digitalWrite(fanOut1, LOW);
      digitalWrite(fanOut2, LOW);
      fanStatus1 = false;
      fanStatus1 = false;
    }

  }
  else
  {
    fanStatus1 = true;
    fanStatus2 = true;
  }

}
void choke()
{
  // set pin 22 high if temp is under 120F
  int engineTempF = getCoolantTemp();
  if (engineTempF < targetChokeTemp)
  {
    digitalWrite(eChoke, HIGH);
  }
  else
  {
    digitalWrite(eChoke, LOW);
  }

}
