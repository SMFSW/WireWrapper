Arduino Wrapper for Wire library (for SAM, ESP8266...)
2017-2018 SMFSW

If you're working with AVR targets, you may consider using cI2C library instead.
WireWrapper is based on cI2C library (for functions, structs seen from the outside) but uses Wire library.


Feel free to share your thoughts @ xgarmanboziax@gmail.com about:
	- issues encountered
	- optimizations
	- improvements & new functionalities

------------

** Actual:
v1.4	11 December 2018:
- Fixed ESP32 compilation issue (Wire.end not defined in the Wire class)

v1.3	13 May 2018:
- Removed call to endTramsission after requestFrom
- Delay between retries is now 1ms
- Adding support for unit tests and doxygen documentation generation with Travis CI
- Updated README.md

v1.2	30 Nov 2017:
- No internal address transmission when reading/writing to next internal address (make sure not to r/w last 16 address right just after init, otherwise make a dummy of address 0 just before)

v1.1	29 Nov 2017:
- Set Frequency higher than Fast Mode (400KHz) for AVR will set bus to Fast Mode
- Set Frequency higher than High Speed (3.4MHz) for SAM, ESP... will set bus to Fast Mode +
- I2C_set_xxx now returns values applied, not bool

v1.0	21 Nov 2017:
- Added const qualifier for function parameters
- Return from comm functions if bytes to R/W set to 0

v0.4	12 Jul 2017:
- compliance with Arduino v1.5+ IDE source located in src subfolder

v0.3	09 Jul 2017:
- surrounded c libs with extern C
- added I2C_OK undef when WireWrapper is included for ESP8266 (ESP8266 core use an equal I2C_OK def too)
- No call to Wire.end on ESP8266 (no function for that)

v0.2	31 Jan 2017:
fixes for issue #1 (https://github.com/SMFSW/WireWrapper/issues/1)
- refactored I2C_SPEED enum names for coherence with I2C specifications
- High Speed mode added in I2C_SPEED enum

v0.1	23 Jan 2017:
- First release
