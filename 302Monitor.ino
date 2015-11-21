# define NUM_SAMPLES 10;

int sum = 0;                    // sum of samples taken
unsigned char sample_count = 0; // current sample number
float voltage = 0.0;            // calculated voltage

float temprF;
float temprFIAT;
const int fanOut = 2;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(fanOut, OUTPUT);
}




// the loop routine runs over and over again forever:
void loop() {

  tempCoolant(); // Calls on Coolant temp sensor that is in the Intake manifold. 
  // tempIAT();  // Calls on IAT sensor in Manifold
  fanControl(temprF);
  // batteryVoltage(); // 1m to 100k voltage divider 1k to ground, 1m to source, measure from middle to ADC.
  
}


float tempCoolant()
{

  float array[44] = {
    // Voltage From Sensor on left // Corresponding Temprature on right  ,
    //You find this by using voltage divider forumla. Current set is for a 4.7k ohm R1 and R2 is your ACdelco Temp Sensor
    2.332, 15,
    2.052, 20,
    1.79, 25,
    1.549, 30,
    1.332, 35,
    1.141, 40,
    0.973, 45,
    0.828, 50,
    0.705, 55,
    0.6, 60,
    0.51, 65,
    0.436, 70,
    0.372, 75,
    0.318, 80,
    0.273, 85,
    0.235, 90,
    0.203, 95,
    0.176, 100,
    0.152, 105,
    0.132, 110,
    0.115, 115,
    0.101, 120,
  };

  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (4.81 / 1023.0);    // change for differant base voltage


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



  temprF = tempr * 1.8 + 32;
//  Serial.println("Temperature:");
//  Serial.println(temprF);
  
  return temprF;
  
  
  
}

float tempIAT()
{

	float array[30] = {
		// Voltage From Sensor on left // Corresponding Temprature on right  ,
		//You find this by using voltage divider forumla. Current set is for a 10k ohm R1 and R2 is your ACdelco Temp Sensor
		
		2.308 , 0,
		2.002 , 5,
		1.718 , 10,
		2.332, 15,
		1.46, 20,
		1.036, 25,
		0.867, 30,
		0.725, 35,
		0.605, 40,
		0.505, 45,
		0.422, 50,
		0.354, 55,
		0.297, 60,
		0.251, 65,
		0.221, 70,

		
	};

	// read the input on analog pin 0:
	int sensorValue = analogRead(A1);

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
	//  Serial.println("Temperature:");
	//  Serial.println(temprF);

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
//  Serial.println("Temperature:");
  Serial.println(temprF);
  
  }

float batteryVoltage()
{
	// https://startingelectronics.org/articles/arduino/measuring-voltage-with-arduino/
	// take a number of analog samples and add them up
	while (sample_count < NUM_SAMPLES) {
		sum += analogRead(A3);
		sample_count++;
		delay(10);
	}
	// calculate the voltage
	// use 5.0 for a 5.0V ADC reference voltage
	// 5.015V is the calibrated reference voltage  4.8v in my case
	voltage = ((float)sum / (float)NUM_SAMPLES * 4.8) / 1024.0;
	// send voltage for display on Serial Monitor
	// voltage multiplied by 11 when using voltage divider that
	// divides by 11. 11.132 is the calibrated voltage divide
	// value
	Serial.print(voltage * 11.132);
	Serial.println(" V");
	sample_count = 0;
	sum = 0;
	return voltage;

}