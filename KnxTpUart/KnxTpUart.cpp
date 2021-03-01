// File: KnxTpUart.cpp
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 2021-02-09 by MiBiMiFlo

#include "KnxTpUart.h"

KnxTpUart::KnxTpUart(Stream* sport, String aAddress)
{
    _serialport = sport;
    setIndividualAddress(getSourceAddress(aAddress));
    init();
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

    // KNX telegram can be 23 byte --> 184 bit --> ~19.1ms in 9600 Bit/s TP
    // as we read in a loop we allow a telegram to be separated into two blocks of 10ms timeout
    _serialport->setTimeout(SERIAL_READ_TIMEOUT_MS);

    #ifdef KNX_SUPPORT_LISTEN_GAS
        mListenGAs      = NULL;
        mListenGAsCount = 0;
        mListenGAsMax   = 0;
    #endif

}

void KnxTpUart::setListenToBroadcasts(bool listen)
{
    _listen_to_broadcasts = listen;
}

void KnxTpUart::uartReset()
{
    const uint8_t sendByte = TPUART_RESET;
    _serialport->write(sendByte);
}

void KnxTpUart::uartStateRequest()
{
    const uint8_t sendByte = TPUART_STATE_REQUEST;
    _serialport->write(sendByte);
}

void KnxTpUart::setIndividualAddress(uint8_t area, uint8_t line, uint8_t member)
{
    mSourceAddress = KNX_IA(area, line, member);
}

void KnxTpUart::setIndividualAddress(uint16_t aAddress)
{
    mSourceAddress = aAddress;
}

KnxTpUartSerialEventType KnxTpUart::serialEvent()
{
    while (_serialport->available() > 0)
    {
        checkErrors();

        int incomingByte = _serialport->peek();
        printByte(incomingByte);

        if (isKNXControlByte(incomingByte & 0xFF))
        {
    	    KnxTpUartSerialEventType readRes = readKNXTelegram();
            if (readRes == KNX_TELEGRAM)
            {
				#if defined(TPUART_DEBUG)
            		TPUART_DEBUG_PORT.println("Event KNX_TELEGRAM");
				#endif
				return readRes;
            }
            else if (readRes == IRRELEVANT_KNX_TELEGRAM)
            {
				#if defined(TPUART_DEBUG)
					TPUART_DEBUG_PORT.println("Event IRRELEVANT_KNX_TELEGRAM");
				#endif
				return readRes;
            }
            else if (readRes == TIMEOUT)
            {
				#if defined(TPUART_DEBUG)
					TPUART_DEBUG_PORT.println("Read Timeout");
					return readRes;
				#endif
            }
        }
        else if (incomingByte == TPUART_RESET_INDICATION_BYTE)
        {
            serialRead();
			#if defined(TPUART_DEBUG)
			    TPUART_DEBUG_PORT.println("Event TPUART_RESET_INDICATION");
			#endif
		    return TPUART_RESET_INDICATION;
        }
        else
        {
            serialRead();
            #if defined(TPUART_DEBUG)
                TPUART_DEBUG_PORT.println("Event UNKNOWN");
            #endif
            return UNKNOWN;
        }
    }
    #if defined(TPUART_DEBUG)
        TPUART_DEBUG_PORT.println("Event UNKNOWN");
    #endif
    return UNKNOWN;
}


bool KnxTpUart::isKNXControlByte(uint8_t aByte)
{
    // Ignore repeat flag and priority flag
    return ( (aByte | B00101100) == B10111100 );
}

