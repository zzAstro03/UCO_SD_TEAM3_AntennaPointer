# UCO_SD_TEAM3_AntennaPointer

## Purpose
  This project was created for senior engineering design at the University of Central Oklahoma.
  Working with the FAA, the device is to aid in the installation of antennas by replacing
  a plumb bob and compass with a single digital device. This will save time and money.

  This GitHub currently focuses more on the electrical engineering side of things as that is what
  I (Nathaniel) am personally responsible for, but will work with my team to update it to include the mechanical side
  of things as well.

## Design
  The device

## Goals
  For the first semester, the goal was to primarily get a physical design done all of the individual electrical components working. 
  We were successful.

  The goal for this semester is to finalize the device. Currently, the custom keypad PCB has been created, soldered, and programmed.
  A rudimentary GUI has been programmed to the display and allows the user to interact with it via the keypad. This allows
  the user to input the latitude, longitude, antenna offset, and date. The user is then able to hit enter, which will
  put them onto the outputs screen, where the current device Azimuth and Elevation are displayed.

  The next steps are to calibrate the IMU we are using. The magnetometer needs to adjust for hard and soft iron offsets, as well
  as tilt. Currently the elevation is reading pretty accurately.

  After the calibration is done, the user inputs will be used to calculate the magnetic declination and adjust the sensor data
  accordingly. The magnetic declination will adjust the azimuth readings, while the antenna offset wiill adjust the elevation readings.
  This adjusted data is what will then be put on the outputs screen as described above.

  After that, the device will be finished on the electrical side. I would like to do some quality of life improvements if I can get there.
  

## Components
### ESP32-S3-DevKitC-1 v1.1 (WORKING)
  This is the main brain of the entire device. It is a microcontroller capable of controlling
  various peripherals. It was chosen due to its widespread support and low power consumption.
  It is currently being programmed via ESP-IDF v5.4.0 with a majority of the code being written in C.

  Documentation:
  https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

### ICM20948 (WORKING, NOT IMPLEMENTED, NEEDS CALIBRATION)
  The orientation of the device is determined by an ICM20948. This is a 9 degree of freedom inertial
  measurement unit that includes an accelerometer, gyroscope, and magnetometer. By using the data
  output from this sensor, the device's orientation can be determined.

  Adafruit:
  https://www.adafruit.com/product/4554
  <br />
  Datasheet:
  https://invensense.tdk.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf

### World Magnetic Model (WORKING, NOT IMPLEMENTED)
  The World Magnetic Model is used to determine the magnetic declination of the device based on
  input latitude, longitude, date, and elevation. This magnetic declination is used to adjust the
  magnetometer readings to give a True North heading. It is updated every 5 years and was just recently
  updated in December of 2024. This model will be accurate until December of 2029.

  In the future I will create a guide on how to update the WMM.COF files to allow the device to update
  the model to be used indefinitely, as long as the WMM keeps updating every 5 years.

  The website is given below to allow you to read up on it. It is widely used throughout the world
  and also has open source MIT Licensed software that can be used.

  World Magnetic Model:
  https://www.ncei.noaa.gov/products/world-magnetic-model

### ILI9488 TFT LCD Display (WORKING)
  The ILI9488 thin-film-transistor liquid-crystal display is used to display output data and allow
  the user to interact with the device via the keypad. A driver was downloaded from the atanisoft
  component via the ESP-IDF Component Registry. This driver was then registered to be used via LVGL.
  The driver is responsible for lower level stuff like drawing pixels and communicating with the ESP32.
  LVGL is an embedded graphics library and is described in the next section.

### LVGL (WORKING, NEEDS BEAUTIFICATION)


### Keypad (WORKING)
  The keypad is a custom keypad created by yours truly. The primary use of this is to navigate the 
  display's input and output screens. The input screen allows the user to input the necessary information 
  to calculate magnetic declination and get the antenna offset. The output screens output the adjusted azimuth
  and elevation from the sensor.

  I designed the keypad in KiCad and had it sent
  to PCBWay to be created. It is a matrix keypad that has the keys:
  0 1 2 3 4 5 6 7 8 9 . Enter Backspace Up Down Left Right

  The circuit and PCB schematics are in the Design folder. 


### Battery Management System (WORKING)
  The BMS chosen is a diymore 18650 battery shield. The battery shield controls the charging and
  IO state of the device. Two 18650 3500 mAh Li-ion batteries in parallel give the device a total
  7000 mAh. The battery life is still to be determined as the amp draw hasn't been measured yet.

  Shield:
  https://www.diymore.cc/products/18650-battery-shield-v8-mobile-power-bank-3v-5v-for-arduino-esp32-esp8266-wifi
  <br />
  Batteries:
  https://imrbatteries.com/products/samsung-35e-18650-3500mah-8a-battery


