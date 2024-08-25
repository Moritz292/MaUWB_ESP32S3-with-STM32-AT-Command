import serial
import time
import re
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque
import numpy as np

# Define the serial port and baud rate. Adjust as needed.
SERIAL_PORT = '/dev/ttyUSB1'  # Adjust this according to your system
BAUD_RATE = 115200
TIMEOUT = 1  # Timeout for the serial read in seconds

# Regular expression to parse range and rssi values from the log
log_pattern = re.compile(r'range:\(([\d\.,\-]+)\),rssi:\(([\d\.,\-]+)\)')

# Data storage
MAX_POINTS = 500
range_values = deque(maxlen=MAX_POINTS)
rssi_values = deque(maxlen=MAX_POINTS)
timestamps = deque(maxlen=MAX_POINTS)

def read_uart(ser):
    """Read a line from UART, decode it, and return the range and rssi values."""
    try:
        if ser.in_waiting > 0:
            log_line = ser.readline().decode('utf-8', errors='replace').strip()
            
            # Match the log line with the regular expression
            match = log_pattern.search(log_line)
            if match:
                range_data = [float(x) for x in match.group(1).split(',')]
                rssi_data = [float(x) for x in match.group(2).split(',')]
                return range_data, rssi_data

    except Exception as e:
        print(f"Error reading UART: {e}")
    
    return None, None

def update_plot(frame, ser, line1, line2):
    """Update the plot with new data from the UART."""
    range_data, rssi_data = read_uart(ser)

    if range_data and rssi_data:
        # Convert the range from cm to meters (divide by 100)
        range_in_meters = range_data[0] / 100  # Use the first range value and convert to meters
        
        # Append new data
        range_values.append(range_in_meters)
        rssi_values.append(rssi_data[0])    # Use the first RSSI value
        timestamps.append(time.time() - start_time)  # Track the time elapsed

        # Update the data of existing lines
        line1.set_data(timestamps, range_values)
        line2.set_data(timestamps, rssi_values)

        # Adjust the plot limits if necessary
        for ax in (ax1, ax2):
            ax.relim()
            ax.autoscale_view()

    return line1, line2

if __name__ == "__main__":
    # Initialize serial connection
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
        print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud rate...")

        # Initialize matplotlib plot
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
        fig.suptitle("UWB Range and RSSI Live Plot")

        # Initialize the plot lines
        line1, = ax1.plot([], [], label="Range (meters)", color="blue")
        line2, = ax2.plot([], [], label="RSSI (dBm)", color="red")

        # Set up axes
        ax1.set_ylabel("Range (m)")
        ax1.set_xlabel("Time (s)")
        ax1.legend()
        ax1.grid(True)

        ax2.set_ylabel("RSSI (dBm)")
        ax2.set_xlabel("Time (s)")
        ax2.legend()
        ax2.grid(True)

        # Start time for timestamps
        start_time = time.time()

        # Start live plot animation
        ani = FuncAnimation(fig, update_plot, fargs=(ser, line1, line2), interval=50, blit=True)

        # Show plot
        plt.show()

    except serial.SerialException as e:
        print(f"Error connecting to {SERIAL_PORT}: {e}")
    except KeyboardInterrupt:
        print("Program interrupted by user. Exiting...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()