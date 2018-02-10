import os
import serial
import sys
import json

import time


def get_usb_device(name, default='/dev/ttyUSB0', device_id=None):
    try:
        results = os.popen('dmesg |grep -i "ttyUSB"| grep -i "now attached"').read().split('\n')
        for line in reversed(results):
            print('line: %s' % line)
            if name in line:
                address = '/dev/'+line.split(' ')[-1]
                if device_id is not None:
                    device = serial.Serial(address, 9600, timeout=.5)
                    device.write('I!'.encode())
                    result = device.readline().decode()
                    print('result: %s' % result)
                    device_info = json.loads(result)
                    if device_info['id'] == device_id:
                        return address
                else:
                    return address
    except Exception as e:
        print('exception: '+str(e))
    return default


address = '/dev/ttyUSB0'#get_usb_device('ch341')

print(address)

nano = serial.Serial(address)#, 9600, timeout=1)
#
# nano.readline()
# nano.readline()
# nano.readline()

nano.write("R!".encode())

result = nano.readline()
print("result:", result.decode("utf-8"))
