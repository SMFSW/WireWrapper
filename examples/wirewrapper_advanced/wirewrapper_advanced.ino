/*
	Master i2c (advanced)
	Redirecting slave write & read functions in setup (to custom functions following typedef)
	Read and Write operations are then called using the same functions
	Function to get Chip ID are device dependant (and will probably only work on FUJITSU devices)

	This example code is in the public domain.

	created Jan 23 2017
	latest mod May 13 2018
	by SMFSW
*/

#include <WireWrapper.h>

const uint8_t blank = 0xEE;		// blank tab filling value for test

I2C_SLAVE FRAM;					// slave declaration

void setup() {
	uint8_t str[3];
	memset(&str, blank, sizeof(str));

	Serial.begin(115200);	// start serial for output
	I2C_init(I2C_FM);		// init with Fast Mode (400KHz)
	I2C_slave_init(&FRAM, 0x50, I2C_16B_REG);
	I2C_slave_set_rw_func(&FRAM, (ci2c_fct_ptr) I2C_wr_advanced, I2C_WRITE);
	I2C_slave_set_rw_func(&FRAM, (ci2c_fct_ptr) I2C_rd_advanced, I2C_READ);

	I2C_get_chip_id(&FRAM, &str[0]);

	Serial.println();
	//for (uint8_t i = 0; i < sizeof(str); i++)	{ Serial.print(str[i], HEX); } // print hex values
	Serial.print("\nManufacturer ID: ");
	Serial.print((str[0] << 4) + (str[1]  >> 4), HEX);
	Serial.print("\nProduct ID: ");
	Serial.print(((str[1] & 0x0F) << 8) + str[2], HEX);
}

void loop() {
	const uint16_t reg_addr = 0;
	uint8_t str[7];
	memset(&str, blank, sizeof(str));

	I2C_read(&FRAM, reg_addr, &str[0], sizeof(str));	// FRAM, Addr 0, str, read chars for size of str

	Serial.println();
	for (uint8_t i = 0; i < sizeof(str); i++)
	{
		Serial.print(str[i], HEX); // print hex values
		Serial.print(" ");
	}

	delay(5000);
}


/*! \brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in] reg_addr - register address in register map
 *  \param [in] data - pointer to the first byte of a block of data to write
 *  \param [in] bytes - indicates how many bytes of data to write
 *  \return Boolean indicating success/fail of write attempt
 */
bool I2C_wr_advanced(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)											{ return false; }

	Wire.beginTransmission(slave->cfg.addr);
	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if writing next
	{
		slave->reg_addr = reg_addr;

		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (Wire.write((uint8_t) (reg_addr >> 8)) == 0)	{ return false; }
		}
		if (Wire.write((uint8_t) reg_addr) == 0)			{ return false; }
	}

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (Wire.write(*(data++)) == 0)						{ return false; }
		slave->reg_addr++;
	}

	if (Wire.endTransmission() != 0)						{ return false; }

	return true;
}


/*! \brief This procedure calls appropriate functions to perform a proper receive transaction on I2C bus.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in] reg_addr - register address in register map
 *  \param [in, out] data - pointer to the first byte of a block of data to read
 *  \param [in] bytes - indicates how many bytes of data to read
 *  \return Boolean indicating success/fail of read attempt
 */
bool I2C_rd_advanced(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)													{ return false; }

	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if reading next
	{
		slave->reg_addr = reg_addr;

		Wire.beginTransmission(slave->cfg.addr);
		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (Wire.write((uint8_t) (reg_addr >> 8)) == 0)			{ return false; }
		}
		if (Wire.write((uint8_t) reg_addr) == 0)					{ return false; }
		if (Wire.endTransmission(false) != 0)						{ return false; }
	}

	if (Wire.requestFrom((int) slave->cfg.addr, (int) bytes) == 0)	{ return false; }
	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		*data++ = Wire.read();
		slave->reg_addr++;
	}

	return true;
}


/*! \brief This procedure calls appropriate functions to get chip ID of FUJITSU devices.
 *  \param [in, out] slave - pointer to the I2C slave structure
 *  \param [in, out] data - pointer to the first byte of a block of data to read
 *  \return Boolean indicating success/fail of read attempt
 */
bool I2C_get_chip_id(I2C_SLAVE * slave, uint8_t * data)
{
	const int16_t bytes = 3;
	I2C_SLAVE FRAM_ID;

	I2C_slave_init(&FRAM_ID, 0xF8 >> 1, I2C_16B_REG);	// Dummy slave init for I2C_sndAddr

	Wire.beginTransmission(FRAM_ID.cfg.addr);
	if (Wire.write(slave->cfg.addr << 1) == 0)						{ return false; }
	if (Wire.endTransmission(false) != 0)							{ return false; }
	if (Wire.requestFrom((int) FRAM_ID.cfg.addr, (int) bytes) == 0)	{ return false; }
	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{ *data++ = Wire.read(); }

	return true;
}
