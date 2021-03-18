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

#define TPUART_SET_ADDRESS B00101000

#define TPUART_SEND_SUCCESS B10001011

#define TPUART_SEND_NOT_SUCCESS B00001011

#define TPUART_ACK B00010001

#define TPUART_NACK B00010000

#define TPUART_RESET 0x01

#define TPUART_STATE_REQUEST 0x02

// Uncomment the following line to enable debugging
//#define TPUART_DEBUG

// define TPUART_DEBUG_V for verbose debug messages
//#define TPUART_DEBUG_V

#ifdef TPUART_DEBUG
	#define DBG_PRINT(aMsg) ({if (_dbg != NULL) {_dbg->print(aMsg);}})
	#define DBG_PRINTLN(aMsg) ({if (_dbg != NULL) {_dbg->println(aMsg);}})
#else
	#define DBG_PRINT(aMsg) ({})
	#define DBG_PRINTLN(aMsg) ({})
#endif


// Timeout for reading a byte from TPUART
// Change only if you know what you're doing
#define SERIAL_READ_TIMEOUT_MS 10

#define SERIAL_READ_TELEGRAM_TIMEOUT_MS 10

// timeout when waiting for ACK from TPUART
#define SERIAL_WAIT_ACK_TIMEOUT_MS 500

// If KNX_SUPPORT_LISTEN_GAS is defined listening GAs can be added.
//#define KNX_SUPPORT_LISTEN_GAS

/*
 * Some config flags related to wait for a response by TPUART
 */

// Wait for a KNX telegram send confirmation.
#define CONFIG_WAIT_FOR_SEND_CONFIRM    B00000001
// Wait for a RESET confirmation.
#define CONFIG_WAIT_FOR_RESET_CONFIRMED B00000010
// Wait for a STATE response.
#define CONFIG_WAIT_FOR_STATE_RESPONSE  B00000100


#define STATUS_DATA_SEND_SUCCESS        B00000001
#define STATUS_DATA_SEND_NOT_SUCCESS    B00000010
#define STATUS_DATA_SEND_TIMEOUT        B00000100
#define STATUS_RESET_CONFIRMED          B00001000
#define STATUS_STATE_RESPONSE_RECEIVED  B00010000



/**
 * The result type for sendTelegram and related functions.
 */
enum KnxTpUartSendResult
{
	SEND_SUCCESSFUL=0,
	SEND_NOT_SUCCESSFUL=1,
	SEND_TIMEOUT=-1
};

enum KnxTpUartSerialEventType
{
  NO_DATA,
  KNX_TELEGRAM,
  TPUART_RESET_INDICATION,
  TPUART_STATE_RESPONSE,
  IRRELEVANT_KNX_TELEGRAM,
  TPUART_RESPONSE_SEND_NOT_SUCCESS,
  TPUART_RESPONSE_SEND_SUCCESS,
  TIMEOUT,
  UNKNOWN
};

/**
 * Definition of callback function type to allow application to check if telegram is of interest.
 * This callback is called after only the first 6 bytes are received and has to return asap (within ~1ms).
 * The result is used to determine if ACK or NACK is returned.
 */
typedef bool (*KnxTelegramCheckType)(KnxTelegram *aTelegram);


/**
 * Callback type for serial event callback.
 * @param aEventType the event type that was encountered.
 * @param aTpUartByte the byte from TpUart that caused the event. This is only of interest for UNKNOWN event.
 */
typedef void (*KnxSerialEventCallback)(KnxTpUartSerialEventType aEventType, uint8_t aTpUartByte);

/*
 * This type is used in the callback for new telegrams.
 * @param aTelegram the new received KNX telegram
 * @param aIsInteresting if the telegram was considered to be of interest before this is true, otherwise false.
 */
