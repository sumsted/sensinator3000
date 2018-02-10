import serial
import json
import time

nano = serial.Serial('/dev/ttyUSB1')
for i in range(500):
    nano.write("E!".encode())
    result = nano.readline()
    ultra = json.loads(result.decode("utf-8"))
    print(ultra)
    time.sleep(.2)

