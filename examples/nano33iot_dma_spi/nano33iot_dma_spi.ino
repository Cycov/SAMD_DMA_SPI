/**
 * @file nano33iot_dma_spi.ino
 * @author Circa Dragos (dragos@okoeng.com)
 * @brief An example that was tested and works with the Nano 33 IOT board
 * @version 1.0
 * @date 2022-08-22
 * @remark To enable each test uncomment the needed tests from lines 22 through 26
 * 
 * @copyright Copyright (c) 2022
 * 
 * DMA memory to memory   ZERO
 * ch 18   beat burst block
 * xdk sam0/drivers/dma/dma.c
 * packages/arduino/tools/CMSIS/4.0.0-atmel/Device/ATMEL/samd21/include/component/dmac.h
 * http://asf.atmel.com/docs/3.16.0/samd21/html/asfdoc_sam0_sercom_spi_dma_use_case.html
 * assume normal SPI setup, then we take over with DMA
 * 
 */

#include <SAMD_DMA_SPI.h>

// #define CALCULATE_TIME

#define TEST_WRITE
// #define TEST_READ
// #define TEST_TRANSFER
// #define TEST_CLASSIC

#define SPI_SPEED 16000000
#define SERCOM_USED SERCOM1

#define GPIO_PA21_Set() (PORT->Group[0].OUTSET.reg = ((uint32_t)1U << 21U))
#define GPIO_PA21_Clear() (PORT->Group[0].OUTCLR.reg = ((uint32_t)1U << 21U))
#define GPIO_PA21_Toggle() (PORT->Group[0].OUTTGL.reg = ((uint32_t)1U << 21U))
#define GPIO_PA21_OutputEnable() (PORT->Group[0].DIRSET.reg = ((uint32_t)1U << 21U))
#define GPIO_PA21_InputEnable() (PORT->Group[0].DIRCLR.reg = ((uint32_t)1U << 21U))
#define GPIO_PA21_Get() (((PORT->Group[0].IN.reg >> 21U)) & 0x01U)
#define GPIO_PA21_PIN PORT_PIN_PA21

#define PRREG(x)          \
  Serial.print(#x " 0x"); \
  Serial.println(x, HEX)

#define BYTES 1024
char txbuf[BYTES], rxbuf[BYTES];

void prmbs(char *lbl, unsigned long us, int bits)
{
  float mbs = (float)bits / us;
  Serial.print(mbs, 2);
  Serial.print(" mbs  ");
  Serial.print(us);
  Serial.print(" us   ");
  Serial.println(lbl);
}

samd_dma_spi samd_dma_spi_instance(SPI_SPEED, SERCOM_USED);

void DMAC_Handler()
{
  samd_dma_spi_instance.dmac_irq_handler();
}

void setup()
{
  Serial.begin(9600);
  delay(1000);
  samd_dma_spi_instance.begin();
  for (size_t i = 0; i < BYTES; i++)
    txbuf[i] = i;

  GPIO_PA21_OutputEnable();
  GPIO_PA21_Clear();
}

int i, errs = 0;
unsigned long t1;
void loop()
{
#ifdef TEST_WRITE
#ifdef CALCULATE_TIME

  t1 = micros();
#endif /* CALCULATE_TIME */

  samd_dma_spi_instance.write(txbuf, BYTES);

#ifdef CALCULATE_TIME
  t1 = micros() - t1;
  prmbs("DMA write", t1, BYTES * 8);
#endif /* CALCULATE_TIME */

#endif /* TEST_WRITE */

#ifdef TEST_READ
#ifdef CALCULATE_TIME
  t1 = micros();
#endif /* CALCULATE_TIME */

  samd_dma_spi_instance.read(rxbuf, BYTES);

#ifdef CALCULATE_TIME
  t1 = micros() - t1;
  prmbs("DMA read", t1, BYTES * 8);
#endif /* CALCULATE_TIME */
#endif /* TEST_READ */

#ifdef TEST_TRANSFER
#ifdef CALCULATE_TIME
  t1 = micros();
#endif /* CALCULATE_TIME */

  samd_dma_spi_instance.transfer(txbuf, rxbuf, BYTES);
#ifdef CALCULATE_TIME
  t1 = micros() - t1;
  prmbs("DMA read/write", t1, BYTES * 8);
#endif /* CALCULATE_TIME */
#endif /* TEST_TRANSFER */

#ifdef TEST_CLASSIC
// Classic SPI transfer, for reference
#ifdef CALCULATE_TIME
  t1 = micros();
#endif /* CALCULATE_TIME */

  for (i = 0; i < BYTES; i++)
    SPI.transfer(txbuf[i]);

#ifdef CALCULATE_TIME
  t1 = micros() - t1;
  prmbs("SPI transfer", t1, BYTES * 8);
#endif /* CALCULATE_TIME */
#endif /* TEST_CLASSIC */

  GPIO_PA21_Set();
  GPIO_PA21_Clear();
}
