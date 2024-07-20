import serial
import json


try:
    ser = serial.Serial('/dev/ttyAMA0', 115200, timeout=0.5)
    while True:
        if ser.in_waiting:
        # json_string = str(ser.readline(), encoding = 'utf-8')
            json_string = ser.readline().decode("utf-8")
            print(json_string)
            data = json.loads(json_string)
            print(data['streamD'])
    # ser.close()
except serial.SerialException as e:
    print(f"Failed to connect on {port}: {e}")
