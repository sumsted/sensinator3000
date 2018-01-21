#define SCL_PIN 5              //Default SDA is Pin5 PORTC for the UNO -- you can set this to any tristate pin
#define SCL_PORT PORTC
#define SDA_PIN 4              //Default SCL is Pin4 PORTC for the UNO -- you can set this to any tristate pin
#define SDA_PORT PORTC
#define I2C_TIMEOUT 100        //Define a timeout of 100 ms -- do not wait for clock stretching longer than this time

#include <SoftI2CMaster.h>     //You will need to install this library

#define MAX_BUFFER_SIZE 50
#define EMA_A 0.6

#define NUM_SENSORS 3
#define MAX_NAME_SIZE 50
typedef struct {
    byte address;
    int range;
    int emas;
    char name[MAX_NAME_SIZE];
} SensorType;

SensorType sensor[3];

void setup(){
    Serial.begin(9600);
    i2c_init();

    OCR0A = 0xAF; // use the same timer as the millis() function
    TIMSK0 |= _BV(OCIE0A);
    interrupts();

    sensor[0].address = 220;
    strcpy(sensor[0].name,"left");
    sensor[1].address = 222;
    strcpy(sensor[1].name,"front");
    sensor[2].address = 223;
    strcpy(sensor[2].name,"right");

    for(int i=0;i<NUM_SENSORS;i++){
        sensor[i].emas = getRange(sensor[i].address);
    }
}

void loop()
{
    serialHandler();
}

int count = 0;
bool checkingSensors = false;
ISR(TIMER0_COMPA_vect){
    count++;
    if(count >= 1000 && !checkingSensors){
        count = 0;
        checkingSensors = true;
        readAllSensors();
        checkingSensors = false;
    }
}

void readAllSensors(){
    for(int i=0;i<NUM_SENSORS;i++){
        sensor[i].range = getRange(sensor[i].address);
        lowPassFilter(sensor[i].range,&sensor[i].emas);
    }
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
    sprintf(result,"{\"%s\":%ld,\"%s\":%ld,\"%s\":%ld}",
       sensor[0].name, sensor[0].range, sensor[1].name, sensor[1].range, sensor[2].name, sensor[2].range);
    Serial.write(result);
}

void lowPassFilter(int range, int *emas){
    *emas = (EMA_A*range) + ((1-EMA_A)* *emas);
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
            Serial.println(i);
            if (range != 0){
                return i;
            }
        }
    }
    return -1;
}
