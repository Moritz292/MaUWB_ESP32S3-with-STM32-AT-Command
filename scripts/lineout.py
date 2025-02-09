import serial
import serial.tools.list_ports
import time
import re
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import numpy as np

# Define the serial port and baud rate. Adjust as needed.
BAUD_RATE = 115200
TIMEOUT = 1  # Timeout for the serial read in seconds

LOG_FILE = "serial_logs.txt"

def detect_serial_port():
    """Detect and return the first available USB serial port."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # You can adjust the condition to match your specific criteria for the USB port
        if 'USB' in port.description:  # Example criteria
            return port.device
    raise Exception("No USB serial port found")

def read_uart(ser):
    with open(LOG_FILE, "a") as log_file:  # Open file in append mode
        while True:
            try:
                if ser.in_waiting > 0:
                    log_line = ser.readline().decode('utf-8').strip()
                    if log_line:
                        # Write to both console and log file
                        print(log_line)
                        log_file.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} - {log_line}\n")
                        log_file.flush()  # Ensure data is written immediately
            except Exception as e:
                print(f"Error reading from serial: {e}")
                break

if __name__ == "__main__":
    # Initialize serial connection
    try:
        SERIAL_PORT = detect_serial_port()  # Adjust this according to your system
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
        print(f"Connected to {SERIAL_PORT}")
        read_uart(ser)  # Continuously read from serial and log to file

    except serial.SerialException as e:
        print(f"Error connecting to {SERIAL_PORT}: {e}")
    except KeyboardInterrupt:
        print("Program interrupted by user. Exiting...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print(f"Closed connection to {SERIAL_PORT}")