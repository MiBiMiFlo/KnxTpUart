// File: KnxTpUart.h
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 06.06.2017

#ifndef KnxTpUart_h
#define KnxTpUart_h

#include "HardwareSerial.h"
#include "Arduino.h"

#include "KnxTelegram.h"

// Services from TPUART
#define TPUART_RESET_INDICATION_BYTE B11

// Services to TPUART
#define TPUART_DATA_START_CONTINUE B10000000
#define TPUART_DATA_END B01000000

// Uncomment the following line to enable debugging
//#define TPUART_DEBUG

#define TPUART_DEBUG_PORT Serial

#define TPUART_SERIAL_CLASS Stream

// Delay in ms between sending of packets to the bus
// Change only if you know what you're doing
#define SERIAL_WRITE_DELAY_MS 100

// Timeout for reading a byte from TPUART
// Change only if you know what you're doing
#define SERIAL_READ_TIMEOUT_MS 10

// Maximum number of group addresses that can be listened on
#define MAX_LISTEN_GROUP_ADDRESSES 15

enum KnxTpUartSerialEventType {
  TPUART_RESET_INDICATION,
  KNX_TELEGRAM,
  IRRELEVANT_KNX_TELEGRAM,
  UNKNOWN
};

class KnxTpUart {


  public:
	/**
	 * Create a new instance.
	 * @param aPort the communication port.
	 * @param aAddress The source address to use.
	 */
    KnxTpUart(TPUART_SERIAL_CLASS* aPort, String aAddress);

	/**
	 * Create a new instance.
	 * @param aPort the communication port.
	 * @param aAddress The source address to use.
	 */
    KnxTpUart(TPUART_SERIAL_CLASS*, uint16_t);

    /**
     * Perform a UART connection reset.
     * This method sends a 0x01 to the UART port.
     */
    void uartReset();

    /**
     * Perform a state request on UART module.
     * This method sends a 0x02 to the UART.
     */
    void uartStateRequest();

    /**
     * Has to be called to fetch a telegram from the UART communication port.
     * @return a enum value to indicate if a KNX telegram of interest can be read or not.
     */
    KnxTpUartSerialEventType serialEvent();

    /**
     * Retrieve the current telegram for further processing.
     * @return a pointer to the current telegram. This is only valid if #serialEvent() returned KNX_TELEGRAM.
     */
    KnxTelegram* getReceivedTelegram();

    /*
     * Set the individual device address by passing in 3 parts.
     * @param aArea the area id (4 bit).
     * @param aLine the line id (4 bit).
     * @param aMember the member if (8bit).
     */
    void setIndividualAddress(uint8_t aArea, uint8_t aLine, uint8_t aMember);

    /**
     * Set the individual device address by passing in a 16 bit address.
     * @param aAddress the address.
     */
    void setIndividualAddress(uint16_t aAddress);

    /**
     * Send an ACK byte to the UART.
     */
    void sendAck();

    /**
     * Send a NOT_ADDRESSED byte to the UART.
     */
    void sendNotAddressed();

    /**
     * Send a boolean (1bit) value to a group address.
     * This can be used for DPT-1.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswerBool
     */
    bool groupWriteBool(String aAddress, bool aValue);

    /**
     * Send a boolean (1bit) value to a group address.
     * This can be used for DPT-1.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswerBool
     */
    bool groupWriteBool(uint16_t aAddress, bool aValue);

    /**
     * Send a 4bit value to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitInt
     */
    bool groupWrite4BitInt(String aAddress, int8_t aValue);

    /**
     * Send a 4bit value to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitInt
     */
    bool groupWrite4BitInt(uint16_t aAddress, int8_t aValue);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    bool groupWrite4BitDim(String aAddress, bool aDirection, byte aSteps);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    bool groupWrite4BitDim(uint16_t aAddress, bool aDirection, byte aSteps);

    /**
	 * Send a 8bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteInt
	 */
    bool groupWrite1ByteInt(String aAddress, int8_t aValue);

    /**
	 * Send a 8bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteInt
	 */
    bool groupWrite1ByteInt(uint16_t aAddress, int8_t aValue);

    /**
	 * Send a 8bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteUInt
	 */
    bool groupWrite1ByteUInt(String aAddress, uint8_t aValue);

    /**
	 * Send a 8bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteUInt
	 */
    bool groupWrite1ByteUInt(uint16_t aAddress, uint8_t aValue);

    /**
	 * Send a 16bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer2ByteUInt
	 */
    bool groupWrite2ByteInt(String aAddress, int16_t aValue);

    /**
	 * Send a 16bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer2ByteInt
	 */
    bool groupWrite2ByteInt(uint16_t aAddress, int16_t aValue);

    /**
	 * Send a 16bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite2ByteUInt(String aAddress, uint16_t aValue);

    /**
	 * Send a 16bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite2ByteUInt(uint16_t aAddress, uint16_t aValue);

    /**
	 * Send a 32bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer4ByteInt
	 */
    bool groupWrite4ByteInt(uint16_t aAddress, int32_t aValue);
    /**
	 * Send a 32bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer4ByteUInt
	 */
    bool groupWrite4ByteUInt(uint16_t aAddress, uint32_t aValue);

