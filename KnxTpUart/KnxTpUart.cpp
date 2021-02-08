// File: KnxTpUart.cpp
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)

// Last modified: 06.06.2017

#include "KnxTpUart.h"

KnxTpUart::KnxTpUart(TPUART_SERIAL_CLASS* sport, String address) {
    _serialport = sport;
    setIndividualAddress(getSourceAddress(address));
    init();
}

KnxTpUart::KnxTpUart(TPUART_SERIAL_CLASS* sport, uint16_t aAddress) {
    _serialport = sport;
    setIndividualAddress(aAddress);
    init();
}

void KnxTpUart::init(void)
{
    _tg = new KnxTelegram();
    _listen_to_broadcasts = false;
    mTelegramCheckCallback = NULL;

#if defined(MAX_LISTEN_GROUP_ADDRESSES)
    _listen_group_address_count = 0;
#endif

}

void KnxTpUart::setListenToBroadcasts(bool listen) {
    _listen_to_broadcasts = listen;
}

void KnxTpUart::uartReset() {
    byte sendByte = 0x01;
    _serialport->write(sendByte);
}

void KnxTpUart::uartStateRequest() {
  byte sendByte = 0x02;
  _serialport->write(sendByte);
}

void KnxTpUart::setIndividualAddress(uint8_t area, uint8_t line, uint8_t member) {
    mSourceAddress = KNX_IA(area, line, member);
}

void KnxTpUart::setIndividualAddress(uint16_t aAddress) {
    mSourceAddress = aAddress;
}

