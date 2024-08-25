/*
For ESP32 UWB AT Demo


Use 2.0.0   Wire
Use 1.11.7   Adafruit_GFX_Library
Use 1.14.4   Adafruit_BusIO
Use 2.0.0   SPI
Use 2.5.7   Adafruit_SSD1306

*/

// User config          ------------------------------------------

#define UWB_INDEX 0

//#define TAG
#define ANCHOR

#define FREQ_850K
//#define FREQ_6800K

#define UWB_TAG_COUNT 5

// User config end       ------------------------------------------

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#define SERIAL_LOG Serial
#define SERIAL_AT mySerial2

HardwareSerial SERIAL_AT(2);

// ESP32
// #define RESET 32

// #define IO_RXD2 18
// #define IO_TXD2 19

// #define I2C_SDA 4
// #define I2C_SCL 5

// ESP32S3
#define RESET 16

#define IO_RXD2 18
#define IO_TXD2 17

#define I2C_SDA 39
#define I2C_SCL 38

Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup()
{
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, HIGH);

    SERIAL_LOG.begin(115200);

    SERIAL_LOG.print(F("Hello! ESP32-S3 AT command V1.0 Test"));
    SERIAL_AT.begin(115200, SERIAL_8N1, IO_RXD2, IO_TXD2);

    SERIAL_AT.println("AT");
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(1000);
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    { // Address 0x3C for 128x32
        SERIAL_LOG.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }
    display.clearDisplay();

    logoshow();

    sendData("AT?", 2000, 1);
    sendData("AT+RESTORE", 5000, 1);

    sendData(config_cmd(), 2000, 1);
    sendData(cap_cmd(), 2000, 1);

    sendData("AT+SETRPT=1", 2000, 1);
    sendData("AT+SAVE", 2000, 1);
    sendData("AT+RESTART", 2000, 1);
}

long int runtime = 0;

String response = "";
String rec_head = "range:";
String rssi_head = "rssi:";
void loop() {
    // Handle incoming data from SERIAL_LOG to SERIAL_AT
    while (SERIAL_LOG.available()) {
        SERIAL_AT.write(SERIAL_LOG.read());
    }

    // Handle incoming data from SERIAL_AT
    while (SERIAL_AT.available()) {
        char c = SERIAL_AT.read();
        
        if (c == '\n') {
            // End of line detected, process the response
            processResponse();
        } else if (c != '\r') {
            // Add character to response, ignoring carriage returns
            response += c;
        }
    }
}

void processResponse() {
    if (response.length() > 0) {
        // Log the response to the serial monitor
        SERIAL_LOG.println(response);

        // Define the header and data indicators
        String rec_head = "range:";
        String rssi_head = "rssi:";

        // Find the positions of the range and RSSI data in the response
        int rangeIndex = response.indexOf(rec_head);
        int rssiIndex = response.indexOf(rssi_head);

        if (rangeIndex != -1 && rssiIndex != -1) {
            // Extract the data between parentheses
            int rangeStartIndex = response.indexOf('(', rangeIndex) + 1;
            int rangeEndIndex = response.indexOf(')', rangeStartIndex);
            int rssiStartIndex = response.indexOf('(', rssiIndex) + 1;
            int rssiEndIndex = response.indexOf(')', rssiStartIndex);

            if (rangeEndIndex != -1 && rssiEndIndex != -1) {
                String rangeData = response.substring(rangeStartIndex, rangeEndIndex);
                String rssiData = response.substring(rssiStartIndex, rssiEndIndex);

                // Convert the comma-separated values to float arrays
                float rangeValues[8];
                float rssiValues[8];
                int count = 0;

                // Parse the range values
                int prevIndex = 0;
                int currIndex = rangeData.indexOf(',');
                while (currIndex != -1) {
                    rangeValues[count++] = rangeData.substring(prevIndex, currIndex).toFloat();
                    prevIndex = currIndex + 1;
                    currIndex = rangeData.indexOf(',', prevIndex);
                }
                rangeValues[count] = rangeData.substring(prevIndex).toFloat();

                // Parse the RSSI values
                count = 0;
                prevIndex = 0;
                currIndex = rssiData.indexOf(',');
                while (currIndex != -1) {
                    rssiValues[count++] = rssiData.substring(prevIndex, currIndex).toFloat();
                    prevIndex = currIndex + 1;
                    currIndex = rssiData.indexOf(',', prevIndex);
                }
                rssiValues[count] = rssiData.substring(prevIndex).toFloat();

                // For demonstration, use the first range and RSSI value
                float range_value_cm = rangeValues[0]; // in cm
                float rssi_value = rssiValues[0];

                // Convert range value to meters
                float range_value_m = range_value_cm / 100.0;

                // Update the display with the parsed range and RSSI values
                updateDisplay(range_value_m, rssi_value);
            } else {
                SERIAL_LOG.println(F("Error: Incomplete range or RSSI data"));
            }
        } else {
            SERIAL_LOG.println(F("Error: 'range:' or 'rssi:' not found"));
        }

        // Clear the response for the next message
        response = "";
    }
}

