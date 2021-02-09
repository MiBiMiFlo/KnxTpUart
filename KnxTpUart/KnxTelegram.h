// File: KnxTelegram.h
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 2021-02-09 by MiBiMiFlo

#ifndef KnxTelegram_h
#define KnxTelegram_h

#include "Arduino.h"

/**
 * Helper that builds a uint16_t from a 3 part group address
 */
#define KNX_GA(aUpper, aMid, aLower) ( (((uint16_t)aUpper) & 0x1F) << 11 | (((uint16_t)aMid) & 0x07) << 8 | (aLower & 0xFF) )

/**
 * Helper that builds a uint16_t from a 3 part individual address
 */
#define KNX_IA(aArea, aLine, aMember) ( (((uint16_t)aArea) & 0x0F) << 12 | (((uint16_t)aLine) & 0x0F) << 8 | (aMember & 0xFF) )

/**
 * The maximum telegram size.
 */
#define MAX_KNX_TELEGRAM_SIZE 23

/**
 * The KNX telegram header size.
 */
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

    uint8_t getSourceArea();
    uint8_t getSourceLine();
    uint8_t getSourceMember();

    void setTargetAddress(uint16_t aAddress, bool aIsGroup);
    void setTargetGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);
    void setTargetGroupAddress(uint16_t aAddress);
    void setTargetIndividualAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);
    void setTargetIndividualAddress(uint16_t aAddress);
    bool isTargetGroup();

    uint16_t getTargetAddress();
    uint16_t getTargetGroupAddress();

    uint8_t getTargetMainGroup();
    uint8_t getTargetMiddleGroup();
    uint8_t getTargetSubGroup();

    uint8_t getTargetArea();
    uint8_t getTargetLine();
    uint8_t getTargetMember();

    void setRoutingCounter(uint8_t aCounter);
    uint8_t getRoutingCounter();
    void setCommand(KnxCommandType aCommand);
    KnxCommandType getCommand();

    void setFirstDataByte(uint8_t aData);
    uint8_t getFirstDataByte();
    bool getBool();

    uint8_t get4BitIntValue();
    bool get4BitDirectionValue();
    uint8_t get4BitStepsValue();

    void set1ByteIntValue(int8_t aValue);
    int8_t get1ByteIntValue();

    void set1ByteUIntValue(uint8_t aValue);
    uint8_t get1ByteUIntValue();

    void set2ByteIntValue(int16_t aValue);
    int16_t get2ByteIntValue();

    void set2ByteUIntValue(uint16_t aValue);
    uint16_t get2ByteUIntValue();

    void set4ByteIntValue(int32_t aValue);
    int32_t get4ByteIntValue();

    void set4ByteUIntValue(uint32_t aValue);
    uint32_t get4ByteUIntValue();

    void set2ByteFloatValue(float aValue);
    float get2ByteFloatValue();

    void set3ByteTime(uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second);
    uint8_t get3ByteWeekdayValue();
    uint8_t get3ByteHourValue();
    uint8_t get3ByteMinuteValue();
    uint8_t get3ByteSecondValue();
    void set3ByteDate(uint8_t day, uint8_t month, uint8_t year);
    uint8_t get3ByteDayValue();
    uint8_t get3ByteMonthValue();
    uint8_t get3ByteYearValue();

    void set4ByteFloatValue(float value);
    float get4ByteFloatValue();

    void set14ByteValue(String value);
    String get14ByteValue();

    /**
     * Receive the value into the given buffer.
     * @param aBuffer the buffer to store the value into
     * @param aSize the size of the buffer.
     * @return the number of bytes copied.
     */
    uint8_t getValue(uint8_t* aBuffer, uint8_t aSize);

    /**
     * Set the data value from given buffer
     */
    void setValue(uint8_t* aBuffer, uint8_t aSize);

    void createChecksum();

    bool verifyChecksum();

    uint8_t getChecksum();

    void print(TPUART_SERIAL_CLASS* aPort);
    uint8_t getTotalLength();
    KnxCommunicationType getCommunicationType();
    void setCommunicationType(KnxCommunicationType aType);
    uint8_t getSequenceNumber();
    void setSequenceNumber(uint8_t aSequence);
    KnxControlDataType getControlData();
    void setControlData(KnxControlDataType aType);
  private:
    uint8_t buffer[MAX_KNX_TELEGRAM_SIZE];
    uint8_t calculateChecksum();

};

#endif
