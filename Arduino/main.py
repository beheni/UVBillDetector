import serial
from test import prepare_and_test
ser = serial.Serial(
    port='COM9',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS,
    timeout=0)



while True:
    for line in ser.read():
        if line-48 == '1':
            response = prepare_and_test()
            ser.write(bytes(response))


ser.close()