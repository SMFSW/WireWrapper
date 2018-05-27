/*!\file WireWrapper.cpp
** \author SMFSW
** \copyright MIT SMFSW (2017-2018)
** \brief Arduino Wrapper for Wire library (for SAM, ESP8266...) code
** \warning Don't access (r/w) last 16b internal address byte alone right after init, this would lead to hazardous result (in such case, make a dummy read of addr 0 before)
**/

// TODO: add interrupt vector / callback for it operations (if not too messy)
// TODO: consider interrupts at least for RX when slave (and TX when master)

#include "WireWrapper.h"


/*!\struct i2c
** \brief static ci2c bus config and control parameters
**/
static struct {
	/*!\struct cfg
	** \brief ci2c bus parameters
	**/
	struct {
		I2C_SPEED	speed;			//!< i2c bus speed
		uint8_t		retries;		//!< i2c message retries when fail
		uint16_t	timeout;		//!< i2c timeout (ms)
	} cfg;
	uint16_t		start_wait;		//!< time start waiting for acknowledge
	bool			busy;			//!< true if already busy (in case of interrupts implementation)
} i2c = { { (I2C_SPEED) 0, DEF_CI2C_NB_RETRIES, DEF_CI2C_TIMEOUT }, 0, false };


// Needed prototypes
static bool I2C_wr(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);
static bool I2C_rd(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes);


/*!\brief Init an I2C slave structure for cMI2C communication
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \param [in] reg_sz - internal register map size
** \return nothing
**/
void I2C_slave_init(I2C_SLAVE * slave, const uint8_t sl_addr, const I2C_INT_SIZE reg_sz)
{
	(void) I2C_slave_set_addr(slave, sl_addr);
	(void) I2C_slave_set_reg_size(slave, reg_sz);
	I2C_slave_set_rw_func(slave, (ci2c_fct_ptr) I2C_wr, I2C_WRITE);
	I2C_slave_set_rw_func(slave, (ci2c_fct_ptr) I2C_rd, I2C_READ);
	slave->reg_addr = (uint16_t) -1;	// To be sure to send address on first access (warning: unless last 16b byte address is accessed alone)
	slave->status = I2C_OK;
}

/*!\brief Redirect slave I2C read/write function (if needed for advanced use)
** \param [in] slave - pointer to the I2C slave structure to init
** \param [in] func - pointer to read/write function to affect
** \param [in] rw - 0 = write function, 1 = read function
** \return nothing
**/
void I2C_slave_set_rw_func(I2C_SLAVE * slave, const ci2c_fct_ptr func, const I2C_RW rw)
{
	ci2c_fct_ptr * pfc = (ci2c_fct_ptr*) (rw ? &slave->cfg.rd : &slave->cfg.wr);
	*pfc = func;
}

/*!\brief Change I2C slave address
** \param [in, out] slave - pointer to the I2C slave structure to init
** \param [in] sl_addr - I2C slave address
** \return true if new address set (false if address is >7Fh)
**/
bool I2C_slave_set_addr(I2C_SLAVE * slave, const uint8_t sl_addr)
{
	if (sl_addr > 0x7F)		{ return false; }
	slave->cfg.addr = sl_addr;
	return true;
}

/*!\brief Change I2C registers map size (for access)
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_sz - internal register map size
** \return true if new size is correct (false otherwise and set to 16bit by default)
**/
bool I2C_slave_set_reg_size(I2C_SLAVE * slave, const I2C_INT_SIZE reg_sz)
{
	slave->cfg.reg_size = reg_sz > I2C_16B_REG ? I2C_16B_REG : reg_sz;
	return !(reg_sz > I2C_16B_REG);
}

/*!\brief Set I2C current register address
** \attribute inline
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address
** \return nothing
**/
static inline void __attribute__((__always_inline__)) I2C_slave_set_reg_addr(I2C_SLAVE * slave, const uint16_t reg_addr) {
	slave->reg_addr = reg_addr; }


/*!\brief Enable I2c module on arduino board (including pull-ups,
 *         enabling of ACK, and setting clock frequency)
** \attribute inline
** \param [in] speed - I2C bus speed in KHz
** \return nothing
**/
void I2C_init(const uint16_t speed)
{
	Wire.begin();
	I2C_set_speed(speed);
}

/*!\brief Change I2C frequency
** \param [in] speed - I2C speed in kHz (max 3.4MHz)
** \return Configured bus speed
**/
uint16_t I2C_set_speed(const uint16_t speed)
{
	#if defined(__TINY__)
		i2c.cfg.speed = I2C_STD;	// Can't change I2C speed through Wire.setClock on TINY platforms
	#else
		#if defined(__AVR__)
			i2c.cfg.speed = (I2C_SPEED) ((speed == 0) ? (uint16_t) I2C_STD : ((speed > (uint16_t) I2C_FM) ? (uint16_t) I2C_FM : speed));	// Up to 400KHz on AVR
		#else
			i2c.cfg.speed = (I2C_SPEED) ((speed == 0) ? (uint16_t) I2C_STD : ((speed > (uint16_t) I2C_HS) ? (uint16_t) I2C_FMP : speed));
		#endif
		Wire.setClock(speed * 1000);
	#endif
	
	return i2c.cfg.speed;
}

