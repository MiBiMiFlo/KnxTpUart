// File: KnxTpUart.h
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 2021-02-09 by MiBiMiFlo

#ifndef KnxTpUart_h
#define KnxTpUart_h

#include "Arduino.h"

#include "KnxTelegram.h"

// Services from TPUART
#define TPUART_RESET_INDICATION_BYTE B11

// Services to TPUART
#define TPUART_DATA_START_CONTINUE B10000000

#define TPUART_DATA_END B01000000

#define TPUART_SEND_SUCCESS B10001011

#define TPUART_SEND_NOT_SUCCESS B00001011

#define TPUART_ACK B00010001

#define TPUART_NACK B00010000

#define TPUART_RESET 0x01

#define TPUART_STATE_REQUEST 0x02

// Uncomment the following line to enable debugging
//#define TPUART_DEBUG

#define TPUART_DEBUG_PORT Serial


// Delay in ms between sending of packets to the bus
// Change only if you know what you're doing
//#define SERIAL_WRITE_DELAY_MS 100

// Timeout for reading a byte from TPUART
// Change only if you know what you're doing
#define SERIAL_READ_TIMEOUT_MS 10

// If KNX_SUPPORT_LISTEN_GAS is defined listening GAs can be added.
#define KNX_SUPPORT_LISTEN_GAS

/**
 * Definition of callback function type to allow application to check if telegram is of interest
 */
typedef bool (*KnxTelegramCheckType)(KnxTelegram *aTelegram);

enum KnxTpUartSerialEventType
{
  TPUART_RESET_INDICATION,
  KNX_TELEGRAM,
  IRRELEVANT_KNX_TELEGRAM,
  TIMEOUT,
  UNKNOWN
};

class KnxTpUart {


  public:
	/**
	 * Create a new instance.
	 * @param aPort the communication port.
	 * @param aAddress The source address to use.
	 */
    KnxTpUart(Stream* aPort, String aAddress);

	/**
	 * Create a new instance.
	 * @param aPort the communication port.
	 * @param aAddress The source address to use.
	 */
    KnxTpUart(Stream*, uint16_t);

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
    bool groupWrite4BitInt(String aAddress, uint8_t aValue);

    /**
     * Send a 4bit value to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitInt
     */
    bool groupWrite4BitInt(uint16_t aAddress, uint8_t aValue);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    bool groupWrite4BitDim(String aAddress, bool aDirection, uint8_t aSteps);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    bool groupWrite4BitDim(uint16_t aAddress, bool aDirection, uint8_t aSteps);

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
     * Send a buffer
     */
    bool groupWriteBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize);

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
    bool groupAnswer4BitInt(String, uint8_t aValue);

    /**
     * Send a 4bit answer to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitInt
     */
    bool groupAnswer4BitInt(uint16_t, uint8_t aValue);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    bool groupAnswer4BitDim(String, bool aDirection, uint8_t aSteps);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    bool groupAnswer4BitDim(uint16_t, bool aDirection, uint8_t aSteps);

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

    bool groupAnswerBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize);

    // Start of definitions for uint16_t address functions

    bool groupRead(String aAddress);
    bool groupRead(uint16_t aAddress);

    bool individualAnswerAddress();

    bool individualAnswerMaskVersion(uint8_t aArea, uint8_t aLine, uint8_t aMember);
    bool individualAnswerMaskVersion(uint16_t);

    bool individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint8_t aArea, uint8_t aLine, uint8_t aMember);
    bool individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint16_t aAddress);


    /**
     * Set if listen to broadcast messages is wanted.
     * This is used for programming mode (to learn device address through ETS).
     * @param aFlag true if broadcast messages should be listened for, false otherwise.
     */
    void setListenToBroadcasts(bool aFlag);

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

    /**
     * Set a callback function that is called within telegram receive and checks if the telegram is of interest or not.
     * @param aCallback the callback function that is used to check if a telegram is of interest.
     */
    void setTelegramCheckCallback(KnxTelegramCheckType aCallback);

    /**
     * Send the given telegram to bus.
     * @param aTelegram the telegram to send.
     * @return true if send (and receive) was successful, false otherwise.
     */
    bool sendTelegram(KnxTelegram* aTelegram);