typedef void (*KnxTelegramCallback)(KnxTelegram *aTelegram, bool aIsInteresting);


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
     * @param aTimeout the number of milliseconds to wait for a response.
     * @return true if reset was successful (reset response received within timeout).
     */
    bool uartReset(uint16_t aTimeout);

    /**
     * Perform a state request on UART module.
     * This method sends a 0x02 to the UART.
     * @param aTimeout the number of milliseconds to wait for a response.
     * @return the received response of 0 if no response was received within timeout.
     */
    uint8_t uartStateRequest(uint16_t aTimeout);

    void uartSetAddress();

    /**
     * @return the last value received as response to a STATE request.
     */
    uint8_t getStateResponse();

    /**
     * Has to be called to fetch a telegram from the UART communication port.
     * @return a enum value to indicate if a KNX telegram of interest can be read or not.
     */
    void serialEvent();

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


    uint16_t getIndividualAddress();

    /**
     * Send a boolean (1bit) value to a group address.
     * This can be used for DPT-1.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswerBool
     */
    KnxTpUartSendResult groupWriteBool(String aAddress, bool aValue);

    /**
     * Send a boolean (1bit) value to a group address.
     * This can be used for DPT-1.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswerBool
     */
    KnxTpUartSendResult groupWriteBool(uint16_t aAddress, bool aValue);

    /**
     * Send a 4bit value to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitInt
     */
    KnxTpUartSendResult groupWrite4BitInt(String aAddress, uint8_t aValue);

    /**
     * Send a 4bit value to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitInt
     */
    KnxTpUartSendResult groupWrite4BitInt(uint16_t aAddress, uint8_t aValue);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    KnxTpUartSendResult groupWrite4BitDim(String aAddress, bool aDirection, uint8_t aSteps);

    /**
     * Send a boolean + 3 bit value (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupAnswer4BitDim
     */
    KnxTpUartSendResult groupWrite4BitDim(uint16_t aAddress, bool aDirection, uint8_t aSteps);

    /**
	 * Send a 8bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteInt
	 */
    KnxTpUartSendResult groupWrite1ByteInt(String aAddress, int8_t aValue);

    /**
	 * Send a 8bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteInt
	 */
    KnxTpUartSendResult groupWrite1ByteInt(uint16_t aAddress, int8_t aValue);

    /**
	 * Send a 8bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteUInt
	 */
    KnxTpUartSendResult groupWrite1ByteUInt(String aAddress, uint8_t aValue);

    /**
	 * Send a 8bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer1ByteUInt
	 */
    KnxTpUartSendResult groupWrite1ByteUInt(uint16_t aAddress, uint8_t aValue);

    /**
	 * Send a 16bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer2ByteUInt
	 */
    KnxTpUartSendResult groupWrite2ByteInt(String aAddress, int16_t aValue);

    /**
	 * Send a 16bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer2ByteInt
	 */
    KnxTpUartSendResult groupWrite2ByteInt(uint16_t aAddress, int16_t aValue);

    /**
	 * Send a 16bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite2ByteUInt(String aAddress, uint16_t aValue);

    /**
	 * Send a 16bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite2ByteUInt(uint16_t aAddress, uint16_t aValue);

    /**
	 * Send a 32bit signed integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer4ByteInt
	 */
    KnxTpUartSendResult groupWrite4ByteInt(uint16_t aAddress, int32_t aValue);
    /**
	 * Send a 32bit unsigned integer value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupAnswer4ByteUInt
	 */
    KnxTpUartSendResult groupWrite4ByteUInt(uint16_t aAddress, uint32_t aValue);

    /**
	 * Send a 16bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite2ByteFloat(String aAddress, float aValue);

    /**
	 * Send a 16bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite2ByteFloat(uint16_t aAddress, float aValue);

    /**
     * Send a 3 byte value of DPT 10 containing the time.
     * @param aAddress the address to write to.
     * @param aWeekday the day of the week
     * @param aHour the hour
     * @param aMinute the minute
     * @param aSecond the second
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupWrite3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    /**
     * Send a 3 byte value of DPT 10 (time).
     * @param aAddress the address to write to.
     * @param aWeekday the day of the week.
     * @param aHour the hour.
     * @param aMinute the minute.
     * @param aSecond the second.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupWrite3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    /**
     * Send a 3 byte value of DPT 11 (date).
     * @param aAddress the address to write to.
     * @param aDay the day of the month.
     * @param aMonth the month of the year.
     * @param aYear the year.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupWrite3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
     * Send a 3 byte value of DPT 11 (date).
     * @param aAddress the address to write to.
     * @param aDay the day of the month.
     * @param aMonth the month of the year.
     * @param aYear the year.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupWrite3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
	 * Send a 32bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite4ByteFloat(String aAddress, float aValue);
    /**
	 * Send a 32bit float value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite4ByteFloat(uint16_t aAddress, float aValue);

    /**
	 * Send a 14 byte text value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite14ByteText(String aAddress, String aValue);

    /**
	 * Send a 14 byte text value to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupWrite14ByteText(uint16_t aAddress, String aValue);

    /**
     * Send a buffer to a group address.
     * @param aAddress the address to write to.
     * @param aBuffer the buffer to send.
     * @param aSize the number of bytes to send from buffer.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupWriteBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize);

    /**
	 * Send a boolean (1bit) value to a group address.
	 * This can be used for DPT-1.
	 * @param aAddress the address to write to.
	 * @param aValue the value to write.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWriteBool
	 */
    KnxTpUartSendResult groupAnswerBool(String aAddress, bool aValue);

    /**
	 * Send a boolean (1bit) answer to a group address.
	 * This can be used for DPT-1.
	 * @param aAddress the address to write to.
	 * @param aValue the value to write.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWriteBool
	 */
    KnxTpUartSendResult groupAnswerBool(uint16_t aAddress, bool aValue);

    /**
     * Send a 4bit answer to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitInt
     */
    KnxTpUartSendResult groupAnswer4BitInt(String, uint8_t aValue);

    /**
     * Send a 4bit answer to a group address.
     * This can be used for DPT-2 or DPT-3.
     * @param aAddress the address to write to.
     * @param aValue the value to write.
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitInt
     */
    KnxTpUartSendResult groupAnswer4BitInt(uint16_t, uint8_t aValue);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    KnxTpUartSendResult groupAnswer4BitDim(String, bool aDirection, uint8_t aSteps);

    /**
     * Send a boolean + 3 bit answer (DPT-3) to a group address.
     * @param aAddress the address to write to.
     * @param aDirection (1vit value).
     * @param aSteps (3bit value)
     * @return true if writing was successful, false otherwise.
     * @see #groupWrite4BitDim
     */
    KnxTpUartSendResult groupAnswer4BitDim(uint16_t, bool aDirection, uint8_t aSteps);

    /**
	 * Send a 8bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteInt
	 */
    KnxTpUartSendResult groupAnswer1ByteInt(String aAddress, int8_t aValue);

    /**
	 * Send a 8bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteInt
	 */
    KnxTpUartSendResult groupAnswer1ByteInt(uint16_t aAddress, int8_t aValue);

    /**
	 * Send a 8bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteUInt
	 */
    KnxTpUartSendResult groupAnswer1ByteUInt(String aAddress, uint8_t aValue);
    /**
	 * Send a 8bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite1ByteUInt
	 */
    KnxTpUartSendResult groupAnswer1ByteUInt(uint16_t aAddress, uint8_t aValue);

    /**
	 * Send a 16bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    KnxTpUartSendResult groupAnswer2ByteInt(String aAddress, int16_t aValue);
    /**
	 * Send a 16bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    KnxTpUartSendResult groupAnswer2ByteInt(uint16_t aAddress, int16_t aValue);

    /**
	 * Send a 16bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteUInt
	 */
    KnxTpUartSendResult groupAnswer2ByteUInt(String aAddress, uint16_t aValue);

    /**
	 * Send a 16bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteUInt
	 */
    KnxTpUartSendResult groupAnswer2ByteUInt(uint16_t aAddress, uint16_t aValue);

    /**
	 * Send a 32bit signed integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteInt
	 */
    KnxTpUartSendResult groupAnswer4ByteInt(uint16_t aAddress, int32_t aValue);

    /**
	 * Send a 32bit unsigned integer answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteUInt
	 */
    KnxTpUartSendResult groupAnswer4ByteUInt(uint16_t aAddress, uint32_t aValue);

    /**
	 * Send a 16bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteFloat
	 */
    KnxTpUartSendResult groupAnswer2ByteFloat(String aAddress, float aValue);

    /**
	 * Send a 16bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite2ByteFloat
	 */
    KnxTpUartSendResult groupAnswer2ByteFloat(uint16_t aAddress, float aValue);

    /**
	 * Send a 3 byte answer of DPT 10 (time).
	 * @param aAddress the address to write to.
	 * @param aWeekday the day of the week.
	 * @param aHour the hour.
	 * @param aMinute the minute.
	 * @param aSecond the second.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupAnswer3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    /**
	 * Send a 3 byte answer of DPT 10 (time).
	 * @param aAddress the address to write to.
	 * @param aWeekday the day of the week.
	 * @param aHour the hour.
	 * @param aMinute the minute.
	 * @param aSecond the second.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupAnswer3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond);

    /**
     * Send a 3 byte value of DPT 11 (date).
     * @param aAddress the address to write to.
     * @param aDay the day of the month.
     * @param aMonth the month of the year.
     * @param aYear the year.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupAnswer3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
     * Send a 3 byte value of DPT 11 (date).
     * @param aAddress the address to write to.
     * @param aDay the day of the month.
     * @param aMonth the month of the year.
     * @param aYear the year.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupAnswer3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear);

    /**
	 * Send a 32bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteFloat
	 */
    KnxTpUartSendResult groupAnswer4ByteFloat(String aAddress, float aValue);

    /**
	 * Send a 32bit float answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 * @see #groupWrite4ByteFloat
	 */
    KnxTpUartSendResult groupAnswer4ByteFloat(uint16_t aAddress, float aValue);

    /**
	 * Send a 14 byte text answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupAnswer14ByteText(String aAddress, String aValue);

    /**
	 * Send a 14 byte text answer to a group address.
	 * @param aAddress the address to write to.
	 * @param aValue the value to send.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult groupAnswer14ByteText(uint16_t aAddress, String aValue);

    /**
     * Send a buffer to a group address.
     * @param aAddress the address to write to.
     * @param aBuffer the buffer to send.
     * @param aSize the number of bytes to send from buffer.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupAnswerBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize);

    // Start of definitions for uint16_t address functions

    /**
     * Request an answer for the actual value of a group address.
     * This does not wait for an answer, waiting for an answer by calling
     * #serialEvent and #getReceivedTelegram need to be performed afterwards.
     * @param aAddress the address to request an answer from.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupRead(String aAddress);

    /**
     * Request an answer for the actual value of a group address.
     * This does not wait for an answer, waiting for an answer by calling
     * #serialEvent and #getReceivedTelegram need to be performed afterwards.
     * @param aAddress the address to request an answer from.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult groupRead(uint16_t aAddress);

    /**
     * Send a KNX telegram with command KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult individualAnswerAddress();

    /**
     * Send a KNX telegram with command KNX_COMMAND_MASK_VERSION_RESPONSE.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult individualAnswerMaskVersion(uint8_t aArea, uint8_t aLine, uint8_t aMember);

    /**
     * Send a KNX telegram with command KNX_COMMAND_MASK_VERSION_RESPONSE.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult individualAnswerMaskVersion(uint16_t);

    /**
     * Send a KNX telegram with command KNX_COMMAND_ESCAPE and KNX_EXT_COMMAND_AUTH_RESPONSE as data.
     * @param aAccessLevel the access level.
     * @param aSequenceNo the sequence number.
     * @param aArea the area part of the target address.
     * @param aLine the line part of the target address.
     * @param aMember the member part of the target address.
     * @return true if writing was successful, false otherwise.
     */
    KnxTpUartSendResult individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint8_t aArea, uint8_t aLine, uint8_t aMember);

    /**
	 * Send a KNX telegram with command KNX_COMMAND_ESCAPE and KNX_EXT_COMMAND_AUTH_RESPONSE as data.
	 * @param aAccessLevel the access level.
	 * @param aSequenceNo the sequence number.
	 * @param aAddress the target address.
	 * @return true if writing was successful, false otherwise.
	 */
    KnxTpUartSendResult individualAnswerAuth(uint8_t aAccessLevel, uint8_t aSequenceNo, uint16_t aAddress);


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
     * Used to set a callback that is to receive serial events.
     */
    void setSerialEventCallback(KnxSerialEventCallback aCallback);

    void setKnxTelegramCallback(KnxTelegramCallback aCallback);

    /**
     * Send the given telegram to bus.
     * @param aTelegram the telegram to send.
     * @return true if send (and receive) was successful, false otherwise.
     */
    KnxTpUartSendResult sendTelegram(KnxTelegram* aTelegram);

    /**
	 * Send the given telegram to bus and repeat up to aCount times on failure.
	 * @param aTelegram the telegram to send.
	 * @param aCount the maximum number of repeats.
	 * @return true if send (and receive) was successful, false otherwise.
	 */
    KnxTpUartSendResult sendTelegramAndRepeat(KnxTelegram* aTelegram, uint8_t aCount);