    /**
	 * Send a 16bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite2ByteFloat(String aAddress, float aValue);

    /**
	 * Send a 16bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite2ByteFloat(uint16_t aAddress, float aValue);

    bool groupWrite3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);
    bool groupWrite3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    bool groupWrite3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);
    bool groupWrite3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
	 * Send a 32bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite4ByteFloat(String aAddress, float aValue);
    /**
	 * Send a 32bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite4ByteFloat(uint16_t aAddress, float aValue);

    /**
	 * Send a 14 byte text value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite14ByteText(String aAddress, String aValue);

    /**
	 * Send a 14 byte text value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    bool groupWrite14ByteText(uint16_t aAddress, String aValue);

    /**
	 * Send a boolean (1bit) value to a group address.
	 * This can be used for DPT-1.
	 * @param aAddress the address to write to.
	 * @param aValue the value to write.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWriteBool
	 */
    bool groupAnswerBool(String aAddress, bool aValue);

    /**
	 * Send a boolean (1bit) answer to a group address.
	 * This can be used for DPT-1.
	 * @param aAddress the address to write to.
	 * @param aValue the value to write.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWriteBool
	 */
    bool groupAnswerBool(uint16_t aAddress, bool aValue);

    /**
     * Send a 4bit answer to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitInt
     */
    bool groupAnswer4BitInt(String, int8_t aValue);

    /**
     * Send a 4bit answer to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitInt
     */
    bool groupAnswer4BitInt(uint16_t, int8_t aValue);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    bool groupAnswer4BitDim(String, bool aDirection, byte aSteps);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    bool groupAnswer4BitDim(uint16_t, bool aDirection, byte aSteps);

    /**
	 * Send a 8bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteInt
	 */
    bool groupAnswer1ByteInt(String aAddress, int8_t aValue);

    /**
	 * Send a 8bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteInt
	 */
    bool groupAnswer1ByteInt(uint16_t aAddress, int8_t aValue);

    /**
	 * Send a 8bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteUInt
	 */
    bool groupAnswer1ByteUInt(String aAddress, uint8_t aValue);
    /**
	 * Send a 8bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteUInt
	 */
    bool groupAnswer1ByteUInt(uint16_t aAddress, uint8_t aValue);

    /**
	 * Send a 16bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    bool groupAnswer2ByteInt(String aAddress, int16_t aValue);
    /**
	 * Send a 16bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    bool groupAnswer2ByteInt(uint16_t aAddress, int16_t aValue);

    /**
	 * Send a 16bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteUInt
	 */
    bool groupAnswer2ByteUInt(String aAddress, uint16_t aValue);

    /**
	 * Send a 16bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteUInt
	 */
    bool groupAnswer2ByteUInt(uint16_t aAddress, uint16_t aValue);

    /**
	 * Send a 32bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    bool groupAnswer4ByteInt(uint16_t aAddress, int32_t aValue);

    /**
	 * Send a 32bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteUInt
	 */
    bool groupAnswer4ByteUInt(uint16_t aAddress, uint32_t aValue);

    /**
	 * Send a 16bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteFloat
	 */
    bool groupAnswer2ByteFloat(String aAddress, float aValue);
    /**
	 * Send a 16bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteFloat
	 */
    bool groupAnswer2ByteFloat(uint16_t aAddress, float aValue);

    bool groupAnswer3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);
    bool groupAnswer3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    bool groupAnswer3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);
    bool groupAnswer3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
	 * Send a 32bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteFloat
	 */
    bool groupAnswer4ByteFloat(String aAddress, float aValue);

    /**
	 * Send a 32bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteFloat
	 */
    bool groupAnswer4ByteFloat(uint16_t aAddress, float aValue);

    bool groupAnswer14ByteText(String aAddress, String aValue);
    bool groupAnswer14ByteText(uint16_t aAddress, String aValue);

    // Start of definitions for uint16_t address functions

    bool groupRead(String aAddress);
    bool groupRead(uint16_t aAddress);

    void addListenGroupAddress(String aAddress);
    void addListenGroupAddress(uint16_t aAddress);

    bool isListeningToGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);
    bool isListeningToGroupAddress(uint16_t aAddress);

    bool individualAnswerAddress();

    bool individualAnswerMaskVersion(uint8_t aArea, uint8_t aLine, uint8_t aMember);
    bool individualAnswerMaskVersion(uint16_t);

    bool individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint8_t aArea, uint8_t aLine, uint8_t aMember);
    bool individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint16_t aAddress);


    void setListenToBroadcasts(bool);

    /**
     * Converts a String of the form 1/2/3 into a 2 byte group address.
     * @param aAddress the address to parse.
     * @return the two byte address.
     */
    uint16_t getGroupAddress(String aAddress);

    /**
	 * Converts a String of the form 1.2.3 into a 2 byte individual address.
	 * @param aAddress the address to parse.
	 * @return the two byte address.
	 */
    uint16_t getSourceAddress(String aAddress);


  private:
    Stream* _serialport;
    KnxTelegram* _tg;       // for normal communication
    KnxTelegram* _tg_ptp;   // for PTP sequence confirmation

    // KNX Address is 16bit value
    uint16_t mSourceAddress;

    uint16_t _listen_group_addresses[MAX_LISTEN_GROUP_ADDRESSES];
    uint8_t _listen_group_address_count;
    bool _listen_to_broadcasts;

    bool isKNXControlByte(uint8_t);
    void checkErrors();
    void printByte(uint8_t);
    bool readKNXTelegram();
    void createKNXMessageFrame(uint8_t, KnxCommandType, String, uint8_t);
    void createKNXMessageFrameIndividual(uint8_t, KnxCommandType, String, uint8_t);

    void createKNXMessageFrame(uint8_t, KnxCommandType, uint16_t, uint8_t);
    void createKNXMessageFrameIndividual(uint8_t, KnxCommandType, uint16_t, uint8_t);

    bool sendMessage();
    bool sendNCDPosConfirm(uint8_t aSequenceNo, uint8_t aArea, uint8_t aLine, uint8_t aMember);
    int serialRead();
};

#endif
