// File: KnxTpUart.cpp
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 2021-02-09 by MiBiMiFlo

#include "KnxTpUart.h"



#ifdef TPUART_DEBUG
	#define DBG_PRINT(aMsg) ({if (_dbg != NULL) {_dbg->print(aMsg);}})
	#define DBG_PRINTLN(aMsg) ({if (_dbg != NULL) {_dbg->println(aMsg);}})
#else
	#define DBG_PRINT(aMsg) ({})
	#define DBG_PRINTLN(aMsg) ({})
#endif


/**
 * Code taken from https://www.norwegiancreations.com/2018/10/arduino-tutorial-avoiding-the-overflow-issue-when-using-millis-and-micros/
 * Simple way to check for a timeout.
 * This method uses 16bit values to reduce calculation overhead for small periods of time (up to ~ 64K time portions).
 * @param aNow the current time as returned by millis() or micros().
 * @param aStart the time value as returned by millis() or micros() at start.
 * @param aTimeout the compare period (e.g. timeout).
 * @return true if aNow is more than aTimeout after aStart.
 */
static inline bool isTimeout16(uint16_t aNow, uint16_t aStart, uint16_t aTimeout)
{
    return (aNow - aStart) > aTimeout;
}

KnxTpUart::KnxTpUart(Stream* sport, uint16_t aAddress)
{
    _serialport = sport;
    setIndividualAddress(aAddress);
    init();
}

void KnxTpUart::init(void)
{
    _tg = new KnxTelegram();
    _listen_to_broadcasts  = false;

    mTelegramCheckCallback = NULL;
    mSerialEventCallback   = NULL;
    mKnxTelegramCallback   = NULL;

    mConfigReg = 0;
    mStatusReg = 0;

    _serialport->setTimeout(SERIAL_READ_TELEGRAM_TIMEOUT_MS);

    #ifdef KNX_SUPPORT_LISTEN_GAS
        mListenGAs      = NULL;
        mListenGAsCount = 0;
        mListenGAsMax   = 0;
    #endif

	#ifdef TPUART_DEBUG
        _dbg = NULL;
	#endif
}


void KnxTpUart::setListenToBroadcasts(bool listen)
{
    _listen_to_broadcasts = listen;
}


bool KnxTpUart::uartReset(uint16_t aTimeout)
{
	DBG_PRINTLN(F("Will send TPUART_RESET to TUPART!"));
    const uint8_t sendByte = TPUART_RESET;

    const uint16_t start = (uint16_t)millis();

    // clear reset confirmed
    mStatusReg &= ~STATUS_RESET_CONFIRMED;

    // set to wait for reset confirmation
    mConfigReg |= CONFIG_WAIT_FOR_RESET_CONFIRMED;

    _serialport->write(sendByte);

    while (!isTimeout16((uint16_t)millis(), start, aTimeout))
    {
    	if ((mStatusReg & STATUS_RESET_CONFIRMED) == STATUS_RESET_CONFIRMED)
    	{
    		// reset was confirmed
    		return true;
    	}
    	serialEvent();
    }

	// timeout
	mConfigReg &= ~CONFIG_WAIT_FOR_RESET_CONFIRMED;
    return false;
}


uint8_t KnxTpUart::uartStateRequest(uint16_t aTimeout)
{
	DBG_PRINTLN(F("Will send TPUART_STATE_REQUEST to TUPART!"));
    const uint8_t sendByte = TPUART_STATE_REQUEST;

    mStateResponse = 0;
    const uint16_t start = (uint16_t)millis();

    // clear state response flag
    mStatusReg &= ~STATUS_STATE_RESPONSE_RECEIVED;

    // set to wait for status response
	mConfigReg |= CONFIG_WAIT_FOR_STATE_RESPONSE;

	_serialport->write(sendByte);

	while (!isTimeout16((uint16_t)millis(), start, aTimeout))
	{
		if ((mStatusReg & STATUS_STATE_RESPONSE_RECEIVED) == STATUS_STATE_RESPONSE_RECEIVED)
		{
			// status response was received.
			return mStateResponse;
		}
		serialEvent();
	}

	// timeout
	mConfigReg &= ~CONFIG_WAIT_FOR_STATE_RESPONSE;
	return 0;

}

