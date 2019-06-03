/* 
  KXTF-9 accelerometer code for 8-bits of valid acceleration data.
  
  With this code you can configure g-range and output data rate for the KXTF9 accelerometer
  and read those values from the serial monitor. Check the board to see the orientation of XYZ axles.
  
  Because the Arduino serial monitor is NOT a normal terminal window, 
  there are few lines of wrongful info at the start of the data.
  When using the values from serial monitor, delete the first few lines to get correct results!
  
  PS = Product Spesifications
  http://www.kionix.com/sites/default/files/KXTF9-2050%20Specifications%20Rev%207.pdf
  
  Registers for configuring interrupt functions like tap detection are not defined here. See PS for these settings.
  
  The form of this code is following:
  Either 
  "command //explanation"
  or
  " /*
  Explanation
  * /
  command"
  
  -Mika Fiskari & Iida Saksi HYV9SN

*/

//Definitions for KXTF-9 register addressess and setup values

//These addresses are from the KXTF9 Product Specifications
#define ACC 0x0F             // KXTF-9 I2C address
							 
#define CTRL_REG1 0x1B	     // Read/write control register that controls the g-range, 8- or 12-bit mode and standby/operating mode.
							 
#define XOUT_L 0x06	     // Lowpass filtered accelerometer x-axis LSB. Rest of the accelerometer axles 
                             // are in consecutive addresses and read in a while loop so their register addresses are not needed here
							 
#define DATA_CTRL_REG 0x21   // Read/write control register that configures the accelerometers output data rate. Note that to properly change the 
			     //	value of this register, the PC1 bit in CTRL_REG1 must first be set to “0”.

char buffer[6]={0,0,0,0,0,0};// Array to store ADC values
int cleanbuffer[6]={0,0,0,0,0,0};
int acc_x=0;
int acc_y=0;
int acc_z=0;
int i=0;
unsigned long aika=0;
#include <Wire.h>

void setup()
{
    Serial.begin(38400); 
    
    Wire.begin();
    
    //Start of setting up the accelerometer. These are initial setups which are done when the device is powered up or reset.
   
    //KXTF-9 configurations
    /*
    The code works by calling writeTo funtion. In the code below:
    ACC is the device address (the accelerometer) where we are sending the configuration command 
    CTRL_REG1 is the register address where we are making configurations
    0x00 is the command we are writing in CTRL_REG1 register
    See PS for register addresses and commands.
    */
   writeTo(ACC, CTRL_REG1, 0x00);   // Put the accelerometer to stand-by so we can change settings
    
   /*
   Choose output data rate (ODR). 
   Leave the line with the ODR you want to use uncommented 
   */ 
   
   //writeTo(ACC, DATA_CTRL_REG, 0x06);  // Output data rate 800 Hz.
   //writeTo(ACC, DATA_CTRL_REG, 0x05);  // ODR 400 Hz.
   writeTo(ACC, DATA_CTRL_REG, 0x04);    // ODR 200 Hz.
   //writeTo(ACC, DATA_CTRL_REG, 0x03);  // ODR 100 Hz.
   //writeTo(ACC, DATA_CTRL_REG, 0x02);  // ODR 50 Hz.
   //writeTo(ACC, DATA_CTRL_REG, 0x01);  // ODR 25 Hz.
   
   /*
   Choose g-range for 8-bit modes
   Leave the line with the g-range you want to use uncommented
   */
   
   writeTo(ACC, CTRL_REG1, 0x80);  // 2g and low current 8-bit mode.
   //writeTo(ACC, CTRL_REG1, 0x88);  // 4g and low current 8-bit mode.
   //writeTo(ACC, CTRL_REG1, 0x90);  // 8g and low current 8-bit mode.	
   
   /*
   Set delay for device setup
   The correct delay depends on the chosen output data rate
   Change the delay below according to your output data rate
   ODR 25Hz -> delay 80
   ODR 50 Hz -> delay 40
   ODR 100 Hz -> delay 20
   ODR 200 Hz -> delay 10
   ODR 400 Hz -> delay 5
   ODR 800 Hz -> delay 3
   */	 
   delay(10);
}

// Write a value to address register on device by using writeTo function
void writeTo(int device, byte address, byte val) {
  Wire.beginTransmission(device); // start transmission to device 
  Wire.write(address);             // send register address
  Wire.write(val);                 // send value to write
  Wire.endTransmission();         // end transmission
}

void loop()
{
   // Read the Accel X, Y and Z all through the ACC. 8-bit mode
      
   // First set the register start address for X on ACC 
    Wire.beginTransmission(ACC);
    Wire.write(XOUT_L); //Register Address XOUT_L - Lowpass filtered
    Wire.endTransmission();

    // Now read the 6 data bytes
    Wire.beginTransmission(ACC);
    Wire.requestFrom(ACC,6); // Read 6 bytes
    i = 0;
    while(Wire.available())
    {
        buffer[i] = Wire.read();
        i++;
    }
    Wire.endTransmission();
    
    // the 8-bit data is all in X/Y/Z OUT_H registers
    acc_x = buffer[1]; 
    acc_y = buffer[3];
    acc_z = buffer[5];
    
    // Print out what we have
    aika=millis(); //you can comment this line off if you don't need time stamp in your data
    Serial.print(aika); //you can comment this line off if you don't need time stamp in your data
   
    Serial.print(" X: ");
    Serial.print(acc_x);  // echo the number received to screen
    Serial.print(", Y: ");
    Serial.print(acc_y);
    Serial.print(", Z: ");
    Serial.print(acc_z);
    Serial.println("");     // prints carriage return
    delay(1);  
}