// SSD1306

void logoshow(void)
{
    display.clearDisplay();

    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.println(F("MaUWB DW3000"));

    display.setCursor(0, 20); // Start at top-left corner
    // display.println(F("with STM32 AT Command"));

    display.setTextSize(2);

    String temp = "";

#ifdef TAG
    temp = temp + "T" + UWB_INDEX;
#endif
#ifdef ANCHOR
    temp = temp + "A" + UWB_INDEX;
#endif
#ifdef FREQ_850K
    temp = temp + "   850k";
#endif
#ifdef FREQ_6800K
    temp = temp + "   6.8M";
#endif
    display.println(temp);

    display.setCursor(0, 40);

    temp = "Total: ";
    temp = temp + UWB_TAG_COUNT;
    display.println(temp);

    display.display();

    delay(2000);
}

void updateDisplay(float range, float rssi) {
    display.clearDisplay(); // Clear previous content

    display.setTextSize(1);              // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);             // Start at top-left corner
    display.println(F("MaUWB DW3000"));

    display.setCursor(0, 20);
    display.setTextSize(2);
    #ifdef TAG
    display.println("TAG");
    #endif
    #ifdef ANCHOR
    display.println("Anchor");
    #endif
    display.setCursor(0, 40);
    display.setTextSize(1);
    display.print(F("Range: "));
    display.print(range, 2); // Display range with 2 decimal places
    display.println(" m");

    display.setCursor(0, 50);
    display.print(F("RSSI: "));
    display.print(rssi, 2); // Display RSSI with 2 decimal places
    display.println(" dBm");

    display.display(); // Update the display
}

String sendData(String command, const int timeout, boolean debug)
{
    String response = "";
    // command = command + "\r\n";

    SERIAL_LOG.println(command);
    SERIAL_AT.println(command); // send the read character to the SERIAL_LOG

    long int time = millis();

    while ((time + timeout) > millis())
    {
        while (SERIAL_AT.available())
        {

            // The esp has data so display its output to the serial window
            char c = SERIAL_AT.read(); // read the next character.
            response += c;
        }
    }

    if (debug)
    {
        SERIAL_LOG.println(response);
    }

    return response;
}

String config_cmd()
{
    String temp = "AT+SETCFG=";

    // Set device id
    temp = temp + UWB_INDEX;

    // Set device role
#ifdef TAG
    temp = temp + ",0";
#endif
#ifdef ANCHOR
    temp = temp + ",1";
#endif

    // Set frequence 850k or 6.8M
#ifdef FREQ_850K
    temp = temp + ",0";
#endif
#ifdef FREQ_6800K
    temp = temp + ",1";
#endif

    // Set range filter
    temp = temp + ",1";

    return temp;
}

String cap_cmd()
{
    String temp = "AT+SETCAP=";

    // Set Tag capacity
    temp = temp + UWB_TAG_COUNT;

    //  Time of a single time slot
#ifdef FREQ_850K
    temp = temp + ",15";
#endif
#ifdef FREQ_6800K
    temp = temp + ",10";
#endif

    return temp;
}