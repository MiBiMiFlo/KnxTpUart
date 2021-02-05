// File: KnxTelegram.h
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 06.06.2017

#ifndef KnxTelegram_h
#define KnxTelegram_h

#include "Arduino.h"

#define KNX_ADDR(aUpper, aMid, aLower) ( (((uint16_t)aUpper) & 0x0F) << 12 | (((uint16_t)aMid) & 0x0F) << 8 | (aLower & 0xFF) )

#define MAX_KNX_TELEGRAM_SIZE 23
#define KNX_TELEGRAM_HEADER_SIZE 6

#define TPUART_SERIAL_CLASS Stream

// KNX priorities
enum KnxPriorityType {
  KNX_PRIORITY_SYSTEM = B00,
  KNX_PRIORITY_ALARM = B10,
  KNX_PRIORITY_HIGH = B01,
  KNX_PRIORITY_NORMAL = B11
};

// KNX commands / APCI Coding
enum KnxCommandType {
  KNX_COMMAND_READ = B0000,
  KNX_COMMAND_WRITE = B0010,
  KNX_COMMAND_ANSWER = B0001,
  KNX_COMMAND_INDIVIDUAL_ADDR_WRITE = B0011,
  KNX_COMMAND_INDIVIDUAL_ADDR_REQUEST = B0100,
  KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE = B0101,
  KNX_COMMAND_MASK_VERSION_READ = B1100,
  KNX_COMMAND_MASK_VERSION_RESPONSE = B1101,
  KNX_COMMAND_RESTART = B1110,
  KNX_COMMAND_ESCAPE = B1111
};

// Extended (escaped) KNX commands
enum KnxExtendedCommandType {
  KNX_EXT_COMMAND_AUTH_REQUEST = B010001,
  KNX_EXT_COMMAND_AUTH_RESPONSE = B010010
};

// KNX Transport Layer Communication Type
enum KnxCommunicationType {
  KNX_COMM_UDP = B00, // Unnumbered Data Packet
  KNX_COMM_NDP = B01, // Numbered Data Packet
  KNX_COMM_UCD = B10, // Unnumbered Control Data
  KNX_COMM_NCD = B11  // Numbered Control Data
};

// KNX Control Data (for UCD / NCD packets)
enum KnxControlDataType {
  KNX_CONTROLDATA_CONNECT = B00,      // UCD
  KNX_CONTROLDATA_DISCONNECT = B01,   // UCD
  KNX_CONTROLDATA_POS_CONFIRM = B10,  // NCD
  KNX_CONTROLDATA_NEG_CONFIRM = B11   // NCD
};

class KnxTelegram {
  public:
    KnxTelegram();

    void clear();
    void setBufferByte(uint8_t aIndex, uint8_t aContent);
    uint8_t getBufferByte(uint8_t aIndex);
    void setPayloadLength(uint8_t aSize);
    int getPayloadLength();
    void setRepeated(bool aRepeat);
    bool isRepeated();
    void setPriority(KnxPriorityType aPrio);
    KnxPriorityType getPriority();
    void setSourceAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);
    void setSourceAddress(uint16_t aAddress);
    uint16_t getSourceAddress();
    int getSourceArea();
    int getSourceLine();
    int getSourceMember();
    void setTargetAddress(uint16_t aAddress, bool aIsGroup);
    void setTargetGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);
    void setTargetGroupAddress(uint16_t aAddress);
    void setTargetIndividualAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);
    void setTargetIndividualAddress(uint16_t aAddress);
    bool isTargetGroup();
    int getTargetMainGroup();
    int getTargetMiddleGroup();
    int getTargetSubGroup();
    uint16_t getTargetAddress();
    uint16_t getTargetGroupAddress();
    int getTargetArea();
    int getTargetLine();
    int getTargetMember();
    void setRoutingCounter(uint8_t aCounter);
    int getRoutingCounter();
    void setCommand(KnxCommandType aCommand);
    KnxCommandType getCommand();

    void setFirstDataByte(uint8_t aData);
    uint8_t getFirstDataByte();
    bool getBool();

    int8_t get4BitIntValue();
    bool get4BitDirectionValue();
    byte get4BitStepsValue();

    void set1ByteIntValue(int aValue);
    int get1ByteIntValue();

    void set2ByteIntValue(int aValue);
    int get2ByteIntValue();
    void set2ByteFloatValue(float aValue);
    float get2ByteFloatValue();

    void set3ByteTime(uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second);
    int get3ByteWeekdayValue();
    int get3ByteHourValue();
    int get3ByteMinuteValue();
    int get3ByteSecondValue();
    void set3ByteDate(uint8_t day, uint8_t month, uint8_t year);
    int get3ByteDayValue();
    int get3ByteMonthValue();
    int get3ByteYearValue();

    void set4ByteFloatValue(float value);
    float get4ByteFloatValue();

    void set14ByteValue(String value);
    String get14ByteValue();

    void createChecksum();
    bool verifyChecksum();
    int getChecksum();
    void print(TPUART_SERIAL_CLASS* aPort);
    int getTotalLength();
    KnxCommunicationType getCommunicationType();
    void setCommunicationType(KnxCommunicationType aType);
    int getSequenceNumber();
    void setSequenceNumber(uint8_t aSequence);
    KnxControlDataType getControlData();
    void setControlData(KnxControlDataType aType);
  private:
    byte buffer[MAX_KNX_TELEGRAM_SIZE];
    int calculateChecksum();

};

#endif
