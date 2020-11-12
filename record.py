
import time
from datetime import datetime
import os

import serial
from serial.tools import list_ports

CLOCK_RATE = 1000 # Hz
CLOCK_CYCLE = 1 / CLOCK_RATE # seconds

TEENSY_VENDOR_ID = 5824
BAUD = 115200

FILENAME_FMT = "data_%Y-%m-%d_%H-%M-%S"

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

def plot_data(filename):
   print("Importing matplotlib and numpy...")
   import matplotlib.pyplot as plt # sue me
   import numpy as np

   print("Gathering data...")
   n = 0
   P_lower = []
   P_middle = []
   P_upper = []
   P_average = []
   valve_is_on = None
   valve_turns_on = []
   valve_turns_off = []
   buzzer_is_on = None
   buzzer_turns_on = []
   buzzer_turns_off = []
   with open(f"{filename}.csv", "r") as f:
      rows = f.readlines()[1:]
      for row in rows:
         row = row[:-1].split(",")
         P_lower.append(float(row[1]))
         P_middle.append(float(row[2]))
         P_upper.append(float(row[3]))
         P_average.append(float(row[4]))
         if row[5] == "1" and not (valve_is_on is True):
            valve_is_on = True
            valve_turns_on.append(n)
         elif row[5] == "0" and not (valve_is_on is False):
            valve_is_on = False
            valve_turns_off.append(n)
         if row[6] == "1" and not (buzzer_is_on is True):
            buzzer_is_on = True
            buzzer_turns_on.append(n)
         elif row[6] == "0" and not (buzzer_is_on is False):
            buzzer_is_on = False
            buzzer_turns_off.append(n)
         n += 1
   t = np.arange(0, n) * 0.01 # each sample is 10 ms = 0.01 s
   P_lower = np.array(P_lower)
   P_middle = np.array(P_middle)
   P_upper = np.array(P_upper)
   P_average = np.array(P_average)
   valve_turns_off.append(n)
   buzzer_turns_off.append(n)
   if valve_turns_off[0] < valve_turns_on[0]: valve_turns_off = valve_turns_off[1:]
   if buzzer_turns_off[0] < buzzer_turns_on[0]: buzzer_turns_off = buzzer_turns_off[1:]

   print("Plotting it all...")
   fig, ax = plt.subplots(figsize=(13,6))
   ax.plot(t, P_lower,  ":",  linewidth=2, alpha=0.7, color="#404096", label="Lower pressure")
   ax.plot(t, P_middle, "-.", linewidth=2, alpha=0.7, color="#57a3ad", label="Middle pressure")
   ax.plot(t, P_upper,  "--", linewidth=2, alpha=0.7, color="#dea73a", label="Upper pressure")
   ax.plot(t, P_average, linewidth=3, color="#d92120", label="Average pressure")
   for i in range(min(len(valve_turns_on), len(valve_turns_off))):
      label = None if i > 0 else "Valve activated"
      ax.axvspan(t[valve_turns_on[i]], t[valve_turns_off[i]], alpha=0.2, color="#88ccee")
   for i in range(min(len(buzzer_turns_on), len(buzzer_turns_off))):
      label = None if i > 0 else "Buzzer activated"
      ax.axvspan(t[buzzer_turns_on[i]], t[buzzer_turns_off[i]], alpha=0.2, color="#cc6677")

   ax.set_xlabel("Time (seconds)")
   ax.set_ylabel("Pressure (mmHg)")
   ax.set_title(f"{filename}")

   plt.legend()

   plt.tight_layout()
   print(f"Writing figure to `{filename}.png`...",end="")
   plt.savefig(f"{filename}.png")
   print("done!")

   print("Showing you the plot...")
   plt.show()

   time.sleep(0.5)
   print("Finished!")

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

         print("Connected!")
         input("Press [Enter] to start recording ")

         print(f"Recording! Writing to file `{filename}.csv`...")
         print("Press [Ctrl]+[C] (or [Command]+[.] on a mac keyboard) to finish up...")

         try:
            f = open(f"{filename}.csv", "ab")
            f.write("state,P_lower,P_middle,P_upper,P_average,valve_is_on,buzzer_is_on\n")
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

         answer = input("Plot data into PNG? [y/N] ").lower().strip()
         if answer in {"y", "yes"}:
            plot_data(filename)
      
      time.sleep(0.5)

   except SystemExit as e:
      time.sleep(0.5)
      if e.code: os._exit(e.code)