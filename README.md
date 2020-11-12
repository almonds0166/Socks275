# Socks275
Teensy 3.2 code for a medical device for lyphedema patients. MIT 2.75 Fall 2020 project.

This code isn't intended for use outside of the context of this 2.75 project.

Repo contents:

* `275/`: library to be placed in the Arduino libraries folder
* `prototype.ino`: drives the device
* `record.py`: records CSV data from the Teensy via Serial connection

## Electronics architecture diagram

![](https://i.imgur.com/cT8BKvr.png)

## Using `record.py`

This script uses the `pyserial` module -- `pip install serial`

Plug in the Teensy, start up the script, and send a `KeyboardInterrupt` signal (with `Ctrl + C` if your keyboard has a `Ctrl` key, otherwise use `Command + .` if that's what your keyboard has)

The data will be saved to a file named `data_YYYY-MM-DD_HH-MM-SS.csv`

Note, the columns correspond, respectively, to the system state (0, 1, 2, 3), the lower sensor pressure reading, the middle sensor pressure reading, the upper sensor pressure reading, the average of the three readings, whether the valve is off (0) or activated (1), and whether the buzzer is off (0) or activated (1).