/*
 * pcm5412.h
 *
 *  Created on: Oct 29, 2013
 *      Author: dermeister
 */

#ifndef PCM5412_H_
#define PCM5412_H_

/*
 * Compatibility defines.  This should work on ATmega8, ATmega16,
 * ATmega163, ATmega323 and ATmega128 (IOW: on all devices that
 * provide a builtin TWI interface).
 *
 * On the 128, it defaults to USART 1.
 */
#ifndef UCSRB
# ifdef UCSR1A		/* ATmega128 */
#  define UCSRA UCSR1A
#  define UCSRB UCSR1B
#  define UBRR UBRR1L
#  define UDR UDR1
# else /* ATmega8 */
#  define UCSRA USR
#  define UCSRB UCR
# endif
#endif
#ifndef UBRR
#  define UBRR UBRRL
#endif

#define I2C_ERROR 1
#define I2C_OK 0

// Address of PCM5142 is 1001100
#define I2C_SLA_PCM5142	0x98

#define PCM5412_AUTO_INCREMENT_DISABLE 0
#define PCM5412_AUTO_INCREMENT_ENABLE 0x80

// Register defines
// Page 0
#define PCM5142_REG_PAGE_SEL	0x00
#define PCM5142_REG_PLL			0x04
#define PCM5142_REG_PLL_REF		0x0D
#define PCM5142_REG_I2S_CONF0	0x28
#define PCM5142_REG_PLL_P		0x14
#define PCM5142_REG_PLL_J		0x15
#define PCM5142_REG_PLL_D0		0x16
#define PCM5142_REG_PLL_D1		0x17
#define PCM5142_REG_PLL_R		0x18
#define PCM5142_REG_FS_SPEED	0x22
#define PCM5142_REG_DSP_CLKD	0x1B
#define PCM5142_REG_DAC_CLKD	0x1C
#define PCM5142_REG_CP_CLKD		0x1D
#define PCM5142_REG_OSR_CLKD	0x1E
#define PCM5142_REG_IGNORE		0x25
#define PCM5142_REG_VOL_L		0x3D
#define PCM5142_REG_VOL_R		0x3E
#define PCM5142_REG_AUTOMUTE	0x41

// Page 1
#define PCM5142_REG_AGAIN		0x02

void pcm5142_init();
uint8_t pcm5142_read_byte(uint8_t address, uint8_t *data);
uint8_t pcm5142_write_byte(uint8_t address, uint8_t data);

#endif /* PCM5412_H_ */
