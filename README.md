# heart_rate_monitor
this code is for heart rate monitoring device using ESP32 microcontroller which also measures SpO2, temperature and humidity

OPERATIONAL PROCEDURE FOR THE HEART RATE MONITORING DEVICE USING FIGERTIPS SENSOR

1. Powering the System
The system is powered by a rechargeable battery. The battery supplies power to the boost converter, which regulates the voltage to 5 V. The regulated 5 V supply is connected to the VIN pin of the ESP32 microcontroller and the VCC pin of the LCD module. The ESP32 provides a regulated 3.3 V supply for the MAX30102 pulse oximeter sensor and the DHT11 temperature and humidity sensor.
2. System Initialization
After powering the device, the ESP32 initializes the connected sensors, LCD and Wi-Fi communication. The microcontroller checks the availability of the sensors and establishes a connection to the configured Wi-Fi network. Once initialization is completed, the system becomes ready for measurement.
3. Measurement of Heart Rate and Blood Oxygen Saturation
The user places a fingertip gently over the MAX30102 sensor. The sensor uses its optical sensing mechanism to detect changes in blood volume within the finger. The ESP32 processes the signals received from the sensor to determine the user's heart rate and peripheral oxygen saturation (SpO₂). The calculated heart rate and SpO₂ values are displayed on the monitoring interface.
4. Measurement of Room Temperature and Humidity
The DHT11 sensor measures the surrounding room temperature and relative humidity. The sensor sends the measured values to the ESP32, which processes and displays them. The DHT11 is used strictly for environmental temperature and humidity measurement, not for measuring body temperature.
5. Display and Monitoring
The measured parameters are displayed on the connected LCD module and the web-based monitoring dashboard. The dashboard provides a convenient interface for viewing the patient's heart rate, SpO₂, room temperature and relative humidity.
6. Wi-Fi Web Dashboard Operation
The ESP32 connects to the configured Wi-Fi network and hosts the web-based monitoring dashboard. A user connected to the same network can access the dashboard through a web browser using the ESP32's IP address. The displayed readings are updated as new measurements are processed by the microcontroller.
7. Shutting Down the System
After monitoring is completed, the fingertip should be removed from the MAX30102 sensor. The device can then be switched off using its power switch. This prevents unnecessary battery consumption when the system is not in use.