void KnxTpUart::checkErrors()
{
    #if defined(TPUART_DEBUG)
        #if defined(_SAM3XA_)  // For DUE
            if (USART1->US_CSR & US_CSR_OVRE)
            {
                TPUART_DEBUG_PORT.println("Overrun");
            }

            if (USART1->US_CSR & US_CSR_FRAME)
            {
                TPUART_DEBUG_PORT.println("Frame Error");
            }

            if (USART1->US_CSR & US_CSR_PARE)
            {
                TPUART_DEBUG_PORT.println("Parity Error");
            }
        #elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) // For UNO
            if (UCSR0A & B00010000)
            {
                TPUART_DEBUG_PORT.println("Frame Error");
            }

            if (UCSR0A & B00000100)
            {
                TPUART_DEBUG_PORT.println("Parity Error");
            }
        #elif defined(__AVR_ATtiny1614__) || defined(__AVR_ATtiny3216__) // For new Tiny
            // TODO: check if this works correct
            if (USART0.RXDATAH & B00000100)
            {
                TPUART_DEBUG_PORT.println("Frame Error");
            }

            if (USART0.RXDATAH & B00000010)
            {
                TPUART_DEBUG_PORT.println("Parity Error");
            }

            if (USART0.RXDATAH & B01000000)
            {
                TPUART_DEBUG_PORT.println("Overrun");
            }
        #else
            if (UCSR1A & B00010000)
            {
                TPUART_DEBUG_PORT.println("Frame Error");
            }

            if (UCSR1A & B00000100)
            {
                TPUART_DEBUG_PORT.println("Parity Error");
            }
        #endif
    #endif
}

void KnxTpUart::printByte(uint8_t aByte)
{
    #if defined(TPUART_DEBUG)
        TPUART_DEBUG_PORT.print("Incoming Byte: ");
        TPUART_DEBUG_PORT.print(aByte, DEC);
        TPUART_DEBUG_PORT.print(" - ");
        TPUART_DEBUG_PORT.print(aByte, HEX);
        TPUART_DEBUG_PORT.print(" - ");
        TPUART_DEBUG_PORT.print(aByte, BIN);
        TPUART_DEBUG_PORT.println();
    #endif
}

KnxTpUartSerialEventType KnxTpUart::readKNXTelegram()
{
    // read 9 byte (minimum telegram length)
	size_t offs = 0;

	while (offs < 9)
	{
		int read = _serialport->readBytes(_tg->getBuffer() + offs, 9 - offs);
		if (read > 0)
		{
			// at least one byte received
			offs += read;
		}
		else
		{
			//timeout
			// we do a uart reset and return a false
			uartReset();
			return TIMEOUT;
		}
	}

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
			uartReset();
			return TIMEOUT;
		}
	}

    bool interested = false;

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

    if (interested)
    {
        sendAck();
    }
    else
    {
        sendNotAddressed();
    }

	#if defined(TPUART_DEBUG)
		// Print the received telegram
		_tg->print(&TPUART_DEBUG_PORT);
	#endif


    if (_tg->getCommunicationType() == KNX_COMM_UCD)
    {
        #if defined(TPUART_DEBUG)
		    TPUART_DEBUG_PORT.println("UCD Telegram received");
	    #endif
    }
    else if (_tg->getCommunicationType() == KNX_COMM_NCD)
    {
        #if defined(TPUART_DEBUG)
            TPUART_DEBUG_PORT.print("NCD Telegram ");
            TPUART_DEBUG_PORT.print(_tg->getSequenceNumber());
            TPUART_DEBUG_PORT.println(" received");
        #endif
        if (interested)
        {
            // Thanks to Katja Blankenheim for the help
            sendNCDPosConfirm(_tg->getSequenceNumber(), _tg->getSourceAddress());
        }
    }

    // Returns if we are interested in this diagram
    return interested ? KNX_TELEGRAM : IRRELEVANT_KNX_TELEGRAM;
}

KnxTelegram* KnxTpUart::getReceivedTelegram()
{
    return _tg;
}

// Command Write