#ifdef KNX_SUPPORT_LISTEN_GAS

    bool addListenGroupAddress(String aAddress);
    bool addListenGroupAddress(uint16_t aAddress);

    bool isListeningToGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);
    bool isListeningToGroupAddress(uint16_t aAddress);


    /**
     * Set the maximum number of listening group addresses.
     * This need to be called before a listening GA is added by calling addListenGroupAddress(aAddress).
     * This will clear all previously assigned addresses.
     * This method will reserve 2 byte RAM for each address.
     * @param aCount the maximum number of addresses to be allowed.
     */
    bool setListenAddressCount(uint8_t aCount);

#endif

  private:

    /**
     * The Stream to use for communication with KNX.
     */
    Stream* _serialport;

    /**
     * The KNX telegram buffer used for normal communication.
     * This is allocated in constructor but never freed!
     */
    KnxTelegram* _tg;

    /**
     * The KNX source address used in default KnxTelegrams.
     */
    uint16_t mSourceAddress;

#ifdef KNX_SUPPORT_LISTEN_GAS
    /**
     * The list of group addresses to listen to.
     */
    uint16_t *mListenGAs;

    /**
     * The number of registered group addresses.
     */
    uint8_t mListenGAsCount;

    uint8_t mListenGAsMax;
#endif

    /**
     * A flag to define if broadcast listening is requested.
     */
    bool _listen_to_broadcasts;

    /**
     * The callback function that allows to
     */
    KnxTelegramCheckType mTelegramCheckCallback;

    /**
     * Internal initialization, called from each constructor.
     */
    void init(void);

    bool isKNXControlByte(uint8_t aByte);

    void checkErrors(void);

    /**
     * Print a single incoming byte into debug output stream.
     */
    void printByte(uint8_t aByte);

    /**
     * Read a telegram from BUS into the internal telegram buffer
     */
    KnxTpUartSerialEventType readKNXTelegram();

    /**
     * Initialize the internal telegram buffer for a new message.
     * This message initializes a telegram send to a group address.
     * @param aPayloadLength the payload length
     * @param aCommand the command type
     * @param aAddress the target group address.
     * @param aFirstDataByte the first data byte.
     */
    void createKNXMessageFrame(uint8_t aPayloadLength, KnxCommandType aCommand, String aAddress, uint8_t aFirstDataByte);

    /**
	 * Initialize the internal telegram buffer for a new message.
     * This message initializes a telegram send to a group address.
	 * @param aPayloadLength the payload length
	 * @param aCommand the command type
	 * @param aAddress the target group address.
	 * @param aFirstDataByte the first data byte.
	 */
    void createKNXMessageFrame(uint8_t aPayloadLength, KnxCommandType aCommand, uint16_t aAddress, uint8_t aFirstDataByte);

    /**
     * Initialize the internal telegram buffer for a new message.
     * This message initializes a telegram send to an individual device.
     * @param aPayloadLength the payload length
     * @param aCommand the command type
     * @param aAddress the target individual address.
     * @param aFirstDataByte the first data byte.
     */
    void createKNXMessageFrameIndividual(uint8_t aPayloadLength, KnxCommandType aCommand, String aAddress, uint8_t aFirstDataByte);

    /**
     * Initialize the internal telegram buffer for a new message.
     * This message initializes a telegram send to an individual device.
     * @param aPayloadLength the payload length
     * @param aCommand the command type
     * @param aAddress the target individual address.
     * @param aFirstDataByte the first data byte.
     */
    void createKNXMessageFrameIndividual(uint8_t aPayloadLength, KnxCommandType aCommand, uint16_t aAddress, uint8_t aFirstDataByte);

    /**
     * Send the a KNX message from internal telegram buffer.
     */
    bool sendMessage();

    /**
     * Send a confirm message to the given address.
     * @param aSequenceNo the sequence no of the telegram to confirm.
     * @param aAddress the source address of the telegram to confirm.
     */
    bool sendNCDPosConfirm(uint8_t aSequenceNo, uint16_t aAddress);

    /**
     * Read a single byte from serial interface with timeout.
     * @return the read byte or -1 in case of timeout.
     */
    int serialRead();


};

#endif