/*!\brief Change I2C ack timeout
** \param [in] timeout - I2C ack timeout (500 ms max)
** \return Configured timeout
**/
uint16_t I2C_set_timeout(const uint16_t timeout)
{
	static const uint16_t max_timeout = 500;
	i2c.cfg.timeout = (timeout > max_timeout) ? max_timeout : timeout;
	return i2c.cfg.timeout;
}

/*!\brief Change I2C message retries (in case of failure)
** \param [in] retries - I2C number of retries (max of 8)
** \return Configured number of retries
**/
uint8_t I2C_set_retries(const uint8_t retries)
{
	static const uint16_t max_retries = 8;
	i2c.cfg.retries = (retries > max_retries) ? max_retries : retries;
	return i2c.cfg.retries;
}

/*!\brief Get I2C busy status
** \return true if busy
**/
bool I2C_is_busy(void) {
	return i2c.busy; }


/*!\brief This function reads or writes the provided data to/from the address specified.
 *        If anything in the write process is not successful, then it will be repeated
 *        up till 3 more times (default). If still not successful, returns NACK
** \param [in, out] slave - pointer to the I2C slave structure to init
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \param [in] rw - 0 = write, 1 = read operation
** \return I2C_STATUS status of write attempt
**/
static I2C_STATUS I2C_comm(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes, const I2C_RW rw)
{
	uint8_t	retry = i2c.cfg.retries;
	bool	ack = false;
	ci2c_fct_ptr fc = (ci2c_fct_ptr) (rw ? slave->cfg.rd : slave->cfg.wr);

	if (I2C_is_busy())	{ return slave->status = I2C_BUSY; }
	i2c.busy = true;

	ack = fc(slave, reg_addr, data, bytes);
	while ((!ack) && (retry != 0))	// If com not successful, retry some more times
	{
		delay(1);
		ack = fc(slave, reg_addr, data, bytes);
		retry--;
	}

	i2c.busy = false;
	return slave->status = ack ? I2C_OK : I2C_NACK;
}

/*!\brief This function writes the provided data to the address specified.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return I2C_STATUS status of write attempt
**/
I2C_STATUS I2C_write(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes) {
	return I2C_comm(slave, reg_addr, data, bytes, I2C_WRITE); }

/*!\brief This function reads data from the address specified and stores this
 *        data in the area provided by the pointer.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in, out] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return I2C_STATUS status of read attempt
**/
I2C_STATUS I2C_read(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes) {
	return I2C_comm(slave, reg_addr, data, bytes, I2C_READ); }


/*!\brief This procedure calls appropriate functions to perform a proper send transaction on I2C bus.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in] data - pointer to the first byte of a block of data to write
** \param [in] bytes - indicates how many bytes of data to write
** \return Boolean indicating success/fail of write attempt
**/
static bool I2C_wr(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)												{ return false; }

	Wire.beginTransmission(slave->cfg.addr);
	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if writing next
	{
		(void) I2C_slave_set_reg_addr(slave, reg_addr);

		if (slave->cfg.reg_size >= I2C_16B_REG)	// if size >2, 16bit address is used
		{
			if (Wire.write((uint8_t) (reg_addr >> 8)) == 0)		{ return false; }
		}
		if (Wire.write((uint8_t) reg_addr) == 0)				{ return false; }
	}

	for (uint16_t cnt = 0; cnt < bytes; cnt++)
	{
		if (Wire.write(*data++) == 0)							{ return false; }
		slave->reg_addr++;
	}

	if (Wire.endTransmission() != 0)							{ return false; }

	return true;
}


/*!\brief This procedure calls appropriate functions to perform a proper receive transaction on I2C bus.
** \param [in, out] slave - pointer to the I2C slave structure
** \param [in] reg_addr - register address in register map
** \param [in, out] data - pointer to the first byte of a block of data to read
** \param [in] bytes - indicates how many bytes of data to read
** \return Boolean indicating success/fail of read attempt
**/
static bool I2C_rd(I2C_SLAVE * slave, const uint16_t reg_addr, uint8_t * data, const uint16_t bytes)
{
	if (bytes == 0)													{ return false; }

	if ((slave->cfg.reg_size) && (reg_addr != slave->reg_addr))	// Don't send address if reading next
	{
		(void) I2C_slave_set_reg_addr(slave, reg_addr);

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
