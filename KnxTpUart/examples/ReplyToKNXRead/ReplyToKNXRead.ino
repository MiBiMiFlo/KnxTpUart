// File: ReplyToKNXRead.ino   
// Author: Daniel Kleine-Albers (Since 2012) 
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Test constellation = ARDUINO MEGA <-> 5WG1 117-2AB12

#include <KnxTpUart.h>

// Initialize the KNX TP-UART library on the Serial1 port of ARDUINO MEGA
KnxTpUart knx(&Serial1, KNX_IA(1,1,15));

void setup() {
  Serial.begin(9600);
  Serial.println("TP-UART Test");

  Serial1.begin(19200, SERIAL_8E1);

  Serial.print("UCSR1A: ");
  Serial.println(UCSR1A, BIN);

  Serial.print("UCSR1B: ");
  Serial.println(UCSR1B, BIN);

  Serial.print("UCSR1C: ");
  Serial.println(UCSR1C, BIN);

  knx.uartReset();
  // Define group address to react on
  knx.setListenAddressCount(8);
  knx.addListenGroupAddress(KNX_GA(3,0,1));
  knx.addListenGroupAddress(KNX_GA(3,0,2));
  knx.addListenGroupAddress(KNX_GA(3,0,3));
  knx.addListenGroupAddress(KNX_GA(3,0,4));
  knx.addListenGroupAddress(KNX_GA(3,0,5));
  knx.addListenGroupAddress(KNX_GA(3,0,6));
  knx.addListenGroupAddress(KNX_GA(3,0,7));
  knx.addListenGroupAddress(KNX_GA(3,0,8));
}

void loop() {
}

void serialEvent1() {
  KnxTpUartSerialEventType eType = knx.serialEvent();
  if (eType == KNX_TELEGRAM) {
    KnxTelegram* telegram = knx.getReceivedTelegram();

    uint16_t target = telegram->getTargetGroupAddress();

    // Is it a read request?
    if (telegram->getCommand() == KNX_COMMAND_READ) {

      // Is the destination address equal to group address 3/0/1?
      if (target == KNX_GA(3,0,1)) {
        knx.groupAnswerBool(KNX_GA(3,0,1), true);
        // Display serial port : 1
      }
      // Is the destination address equal to group address 3/0/2?
      if (target == KNX_GA(3,0,2)) {
        knx.groupAnswer1ByteInt(KNX_GA(3,0,2), 126);
        // Display serial port : 49%
      }
      // Is the destination address equal to group address 3/0/3?
      if (target == KNX_GA(3,0,3)) {
        knx.groupAnswer2ByteInt(KNX_GA(3,0,3), 1000);
        // Display serial port : 1000
      }
      // Is the destination address equal to group address 3/0/4?
      if (target == KNX_GA(3,0,4)) {
        knx.groupAnswer2ByteFloat(KNX_GA(3,0,4), 25.28);
        // Display serial port : 25,28
      }
      // Is the destination address equal to group address 3/0/5?
      if (target == KNX_GA(3,0,5)) {
        knx.groupAnswer3ByteTime(KNX_GA(3,0,5), 7, 0, 0, 1);
        // Display serial port : Sonntag 00:00:01
      }
      // Is the destination address equal to group address 3/0/6?
      if (target == KNX_GA(3,0,6)) {
        knx.groupAnswer3ByteDate(KNX_GA(3,0,6), 31, 1, 3);
        // Display serial port : 31.01.2003
      }
      // Is the destination address equal to group address 3/0/7?
      if (target == KNX_GA(3,0,7)) {
        knx.groupAnswer4ByteFloat(KNX_GA(3,0,7), -100);
        // Display serial port : -100
      }
      // Is the destination address equal to group address 3/0/8?
      if (target == KNX_GA(3,0,8)) {
        knx.groupAnswer14ByteText(KNX_GA(3,0,8), "Hallo");
        // Display serial port : "Hallo"
      }
    }
  }
}
