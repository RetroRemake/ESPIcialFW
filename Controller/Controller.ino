/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/Blink
*/

#include <SPI.h>
#include "flashmem.h"

#define ESP_BUILTIN_LED 2

#define CFG_SPI_SCK 36
#define CFG_SPI_MISO 35
#define CFG_SPI_MOSI 37
#define CFG_SPI_SS 38

#define BOOT0 0
#define BUS_IRQ 3
#define SCL 1
#define SDA 8

#define SD_SPI_SS 9
#define SD_SPI_MOSI 10
#define SD_SPI_SCK 11
#define SD_SPI_MISO 12

#define FPGA_CFG_EN 13
#define FPGA_CDONE 14

#define SPI_BUS_EN 23
#define SPI_BUS_CS 45
#define FPGA_SD_EN 46
#define FPGA_CRESET_B 47
#define ESP_FPGA_RESET 48

SPIClass* cfg_spi = NULL;
SPIClass* sd_spi = NULL;

static const int spiClk = 1000000; // 1 MHz
uint8_t fpga_configured = 0;
uint8_t last_fpga_creset = 0;

void spi_begin() {
    pinMode(CFG_SPI_SCK, OUTPUT);
    pinMode(CFG_SPI_MISO, INPUT);
    pinMode(CFG_SPI_MOSI, OUTPUT);
    pinMode(CFG_SPI_SS, OUTPUT);
  
    cfg_spi->begin(CFG_SPI_SCK, CFG_SPI_MISO, CFG_SPI_MOSI, CFG_SPI_SS);
    digitalWrite(CFG_SPI_SS, HIGH);
}

void spi_end() {
    cfg_spi->end();
    pinMode(CFG_SPI_SCK, INPUT);
    pinMode(CFG_SPI_MISO, INPUT);
    pinMode(CFG_SPI_MOSI, INPUT);
    pinMode(CFG_SPI_SS, INPUT);
    // Send everything to high-Z
    //digitalWrite(CFG_SPI_SCK, HIGH);
    //digitalWrite(CFG_SPI_MISO, HIGH);
    //digitalWrite(CFG_SPI_MOSI, HIGH);
    //digitalWrite(CFG_SPI_SS, HIGH);
}

#define ENABLE_FPGA_CFG() do { \
      digitalWrite(FPGA_SD_EN, 0); \
      digitalWrite(FPGA_CFG_EN, 1); \
  } while(0)

#define ENABLE_FPGA_SD() do { \
      digitalWrite(FPGA_CFG_EN, 0); \
      digitalWrite(FPGA_SD_EN, 1); \
  } while(0)

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(FPGA_CRESET_B, INPUT);
  pinMode(ESP_FPGA_RESET, OUTPUT);
  digitalWrite(ESP_FPGA_RESET, 0);
  
  pinMode(FPGA_CFG_EN, OUTPUT);
  digitalWrite(FPGA_CFG_EN, 0);
  
  pinMode(FPGA_SD_EN, OUTPUT);
  digitalWrite(FPGA_SD_EN, 0);
  
  pinMode(SPI_BUS_EN, OUTPUT);
  digitalWrite(SPI_BUS_EN, 0);

  cfg_spi = new SPIClass(HSPI);
  spi_end();

  pinMode(SD_SPI_SCK, INPUT);
  pinMode(SD_SPI_MISO, INPUT);
  pinMode(SD_SPI_MOSI, INPUT);
  pinMode(SD_SPI_SS, INPUT);


  sd_spi = new SPIClass(FSPI);

  //sd_spi->begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_SS);
  
  //pinMode(sd_spi->pinSS(), OUTPUT);

  Serial.begin(115200);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(ESP_BUILTIN_LED, OUTPUT);

  ENABLE_FPGA_CFG();
  digitalWrite(ESP_FPGA_RESET, 0);
  delay(20);
  digitalWrite(ESP_FPGA_RESET, 1);
  last_fpga_creset = 1;
  fpga_configured = 0;
}

#define SPI_BEGIN_MAGIC 0x96
#define SPI_END_MAGIC 0x69
#define TRANSFER_MAGIC 0xAA
#define FPGA_RESET_MAGIC 0xF0

uint8_t header_bytes_read = 0;

uint8_t magic;
uint16_t transfer_size;

uint16_t bytes_transferred = 0;

void loop() {
  uint8_t cur_fpga_creset = digitalRead(FPGA_CRESET_B);

  if (cur_fpga_creset == 0 && last_fpga_creset == 1)
  {
    // Reset was just asserted
    // Make sure that FPGA SPI bus is connected to config flash
    fpga_configured = 0;
    ENABLE_FPGA_CFG();
  } else if (cur_fpga_creset == 1 && fpga_configured == 0 && digitalRead(FPGA_CDONE) == 1) {
      // FPGA has finished configuration
      fpga_configured = 1;
      ENABLE_FPGA_SD();
  }

  while (Serial.available() > 0)
  {
    char serNext = Serial.read();
    
    if (header_bytes_read == 0 && serNext == FPGA_RESET_MAGIC) {
      digitalWrite(FPGA_CFG_EN, 1); // Enable communication between FPGA and Flash
      digitalWrite(ESP_FPGA_RESET, 0);
      digitalWrite(ESP_BUILTIN_LED, 1);
      delay(250);
      digitalWrite(ESP_BUILTIN_LED, 0);
      digitalWrite(ESP_FPGA_RESET, 1);
    } else if (header_bytes_read == 0 && serNext == SPI_BEGIN_MAGIC) {
      digitalWrite(ESP_BUILTIN_LED, 1);
      digitalWrite(FPGA_CFG_EN, 0); // Disable communication between FPGA and Flash
      spi_begin();
    } else if (header_bytes_read == 0 && serNext == SPI_END_MAGIC) {
      digitalWrite(ESP_BUILTIN_LED, 0);
      // Release SPI bus
      spi_end();
    } else if (header_bytes_read == 0 && serNext == TRANSFER_MAGIC) {
      magic = serNext;
      header_bytes_read++;
    } else if (header_bytes_read > 0 && header_bytes_read < 3) {
      transfer_size |= serNext << ((2 - header_bytes_read)*8);
      header_bytes_read++;

      if (header_bytes_read == 3) {
        cfg_spi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
        digitalWrite(cfg_spi->pinSS(), LOW);
        bytes_transferred = 0;
      }
    } else if (bytes_transferred < transfer_size ) {
      uint8_t r = cfg_spi->transfer(serNext);
      Serial.write(r);
      bytes_transferred++;

      if (bytes_transferred == transfer_size) {
        digitalWrite(cfg_spi->pinSS(), HIGH);
        cfg_spi->endTransaction();
        header_bytes_read = 0;
        magic = 0;
        transfer_size = 0;
        bytes_transferred = 0;
      }
    }
  }
  digitalWrite(ESP_BUILTIN_LED, (millis() & 0x200) == 0 ? 0 : 1);
  last_fpga_creset = cur_fpga_creset;
}