KnxTpUartSerialEventType KnxTpUart::serialEvent() {
  while (_serialport->available() > 0) {
    checkErrors();

    int incomingByte = _serialport->peek();
    printByte(incomingByte);

    if (isKNXControlByte(incomingByte)) {
      bool interested = readKNXTelegram();
      if (interested) {
#if defined(TPUART_DEBUG)
        TPUART_DEBUG_PORT.println("Event KNX_TELEGRAM");
#endif
        return KNX_TELEGRAM;
      }
      else {
#if defined(TPUART_DEBUG)
        TPUART_DEBUG_PORT.println("Event IRRELEVANT_KNX_TELEGRAM");
#endif
        return IRRELEVANT_KNX_TELEGRAM;
      }
    }
    else if (incomingByte == TPUART_RESET_INDICATION_BYTE) {
      serialRead();
#if defined(TPUART_DEBUG)
      TPUART_DEBUG_PORT.println("Event TPUART_RESET_INDICATION");
#endif
      return TPUART_RESET_INDICATION;
    }
    else {
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


bool KnxTpUart::isKNXControlByte(uint8_t aByte) {
    // Ignore repeat flag and priority flag
	return ( (aByte | B00101100) == B10111100 );
}

void KnxTpUart::checkErrors() {
#if defined(TPUART_DEBUG)
#if defined(_SAM3XA_)  // For DUE
  if (USART1->US_CSR & US_CSR_OVRE) {
    TPUART_DEBUG_PORT.println("Overrun");
  }

  if (USART1->US_CSR & US_CSR_FRAME) {
    TPUART_DEBUG_PORT.println("Frame Error");
  }

  if (USART1->US_CSR & US_CSR_PARE) {
    TPUART_DEBUG_PORT.println("Parity Error");
  }
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) // For UNO
  if (UCSR0A & B00010000) {
    TPUART_DEBUG_PORT.println("Frame Error");
  }

  if (UCSR0A & B00000100) {
    TPUART_DEBUG_PORT.println("Parity Error");
  }
#elif defined(__AVR_ATtiny1614__) || defined(__AVR_ATtiny3216__) // For new Tiny
    // TODO: check if this works correct
    if (USART0.RXDATAH & B00000100) {
      TPUART_DEBUG_PORT.println("Frame Error");
    }

    if (USART0.RXDATAH & B00000010) {
      TPUART_DEBUG_PORT.println("Parity Error");
    }

    if (USART0.RXDATAH & B01000000) {
      TPUART_DEBUG_PORT.println("Overrun");
    }
#else
  if (UCSR1A & B00010000) {
    TPUART_DEBUG_PORT.println("Frame Error");
  }

  if (UCSR1A & B00000100) {
    TPUART_DEBUG_PORT.println("Parity Error");
  }
#endif
#endif
}

void KnxTpUart::printByte(uint8_t aByte) {
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

bool KnxTpUart::readKNXTelegram() {
  // Receive header
  for (int i = 0; i < 6; i++) {
    _tg->setBufferByte(i, serialRead());
  }

#if defined(TPUART_DEBUG)
  TPUART_DEBUG_PORT.print("Payload Length: ");
  TPUART_DEBUG_PORT.println(_tg->getPayloadLength());
#endif
  int bufpos = 6;
  for (int i = 0; i < _tg->getPayloadLength(); i++) {
    _tg->setBufferByte(bufpos, serialRead());
    bufpos++;
  }

  // Checksum
  _tg->setBufferByte(bufpos, serialRead());

#if defined(TPUART_DEBUG)
  // Print the received telegram
  _tg->print(&TPUART_DEBUG_PORT);
#endif

  bool interested = false;
  if (mTelegramCheckCallback != NULL)
  {
	  interested |=mTelegramCheckCallback(_tg);
  }

  if (!interested)
  {
	  // Verify if we are interested in this message - GroupAddress
	  interested = _tg->isTargetGroup() && isListeningToGroupAddress(_tg->getTargetGroupAddress());
  }

  // Physical address
  interested |= ((!_tg->isTargetGroup()) && _tg->getTargetAddress() == mSourceAddress);

  // Broadcast (Programming Mode)
  interested |= (_listen_to_broadcasts && _tg->isTargetGroup() && _tg->getTargetGroupAddress() == 0x0000);

  if (interested)
  {
      sendAck();
  }
  else
  {
      sendNotAddressed();
  }

  if (_tg->getCommunicationType() == KNX_COMM_UCD) {
#if defined(TPUART_DEBUG)
    TPUART_DEBUG_PORT.println("UCD Telegram received");
#endif
  }
  else if (_tg->getCommunicationType() == KNX_COMM_NCD) {
#if defined(TPUART_DEBUG)
    TPUART_DEBUG_PORT.print("NCD Telegram ");
    TPUART_DEBUG_PORT.print(_tg->getSequenceNumber());
    TPUART_DEBUG_PORT.println(" received");
#endif
    if (interested) {
        // Thanks to Katja Blankenheim for the help
        sendNCDPosConfirm(_tg->getSequenceNumber(), _tg->getSourceAddress());
    }
  }

  // Returns if we are interested in this diagram
  return interested;
}

KnxTelegram* KnxTpUart::getReceivedTelegram() {
  return _tg;
}

// Command Write

bool KnxTpUart::groupWriteBool(String Address, bool aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, (aValue) ? 0x01 : 0x00);
  return sendMessage();
}

bool KnxTpUart::groupWriteBool(uint16_t aAddress, bool aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, (aValue) ? 0x01 : 0x00);
  return sendMessage();
}

bool KnxTpUart::groupWrite4BitInt(String aAddress, uint8_t aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, (aValue & B00001111));
  return sendMessage();
}

bool KnxTpUart::groupWrite4BitInt(uint16_t aAddress, uint8_t aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, (aValue & B00001111));
  return sendMessage();
}

bool KnxTpUart::groupWrite4BitDim(String aAddress, bool aDirection, byte aSteps) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
  return sendMessage();
}

bool KnxTpUart::groupWrite4BitDim(uint16_t aAddress, bool aDirection, byte aSteps) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
  return sendMessage();
}


