#define SCL_PIN 5              //Default SDA is Pin5 PORTC for the UNO -- you can set this to any tristate pin
#define SCL_PORT PORTC
#define SDA_PIN 4              //Default SCL is Pin4 PORTC for the UNO -- you can set this to any tristate pin
#define SDA_PORT PORTC
#define I2C_TIMEOUT 100        //Define a timeout of 100 ms -- do not wait for clock stretching longer than this time
#include <SoftI2CMaster.h>     //You will need to install this library

#define EMA_A 0.6
int emaS;
byte sensorAddress;

void setup(){
  Serial.begin(9600);
  i2c_init();
  sensorAddress = locateSensorAddress();
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

int lowPassFilter(int range){
    emaS = (EMA_A*range) + ((1-EMA_A)*emaS);
    return emaS;
}

boolean startSensor(byte bit8address){
    boolean errorlevel = 0;
    bit8address = bit8address & B11111110;
    errorlevel = !i2c_start(bit8address) | errorlevel;
    errorlevel = !i2c_write(81) | errorlevel;
    i2c_stop();
    return errorlevel;
}

int getRange(byte address){
    boolean error = 0;
    int range;
    error = startSensor(address);
    if (!error){
        delay(100);
        range = readSensor(address);
        return range;
    }
}

int readSensor(byte bit8address){
    boolean errorlevel = 0;
    int range = 0;
    byte range_highbyte = 0;
    byte range_lowbyte = 0;
    bit8address = bit8address | B00000001;
    errorlevel = !i2c_start(bit8address) | errorlevel;
    range_highbyte = i2c_read(0);
    range_lowbyte  = i2c_read(1);
    i2c_stop();
    range = (range_highbyte * 256) + range_lowbyte;
    if(errorlevel){
        return 0;
    }
    else{
        return range;
    }
}

byte locateSensorAddress(){
    boolean error = 0;
    int range = 0;
    for (byte i=2; i!=0; i+=2){
        error = 0;
        error = startSensor(i);
        if (!error){
            delay(100);
            range = readSensor(i);
            Serial.println(i);
            if (range != 0){
                return i;
            }
        }
    }
    return -1;
}
