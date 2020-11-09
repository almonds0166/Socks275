
import time
from datetime import datetime
import os

import serial
from serial.tools import list_ports

CLOCK_RATE = 1000 # Hz
CLOCK_CYCLE = 1 / CLOCK_RATE # seconds

TEENSY_VENDOR_ID = 5824
BAUD = 115200

FILENAME_FMT = "data_%Y-%m-%d_%H-%M-%S.csv"

def find_device():
   device = None
   usb_ports = list(list_ports.grep("USB-Serial Controller"))
   if not len(usb_ports):
      print("Hm, there's no device clearly labeled a USB-Serial Controller...")
   elif len(usb_ports) == 1:
      print(f"Found MCU: {usb_ports[0].description}")
      return usb_port[0].device

   ports = list(list_ports.comports())
   ports = {i: (ports[i], ports[i].vid) for i in range(len(ports))}
   usb_id = None
   for p in ports:
      if ports[p][1] == TEENSY_VENDOR_ID:
         usb_id = p
         break
   if usb_id is None:
      print("I can see a couple of potential ports, but I don't think any of them are Teensies...")
      print("I'll print out the list I see in case it helps:")
      for p in ports:
         print(f" * {p}:\t{ports[p][0]} (vendor ID: {ports[p][1]})")
   else:
      if not len(usb_ports):
         print("Oh wait, I found something!")
      print(f"Found MCU: {ports[p][0]}")
      device = ports[usb_id][0].device

   return device

if __name__ == "__main__":
   try:
      filename = datetime.now().strftime(FILENAME_FMT)
      print("Looking for a device...")
      device = find_device()
      if device is not None:
         print("Connecting...")
         try:
            ser = serial.Serial(
               port=device,
               baudrate=BAUD,
               parity=serial.PARITY_NONE,
               stopbits=serial.STOPBITS_ONE,
               bytesize=serial.EIGHTBITS,
               timeout=0.01
            )
         except serial.serialutil.SerialException:
            print("Uhh, port seems busy... Is a serial monitor/plotter using it?")
            exit()
         finally:
            pass

         print(f"Connected! Writing to file `{filename}`...")
         print("Press [Ctrl]+[C] (or [Command]+[.] on a mac keyboard) to finish up...")

         try:
            f = open(f"{filename}", "ab")
            timer = time.time()
            while True:
               f.write(ser.readline())
               time.sleep(max(0, CLOCK_CYCLE - (time.time() - timer)))
               timer = time.time()
         except KeyboardInterrupt:
            print("Finishing up...")
         finally:
            f.close()
            print("Data saved!")
      
      time.sleep(0.5)

   except SystemExit as e:
      time.sleep(0.5)
      if e.code: os._exit(e.code)