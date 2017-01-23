# WireWrapper
Arduino Wrapper for Wire librarry (for SAM, ESP8266...)
A wrapper for Wire library meant to be put in place of cI2C library on SAM, ESP8266... targets

## Notes:
* WireWrapper does not use any interrupt (yet, but soon will have to)
* WireWrapper is designed to act as bus Master (Slave mode will be considered in future releases)
* WireWrapper is set to work on targets with Wire library (AVR, SAM, ESP8266...)
  * for AVR targets, you may use **cI2C** instead (plain c Wire replacement)
  * **WireWrapper** & **cI2C** libs declare same structures & functions as seen from the outside
    (switch between libs without changing anyhting but the include)

## Usage: 
refer to Doxygen generated documentation & example sketches

## Examples included:
following examples should work with any I2C EEPROM/FRAM with address 0x50
(yet function to get Chip ID are device dependant (and will probably only work on FUJITSU devices))
* wirewrapper_master_write.ino: Write some bytes to FRAM and compare them with what's read afterwards
* wirewrapper_master_read.ino: Read some bytes in FRAM
* wirewrapper_advanced.ino: Redirecting slave write & read functions (to custom functions following typedef)

Doxygen doc can be generated for the library using doxyfile

## Links:

Feel free to share your thoughts @ xgarmanboziax@gmail.com about:
* issues encountered
* optimisations
* improvements & new functionalities

**cI2C**
- https://github.com/SMFSW/cI2C
- https://bitbucket.org/SMFSW/ci2c

**WireWrapper**
- https://github.com/SMFSW/WireWrapper
- https://bitbucket.org/SMFSW/wirewrapper
