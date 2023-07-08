#include <Arduino.h>
#include <cstdint>
#include <SPI.h>

uint32_t simple_transaction(SPIClass* spi, uint8_t cmd, uint8_t res_size, uint32_t arg=0, uint8_t arg_size=0, uint32_t freq=1000000)
{
  uint32_t response = 0;
  
  spi->beginTransaction(SPISettings(freq, MSBFIRST, SPI_MODE3));
  digitalWrite(spi->pinSS(), LOW);

  spi->transfer(cmd);
  for(uint8_t i = 0; i < arg_size; i++) {
    spi->transfer((arg >> ((arg_size - i - 1) * 8)) & 0xFF);
  }

  for(uint8_t i = 0; i < res_size; i++) {
    response = (response << 8) | spi->transfer(0xff);
  }

  digitalWrite(spi->pinSS(), HIGH);
  spi->endTransaction();

  return response;
}

void reset_flash(SPIClass* spi)
{
  simple_transaction(spi, 0x99, 0);
}

uint32_t get_jedec_id(SPIClass* spi)
{
  uint32_t id;
  id = simple_transaction(spi, 0x9F, 3);

  return id;
}

uint8_t read_status_1(SPIClass* spi)
{
  uint32_t status_reg;
  status_reg = simple_transaction(spi, 0x05, 1);
  return (uint8_t)(status_reg & 0xFF);
}

uint8_t read_status_2(SPIClass* spi)
{
  uint32_t status_reg;
  status_reg = simple_transaction(spi, 0x35, 1);
  return (uint8_t)(status_reg & 0xFF);
}

uint8_t read_status_3(SPIClass* spi)
{
  uint32_t status_reg;
  status_reg = simple_transaction(spi, 0x15, 1);
  return (uint8_t)(status_reg & 0xFF);
}

void erase_sector(SPIClass* spi, uint32_t sector_addr)
{
  simple_transaction(spi, 0x20, 0, sector_addr, 3);
}
