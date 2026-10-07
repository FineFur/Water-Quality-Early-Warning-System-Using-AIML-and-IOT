import serial
import time
import sys

try:
    print("Opening COM9...")
    ser = serial.Serial('COM9', 115200, timeout=1)
    
    # Toggle DTR/RTS to reset ESP8266
    ser.dtr = False
    ser.rts = False
    time.sleep(0.1)
    ser.dtr = True
    ser.rts = True
    time.sleep(0.1)
    ser.dtr = False
    ser.rts = False
    
    print("Reset ESP8266. Listening for 5 seconds...")
    with open('serial_out.txt', 'w', encoding='utf-8') as f:
        start = time.time()
        while time.time() - start < 5:
            line = ser.readline()
            if line:
                f.write(line.decode('utf-8', errors='ignore'))
            
    ser.close()
    print("Done listening.")
except Exception as e:
    print(f"Error: {e}")
