// File: LearnKNXAddress.ino  
// Author: Daniel Kleine-Albers (Since 2012) 
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Test constellation = ARDUINO MEGA <-> 5WG1 117-2AB12

#include <KnxTpUart.h>

// Initialize the KNX TP-UART library on the Serial1 port of ARDUINO MEGA
KnxTpUart knx(&Serial1, KNX_IA(15,15,20));

// Start in programming mode
boolean programmingMode = true;

void setup() {
  Serial.begin(115200);
  Serial.println("TP-UART Test");  

  Serial1.begin(19200, SERIAL_8E1); // Even parity

  knx.uartReset();
  knx.setListenToBroadcasts(true);
}


void loop() {
}

void serialEvent1() {
  KnxTpUartSerialEventType eType = knx.serialEvent();
  if (eType == KNX_TELEGRAM) {
    KnxTelegram* telegram = knx.getReceivedTelegram();

    if (telegram->isTargetGroup() && telegram->getCommand() == KNX_COMMAND_INDIVIDUAL_ADDR_WRITE && programmingMode) {
      // Broadcast to all devices in programming mode to store new physical address
      Serial.print("Received IndvAddrWrite: ");

      uint16_t address = telegram->getBufferByte(8);
      address = (address<<8) | telegram->getBufferByte(9);

      Serial.print((address >> 12) & 0x0F);
      Serial.print(".");
      Serial.print((address >> 8) & 0x0F);
      Serial.print(".");
      Serial.println(address & 0xFF);

      knx.setIndividualAddress(address);

      // Here the new address could be stored to EEPROM and be reloaded after restart of Arduino   
    } 
    else if (telegram->getCommand() == KNX_COMMAND_MASK_VERSION_READ) {
      // Request for mask version (version of bus interface
      knx.individualAnswerMaskVersion(telegram->getSourceAddress());
    } 
    else if (telegram->getCommand() == KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST && programmingMode) {
      // Broadcast request for individual addresses of all devices in programming mode
      knx.individualAnswerAddress(); 
    } 
    else if (telegram->getFirstDataByte() == KNX_EXT_COMMAND_AUTH_REQUEST && programmingMode) {
      // Authentication request to allow memory access
      knx.individualAnswerAuth(15, telegram->getSequenceNumber(), telegram->getSourceAddress());
    } 
    else if (telegram->getCommand() == KNX_COMMAND_RESTART && programmingMode) {
      // Restart the device -> end programming mode
      programmingMode = false;
      knx.setListenToBroadcasts(false);
      Serial.println("Received restart, ending programming mode"); 
    }
  }
}