void KnxTpUart::uartSetAddress()
{
	DBG_PRINTLN(F("Will send TPUART_SET_ADDRESS to TUPART!"));

	uint8_t buffer[3];
	buffer[0] = TPUART_SET_ADDRESS;
	buffer[1] = (mSourceAddress >> 8) & 0xFF;
	buffer[2] = (mSourceAddress & 0xFF);
	_serialport->write(buffer, 3);
}


uint8_t KnxTpUart::getStateResponse()
{
	return mStateResponse;
}

void KnxTpUart::setIndividualAddress(uint8_t area, uint8_t line, uint8_t member)
{
    mSourceAddress = KNX_IA(area, line, member);
}

void KnxTpUart::setIndividualAddress(uint16_t aAddress)
{
    mSourceAddress = aAddress;
}

uint16_t KnxTpUart::getIndividualAddress()
{
	return mSourceAddress;
}

void KnxTpUart::serialEvent()
{
	// we need a loop to be able to ignore received bytes
	while (true)
	{
		if (_serialport->available() <= 0)
		{
			return;
		}
		else
		{
			checkErrors();

			// check the next char (without remove from buffer)
			int incomingByte = _serialport->peek();
			printByte(incomingByte);

			if ((incomingByte | B00101100) == B10111100 )
			{
				// first byte of a KNX telegram --> read the telegram
				KnxTpUartSerialEventType readRes = readKNXTelegram();

				if (readRes == KNX_TELEGRAM)
				{
					DBG_PRINTLN(F("Event KNX_TELEGRAM"));
				}
				else if (readRes == IRRELEVANT_KNX_TELEGRAM)
				{
					DBG_PRINTLN(F("Event IRRELEVANT_KNX_TELEGRAM"));
				}
				else
				{
					// has to be timeout!
					DBG_PRINTLN(F("Read Timeout"));
				}
				sendSerialEventCallback(readRes, incomingByte);
				return;
			}
			else if (incomingByte == TPUART_RESET_INDICATION_BYTE)
			{
				serialRead(1);
				DBG_PRINTLN(F("Event TPUART_RESET_INDICATION"));

				// set reset confirmed
				mStatusReg |= STATUS_RESET_CONFIRMED;

				// clear wait for reset
				mConfigReg &= ~CONFIG_WAIT_FOR_RESET_CONFIRMED;

				sendSerialEventCallback(TPUART_RESET_INDICATION, incomingByte);
				return;
			}
			else if (incomingByte == TPUART_SEND_SUCCESS)
			{
				// TPUART_SEND_SUCCESS
				serialRead(1);
				DBG_PRINTLN(F("Event TPUART_SEND_SUCCESS"));

				mStatusReg |= STATUS_DATA_SEND_SUCCESS;
				mStatusReg &= ~(STATUS_DATA_SEND_NOT_SUCCESS|STATUS_DATA_SEND_TIMEOUT);

				mConfigReg &= ~CONFIG_WAIT_FOR_SEND_CONFIRM;

				sendSerialEventCallback(TPUART_RESPONSE_SEND_SUCCESS, incomingByte);
				return;
			}
			else if (incomingByte == TPUART_SEND_NOT_SUCCESS)
			{
				// TPUART_SEND_NOT_SUCCESS
				serialRead(1);
				DBG_PRINTLN(F("Event TPUART_SEND_NOT_SUCCESS"));

				sendSerialEventCallback(TPUART_RESPONSE_SEND_NOT_SUCCESS, incomingByte);
				return;
			}
			else if ((incomingByte & B00000111) == B00000111)
			{
				// STATE response
				mStateResponse = incomingByte;
				serialRead(1);
				DBG_PRINTLN(F("Event TPUART_STATE_RESPONSE"));

				mStatusReg |= STATUS_STATE_RESPONSE_RECEIVED;

				mConfigReg &= ~CONFIG_WAIT_FOR_STATE_RESPONSE;

				sendSerialEventCallback(TPUART_STATE_RESPONSE, incomingByte);
				return;
			}
			else if (incomingByte == 0x00)
			{
				// ignore 0x00 bytes at this point. those are often send before a RESET.
				serialRead(1);
				continue;
			}
			else
			{
				// this can happen in case a previous timeout occurred
				#ifdef TPUART_DEBUG
				if (_dbg!= NULL)
				{
					DBG_PRINT(F("Event UNKNOWN byte = b"));
					_dbg->println(incomingByte, BIN);
				}
				#endif
				serialRead(1);

				sendSerialEventCallback(UNKNOWN, incomingByte);
				return;
			}
			// we only loop in case of incomingByte == 0x00 --> lets return here
			return;
		}
	}
}

