/*
  
  ESPIcial firmware

  created 2023
  by Wavicle
  modified 12 Feb 2024
  by AndyMt

*/

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include "flashmem.h"

#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPmDNS.h>
#include "ESPIcialWebDAV.h"

#define HOSTNAME    "X16WebDAV"
#define AP_NAME     "X16Connect"
#define AP_PASSWORD "12345678"
#define RESET_FILE  "/RESETWIFI"

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

//---------------------------------------------------------------------
void switchToESP();

FS& gfs = SD;
//WiFiServerSecure tcp(443); // ToDo: try https
WiFiServer tcp(80);
ESPIcialWebDAV dav;
uint32_t startTimeoutSD = 0;

SPIClass* cfg_spi = NULL;
SPIClass* sd_spi = NULL;

static const int spiClk = 1000000; // 1 MHz
uint8_t fpga_configured = 0;
uint8_t last_fpga_creset = 0;

//---------------------------------------------------------------------
// configure and start SPI interface
//---------------------------------------------------------------------
void spi_begin() {
    pinMode(FPGA_CDONE, OUTPUT);
    digitalWrite(FPGA_CDONE, HIGH);

    pinMode(CFG_SPI_SCK, OUTPUT);
    pinMode(CFG_SPI_MISO, INPUT);
    pinMode(CFG_SPI_MOSI, OUTPUT);
    pinMode(CFG_SPI_SS, OUTPUT);
  
    cfg_spi->begin(CFG_SPI_SCK, CFG_SPI_MISO, CFG_SPI_MOSI, CFG_SPI_SS);
    digitalWrite(CFG_SPI_SS, HIGH);
}

//---------------------------------------------------------------------
// stop SPI interface
//---------------------------------------------------------------------
void spi_end() {
    cfg_spi->end();
    pinMode(FPGA_CDONE, INPUT);
    pinMode(CFG_SPI_SCK, INPUT);
    pinMode(CFG_SPI_MISO, INPUT);
    pinMode(CFG_SPI_MOSI, INPUT);
    pinMode(CFG_SPI_SS, INPUT);
}

//---------------------------------------------------------------------
// SD access macros
//---------------------------------------------------------------------
#define ENABLE_FPGA_CFG() do { \
      digitalWrite(FPGA_SD_EN, 0); \
      digitalWrite(FPGA_CFG_EN, 1); \
  } while(0)

#define ENABLE_FPGA_SD() do { \
      digitalWrite(FPGA_CFG_EN, 0); \
      digitalWrite(FPGA_SD_EN, 1); \
  } while(0)
#define DISABLE_FPGA_SD() do { \
      digitalWrite(FPGA_CFG_EN, 0); \
      digitalWrite(FPGA_SD_EN, 0); \
  } while(0)


