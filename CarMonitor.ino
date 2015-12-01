#include <Wire.h>




;
int temprFIAT;
const int fanOut = 2;

void setup() {
  // Start i2c master
	Wire.begin();
	// initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
 pinMode(fanOut, OUTPUT);
}




// the loop routine runs over and over again forever:
void loop() {

int oilp = oilpressure();  // calls for OilPressure calculation
int temprF = tempCoolant(); // Calls on Coolant temp sensor that is in the Intake manifold. 
 //int temprFiat = tempIAT();  // Calls on IAT sensor in Manifold
  fanControl(temprF); // controls fan
// batteryVoltage(); // 1m to 100k voltage divider 1k to ground, 1m to source, measure from middle to ADC.
 delay(200);
 Wire.beginTransmission(9); // transmit to device #9
 Wire.write(oilp);              // sends oil pressure
 Wire.write(temprF);
 Wire.endTransmission();    // stop transmitting
}


int tempCoolant()
{

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
  float sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (5.00 / 1023.0);    // change for differant base voltage


  float tempr ;

  if ( voltage >= array[0] )
  {
    tempr = array[1] ;

  }
  else if ( voltage <= array[42] )
  {
    tempr = array[43] ;

  }
  else
  {
    for ( int i = 1 ; i < 44 ; i++ )
    {

      int ii = 2 * i ;

      if ( voltage <= array[ii] && voltage >= array[ii + 2] )
      {
        tempr = array[ii + 1] + ( (array[ii + 3] - array[ii + 1]) * ( voltage - array[ii] ) / ( array[ii + 2] - array[ii] )) ;
        break ;
      }
    }
  }



  int temprF = tempr * 1.8 + 32;

  
  
 
  return temprF;

  
  
}

int oilpressure()
{

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
	float sensorValue = analogRead(A6);

	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (5.00 / 1023.0);    // change for differant base voltage
													  //Serial.println(sensorValue);

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

float tempIAT()
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
	int sensorValue = analogRead(A4);

	// Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
	float voltage = sensorValue * (4.81 / 1023.0);    // change for differant base voltage


	float temprIAT;

	if (voltage >= array[0])
	{
		temprIAT = array[1];

	}
	else if (voltage <= array[42])
	{
		temprIAT = array[43];

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



	temprFIAT = temprIAT * 1.8 + 32;
	

	return temprFIAT;



}

void fanControl(float a)
{

  if (a > 190  )
    {
      digitalWrite(fanOut,HIGH); // statment to turn on fan Digital output.
                                 // set pin high
    }
    else if (a < 180)
      {
      //Statement to turn fan digital output off.
      digitalWrite(fanOut,LOW);
      }

  
  }

float batteryVoltage()
{
	

}

