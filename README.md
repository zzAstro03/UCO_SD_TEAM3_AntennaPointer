# UCO_SD_TEAM3_AntennaPointer

## Purpose
  This project was created for senior engineering design at the University of Central Oklahoma.
  Working with the FAA, the device is to aid in the installation of antennas by replacing
  a plumb bob and compass with a single digital device. This will save time and money.

## Goals
  The goal for this semester is to get each individual component working. So far, 
  each individual component works, and next semester will be incorporating
  each component with one another to create the final product. There are 6 main 
  components to this device. 

## Components
### ESP32-S3-DevKitC-1 v1.1 (WORKING)
  This is the main brain of the entire device. It is a microcontroller capable of controlling
  various peripherals. It was chosen due to its widespread support and low power consumption.
  It is currently being programmed using ESP-IDF v5.3.1 via the VSCode extension with a majority 
  of the code being written in C.

  Documentation:
  https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html

### ICM20948 (WORKING)
  The orientation of the device is determined by an ICM20948. This is a 9 degree of freedom inertial
  measurement unit that includes an accelerometer, gyroscope, and magnetometer. By using the data
  output from this sensor, the device's orientation can be determined.

  Adafruit:
  https://www.adafruit.com/product/4554
  <br />
  Datasheet:
  https://invensense.tdk.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf

### World Magnetic Model (WORKING)
  The World Magnetic Model is used to determine the magnetic declination of the device based on
  input latitude, longitude, date, and elevation. This magnetic declination is used to adjust the
  magnetometer readings to give a True North heading.

  World Magnetic Model:
  https://www.ncei.noaa.gov/products/world-magnetic-model

### ILI9488 TFT LCD Display (WORKING)
  The ILI9488 thin-film-transistor liquid-crystal display is used to display output data and allow
  the user to interact with the device via the keypad. A driver was downloaded from the atanisoft
  component via the ESP-IDF Component Registry. LVGL is being used to create the graphical user interface.
  The driver uses an older version of LVGL (v8.4) compared to the newest (v9.2). The newest version changes
  some of the functions used and breaks the driver.

  Driver:
  https://components.espressif.com/components/atanisoft/esp_lcd_ili9488
  <br />
  LVGL:
  https://docs.lvgl.io/8.4/

### Keypad (WORKING)
  The keypad is just a simple matrix keypad so far. Ideally in the future, a custom PCB would
  be created to allow all of the required user inputs. It needs to include:
  0 1 2 3 4 5 6 7 8 9 . Enter Backspace Up Down Left Right

  No links included, simple to find.

### Battery Management System (WORKING)
  The BMS chosen is a diymore 18650 battery shield. The battery shield controls the charging and
  IO state of the device. Two 18650 3500 mAh Li-ion batteries in parallel give the device a total
  7000 mAh. The battery life is still to be determined as the amp draw hasn't been measured yet.

  Shield:
  https://www.diymore.cc/products/18650-battery-shield-v8-mobile-power-bank-3v-5v-for-arduino-esp32-esp8266-wifi
  <br />
  Batteries:
  https://imrbatteries.com/products/samsung-35e-18650-3500mah-8a-battery


