#ifndef SAMD_DMA_SPI
#define SAMD_DMA_SPI

#include <stdint.h>
#include <SPI.h>

class samd_dma_spi
{
protected:
    uint32_t spi_speed;
    volatile uint32_t dmadone;
    Sercom *sercom; 

    void spi_xfr(void *txdata, void *rxdata, size_t n);

    // DMA   12 channels
    typedef struct
    {
        uint16_t btctrl;
        uint16_t btcnt;
        uint32_t srcaddr;
        uint32_t dstaddr;
        uint32_t descaddr;
    } dmacdescriptor;
    volatile dmacdescriptor wrb[12] __attribute__((aligned(16)));
    dmacdescriptor descriptor_section[12] __attribute__((aligned(16)));
    dmacdescriptor descriptor __attribute__((aligned(16)));

    enum XfrType
    {
        DoTX,
        DoRX,
        DoTXRX
    };
    XfrType xtype;

public:
    samd_dma_spi(uint32_t speed = 16000000, Sercom *sercom = SERCOM1);
    ~samd_dma_spi();
    void begin();
    void write(void *data, size_t buffer_len);
    void write(void *data, size_t buffer_len, uint16_t ss_pin);
    void read(void *data, size_t buffer_len);
    void read(void *data, size_t buffer_len, uint16_t ss_pin);
    void transfer(void *tx, void *rx, size_t buffer_len);
    void transfer(void *tx, void *rx, size_t buffer_len, uint16_t ss_pin);
    void dmac_irq_handler();
};

#endif /* SAMD_DMA_SPI */
