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
 * A KNX Group address is a 16 bit number separated into 3 fields.
 * The field order is required to match internal bit order in ATmega328P and similar chips.
 */
struct _KNX_GROUP_ADDR
{
	/**
	 * 1 byte sub group (0-255)
	 */
	uint8_t sub;

	/**
	 * 3 bit middle group (0-7)
	 */
	uint8_t mid :3;

	/**
	 * 5 bit main group (0-31)
	 */
	uint8_t main :5;
};

/**
 * Union of the _KNX_GROUP_ADDR struct and a 16bit unsigned int for more convenient access.
 */
union KNX_GROUP_ADDR
{
	uint16_t mAddress;
	_KNX_GROUP_ADDR mGA;
};

/**
 * A KNX individual address is a 16 bit number separated into 3 fields.
 * The field order is required to match internal bit order in ATmega328P and similar chips.
 */
struct _KNX_INDIVIDUAL_ADDR
{
	/**
	 * 1 byte member (0-255)
	 */
	uint8_t member;

	/**
	 * 4 bit line (0-15)
	 */
	uint8_t line :4;
	/**
	 * 4 bit Area (0-15)
	 */
	uint8_t area :4;
};

union KNX_INDIVIDUAL_ADDR
{
	uint16_t mAddress;
	_KNX_INDIVIDUAL_ADDR mIA;
};


/**
 * A union to merge all possible KNX addresses.
 */
union KNX_ADDR
{
	uint16_t mAddress;
	_KNX_GROUP_ADDR mGA;
	_KNX_INDIVIDUAL_ADDR mIA;
};


/**
 * Helper macro that builds a uint16_t from a 3 part group address.
 * @param aMain the 5 bit main group.  (0-31)
 * @param aMid the 3 bit middle group. (0-7)
 * @param aSub the 8 bit sub group.    (0-255)
 * @return a uint16_t representing the KNX group address.
 */
#define KNX_GA(aMain, aMid, aSub) ( (((uint16_t)aMain) & 0x1F) << 11 | (((uint16_t)aMid) & 0x07) << 8 | ((uint8_t)aSub & 0xFF) )

/**
 * Helper macro that builds a uint16_t from a 3 part individual address.
 * @param aArea the 4 bit area.     (0-15)
 * @param aLine the 4 bit line.     (0-15)
 * @param aMember the 8 bit member. (0-255)
 * @return a uint16_t representing the KNX individual address.
 */
#define KNX_IA(aArea, aLine, aMember) ( (((uint16_t)aArea) & 0x0F) << 12 | (((uint16_t)aLine) & 0x0F) << 8 | ((uint8_t)aMember & 0xFF) )

/**
 * The maximum telegram size.
 * This should never be changed!
 */
#define MAX_KNX_TELEGRAM_SIZE 23

/**
 * The KNX telegram header size.
 * This should never be changed!
 */
#define KNX_TELEGRAM_HEADER_SIZE 6


/**
 * KNX priorities
 */
enum KnxPriorityType {
  KNX_PRIORITY_SYSTEM = B00,
  KNX_PRIORITY_ALARM = B10,
  KNX_PRIORITY_HIGH = B01,
  KNX_PRIORITY_NORMAL = B11
};

/**
 * KNX commands / APCI Coding
 */
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

/**
 * KNX Transport Layer Communication Type
 */
enum KnxCommunicationType {
  KNX_COMM_UDP = B00, // Unnumbered Data Packet
  KNX_COMM_NDP = B01, // Numbered Data Packet
  KNX_COMM_UCD = B10, // Unnumbered Control Data
  KNX_COMM_NCD = B11  // Numbered Control Data
};

/**
 * KNX Control Data (for UCD / NCD packets)
 */
enum KnxControlDataType {
  KNX_CONTROLDATA_CONNECT = B00,      // UCD
  KNX_CONTROLDATA_DISCONNECT = B01,   // UCD
  KNX_CONTROLDATA_POS_CONFIRM = B10,  // NCD
  KNX_CONTROLDATA_NEG_CONFIRM = B11   // NCD
};