bool KnxTpUart::groupWriteBool(String aAddress, bool aValue)
{
	return groupWriteBool(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWriteBool(uint16_t aAddress, bool aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, (aValue) ? 0x01 : 0x00);
    return sendMessage();
}

bool KnxTpUart::groupWrite4BitInt(String aAddress, uint8_t aValue)
{
	return groupWrite4BitInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite4BitInt(uint16_t aAddress, uint8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, (aValue & B00001111));
    return sendMessage();
}

bool KnxTpUart::groupWrite4BitDim(String aAddress, bool aDirection, uint8_t aSteps)
{
	return groupWrite4BitDim(getGroupAddress(aAddress), aDirection, aSteps);
}

bool KnxTpUart::groupWrite4BitDim(uint16_t aAddress, bool aDirection, uint8_t aSteps)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
    return sendMessage();
}


bool KnxTpUart::groupWrite1ByteInt(String aAddress, int8_t aValue)
{
	return groupWrite1ByteInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite1ByteInt(uint16_t aAddress, int8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set1ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite1ByteUInt(String aAddress, uint8_t aValue)
{
	return groupWrite1ByteUInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite1ByteUInt(uint16_t aAddress, uint8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set1ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupWrite2ByteInt(String aAddress, int16_t aValue)
{
	return groupWrite2ByteInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite2ByteInt(uint16_t aAddress, int16_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite2ByteUInt(uint16_t aAddress, uint16_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteUIntValue(aValue);
    _tg->createChecksum();
  	return sendMessage();
}

bool KnxTpUart::groupWrite2ByteUInt(String aAddress, uint16_t aValue)
{
	return groupWrite2ByteUInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite4ByteInt(uint16_t aAddress, int32_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite4ByteUInt(uint16_t aAddress, uint32_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupWrite2ByteFloat(String aAddress, float aValue)
{
	return groupWrite2ByteFloat(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite2ByteFloat(uint16_t aAddress, float aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set2ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond)
{
	return groupWrite3ByteTime(getGroupAddress(aAddress), aWeekday, aHour, aMinute, aSecond);
}

bool KnxTpUart::groupWrite3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t aSecond)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set3ByteTime(aWeekday, aHour, aMinute, aSecond);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupWrite3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear)
{
	return groupWrite3ByteDate(getGroupAddress(aAddress), aDay, aMonth, aYear);
}

bool KnxTpUart::groupWrite3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set3ByteDate(aDay, aMonth, aYear);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite4ByteFloat(String aAddress, float aValue)
{
	return groupWrite4ByteFloat(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite4ByteFloat(uint16_t aAddress, float aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set4ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWrite14ByteText(String aAddress, String aValue)
{
	return groupWrite14ByteText(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupWrite14ByteText(uint16_t aAddress, String aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->set14ByteValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupWriteBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize)
{
	if (aSize > 14)
	{
		return false;
	}
    createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
    _tg->setValue(aBuffer, aSize);
    _tg->createChecksum();
    return sendMessage();
}

// Command Answer

bool KnxTpUart::groupAnswerBool(String aAddress, bool aValue)
{
	return groupAnswerBool(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswerBool(uint16_t aAddress, bool aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, aValue ? 0x01 : 0x00);
    return sendMessage();
}


bool KnxTpUart::groupAnswer4BitInt(String aAddress, uint8_t aValue)
{
	return groupAnswer4BitInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer4BitInt(uint16_t aAddress, uint8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, (aValue & B00001111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer4BitDim(String aAddress, bool aDirection, uint8_t aSteps)
{
	return groupAnswer4BitDim(getGroupAddress(aAddress), aDirection, aSteps);
}

bool KnxTpUart::groupAnswer4BitDim(uint16_t aAddress, bool aDirection, uint8_t aSteps)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer1ByteInt(String aAddress, int8_t aValue)
{
	return groupAnswer1ByteInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer1ByteInt(uint16_t aAddress, int8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set1ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupAnswer1ByteUInt(String aAddress, uint8_t aValue)
{
	return groupAnswer1ByteUInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer1ByteUInt(uint16_t aAddress, uint8_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set1ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupAnswer2ByteInt(String aAddress, int16_t aValue)
{
	return groupAnswer2ByteInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer2ByteInt(uint16_t aAddress, int16_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set2ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteUInt(String aAddress, uint16_t aValue)
{
	return groupAnswer2ByteUInt(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer2ByteUInt(uint16_t aAddress, uint16_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set2ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer4ByteInt(uint16_t aAddress, int32_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set4ByteIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer4ByteUInt(uint16_t aAddress, uint32_t aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set4ByteUIntValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteFloat(String aAddress, float aValue)
{
    return groupAnswer2ByteFloat(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer2ByteFloat(uint16_t aAddress, float aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set2ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteTime(String aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t sSecond)
{
	return groupAnswer3ByteTime(getGroupAddress(aAddress), aWeekday, aHour, aMinute, sSecond);
}

bool KnxTpUart::groupAnswer3ByteTime(uint16_t aAddress, uint8_t aWeekday, uint8_t aHour, uint8_t aMinute, uint8_t sSecond)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set3ByteTime(aWeekday, aHour, aMinute, sSecond);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteDate(String aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear)
{
	return groupAnswer3ByteDate(getGroupAddress(aAddress), aDay, aMonth, aYear);
}

bool KnxTpUart::groupAnswer3ByteDate(uint16_t aAddress, uint8_t aDay, uint8_t aMonth, uint8_t aYear)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set3ByteDate(aDay, aMonth, aYear);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupAnswer4ByteFloat(String aAddress, float aValue)
{
    return groupAnswer4ByteFloat(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer4ByteFloat(uint16_t aAddress, float aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set4ByteFloatValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::groupAnswer14ByteText(String aAddress, String aValue)
{
	return groupAnswer14ByteText(getGroupAddress(aAddress), aValue);
}

bool KnxTpUart::groupAnswer14ByteText(uint16_t aAddress, String aValue)
{
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->set14ByteValue(aValue);
    _tg->createChecksum();
    return sendMessage();
}


bool KnxTpUart::groupAnswerBuffer(uint16_t aAddress, uint8_t* aBuffer, uint8_t aSize)
{
	if (aSize > 14)
	{
		return false;
	}
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, 0);
    _tg->setValue(aBuffer, aSize);
    _tg->createChecksum();
    return sendMessage();
}

// Command Read

bool KnxTpUart::groupRead(String aAddress)
{
	return groupRead(getGroupAddress(aAddress));
}

bool KnxTpUart::groupRead(uint16_t aAddress) {
    createKNXMessageFrame(2, KNX_COMMAND_READ, aAddress, 0);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::individualAnswerAddress() {
    createKNXMessageFrame(2, KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE, 0x0000, 0);
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::individualAnswerMaskVersion(uint8_t area, uint8_t line, uint8_t member)
{
	return individualAnswerMaskVersion(KNX_IA(area, line, member));
}

bool KnxTpUart::individualAnswerMaskVersion(uint16_t aAddress)
{
    createKNXMessageFrameIndividual(4, KNX_COMMAND_MASK_VERSION_RESPONSE, aAddress, 0);
    _tg->setCommunicationType(KNX_COMM_NDP);
    _tg->setBufferByte(8, 0x07); // Mask version part 1 for BIM M 112
    _tg->setBufferByte(9, 0x01); // Mask version part 2 for BIM M 112
    _tg->createChecksum();
    return sendMessage();
}

bool KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint8_t area, uint8_t line, uint8_t member)
{
	return individualAnswerAuth(accessLevel, sequenceNo, KNX_IA(area, line, member));
}

bool KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint16_t aAddress)
{
    createKNXMessageFrameIndividual(3, KNX_COMMAND_ESCAPE, aAddress, KNX_EXT_COMMAND_AUTH_RESPONSE);
    _tg->setCommunicationType(KNX_COMM_NDP);
    _tg->setSequenceNumber(sequenceNo);
    _tg->setBufferByte(8, accessLevel);
    _tg->createChecksum();
    return sendMessage();
}

void KnxTpUart::createKNXMessageFrame(uint8_t payloadlength, KnxCommandType command, String aAddress, uint8_t firstDataByte)
{
    return createKNXMessageFrame(payloadlength, command, getGroupAddress(aAddress), firstDataByte);
}


void KnxTpUart::createKNXMessageFrame(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte)
{
    _tg->clear();
    _tg->setSourceAddress(mSourceAddress);
    _tg->setTargetGroupAddress(aAddress);
    _tg->setFirstDataByte(firstDataByte);
    _tg->setCommand(command);
    _tg->setPayloadLength(payloadlength);
    _tg->createChecksum();
}


void KnxTpUart::createKNXMessageFrameIndividual(uint8_t payloadlength, KnxCommandType command, String aAddress, uint8_t firstDataByte)
{
    return createKNXMessageFrameIndividual(payloadlength, command, getSourceAddress(aAddress), firstDataByte);
}


void KnxTpUart::createKNXMessageFrameIndividual(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte)
{
    _tg->clear();
    _tg->setSourceAddress(mSourceAddress);
    _tg->setTargetIndividualAddress(aAddress);
    _tg->setFirstDataByte(firstDataByte);
    _tg->setCommand(command);
    _tg->setPayloadLength(payloadlength);
    _tg->createChecksum();
}


bool KnxTpUart::sendNCDPosConfirm(uint8_t sequenceNo, uint16_t aAddress)
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


    int confirmation;
    while (true)
    {
        confirmation = serialRead();
        if (confirmation == TPUART_SEND_SUCCESS)
        {
            return true; // Sent successfully
        }
        else if (confirmation == TPUART_SEND_NOT_SUCCESS)
        {
            return false;
        }
        else if (confirmation == -1)
        {
            // Read timeout
            return false;
        }
    }

    return false;
}

bool KnxTpUart::sendMessage()
{
    return sendTelegram(_tg);
}

bool KnxTpUart::sendTelegram(KnxTelegram* aTelegram)
{
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

        _serialport->write(sendbuf, 2);
    }


    int confirmation;
    bool res;
    while (true)
    {
        confirmation = serialRead();
        if (confirmation == TPUART_SEND_SUCCESS)
        {
        	// Sent successfully
            res = true;
            break;
        }
        else if (confirmation == TPUART_SEND_NOT_SUCCESS)
        {
        	// Sent unsuccessfully
            res = false;
            break;
        }
        else if (confirmation == -1)
        {
            // Read timeout
            res = false;
            break;
        }
    }

#if defined(SERIAL_WRITE_DELAY_MS)
    //TODO: change to wait on next if needed but do not always delay
    delay(SERIAL_WRITE_DELAY_MS);
#endif
    return res;
}


void KnxTpUart::sendAck()
{
    uint8_t sendByte = TPUART_ACK;
    _serialport->write(sendByte);
}


void KnxTpUart::sendNotAddressed()
{
    uint8_t sendByte = TPUART_NACK;
    _serialport->write(sendByte);
}

int KnxTpUart::serialRead() {
  unsigned long startTime = millis();
#if defined(TPUART_DEBUG)
  TPUART_DEBUG_PORT.print("Available: ");
  TPUART_DEBUG_PORT.println(_serialport->available());
#endif

  while (! (_serialport->available() > 0)) {
    if (abs(millis() - startTime) > SERIAL_READ_TIMEOUT_MS) {
      // Timeout
#if defined(TPUART_DEBUG)
      TPUART_DEBUG_PORT.println("Timeout while receiving message");
#endif
      return -1;
    }
    delay(1);
  }

  int inByte = _serialport->read();
  checkErrors();
  printByte(inByte);

  return inByte;
}


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

void KnxTpUart::setTelegramCheckCallback(KnxTelegramCheckType aCallback)
{
	mTelegramCheckCallback = aCallback;
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


bool KnxTpUart::addListenGroupAddress(String aAddress) {
	return addListenGroupAddress(getGroupAddress(aAddress));
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