void KnxTpUart::checkErrors()
{
    #if defined(TPUART_DEBUG)
        #if defined(_SAM3XA_)  // For DUE
            if (USART1->US_CSR & US_CSR_OVRE)
            {
            	DBG_PRINTLN(F("Overrun"));
            }

            if (USART1->US_CSR & US_CSR_FRAME)
            {
            	DBG_PRINTLN(F("Frame Error"));
            }

            if (USART1->US_CSR & US_CSR_PARE)
            {
            	DBG_PRINTLN(F("Parity Error"));
            }
        #elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) // For UNO
            if (UCSR0A & B00010000)
            {
            	DBG_PRINTLN(F("Frame Error"));
            }

            if (UCSR0A & B00000100)
            {
            	DBG_PRINTLN(F("Parity Error"));
            }
        #elif defined(__AVR_ATtiny1614__) || defined(__AVR_ATtiny3216__) // For new Tiny
            // TODO: check if this works correct
            if (USART0.RXDATAH & B00000100)
            {
            	DBG_PRINTLN(F("Frame Error"));
            }

            if (USART0.RXDATAH & B00000010)
            {
            	DBG_PRINTLN(F("Parity Error"));
            }

            if (USART0.RXDATAH & B01000000)
            {
            	DBG_PRINTLN(F("Overrun"));
            }
        #else
            if (UCSR1A & B00010000)
            {
            	DBG_PRINTLN(F("Frame Error"));
            }

            if (UCSR1A & B00000100)
            {
            	DBG_PRINTLN(F("Parity Error"));
            }
        #endif
    #endif
}

void KnxTpUart::printByte(uint8_t aByte)
{
    #if defined(TPUART_DEBUG_V)
		if (_dbg != NULL)
		{
			_dbg->print("Incoming Byte: ");
			_dbg->print(aByte, DEC);
			_dbg->print(" - ");
			_dbg->print(aByte, HEX);
			_dbg->print(" - ");
			_dbg->print(aByte, BIN);
			_dbg->println();
		}
    #endif
}


bool KnxTpUart::handleAck()
{
    bool interested = false;


    if (_tg->getSourceAddress()!=mSourceAddress)
    {
    	// we only handle telegrams not send by ourself

		// fastest checks first
		// additionally broadcast is the most important one as it's for address assignment
		if (_tg->isTargetGroup())
		{
			// Broadcast (Programming Mode)
			interested |= (_listen_to_broadcasts && _tg->getTargetGroupAddress() == 0x0000);
		}
		else
		{
			// Physical address
			interested |= (_tg->getTargetAddress() == mSourceAddress);
		}

		if (!interested)
		{
			if (mTelegramCheckCallback != NULL)
			{
				interested |= mTelegramCheckCallback(_tg);
			}

			#ifdef KNX_SUPPORT_LISTEN_GAS
				if (!interested)
				{
					// Verify if we are interested in this message - GroupAddress
					interested = _tg->isTargetGroup() && isListeningToGroupAddress(_tg->getTargetGroupAddress());
				}
			#endif
		}
    }

    if (interested)
    {
        _serialport->write((uint8_t)TPUART_ACK);
    }
    else
    {
        _serialport->write((uint8_t)TPUART_NACK);
    }
    return interested;
}


