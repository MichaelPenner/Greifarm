import serial
import time
import sys

try:
    s = serial.Serial('COM16', 921600, timeout=1)  # Or 115200, let's try 115200 since it's common for esp32 serial monitor, wait earlier log showed 921600 for upload, but usually monitor is 115200.
    s.dtr = False
    s.rts = False
    time.sleep(0.1)
    s.dtr = True
    s.rts = True
    time.sleep(0.1)
    s.dtr = False
    s.rts = False
    
    t = time.time()
    out = ""
    while time.time() - t < 5:
        if s.in_waiting:
            out += s.read(s.in_waiting).decode('utf-8', 'ignore')
        time.sleep(0.1)
    
    print(out)
except Exception as e:
    print(e)
