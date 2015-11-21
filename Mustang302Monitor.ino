
float temprF;
const int fanOut = 2;

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(fanOut, OUTPUT);
}




// the loop routine runs over and over again forever:
void loop() {

  tempCoolant();
  fanControl(temprF);
  
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
  return temprF;
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

