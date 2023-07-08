void reset_flash(SPIClass* spi);
uint32_t get_jedec_id(SPIClass* spi);
uint8_t read_status_1(SPIClass* spi);
uint8_t read_status_2(SPIClass* spi);
uint8_t read_status_3(SPIClass* spi);