KnxTpUartSerialEventType KnxTpUart::readKNXTelegram()
{
    // read 9 byte (minimum telegram length)
	size_t offs = 0;

	// read first 6 bytes (source and target address)
	while (offs < 6)
	{
		int read = _serialport->readBytes(_tg->getBuffer() + offs, 6 - offs);
		if (read > 0)
		{
			// at least one byte received
			offs += read;
		}
		else
		{
			//timeout
			// we do a uart reset and return a false
			uartReset(0);
			return TIMEOUT;
		}
	}

	// need to send ACK as soon as possible
	bool interested = handleAck();

	// now read the rest of the telegram
	uint8_t fullLen = 9 + _tg->getPayloadLength() - 2;
	while (offs < fullLen)
	{
		int read = _serialport->readBytes(_tg->getBuffer() + offs, fullLen - offs);
		if (read > 0)
		{
			offs += read;
		}
		else
		{
			//timeout
			// we do a uart reset and return a false
			uartReset(0);
			return TIMEOUT;
		}
	}


	#if defined(TPUART_DEBUG)
		if (_dbg != NULL)
		{
			// Print the received telegram
			_tg->print(_dbg);
		}
	#endif


    if (_tg->getCommunicationType() == KNX_COMM_UCD)
    {
        #if defined(TPUART_DEBUG)
		    DBG_PRINTLN(F("UCD Telegram received"));
	    #endif
    }
    else if (_tg->getCommunicationType() == KNX_COMM_NCD)
    {
		DBG_PRINT(F("NCD Telegram "));
		DBG_PRINT(_tg->getSequenceNumber());
		DBG_PRINTLN(F(" received"));
        if (interested)
        {
            // Thanks to Katja Blankenheim for the help
            sendNCDPosConfirm(_tg->getSequenceNumber(), _tg->getSourceAddress());
        }
    }

    if (mKnxTelegramCallback != NULL)
    {
    	mKnxTelegramCallback(_tg, interested);
    }

    // Returns if we are interested in this diagram
    return interested ? KNX_TELEGRAM : IRRELEVANT_KNX_TELEGRAM;
}

KnxTelegram* KnxTpUart::getReceivedTelegram()
{
    return _tg;
}

// Command Write

