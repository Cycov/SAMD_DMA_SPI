#include "SAMD_DMA_SPI.h"

static uint8_t rxsink[1], txsrc[1] = {0xff};
static uint32_t chnltx = 0, chnlrx = 1; // DMA channels

void samd_dma_spi::begin()
{
    SPI.begin();
    SPI.beginTransaction(SPISettings(this->spi_speed, MSBFIRST, SPI_MODE0));

    // probably on by default
    PM->AHBMASK.reg |= PM_AHBMASK_DMAC;
    PM->APBBMASK.reg |= PM_APBBMASK_DMAC;
    NVIC_EnableIRQ(DMAC_IRQn);

    DMAC->BASEADDR.reg = (uint32_t)descriptor_section;
    DMAC->WRBADDR.reg = (uint32_t)wrb;
    DMAC->CTRL.reg = DMAC_CTRL_DMAENABLE | DMAC_CTRL_LVLEN(0xf);
}

void samd_dma_spi::write(void *data, size_t buffer_len, uint16_t ss_pin) {
    digitalWrite(ss_pin, LOW);
    this->write(data, buffer_len);
    digitalWrite(ss_pin, HIGH);
}

void samd_dma_spi::write(void *data, size_t buffer_len)
{
    this->xtype = this->DoTX;
    this->spi_xfr(data, rxsink, buffer_len);
}

void samd_dma_spi::read(void *data, size_t buffer_len, uint16_t ss_pin) {
    digitalWrite(ss_pin, LOW);
    this->read(data, buffer_len);
    digitalWrite(ss_pin, HIGH);
}

void samd_dma_spi::read(void *data, size_t buffer_len)
{
    this->xtype = this->DoRX;
    this->spi_xfr(txsrc, data, buffer_len);
}

void samd_dma_spi::transfer(void *txdata, void *rxdata, size_t buffer_len, uint16_t ss_pin) {
    digitalWrite(ss_pin, LOW);
    this->transfer(txdata, rxdata, buffer_len);
    digitalWrite(ss_pin, HIGH);
}

void samd_dma_spi::transfer(void *txdata, void *rxdata, size_t buffer_len)
{
    this->xtype = this->DoTXRX;
    this->spi_xfr(txdata, rxdata, buffer_len);
}

void samd_dma_spi::spi_xfr(void *txdata, void *rxdata, size_t buffer_len)
{
    uint32_t temp_CHCTRLB_reg;

    // set up transmit channel
    DMAC->CHID.reg = DMAC_CHID_ID(chnltx);
    DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << chnltx));
    temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) |
                       DMAC_CHCTRLB_TRIGSRC(SERCOM1_DMAC_ID_TX) | DMAC_CHCTRLB_TRIGACT_BEAT;
    DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK; // enable all 3 interrupts
    descriptor.descaddr = 0;
    descriptor.dstaddr = (uint32_t)&sercom->SPI.DATA.reg;
    descriptor.btcnt = buffer_len;
    descriptor.srcaddr = (uint32_t)txdata;
    descriptor.btctrl = DMAC_BTCTRL_VALID;
    if (xtype != DoRX)
    {
        descriptor.srcaddr += buffer_len;
        descriptor.btctrl |= DMAC_BTCTRL_SRCINC;
    }
    memcpy(&descriptor_section[chnltx], &descriptor, sizeof(dmacdescriptor));

    // rx channel    enable interrupts
    DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
    DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->CHCTRLA.reg = DMAC_CHCTRLA_SWRST;
    DMAC->SWTRIGCTRL.reg &= (uint32_t)(~(1 << chnlrx));
    temp_CHCTRLB_reg = DMAC_CHCTRLB_LVL(0) |
                       DMAC_CHCTRLB_TRIGSRC(SERCOM1_DMAC_ID_RX) | DMAC_CHCTRLB_TRIGACT_BEAT;
    DMAC->CHCTRLB.reg = temp_CHCTRLB_reg;
    DMAC->CHINTENSET.reg = DMAC_CHINTENSET_MASK; // enable all 3 interrupts
    dmadone = 0;
    descriptor.descaddr = 0;
    descriptor.srcaddr = (uint32_t)&sercom->SPI.DATA.reg;
    descriptor.btcnt = buffer_len;
    descriptor.dstaddr = (uint32_t)rxdata;
    descriptor.btctrl = DMAC_BTCTRL_VALID;
    if (xtype != DoTX)
    {
        descriptor.dstaddr += buffer_len;
        descriptor.btctrl |= DMAC_BTCTRL_DSTINC;
    }
    memcpy(&descriptor_section[chnlrx], &descriptor, sizeof(dmacdescriptor));

    // start both channels  ? order matter ?
    DMAC->CHID.reg = DMAC_CHID_ID(chnltx);
    DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;
    DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
    DMAC->CHCTRLA.reg |= DMAC_CHCTRLA_ENABLE;

    while (!dmadone)
        ; // await DMA done isr

    DMAC->CHID.reg = DMAC_CHID_ID(chnltx); // disable DMA to allow lib SPI
    DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
    DMAC->CHID.reg = DMAC_CHID_ID(chnlrx);
    DMAC->CHCTRLA.reg &= ~DMAC_CHCTRLA_ENABLE;
}

void samd_dma_spi::dmac_irq_handler()
{
    // interrupts DMAC_CHINTENCLR_TERR DMAC_CHINTENCLR_TCMPL DMAC_CHINTENCLR_SUSP
    uint8_t active_channel;

    // disable irqs ?
    __disable_irq();
    active_channel = DMAC->INTPEND.reg & DMAC_INTPEND_ID_Msk; // get channel number
    DMAC->CHID.reg = DMAC_CHID_ID(active_channel);
    this->dmadone = DMAC->CHINTFLAG.reg;
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TCMPL; // clear
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_TERR;
    DMAC->CHINTFLAG.reg = DMAC_CHINTENCLR_SUSP;
    __enable_irq();
}

samd_dma_spi::samd_dma_spi(uint32_t speed, Sercom *sercom)
{
    this->spi_speed = speed;
    this->sercom = sercom;
}

samd_dma_spi::~samd_dma_spi()
{
}