#ifdef KNX_SUPPORT_LISTEN_GAS

    /**
     * Add a group address to the list of listening addresses.
     * @param aAddress the address to listen to.
     * @return true if the address was added, false otherwise.
     * False very likely means that no space is left in the list and
     * #setListenAddressCount need to be called with a bigger value.
     */
    bool addListenGroupAddress(String aAddress);

    /**
	 * Add a group address to the list of listening addresses.
	 * @param aAddress the address to listen to.
	 */
    bool addListenGroupAddress(uint16_t aAddress);

    /**
     * Check if the given address is added to the list of listening addresses.
     * @param aMain the main part of the address.
     * @param aMiddle	the middle part of the address.
     * @param aSub the sub part of the address.
     * @return true if the address is contained in the list of listening addresses.
     */
    bool isListeningToGroupAddress(uint8_t aMain, uint8_t aMiddle, uint8_t aSub);

    /**
     * Check if the given address is added to the list of listening addresses.
     * @param aAddress the address.
     * @return true if the address is contained in the list of listening addresses.
     */
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

#ifdef TPUART_DEBUG
    void setDebugPort(Stream * aStream);
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

    /**
     * The maximum number of GAs to listen to.
     */
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
     * This callback is called in case of a new serial event.
     */
    KnxSerialEventCallback mSerialEventCallback;

    /**
     * This callback is called in case a new KNX telegram was received.
     */
    KnxTelegramCallback mKnxTelegramCallback;

    /**
     * The value of the last received state response.
     */
    uint8_t mStateResponse;

    /**
     * A 8bit register storing current config flags (waiting for specific results).
     */
    uint8_t mConfigReg;

    /**
     * A 8 bit status register where response results are set.
     */
    uint8_t mStatusReg;


