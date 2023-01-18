# import serial
from Arduino import test_for_one
from test import prepare_and_test
# ser = serial.Serial(
#     port='COM9',
#     baudrate=115200,
#     parity=serial.PARITY_NONE,
#     stopbits=serial.STOPBITS_ONE,
#     bytesize=serial.EIGHTBITS,
#     timeout=0)


prepare_and_test()
test_for_one.predict("saved-photo.jpg")

# ser.write(bytes(response))
#
#
# ser.close()