//---------------------------------------------------------------------
// Setup Web DAV
//---------------------------------------------------------------------
void setupWebDav()
{
    // ------------------------
    Serial.println("");
    Serial.print("Connected to "); Serial.println(WiFi.SSID());
    Serial.print("IP address: "); Serial.println(WiFi.localIP());
    Serial.print("RSSI: "); Serial.println(WiFi.RSSI());
    Serial.print("Mode: "); Serial.println(WiFi.getMode());

    MDNS.begin(HOSTNAME);
    tcp.begin();

    dav.begin(&tcp, &gfs);
    dav.setTransferStatusCallback([](const char* name, int percent, bool receive)
    {
        startTimeoutSD = millis();
        Serial.printf("%s: '%s': %d%%\n", receive ? "recv" : "send", name, percent);
    });

    dav.setActivityCallback([](const char* name)
    {
      uint8_t isX16SD = digitalRead(FPGA_SD_EN);      
      startTimeoutSD = millis();
      if (isX16SD)
      {
        switchToESP();
      }
    });

    Serial.println("WebDAV server started");
}
//---------------------------------------------------------------------
void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.name(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

//---------------------------------------------------------------------
// Setup WiFi manager
//---------------------------------------------------------------------
void setupWiFi() {
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;
 
    DISABLE_FPGA_SD();
    switchToESP();
    // check if reset file exists. If yes, reset WiFi settings
    if (SD.exists(RESET_FILE))
    {
      Serial.printf("Reset file found! Reset WiFi Settings and activate connection AP.");
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(200);
      WiFi.mode(WIFI_STA);      // explicitly set mode, esp defaults to STA+AP

      wm.resetSettings();       // this triggers the connection AP
      SD.remove(RESET_FILE);    // remove the reset file
      switchToX16();
    }
 
    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result
 
    wm.setConfigPortalTimeout(60);
    wm.setCaptivePortalEnable(true);
    wm.setEnableConfigPortal(true);
    wm.setDarkMode(true);
    wm.setTitle("VERA ESPIcial");
    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    res = wm.autoConnect(AP_NAME); // anonymous, open ap
    //res = wm.autoConnect(AP_NAME, AP_PASSWORD); // password protected ap, doesn't work for some reason
 
    if(!res) {
        Serial.println("Failed to connect");
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
 
}

//---------------------------------------------------------------------
// the setup function runs once when you press reset or power the board
//---------------------------------------------------------------------
void setup() {

  // setup GPIO pins
  pinMode(FPGA_CDONE, INPUT);
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

  // prepare SD card access
  pinMode(SD_SPI_SCK, INPUT);
  pinMode(SD_SPI_MISO, INPUT);
  pinMode(SD_SPI_MOSI, INPUT);
  pinMode(SD_SPI_SS, INPUT);

  sd_spi = new SPIClass(FSPI);

  // make ESP access sd card first
  sd_spi->begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_SS);
  pinMode(sd_spi->pinSS(), OUTPUT);
  SD.begin(sd_spi->pinSS(),*sd_spi);

  Serial.begin(1000000);
  Serial.println("");
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(ESP_BUILTIN_LED, OUTPUT);

  // show reset is done
  ENABLE_FPGA_CFG();
  digitalWrite(ESP_FPGA_RESET, 0);
  delay(20);
  digitalWrite(ESP_FPGA_RESET, 1);
  last_fpga_creset = 1;
  fpga_configured = 0;

}

//---------------------------------------------------------------------
#define SPI_BEGIN_MAGIC 0x96
#define SPI_END_MAGIC 0x69
#define TRANSFER_MAGIC 0xAA
#define FPGA_RESET_MAGIC 0xF0

//---------------------------------------------------------------------
// variables for FPGA binary transfer
uint8_t header_bytes_read = 0;
uint8_t magic;
uint16_t transfer_size;
uint16_t bytes_transferred = 0;

//---------------------------------------------------------------------
// switches SD card access to ESP, start timeout to switch back to FPGA
//---------------------------------------------------------------------
void switchToESP()
{
  startTimeoutSD = millis();

  Serial.println("ESP in control of SD");
  digitalWrite(FPGA_SD_EN, LOW); 
  pinMode(sd_spi->pinSS(), OUTPUT);
  sd_spi->begin(SD_SPI_SCK, SD_SPI_MISO, SD_SPI_MOSI, SD_SPI_SS);
  SD.begin(sd_spi->pinSS(),*sd_spi);
}

//---------------------------------------------------------------------
// Switch SD card access to FPGA
//---------------------------------------------------------------------
void switchToX16()
{
  startTimeoutSD = 0;

  Serial.println("X16 in control of SD");
  pinMode(SD_SPI_SS, INPUT);
  digitalWrite(FPGA_SD_EN, HIGH); 
}

//---------------------------------------------------------------------
// main loop
//---------------------------------------------------------------------
void loop() 
{
  uint8_t isX16SD = digitalRead(FPGA_SD_EN); // who is in control of the SD card?

  if (fpga_configured)
  {
    dav.handleClient();

    // switch back SD card to FPGA when timeout is due
    if (startTimeoutSD
     && millis()-startTimeoutSD > 5000
     && !isX16SD)
      switchToX16();
  }
  
  uint8_t cur_fpga_creset = digitalRead(FPGA_CRESET_B);

  if (cur_fpga_creset == 0 && last_fpga_creset == 1)
  {
    Serial.println("ENABLE_FPGA_CFG()");
    // Reset was just asserted
    // Make sure that FPGA SPI bus is connected to config flash
    fpga_configured = 0;
    ENABLE_FPGA_CFG();
  } else if (cur_fpga_creset == 1 && fpga_configured == 0 && digitalRead(FPGA_CDONE) == 1) {
      // FPGA has finished configuration, switch SD card access to SD card
      fpga_configured = 1;

      // setup WiFi
      setupWiFi();

      // setup SebDAV access
      setupWebDav();

      // give sd card control to X16
      ENABLE_FPGA_SD();
      switchToX16();
      Serial.println("Init done");

  }

  //
  // serial management, for uploading FPGA binary
  //
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

  // make led blink
  digitalWrite(ESP_BUILTIN_LED, (millis() & 0x200) == 0 ? 0 : 1);
  last_fpga_creset = cur_fpga_creset;
}