/**
 * A KNX telegram is a single message that will either be send or was received.
 */
class KnxTelegram {
  public:

	/**
	 * Create a new (empty) instance.
	 * This constructor calls #clean() to ensure the buffer is cleared and correctly initialized.
	 */
    KnxTelegram();

    /**
     * Clear and initialize the buffer.
     */
    void clear();

    /**
     * Update the value of a single byte in the buffer.
     * This method does not perform an array bound check!
     * @param aIndex the offset in buffer to change.
     * @param aValue the value to set the buffer to.
     */
    void setBufferByte(uint8_t aIndex, uint8_t aValue);

    /**
     * Retrieve the current value of the buffer at a given offset.
     * This method does not perform an array bound check!
     * @param aIndex the offset in buffer to retrieve.
     * @return the actual value of the buffer at the given offset.
     */
    uint8_t getBufferByte(uint8_t aIndex);

    /**
     * Return a pointer to the internal buffer.
     * The buffer is of length MAX_KNX_TELEGRAM_SIZE what is 23 (7+16)
     * @return a pointer to the internal telegram buffer.
     */
    uint8_t * getBuffer();

    /**
     * Set the payload length. This affects the payload length field in the buffer as well as the number of bytes being transfered if the telegram is send.
     * @param aLength the payload length. Valid values are 2 to 16.
     */
    void setPayloadLength(uint8_t aLength);

    /**
     * @return the payload length as defined in buffer.
     */
    uint8_t getPayloadLength();

    /**
     * Set or clear the repeat flag in buffer.
     * @param aRepeat if true the repeat flag will be set, otherwise cleared.
     */
    void setRepeated(bool aRepeat);

    /**
     * @return true if the telegram repeat flag is set, false otherwise.
     */
    bool isRepeated();

    /**
     * Set the telegram priority.
     * @param aPrio the new priority.
     */
    void setPriority(KnxPriorityType aPrio);

    /**
     * @return the priority as defined in the buffer.
     */
    KnxPriorityType getPriority();