#ifdef TPUART_DEBUG
    Stream* _dbg;
#endif
    /**
     * Internal initialization, called from each constructor.
     */
    void init(void);

    /**
     * Check for errors in USART control registers.
     */
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
     * Send a confirm message to the given address.
     * @param aSequenceNo the sequence no of the telegram to confirm.
     * @param aAddress the source address of the telegram to confirm.
     */
    KnxTpUartSendResult sendNCDPosConfirm(uint8_t aSequenceNo, uint16_t aAddress);

    /**
     * Read a single byte from serial interface with timeout.
     * @param aTimeout the timeout in ms to wait for a byte to get available on serial.
     * @return the read byte or -1 in case of timeout.
     */
    int serialRead(uint16_t aTimeout);

    /**
     * This method handles the ACK response on receive.
     * @return true if the telegram is of interest (ACK was send) or false if telegram is not of interest (NACK was send).
     */
    bool handleAck();

    /**
     * Send the given telegram to bus.
     * @param aTelegram the telegram to send.
     * @param aIsRepeat a flag to indicate if this is a repeated send.
     * @return true if send (and receive) was successful, false otherwise.
     */
    KnxTpUartSendResult sendTelegram(KnxTelegram* aTelegram, bool aIsRepeat);

    /**
     * Internal function used to wait for a send result while still reading serial data.
     */
    KnxTpUartSendResult waitForSendResult();

    /**
     * Internal function that checks if #mSerialEventCallback is given and if so sends the given
     */
    void sendSerialEventCallback(KnxTpUartSerialEventType aEventType, uint8_t aTpUartByte);

};

#endif
