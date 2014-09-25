#include <avr/io.h>
#include "i2cmaster.h"
#include "pcm5142.h"

void pcm5142_init() {
	uint8_t ret;
	// Enable DAC LDOs
	DDRE |= (1 << PB4);
	PORTE |= (1 << PB4);

	// Set XSMT of DAC to rising
	DDRE |= (1 << PB7);
	PORTE |= (1 << PB7);

	// Wait for DAC to start up
	nrk_spin_wait_us(100);

	// Initialize I2C
	i2c_init();

	// Select register page 0
	pcm5142_write_byte(PCM5142_REG_PAGE_SEL, 0x00);
	// Stop PLL
	pcm5142_write_byte(PCM5142_REG_PLL, 0x0);
	// Disable SCK detection, clock halt detection and auto clock mode
	pcm5142_write_byte(PCM5142_REG_IGNORE, 0x1A);
	// Set PLL reference to BCK
	pcm5142_write_byte(PCM5142_REG_PLL_REF, 0x10);
	// Set I2S mode to left justified and 16 bit
	pcm5142_write_byte(PCM5142_REG_I2S_CONF0, 0x30);

	// Set volume L to -10dB
	pcm5142_write_byte(PCM5142_REG_VOL_L, 0x64);
	// Set volume R to -10dB
	pcm5142_write_byte(PCM5142_REG_VOL_R, 0x64);
	//disable auto-mute
	pcm5142_write_byte(PCM5142_REG_AUTOMUTE, 0x00);

	// Set PLL P = 1
	pcm5142_write_byte(PCM5142_REG_PLL_P, 0x00);
	// Set PLL J = 16
	pcm5142_write_byte(PCM5142_REG_PLL_J, 0x10);
	// Set PLL R = 2
	pcm5142_write_byte(PCM5142_REG_PLL_R, 0x01);
	// Set PLL D0 = 0
	pcm5142_write_byte(PCM5142_REG_PLL_D0, 0x00);
	// Set PLL D1 = 0
	pcm5142_write_byte(PCM5142_REG_PLL_D1, 0x00);

	// Set fs double speed mode
	pcm5142_write_byte(PCM5142_REG_FS_SPEED, 0x01);
	// Set DSP clock divider to 2
	pcm5142_write_byte(PCM5142_REG_DSP_CLKD, 0x01);
	// Set DAC clock divider to 16
	pcm5142_write_byte(PCM5142_REG_DAC_CLKD, 0x0F);
	// Set CP clock divider to 4
	pcm5142_write_byte(PCM5142_REG_CP_CLKD, 0x03);
	// Set OSR clock divider to 4
	pcm5142_write_byte(PCM5142_REG_OSR_CLKD, 0x03);

	// Select register page 1
	pcm5142_write_byte(PCM5142_REG_PAGE_SEL, 0x01);
	// Set analog gain to -6dB
	pcm5142_write_byte(PCM5142_REG_AGAIN, 0x11);

	// Select register page 0
	pcm5142_write_byte(PCM5142_REG_PAGE_SEL, 0x00);
	// Start PLL
	pcm5142_write_byte(PCM5142_REG_PLL, 0x1);
}

uint8_t pcm5142_read_byte(uint8_t address, uint8_t *data) {
	uint8_t ret;

	// Set device address and write mode
	ret = i2c_start(I2C_SLA_PCM5142 + I2C_WRITE);
	if (ret) {
		i2c_stop();
		return ret;
	}

	// Set register to read from
	ret = i2c_write(address);
	if (ret) {
		i2c_stop();
		return ret;
	}

	// Set device address and read
	ret = i2c_start(I2C_SLA_PCM5142 + I2C_READ);
	if (ret) {
		i2c_stop();
		return ret;
	}

	*data = i2c_readNak(); // Receive register data
	i2c_stop();

	return ret;
}

uint8_t pcm5142_write_byte(uint8_t address, uint8_t data) {
	uint8_t ret;

	// Set device address and write mode
	ret = i2c_start(I2C_SLA_PCM5142 + I2C_WRITE);
	if (ret) {
		i2c_stop();
		return ret;
	}

	// Set register to write to
	ret = i2c_write(address);
	if (ret) {
		i2c_stop();
		return ret;
	}

	// Set device address and write
	ret = i2c_write(data + I2C_WRITE);
	if (ret) {
		i2c_stop();
		return ret;
	}

	i2c_stop();
	return ret;
}

