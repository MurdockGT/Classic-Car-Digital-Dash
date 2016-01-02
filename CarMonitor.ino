#include <Wire.h>
#include <Math.h>
#include <EEPROM.h>  
// #include <Time.h>



// analog pins

const byte AFRatioPin = A7; // pin for LSU 4.9 O2 sensor controller linear output
const byte coolantTempPin = A8; // pin for coolant temp
const byte oilPressPin = A9; // pin for oil pressure
const byte fuelLevelPin = A10; // pin for fuel level
const byte iatTempPin = A11;  // Air intake temperature
const byte fuelPressPin = A12; // Fuel Line Pressure Pin
const byte battVoltagePin = A13; // pin for battery voltage
const byte MiscAnalogInput = A14;  // Placeholder for another analog input.
const byte intakePressPin = A15; // pin for intake manifold pressure (vac or boost)

								 // analog input setup
const float aRef = 5.0; // analog reference for board (Volts)
const float regVoltage = 5.0; // instrument unit voltage regulator output (Volts)
const float fuelGaugeOhms = 13.0; // resistance of fuel level gauge (ohms)
								  //digital Pins
const byte tachPin = 3; // tach signal on digital pin 3 (interrupt 1)
const byte wireSDAPin = 20; // I2C SDA
const byte wireSCLPin = 21; // I2C SCL



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
const int tachSensorPin = 21;
const int sensorInterrupt = 2;
const int timeoutValue = 5;
volatile unsigned long lastPulseTime;
volatile unsigned long interval = 0;
volatile int timeoutCounter;
int rpm;
int rpmlast = 3000;
int rpm_interval = 3000;
const int fanOut = 2;

void setup() {
	// Start i2c master
	Wire.begin();
	Serial.begin(9600); // initialize serial communication at 9600 bits per second:
						// Fan Pin Setup
	pinMode(fanOut, OUTPUT);
	// Tach Setup
	pinMode(tachSensorPin, INPUT);
	attachInterrupt(sensorInterrupt, sensorIsr, RISING);
	lastPulseTime = micros();
	timeoutCounter = 0;


}





// the loop routine runs over and over again forever:
void loop() {



	int oilp = getOilpressure();  // calls for OilPressure calculation
	int engineTemp = getCoolantTemp(); // Calls on Coolant temp sensor that is in the Intake manifold. 
									   //int intakeAirTemp = getIATtemp();  // Calls on IAT sensor in Manifold
	fanControl(engineTemp); // controls fan
							//int batteryVoltage = getBatteryVoltage(); // 1m to 100k voltage divider 1k to ground, 1m to source, measure from middle to ADC.
	int currentRPM = getRPM();

	// Transfer information to display arduino/4ds device
	Wire.beginTransmission(9); // transmit to device #9
	Wire.write(oilp);// sends oil pressure
	Wire.write(engineTemp);
	Wire.write(currentRPM);
	//Wire.write(batteryVoltage);
	//Wire.write(iatTemp);
	Wire.endTransmission();    // stop transmitting


}

int getCoolantTemp()
{
	// Using AC Delco/GM 2 wire temp sensor.
	float array[44] = {
		// Voltage From Sensor on left // Corresponding Temprature on right  ,
		//You find this by using voltage divider forumla. Current set is for a 4.7k ohm R1 and R2 is your ACdelco Temp Sensor
		2.429165 , 15 ,
		2.137986 , 20 ,
		1.864576 , 25 ,
		1.613833 , 30 ,
		1.38795  , 35 ,
		1.188159 , 40,
		1.013571 , 45,
		0.8626761 ,50,
		0.734253 , 55,
		0.6246509 , 60 ,
		0.5314699 ,65,
		0.4536661 ,70,
		0.3876349 ,75,
		0.3317441 ,80,
		0.286803  ,85 ,
		0.2446477 ,90,
		0.2110208 ,95,
		0.1824518 ,100,
		0.1582331 ,105,
		0.1376963 ,110,
		0.1201279 ,115,
		0.1050845 ,120,
	};

	// read the input on analog pin 0:
	float sensorValue = analogRead(coolantTempPin);

	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (5.00 / 1023.0);    // change for differant base voltage
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
	// read the input on analog pin 1:
	float sensorValue = analogRead(oilPressPin);
	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (5.00 / 1023.0);    // change for differant base voltage                         
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
	// read the input on analog pin 0:
	int sensorValue = analogRead(iatTempPin);
	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (5.00 / 1023.0);    // change for differant base voltage
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

}
float getMAFR()
{
	int currentRPM = getRPM();
	float VE = 0.9; // volumetric efficiency (dependent on intake pressure and engine speed, assumed constant for now)

	float intakeTempAbsolute = getIATtemp + 273.15; // intake temp in Kelvin
	float coolantTempAbsolute = getCoolantTemp() + 273.15; // coolant temp in Kelvin
	float intakePressureAbsolute = (getIntakePress() + 14.7) * 0.06895; // intake pressure in bar (absolute)
	float airDensity = intakePressureAbsolute * 29.0 / 8.314e-2 / (0.5 * intakeTempAbsolute + 0.5 * coolantTempAbsolute); // calculate air density in g/L. Relative weightings of intake and engine temperature will change with engine speed. Assumed constant 50/50 for now
	float MAFR = currentRPM * displacement / 61.024 / 2.0 * VE * airDensity / 1000; // calculate MAFR in kg/min
	return MAFR;
}
float getBattVoltage()
{
	// reads input voltage
	// returns battery voltage in Volts
	// resolution: 0.0176 V = 17.6 mV
	// Voltage divider maps 18 V to 5 V
	float R1 = 100000.0; // value of R1 in voltage divider (ohms)
	float R2 = 38300.0; // value of R2 in voltage divider (ohms)
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
	// reads voltage between fuel level gauge and sensor
	// returns Fuel Level in %
	// Resolution:
	// Voltage divider maps 6.1 V to 5 V
	float R1 = 22000.0; // value of R1 in voltage divider (ohms)
	float R2 = 100000.0; // value of R2 in voltage divider (ohms)
						 // take 10 readings and sum them
	int val = 0;
	for (int i = 1; i <= 10; i++)
	{
		val += analogRead(oilPressPin);
	}
	val += 5; // allows proper rounding due to using integer math
	val /= 10; // get average value
	float Vout = val / 1023.0 * aRef; // convert 10-bit value to Voltage
	float VI = Vout * (R1 + R2) / R2; // solve for input Voltage
	float Rsender = VI * fuelGaugeOhms / (regVoltage - VI); // solve for sensor resistance
	float level = 108.4 - 0.56 * Rsender; // solve for fuel level based on calibration curve
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
	if (rpm > rpmlast + 500) { rpm = rpmlast; }

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

	if (a > 190)
	{
		digitalWrite(fanOut, HIGH); // statment to turn on fan Digital output.
									// set pin high
	}
	else if (a < 180)
	{
		//Statement to turn fan digital output off.
		digitalWrite(fanOut, LOW);
	}
}