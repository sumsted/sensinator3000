/*
 * Read our maxbotix ultrasonic sensors, code used to interface with sensor from datasheet sample
 * code to handle continual read, low pass filter, and serial handling ours.
 * 1. Continually read from sensors alternating between each on each loop iteration
 *    In this way we reduce the delay 100ms max between reads and ability to respond with
 *    latest readings.
 * 2. An exponential moving average is used to soften the readings. This finlter also
 *    eliminates spikes above 750 where the echo is never received.
 * 3. Listen for serial data at least two bytes with the last an !.
 * 4. Build a JSON string using the collection of current sensor states. Write it back on serial.
 */
#define SCL_PIN 5              //Default SDA is Pin5 PORTC for the UNO -- you can set this to any tristate pin
#define SCL_PORT PORTC
#define SDA_PIN 4              //Default SCL is Pin4 PORTC for the UNO -- you can set this to any tristate pin
#define SDA_PORT PORTC
#define I2C_TIMEOUT 100        //Define a timeout of 100 ms -- do not wait for clock stretching longer than this time

#include <SoftI2CMaster.h>     //You will need to install this library

#define MAX_BUFFER_SIZE 50
#define EMA_A 0.7
#define BAD_ECHO_THRESHOLD 750
#define ECHO_DELAY 100

#define NUM_SENSORS 3
#define MAX_NAME_SIZE 50
typedef struct {
    byte address;
    int range;
    int filteredRange;
    char name[MAX_NAME_SIZE];
} SensorType;

SensorType sensor[3];

byte thisSensor = 0;

void setup(){
    Serial.begin(9600);
    i2c_init();

    // setup sensor addresses
    sensor[0].address = 224;
    strcpy(sensor[0].name,"front");
    sensor[1].address = 224;
    strcpy(sensor[1].name,"left");
    sensor[2].address = 224;
    strcpy(sensor[2].name,"right");

    for(int i=0;i<NUM_SENSORS;i++){
        sensor[i].filteredRange = getRange(sensor[i].address);
    }
}

void loop()
{
    serialHandler();

    sensor[thisSensor].range = getRange(sensor[thisSensor].address);
    lowPassFilter(sensor[thisSensor].range,&sensor[thisSensor].filteredRange);

    thisSensor = ++thisSensor > 2 ? 0 : thisSensor;
}


void serialHandler(){
    char readBuffer[MAX_BUFFER_SIZE] = "";
    String readString;
    while(Serial.available() > 0){
        readString = Serial.readStringUntil('!');
    }
    if(readString.length() > 0){
        readString.toCharArray(readBuffer, MAX_BUFFER_SIZE);
        char command = readBuffer[0];
        switch(command){
            case 'R':
                writeSensorData();
                break;
            default:
                writeSensorData();
                break;
        }
    }
}

void writeSensorData(){
    char result[100];
    sprintf(result,"{\"%s\":%d,\"%s\":%d,\"%s\":%d}\n",
       sensor[0].name, sensor[0].filteredRange,
       sensor[1].name, sensor[1].filteredRange,
       sensor[2].name, sensor[2].filteredRange);
    Serial.write(result);
}

void lowPassFilter(int range, int *filteredRange){
    if(range > BAD_ECHO_THRESHOLD){
        return; // don't change filtered as range is a bad echo
    }
    *filteredRange = (EMA_A*range) + ((1-EMA_A)* *filteredRange);
}

boolean startSensor(byte bit8address){
    boolean errorlevel = 0;
    bit8address = bit8address & B11111110;               //Do a bitwise 'and' operation to force the last bit to be zero -- we are writing to the address.
    errorlevel = !i2c_start(bit8address) | errorlevel;   //Run i2c_start(address) while doing so, collect any errors where 1 = there was an error.
    errorlevel = !i2c_write(81) | errorlevel;            //Send the 'take range reading' command. (notice how the library has error = 0 so I had to use "!" (not) to invert the error)
    i2c_stop();
return errorlevel;
}

int getRange(byte address){
    boolean error = 0;
    int range;
    error = startSensor(address);
    if (!error){
        delay(ECHO_DELAY);
        range = readSensor(address);
        return range;
    }
}

int readSensor(byte bit8address){
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

byte locateSensorAddress(){
    boolean error = 0;
    int range = 0;
    for (byte i=2; i!=0; i+=2){
        error = 0;
        error = startSensor(i);
        if (!error){
            delay(100);
            range = readSensor(i);
            if (range != 0){
                return i;
            }
        }
    }
    return -1;
}
