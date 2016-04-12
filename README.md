# Classic-Car-Digital-Dash
Full digital monitoring of classic car with coolant fan control with Arduino Mega

Current Features  Tested

Coolant Temperature
	AC Delco 213-928 - Connected with 4.7k pull up resistor to create a voltage divider for 
	Arduino to read on port A0 Programmed range 41F to 248F
	Sensor is Non Linear, programmed multiple calibration points to correct the readings.

Intake Air Temp
	AC Delco 213-190 - Connected with 10k pull up resistor to create a voltage divider for 
	Arduino to read on port A1 Programmed range 32f to 158f
	Sensor is Non Linear, programmed multiple calibration points to correct the readings.

Oil Pressure 
	VDO type Oil pressure sender,100 psi,240-33 ohms,low 11 psi alarm/warning switch
	Sensor is linear.  Calculations will be made with 240 to 33 ohm range.  Low PSI warning will also
	be wired into arduino to trigger a low oil alarm.

Battery Level
	Will use a voltage divider and calibration. R1 will be 1m ohm and R2 will be 100k ohms. We will sence from the center
	of R1 and R2.  R2 will be grounded. R1 will go to positive on the battery.
Fan Control - Tempurature dependent with master switch on touchscreen for overide.

Future Additions - Code written, untested

Added code for 4Dsystem 7 inch touchscreen gauge cluster.

Fuel Level indicator  - Code is there for a 80 ohm sending unit, need to calibrate.  This can be adjust in circuit and code for other brand sending units.
	
Wideband Sensor - LSU 4.9 Wideband sensor and controller  14point7.com controller.

Intake manifold pressure   -  http://www.14point7.com/products/boost-vac-sensor

Fuel Pressure - Function is there, same sensor as the Oil pressure gauge.