KnxTpUartSendResult KnxTpUart::groupWriteBool(uint16_t aAddress, bool aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, (aValue) ? 0x01 : 0x00);
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite4BitInt(uint16_t aAddress, uint8_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, (aValue & B00001111));
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite4BitDim(uint16_t aAddress, bool aDirection, uint8_t aSteps, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite1ByteInt(uint16_t aAddress, int8_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set1ByteIntValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite1ByteUInt(uint16_t aAddress, uint8_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set1ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite2ByteInt(uint16_t aAddress, int16_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteIntValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite2ByteUInt(uint16_t aAddress, uint16_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteUIntValue(aValue);
    _tg->createChecksum();
  	return sendTelegram(_tg);
}


KnxTpUartSendResult KnxTpUart::groupWrite4ByteInt(uint16_t aAddress, int32_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteIntValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite4ByteUInt(uint16_t aAddress, uint32_t aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite2ByteFloat(uint16_t aAddress, float aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set3ByteTime(aWeekday, aHour, aMinute, aSecond);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set3ByteDate(aDay, aMonth, aYear);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite4ByteFloat(uint16_t aAddress, float aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWrite14ByteText(uint16_t aAddress, String aValue, boolean aIsAnswer)
{
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set14ByteValue(aValue);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::groupWriteBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize, boolean aIsAnswer)
{
	if (aSize > 14)
	{
		return SEND_NOT_SUCCESSFUL;
	}
    createKNXMessageFrame(2, aIsAnswer ? KNX_COMMAND_ANSWER : KNX_COMMAND_WRITE, aAddress, 0);
    _tg->setValue(aBuffer, aSize);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

// Command Read

KnxTpUartSendResult KnxTpUart::groupRead(uint16_t aAddress) {
    createKNXMessageFrame(2, KNX_COMMAND_READ, aAddress, 0);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::individualAnswerAddress() {
    createKNXMessageFrame(2, KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE, 0x0000, 0);
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::individualAnswerMaskVersion(uint8_t area, uint8_t line, uint8_t member)
{
	return individualAnswerMaskVersion(KNX_IA(area, line, member));
}

KnxTpUartSendResult KnxTpUart::individualAnswerMaskVersion(uint16_t aAddress)
{
    createKNXMessageFrameIndividual(4, KNX_COMMAND_MASK_VERSION_RESPONSE, aAddress, 0);
    _tg->setCommunicationType(KNX_COMM_NDP);
    _tg->setBufferByte(8, 0x07); // Mask version part 1 for BIM M 112
    _tg->setBufferByte(9, 0x01); // Mask version part 2 for BIM M 112
    _tg->createChecksum();
    return sendTelegram(_tg);
}

KnxTpUartSendResult KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint8_t area, uint8_t line, uint8_t member)
{
	return individualAnswerAuth(accessLevel, sequenceNo, KNX_IA(area, line, member));
}

KnxTpUartSendResult KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint16_t aAddress)
{
    createKNXMessageFrameIndividual(3, KNX_COMMAND_ESCAPE, aAddress, KNX_EXT_COMMAND_AUTH_RESPONSE);
    _tg->setCommunicationType(KNX_COMM_NDP);
    _tg->setSequenceNumber(sequenceNo);
    _tg->setBufferByte(8, accessLevel);
    _tg->createChecksum();
    return sendTelegram(_tg);
}


void KnxTpUart::createKNXMessageFrame(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte)
{
	_tg->initKNXMessageFrame(mSourceAddress, payloadlength, command, aAddress, true, firstDataByte);
}


void KnxTpUart::createKNXMessageFrameIndividual(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte)
{
	_tg->initKNXMessageFrame(mSourceAddress, payloadlength, command, aAddress, false, firstDataByte);
}


KnxTpUartSendResult KnxTpUart::waitForSendResult()
{
    const uint16_t start = (uint16_t)millis();

    // mask of interesting status flags
    const uint8_t status_mask = STATUS_DATA_SEND_NOT_SUCCESS|STATUS_DATA_SEND_SUCCESS|STATUS_DATA_SEND_TIMEOUT|STATUS_RESET_CONFIRMED;

    // clear previous status
    // we also clear reset confirmation as sometimes reset happens on send
    mStatusReg &= ~status_mask;

    // set to wait for response
    mConfigReg |= CONFIG_WAIT_FOR_SEND_CONFIRM;

    while (!isTimeout16((uint16_t)millis(), start, 500))
    {
    	if ((mStatusReg & status_mask) != 0)
    	{
			if ((mStatusReg & STATUS_DATA_SEND_SUCCESS) == STATUS_DATA_SEND_SUCCESS)
			{
				// send successful
				return SEND_SUCCESSFUL;
			}

			if ((mStatusReg & STATUS_DATA_SEND_NOT_SUCCESS) == STATUS_DATA_SEND_NOT_SUCCESS)
			{
				// send not successful
				return SEND_NOT_SUCCESSFUL;
			}
			if ((mStatusReg & STATUS_RESET_CONFIRMED) == STATUS_RESET_CONFIRMED)
			{
				return SEND_NOT_SUCCESSFUL;
			}
    	}

    	serialEvent();
    }

    return SEND_TIMEOUT;
}

KnxTpUartSendResult KnxTpUart::sendNCDPosConfirm(uint8_t sequenceNo, uint16_t aAddress)
{
    KnxTelegram _tg_ptp;
    _tg_ptp.clear();
    _tg_ptp.setSourceAddress(mSourceAddress);
    _tg_ptp.setTargetIndividualAddress(aAddress);
    _tg_ptp.setSequenceNumber(sequenceNo);
    _tg_ptp.setCommunicationType(KNX_COMM_NCD);
    _tg_ptp.setControlData(KNX_CONTROLDATA_POS_CONFIRM);
    _tg_ptp.setPayloadLength(1);
    _tg_ptp.createChecksum();


    uint8_t messageSize = _tg_ptp.getTotalLength();

    uint8_t sendbuf[2];
    for (uint8_t i = 0; i < messageSize; i++)
    {
        if (i == (messageSize - 1))
        {
            sendbuf[0] = TPUART_DATA_END;
        }
        else
        {
            sendbuf[0] = TPUART_DATA_START_CONTINUE;
        }

        sendbuf[0] |= i;
        sendbuf[1] = _tg_ptp.getBufferByte(i);

        _serialport->write(sendbuf, 2);
    }

    return waitForSendResult();
}

KnxTpUartSendResult KnxTpUart::sendTelegram(KnxTelegram* aTelegram)
{
	return sendTelegramAndRepeat(aTelegram, 1);
}

KnxTpUartSendResult KnxTpUart::sendTelegramAndRepeat(KnxTelegram* aTelegram, uint8_t aCount)
{
	KnxTpUartSendResult res = sendTelegram(aTelegram, false);

	if (res == SEND_SUCCESSFUL)
	{
		return SEND_SUCCESSFUL;
	}

	#ifdef TPUART_DEBUG
		if (res == SEND_TIMEOUT)
		{
			DBG_PRINTLN(F("Send timeout."));
		}
		else
		{
			DBG_PRINTLN(F("Send unsuccessful."));
		}
	#endif

	for (uint8_t i = 0; i < aCount; i++)
	{
		res = sendTelegram(aTelegram, false);
		if (res == SEND_SUCCESSFUL)
		{
			return res;
		}

		#ifdef TPUART_DEBUG
			if (res == SEND_TIMEOUT)
			{
				DBG_PRINTLN(F("Send timeout."));
			}
			else
			{
				DBG_PRINTLN(F("Send unsuccessful."));
			}
		#endif

	}

	return res;
}



KnxTpUartSendResult KnxTpUart::sendTelegram(KnxTelegram* aTelegram, bool aIsRepeat)
{
	if (aTelegram->getSourceAddress() != mSourceAddress)
	{
		// enforce correct source address
		aTelegram->setSourceAddress(mSourceAddress);
	}

	// ensure valid checksum
	aTelegram->createChecksum();


    uint8_t messageSize = aTelegram->getTotalLength();

    uint8_t sendbuf[2];
    for (int i = 0; i < messageSize; i++)
    {
        if (i == (messageSize - 1))
        {
            sendbuf[0] = TPUART_DATA_END;
        }
        else
        {
            sendbuf[0] = TPUART_DATA_START_CONTINUE;
        }

        sendbuf[0] |= i;
        sendbuf[1] = aTelegram->getBufferByte(i);

        if (i==0)
        {
        	// enforce repeat flag
        	if (aIsRepeat)
        	{
        		// this is a repeated message so repeat flag should be cleared
				sendbuf[1] &= ~0x20;
        	}
        }

        _serialport->write(sendbuf, 2);
    }
    //_serialport->flush();


    return waitForSendResult();
}


int KnxTpUart::serialRead(uint16_t aTimeout)
{
    #if defined(TPUART_DEBUG_V)
		if (_dbg != NULL)
		{
			_dbg->print("Available: ");
			_dbg->println(_serialport->available());
		}
    #endif

    while (! (_serialport->available() > 0))
    {
    	if (aTimeout == 0)
    	{
            #if defined(TPUART_DEBUG)
    			if (_dbg != NULL)
				{
    				_dbg->println("Timeout while receiving message");
				}
            #endif
    		return -1;
    	}
    	aTimeout--;
    	delay(1);
    }

    int inByte = _serialport->read();
    checkErrors();
    printByte(inByte);

    return inByte;
}
/**/

uint16_t KnxTpUart::getGroupAddress(String aAddress)
{
	const char aDelimiter = '/';
	uint16_t addr = aAddress.substring(0, aAddress.indexOf(aDelimiter)).toInt();
	addr = addr << 5;
	addr |= aAddress.substring(aAddress.indexOf(aDelimiter) + 1, aAddress.length()).substring(0, aAddress.substring(aAddress.indexOf(aDelimiter) + 1, aAddress.length()).indexOf(aDelimiter)).toInt();
	addr = addr << 3;
	addr |= aAddress.substring(aAddress.lastIndexOf(aDelimiter) + 1, aAddress.length()).toInt();
	return addr;
}


uint16_t KnxTpUart::getSourceAddress(String aAddress)
{
	const char aDelimiter = '.';
	uint16_t addr = aAddress.substring(0, aAddress.indexOf(aDelimiter)).toInt();
	addr = addr << 4;
	addr |= aAddress.substring(aAddress.indexOf(aDelimiter) + 1, aAddress.length()).substring(0, aAddress.substring(aAddress.indexOf(aDelimiter) + 1, aAddress.length()).indexOf(aDelimiter)).toInt();
	addr = addr << 4;
	addr |= aAddress.substring(aAddress.lastIndexOf(aDelimiter) + 1, aAddress.length()).toInt();
	return addr;
}

void KnxTpUart::sendSerialEventCallback(KnxTpUartSerialEventType aEventType, uint8_t aTpUartByte)
{
	if (mSerialEventCallback != NULL)
	{
		mSerialEventCallback(aEventType, aTpUartByte);
	}
}

void KnxTpUart::setTelegramCheckCallback(KnxTelegramCheckType aCallback)
{
	mTelegramCheckCallback = aCallback;
}


void KnxTpUart::setSerialEventCallback(KnxSerialEventCallback aCallback)
{
	mSerialEventCallback = aCallback;
}

void KnxTpUart::setKnxTelegramCallback(KnxTelegramCallback aCallback)
{
	mKnxTelegramCallback=aCallback;
}


#ifdef KNX_SUPPORT_LISTEN_GAS

bool KnxTpUart::setListenAddressCount(uint8_t aCount)
{
	if (mListenGAs != NULL)
	{
		// free the previously allocated buffer
		free(mListenGAs);
		mListenGAs = NULL;
	}
	mListenGAsCount = 0;

	if (aCount > 0)
	{
		// allocate new buffer (2 bytes per address)
		mListenGAs      = (uint16_t *)malloc(2*aCount);
		if (mListenGAs == NULL)
		{
			// not possible to allocate buffer
			mListenGAsMax = 0;
			return false;
		}
	}

	mListenGAsMax   = aCount;
	return true;
}

bool KnxTpUart::addListenGroupAddress(uint16_t aAddress)
{
    if (mListenGAsCount >= mListenGAsMax)
    {
#if defined(TPUART_DEBUG)
    TPUART_DEBUG_PORT.println("Maximum number of listening addresses already added.");
#endif
    return false;
    }
    mListenGAs[mListenGAsCount] = aAddress;
    mListenGAsCount++;
    return true;
}

bool KnxTpUart::isListeningToGroupAddress(uint8_t main, uint8_t middle, uint8_t sub) {
    return isListeningToGroupAddress(KNX_GA(main, middle, sub));
}

bool KnxTpUart::isListeningToGroupAddress(uint16_t aAddress)
{
    for (int i = 0; i < mListenGAsCount; i++)
    {
        if (mListenGAs[i] == aAddress)
        {
        	return true;
        }
    }
    return false;
}

#endif


#ifdef TPUART_DEBUG
void KnxTpUart::setDebugPort(Stream * aStream)
{
	_dbg = aStream;
}
#endif