bool KnxTpUart::groupWrite1ByteInt(String aAddress, int8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, aAddress, 0);
  _tg->set1ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite1ByteInt(uint16_t Address, int8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set1ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite1ByteUInt(String Address, uint8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set1ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite1ByteUInt(uint16_t Address, uint8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set1ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupWrite2ByteInt(String Address, int16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite2ByteInt(uint16_t Address, int16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite2ByteUInt(uint16_t Address, uint16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite2ByteUInt(String Address, uint16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite4ByteInt(uint16_t Address, int32_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set4ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite4ByteUInt(uint16_t Address, uint32_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set4ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupWrite2ByteFloat(String Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite2ByteFloat(uint16_t Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set2ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite3ByteTime(String Address, uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set3ByteTime(weekday, hour, minute, second);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite3ByteTime(uint16_t Address, uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set3ByteTime(weekday, hour, minute, second);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupWrite3ByteDate(String Address, uint8_t day, uint8_t month, uint8_t year) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set3ByteDate(day, month, year);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite3ByteDate(uint16_t Address, uint8_t day, uint8_t month, uint8_t year) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set3ByteDate(day, month, year);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite4ByteFloat(String Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set4ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite4ByteFloat(uint16_t Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set4ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite14ByteText(String Address, String value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set14ByteValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWrite14ByteText(uint16_t Address, String value) {
  createKNXMessageFrame(2, KNX_COMMAND_WRITE, Address, 0);
  _tg->set14ByteValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupWriteBuffer(uint16_t aAddress, byte* aBuffer, uint8_t aSize)
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

bool KnxTpUart::groupAnswerBool(String aAddress, bool aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, aValue ? 0x01 : 0x00);
  return sendMessage();
}

bool KnxTpUart::groupAnswerBool(uint16_t aAddress, bool aValue) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, aValue ? 0x01 : 0x00);
  return sendMessage();
}


bool KnxTpUart::groupAnswer4BitInt(String aAddress, uint8_t aValue) {
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, (aValue & B00001111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer4BitInt(uint16_t aAddress, uint8_t aValue) {
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, (aValue & B00001111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer4BitDim(String aAddress, bool aDirection, byte aSteps) {
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer4BitDim(uint16_t aAddress, bool aDirection, byte aSteps) {
    createKNXMessageFrame(2, KNX_COMMAND_ANSWER, aAddress, ((aDirection & 0x01) << 3) | (aSteps & B00000111));
    return sendMessage();
}

bool KnxTpUart::groupAnswer1ByteInt(String Address, int8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set1ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer1ByteInt(uint16_t Address, int8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set1ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupAnswer1ByteUInt(String Address, uint8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set1ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer1ByteUInt(uint16_t Address, uint8_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set1ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupAnswer2ByteInt(String Address, int16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteInt(uint16_t Address, int16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteUInt(String Address, uint16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteUInt(uint16_t Address, uint16_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer4ByteInt(uint16_t Address, int32_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set4ByteIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer4ByteUInt(uint16_t Address, uint32_t value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set4ByteUIntValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteFloat(String Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer2ByteFloat(uint16_t Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set2ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteTime(String Address, uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set3ByteTime(weekday, hour, minute, second);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteTime(uint16_t Address, uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set3ByteTime(weekday, hour, minute, second);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteDate(String Address, uint8_t day, uint8_t month, uint8_t year) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set3ByteDate(day, month, year);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer3ByteDate(uint16_t Address, uint8_t day, uint8_t month, uint8_t year) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set3ByteDate(day, month, year);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupAnswer4ByteFloat(String Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set4ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer4ByteFloat(uint16_t Address, float value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set4ByteFloatValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer14ByteText(String Address, String value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set14ByteValue(value);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupAnswer14ByteText(uint16_t Address, String value) {
  createKNXMessageFrame(2, KNX_COMMAND_ANSWER, Address, 0);
  _tg->set14ByteValue(value);
  _tg->createChecksum();
  return sendMessage();
}


bool KnxTpUart::groupAnswerBuffer(uint16_t aAddress, byte* aBuffer, uint8_t aSize)
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

bool KnxTpUart::groupRead(String Address) {
  createKNXMessageFrame(2, KNX_COMMAND_READ, Address, 0);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::groupRead(uint16_t Address) {
  createKNXMessageFrame(2, KNX_COMMAND_READ, Address, 0);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::individualAnswerAddress() {
  createKNXMessageFrame(2, KNX_COMMAND_INDIVIDUAL_ADDR_RESPONSE, 0x0000, 0);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::individualAnswerMaskVersion(uint8_t area, uint8_t line, uint8_t member) {
  createKNXMessageFrameIndividual(4, KNX_COMMAND_MASK_VERSION_RESPONSE, KNX_IA(area, line, member), 0);
  _tg->setCommunicationType(KNX_COMM_NDP);
  _tg->setBufferByte(8, 0x07); // Mask version part 1 for BIM M 112
  _tg->setBufferByte(9, 0x01); // Mask version part 2 for BIM M 112
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::individualAnswerMaskVersion(uint16_t aAddress) {
  createKNXMessageFrameIndividual(4, KNX_COMMAND_MASK_VERSION_RESPONSE, aAddress, 0);
  _tg->setCommunicationType(KNX_COMM_NDP);
  _tg->setBufferByte(8, 0x07); // Mask version part 1 for BIM M 112
  _tg->setBufferByte(9, 0x01); // Mask version part 2 for BIM M 112
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint8_t area, uint8_t line, uint8_t member) {



  createKNXMessageFrameIndividual(3, KNX_COMMAND_ESCAPE, KNX_IA(area, line, member), KNX_EXT_COMMAND_AUTH_RESPONSE);
  _tg->setCommunicationType(KNX_COMM_NDP);
  _tg->setSequenceNumber(sequenceNo);
  _tg->setBufferByte(8, accessLevel);
  _tg->createChecksum();
  return sendMessage();
}

bool KnxTpUart::individualAnswerAuth(uint8_t accessLevel, uint8_t sequenceNo, uint16_t aAddress) {
  createKNXMessageFrameIndividual(3, KNX_COMMAND_ESCAPE, aAddress, KNX_EXT_COMMAND_AUTH_RESPONSE);
  _tg->setCommunicationType(KNX_COMM_NDP);
  _tg->setSequenceNumber(sequenceNo);
  _tg->setBufferByte(8, accessLevel);
  _tg->createChecksum();
  return sendMessage();
}

void KnxTpUart::createKNXMessageFrame(uint8_t payloadlength, KnxCommandType command, String address, uint8_t firstDataByte) {
  uint16_t ga = getGroupAddress(address);

  _tg->clear();
  _tg->setSourceAddress(mSourceAddress);
  _tg->setTargetGroupAddress(ga);
  _tg->setFirstDataByte(firstDataByte);
  _tg->setCommand(command);
  _tg->setPayloadLength(payloadlength);
  _tg->createChecksum();
}


void KnxTpUart::createKNXMessageFrame(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte) {
  _tg->clear();
  _tg->setSourceAddress(mSourceAddress);
  _tg->setTargetGroupAddress(aAddress);
  _tg->setFirstDataByte(firstDataByte);
  _tg->setCommand(command);
  _tg->setPayloadLength(payloadlength);
  _tg->createChecksum();
}


void KnxTpUart::createKNXMessageFrameIndividual(uint8_t payloadlength, KnxCommandType command, String address, uint8_t firstDataByte) {
  uint16_t ia = getSourceAddress(address);
  _tg->clear();
  _tg->setSourceAddress(mSourceAddress);
  _tg->setTargetIndividualAddress(ia);
  _tg->setFirstDataByte(firstDataByte);
  _tg->setCommand(command);
  _tg->setPayloadLength(payloadlength);
  _tg->createChecksum();
}


void KnxTpUart::createKNXMessageFrameIndividual(uint8_t payloadlength, KnxCommandType command, uint16_t aAddress, uint8_t firstDataByte) {
  _tg->clear();
  _tg->setSourceAddress(mSourceAddress);
  _tg->setTargetIndividualAddress(aAddress);
  _tg->setFirstDataByte(firstDataByte);
  _tg->setCommand(command);
  _tg->setPayloadLength(payloadlength);
  _tg->createChecksum();
}


bool KnxTpUart::sendNCDPosConfirm(uint8_t sequenceNo, uint16_t aAddress) {
    KnxTelegram _tg_ptp;
    _tg_ptp.clear();
    _tg_ptp.setSourceAddress(mSourceAddress);
    _tg_ptp.setTargetIndividualAddress(aAddress);
    _tg_ptp.setSequenceNumber(sequenceNo);
    _tg_ptp.setCommunicationType(KNX_COMM_NCD);
    _tg_ptp.setControlData(KNX_CONTROLDATA_POS_CONFIRM);
    _tg_ptp.setPayloadLength(1);
    _tg_ptp.createChecksum();


    int messageSize = _tg_ptp.getTotalLength();

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
        sendbuf[1] = _tg_ptp.getBufferByte(i);

        _serialport->write(sendbuf, 2);
    }


    int confirmation;
    while (true)
    {
        confirmation = serialRead();
        if (confirmation == B10001011)
        {
            return true; // Sent successfully
        }
        else if (confirmation == B00001011)
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

bool KnxTpUart::sendTelegram(KnxTelegram* aTelegram) {
  int messageSize = aTelegram->getTotalLength();

  uint8_t sendbuf[2];
  for (int i = 0; i < messageSize; i++) {
    if (i == (messageSize - 1)) {
      sendbuf[0] = TPUART_DATA_END;
    }
    else {
      sendbuf[0] = TPUART_DATA_START_CONTINUE;
    }

    sendbuf[0] |= i;
    sendbuf[1] = aTelegram->getBufferByte(i);

    _serialport->write(sendbuf, 2);
  }


  int confirmation;
  while (true) {
    confirmation = serialRead();
    if (confirmation == B10001011) {
      delay (SERIAL_WRITE_DELAY_MS);
      return true; // Sent successfully
    }
    else if (confirmation == B00001011) {
      delay (SERIAL_WRITE_DELAY_MS);
      return false;
    }
    else if (confirmation == -1) {
      // Read timeout
      delay (SERIAL_WRITE_DELAY_MS);
      return false;
    }
  }

  return false;
}

void KnxTpUart::sendAck() {
  byte sendByte = B00010001;
  _serialport->write(sendByte);
  delay(SERIAL_WRITE_DELAY_MS);
}

void KnxTpUart::sendNotAddressed() {
  byte sendByte = B00010000;
  _serialport->write(sendByte);
  delay(SERIAL_WRITE_DELAY_MS);
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

void KnxTpUart::addListenGroupAddress(String address) {
  int area = address.substring(0, address.indexOf('/')).toInt();
  int line = address.substring(address.indexOf('/') + 1, address.length()).substring(0, address.substring(address.indexOf('/') + 1, address.length()).indexOf('/')).toInt();
  int member = address.substring(address.lastIndexOf('/') + 1, address.length()).toInt();
  uint16_t ga = ((area & 0xF0)<<12) | ((line & 0x0F) <<8) | (member & 0xFF);
  addListenGroupAddress(ga);
}

void KnxTpUart::addListenGroupAddress(uint16_t address) {
  if (_listen_group_address_count >= MAX_LISTEN_GROUP_ADDRESSES) {
#if defined(TPUART_DEBUG)
    TPUART_DEBUG_PORT.println("Already listening to MAX_LISTEN_GROUP_ADDRESSES, cannot listen to another");
#endif
    return;
  }

  _listen_group_addresses[_listen_group_address_count] = address;

  _listen_group_address_count++;
}

bool KnxTpUart::isListeningToGroupAddress(uint8_t main, uint8_t middle, uint8_t sub) {
    uint16_t ga = ((main & 0xF0)<<12) | ((middle & 0x0F) <<8) | (sub & 0xFF);
    return isListeningToGroupAddress(ga);
}

bool KnxTpUart::isListeningToGroupAddress(uint16_t aAddress) {
    for (int i = 0; i < _listen_group_address_count; i++)
    {
        if (_listen_group_addresses[i] == aAddress)
        {
        	return true;
        }
    }
    return false;
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