    /**
     * Set the source address for this telegram by providing three separate values.
     * @param aArea the area.
     * @param aLine the line.
     * @param aMember the member.
     */
    void setSourceAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);

    /**
     * Set the source address for this telegram by providing the address as 16bit integer.
     * @param aAddress the source address.
     */
    void setSourceAddress(uint16_t aAddress);

    /**
     * @return the source address as 16bit integer.
     */
    uint16_t getSourceAddress();

    /**
     * @return the area part of the source address.
     */
    uint8_t getSourceArea();

    /**
     * @return the line part of the source address.
     */
    uint8_t getSourceLine();

    /**
     * @return the member part of the source address.
     */
    uint8_t getSourceMember();

    /**
     * Set the target address for the telegram.
     * @param aAddress the address to set. This can either be an individual address or a group address.
     * @param aIsGroup a flag to indicate if the target is a group address or an individual address.
     */
    void setTargetAddress(uint16_t aAddress, bool aIsGroup);

    /**
     * Set the target to be the given group address.
     * @param aMain the main group of the address.
     * @param aMiddle the middle group of the address.
     * @param aSub the sub group of the address.
     */
    void setTargetGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);

    /**
     * Set the target to be the given group address.
     */
    void setTargetGroupAddress(uint16_t aAddress);

    /**
     * Set the target to be the given individual address.
     * @param aArea the area part of the individual address.
     * @param aLine the line part of the individual address.
     * @param aMember the member part of the individual address.
     */
    void setTargetIndividualAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);

    /**
     * Set the target to be the given individual address.
     * @param aAddress the individual address.
     */
    void setTargetIndividualAddress(uint16_t aAddress);

    /**
     * @return true if the target address is a group address, false otherwise.
     */
    bool isTargetGroup();

    /**
     * @return the target address of the telegram.
     */
    uint16_t getTargetAddress();

    /**
     * @return the target group address of the telegram.
     * This method does not check if the target is a group address and might return a part of the target individual address instead.
     */
    uint16_t getTargetGroupAddress();

    /**
     * @return the main group portion of the target address.
     * This method does not check if the target is a group address and might return a part of the target individual address instead.
     */
    uint8_t getTargetMainGroup();

    /**
	 * @return the middle group portion of the target address.
	 * This method does not check if the target is a group address and might return a part of the target individual address instead.
	 */
    uint8_t getTargetMiddleGroup();

    /**
	 * @return the sub group portion of the target address.
	 * This method does not check if the target is a group address and might return a part of the target individual address instead.
	 */
    uint8_t getTargetSubGroup();

    /**
	 * @return the area portion of the target address.
	 * This method does not check if the target is a individual address and might return a part of the target group address instead.
	 */
    uint8_t getTargetArea();

    /**
	 * @return the line portion of the target address.
	 * This method does not check if the target is a individual address and might return a part of the target group address instead.
	 */
    uint8_t getTargetLine();

    /**
	 * @return the member portion of the target address.
	 * This method does not check if the target is a individual address and might return a part of the target group address instead.
	 */
    uint8_t getTargetMember();

    /**
     * Set the routing counter to the given value.
     * @param aCounter the value to set the routing counter to.
     */
    void setRoutingCounter(uint8_t aCounter);

    /**
     * @return the current value of the routing counter.
     */
    uint8_t getRoutingCounter();

    /**
     * Set the command value.
     * @param aCommand the command value to set.
     */
    void setCommand(KnxCommandType aCommand);

    /**
     * @return the current command.
     */
    KnxCommandType getCommand();

    /**
     * Set the value of the first data byte in buffer.
     * This is only of interest for telegrams with 6 bit or less data.
     * @param aData the data byte to be placed into the first 6 bit of payload data.
     */
    void setFirstDataByte(uint8_t aData);

    /**
     * @return the first 6 bit of data from the first payload data byte.
     */
    uint8_t getFirstDataByte();

    /**
     * @return the payload data as boolean value.
     */
    bool getBool();

    /**
     * @return the payload data as 4bit integer value.
     */
    uint8_t get4BitIntValue();

    /**
     * @return the payload data as direction of a 4bit value.
     */
    bool get4BitDirectionValue();

    /**
     * @return the payload data as steps for a 4bit direction value.
     */
    uint8_t get4BitStepsValue();

    /**
     * Set the payload data to be a 1 byte signed integer.
     * @param aValue the value to set into payload data.
     */
    void set1ByteIntValue(int8_t aValue);

    /**
     * @return the payload data as 1 byte signed integer.
     */
    int8_t get1ByteIntValue();

    /**
     * Set the payload data to be a 1 byte unsigned integer.
     * @param aValue the value to set into payload data.
     */
    void set1ByteUIntValue(uint8_t aValue);

    /**
     * @return the payload data as a 1 byte unsigned integer.
     */
    uint8_t get1ByteUIntValue();

    /**
     * Set the payload data to be a 2 byte signed integer.
     * @param aValue the value to set into payload data.
     */
    void set2ByteIntValue(int16_t aValue);
    /**
     * @return the payload data as 2 byte signed integer.
     */
    int16_t get2ByteIntValue();

    /**
     * Set the payload data to be a 2 byte unsigned integer.
     * @param aValue the value to set into payload data.
     */
    void set2ByteUIntValue(uint16_t aValue);
    /**
     * @return the payload data as
     */
    uint16_t get2ByteUIntValue();

    /**
     * Set the payload data to be a 4 byte signed integer.
     * @param aValue the value to set into payload data.
     */
    void set4ByteIntValue(int32_t aValue);
    /**
     * @return the payload data as 4 byte signed integer.
     */
    int32_t get4ByteIntValue();

    /**
     * Set the payload data to be a 4 byte unsigned integer.
     * @param aValue the value to set into payload data.
     */
    void set4ByteUIntValue(uint32_t aValue);
    /**
     * @return the payload data as 4 byte unsigned integer.
     */
    uint32_t get4ByteUIntValue();

    /**
     * Set the payload data to be an 2 byte float.
     * @param aValue the value to set into payload data.
     */
    void set2ByteFloatValue(float aValue);
    /**
     * @return the payload data as 2 byte float.
     */
    float get2ByteFloatValue();

    /**
     * Set the payload data to be a 3 byte time value.
     * @param aWeekday the weekday (0-7)
     * @param aHour the hour (0-23)
     * @param aMinute the minute (0-59)
     * @param aSecond the second (0-59)
     */
    void set3ByteTime(uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    /**
     * @return the payload data as weekday of a 3 byte time.
     */
    uint8_t get3ByteWeekdayValue();
    /**
     * @return the payload data as hour of a 3 byte time.
     */
    uint8_t get3ByteHourValue();
    /**
     * @return the payload data as minute of a 3 byte time.
     */
    uint8_t get3ByteMinuteValue();
    /**
     * @return the payload data as second of a 3 byte time.
     */
    uint8_t get3ByteSecondValue();

    /**
     * Set the payload data to be an 3 byte date.
     * @param aDay the day part of the date (1-31).
     * @param aMonth the month part of the date (1-12).
     * @param aYear the year part of the date 0 = 1990 so valid range is (1990-2089).
     */
    void set3ByteDate(uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
     * @return the payload data as day of a 3 byte date value.
     */
    uint8_t get3ByteDayValue();
    /**
     * @return the payload data as month of a 3 byte date value.
     */
    uint8_t get3ByteMonthValue();
    /**
     * @return the payload data as year of a 3 byte date value.
     */
    uint8_t get3ByteYearValue();

    /**
     * Set the payload data to be an 4 byte float.
     * @param aValue the value to set into payload data.
     */
    void set4ByteFloatValue(float value);

    /**
     * @return the payload data as 4 byte float.
     */
    float get4ByteFloatValue();

    /**
     * Set the payload data to be an 14 char string.
     * @param aValue the value to set into payload data.
     */
    void set14ByteValue(String value);

    /**
     * @return the payload data as String
     */
    String get14ByteValue();

    /**
     * Receive the value into the given buffer.
     * @param aBuffer the buffer to store the value into.
     * @param aSize the size of the buffer.
     * @return the number of bytes copied.
     */
    uint8_t getValue(void* aBuffer, uint8_t aSize);

    /**
     * Set the data value from given buffer.
     * @param aBuffer the buffer to set the value from.
     * @param aSize the number of bytes to place into the buffer. This should not be more than 16.
     */
    void setValue(uint8_t* aBuffer, uint8_t aSize);

    /**
     * Create and assign the checksum to the telegram buffer. This need to be called befor sending the telegram.
     */
    void createChecksum();

    /**
     * Create a checksum and compare against the current checksum.
     * @return true if the created checksum matches the actual one.
     */
    bool verifyChecksum();

    /**
     * @return the checksum as currently in the buffer.
     */
    uint8_t getChecksum();

    /**
     * Print debug output to the given stream.
     * This method only prints in case TPUART_DEBUG is defined.
     */
    void print(Stream* aPort);

    /**
     * @return the total buffer length. This is the payload + header + checksum.
     */
    uint8_t getTotalLength();

    /**
     * @return the communication type.
     */
    KnxCommunicationType getCommunicationType();

    /**
     * Set the communication type.
     * @param the communication type.
     */
    void setCommunicationType(KnxCommunicationType aType);


    /**
     * @return the current sequence number.
     */
    uint8_t getSequenceNumber();

    /**
     * Set the sequence number.
     * @param aSequence the new sequence number.
     */
    void setSequenceNumber(uint8_t aSequence);

    /**
     * @return the control data.
     */
    KnxControlDataType getControlData();
    /**
     * Set the control data.
     * @param aControlData the control data to set.
     */
    void setControlData(KnxControlDataType aControlData);

  private:

    /**
     * The telegram buffer. This is always declared as 23 byte to fit to all possible telegrams.
     */
    uint8_t buffer[MAX_KNX_TELEGRAM_SIZE];

    /**
     * Calculate the checksum based on actual buffer.
     * @return the calculated checksum.
     */
    uint8_t calculateChecksum();

};

#endif
