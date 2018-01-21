#define SCL_PIN 5              //Default SDA is Pin5 PORTC for the UNO -- you can set this to any tristate pin
#define SCL_PORT PORTC
#define SDA_PIN 4              //Default SCL is Pin4 PORTC for the UNO -- you can set this to any tristate pin
#define SDA_PORT PORTC
#define I2C_TIMEOUT 100        //Define a timeout of 100 ms -- do not wait for clock stretching longer than this time

#include <SoftI2CMaster.h>     //You will need to install this library

#define NEW_ADDRESS 222

#define EMA_A 0.6
int emaS;
byte sensorAddress;

void setup(){
  // Initialize both the serial and I2C bus
  Serial.begin(9600);
  i2c_init();
  sensorAddress = locateSensorAddress();
  byte checkAddress = changeAddress(sensorAddress, NEW_ADDRESS);
  if(checkAddress == NEW_ADDRESS){
    sensorAddress = checkAddress;
    Serial.println("unable to change address");
  }
  emaS = getRange(sensorAddress);
}

void loop()
{
  int range = getRange(sensorAddress);
  int filteredRange = lowPassFilter(range);
  Serial.print(range);
  Serial.print(",");
  Serial.println(filteredRange);
}


byte changeAddress(byte oldAddress, byte newAddress){
  boolean error = 0;  //Create a bit to check for catch errors as needed.
  int range;

  Serial.print("Take a reading at the old address: ");
  Serial.println(oldAddress);

  //Take a range reading at the default address of 224
  error = start_sensor(oldAddress);    //Start the sensor and collect any error codes.
  if (!error){                  //If you had an error starting the sensor there is little point in reading it.
    delay(100);
    range = getRange(oldAddress);   //reading the sensor will return an integer value -- if this value is 0 there was an error
    Serial.print("R:");Serial.println(range);
  }else{
    return -1;
  }

  Serial.println("Change the sensor at the default address to 222");
  //Change the address from 224 to 222
  error = 0;
  error = change_address(oldAddress,newAddress);  //Change the address -- I don't do anything with the error handler at this point but you can if you want.
  delay(200);    //Wait 125ms for the sensor to save the new address and reset

  Serial.print("Take a reading at the new address: ");
  Serial.println(newAddress);

  error = 0;
  error = start_sensor(newAddress);     //Same as above but at the new address
  if (!error){
    delay(100);
    range = getRange(newAddress);
    Serial.print("N:");Serial.println(range);
  }else{
    return -1;
  }
  return newAddress;
}

boolean change_address(byte oldaddress,byte newaddress){
  //note that the new address will only work as an even number (odd numbers will round down)
  boolean errorlevel = 0;
  oldaddress = oldaddress & B11111110;  //Do a bitwise 'and' operation to force the last bit to be zero -- we are writing to the address.
  errorlevel = !i2c_start(oldaddress) | errorlevel; //Start communication at the new address and track error codes
  errorlevel = !i2c_write(170) | errorlevel;        //Send the unlock code and track the error codes
  errorlevel = !i2c_write(165) | errorlevel;        //Send the unlock code and track the error codes
  errorlevel = !i2c_write(newaddress) | errorlevel; //Send the new address
  i2c_stop();
  return errorlevel;
}
int lowPassFilter(int range){
    emaS = (EMA_A*range) + ((1-EMA_A)*emaS);
    return emaS;
}

///////////////////////////////////////////////////
// Function: Start a range reading on the sensor //
///////////////////////////////////////////////////
//Uses the I2C library to start a sensor at the given address
//Collects and reports an error bit where: 1 = there was an error or 0 = there was no error.
//INPUTS: byte bit8address = the address of the sensor that we want to command a range reading
//OUPUTS: bit  errorlevel = reports if the function was successful in taking a range reading: 1 = the function
//	had an error, 0 = the function was successful
boolean start_sensor(byte bit8address){
  boolean errorlevel = 0;
  bit8address = bit8address & B11111110;               //Do a bitwise 'and' operation to force the last bit to be zero -- we are writing to the address.
  errorlevel = !i2c_start(bit8address) | errorlevel;   //Run i2c_start(address) while doing so, collect any errors where 1 = there was an error.
  errorlevel = !i2c_write(81) | errorlevel;            //Send the 'take range reading' command. (notice how the library has error = 0 so I had to use "!" (not) to invert the error)
  i2c_stop();
  return errorlevel;
}


int getRange(byte address){
  boolean error = 0;  //Create a bit to check for catch errors as needed.
  int range;
  //Take a range reading at the default address of 224
  error = start_sensor(224);    //Start the sensor and collect any error codes.
  if (!error){                  //If you had an error starting the sensor there is little point in reading it as you will get old data.
    delay(100);
    range = read_sensor(224);   //reading the sensor will return an integer value -- if this value is 0 there was an error
    return range;
  }
}


///////////////////////////////////////////////////////////////////////
// Function: Read the range from the sensor at the specified address //
///////////////////////////////////////////////////////////////////////
//Uses the I2C library to read a sensor at the given address
//Collects errors and reports an invalid range of "0" if there was a problem.
//INPUTS: byte  bit8address = the address of the sensor to read from
//OUPUTS: int   range = the distance in cm that the sensor reported; if "0" there was a communication error
int read_sensor(byte bit8address){
  boolean errorlevel = 0;
  int range = 0;
  byte range_highbyte = 0;
  byte range_lowbyte = 0;
  bit8address = bit8address | B00000001;  //Do a bitwise 'or' operation to force the last bit to be 'one' -- we are reading from the address.
  errorlevel = !i2c_start(bit8address) | errorlevel;
  range_highbyte = i2c_read(0);           //Read a byte and send an ACK (acknowledge)
  range_lowbyte  = i2c_read(1);           //Read a byte and send a NACK to terminate the transmission
  i2c_stop();
  range = (range_highbyte * 256) + range_lowbyte;  //compile the range integer from the two bytes received.
  if(errorlevel){
    return 0;
  }
  else{
    return range;
  }
}


////////////////////////////////////////////////////////////////
// Code Example: Poll all possible addresses to find a sensor //
////////////////////////////////////////////////////////////////
byte locateSensorAddress(){
  boolean error = 0;  //Create a bit to check for catch errors as needed.
  int range = 0;
  for (byte i=2; i!=0; i+=2){   //start at 2 and count up by 2 until wrapping to 0. Checks all addresses (2-254) except 0 (which cannot be used by a device)
    error = 0;
    error = start_sensor(i);    //Start the sensor and collect any error codes.
    if (!error){                //If you had an error starting the sensor there is little point in reading it.
      delay(100);
      range = read_sensor(i);   //reading the sensor will return an integer value -- if this value is 0 there was an error
      Serial.println(i);
      if (range != 0){
        return i;
      }
    }
  }
  return -1;
}
