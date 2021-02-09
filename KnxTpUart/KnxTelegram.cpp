// File: KnxTelegram.cpp
// Author: Daniel Kleine-Albers (Since 2012)
// Modified: Thorsten Gehrig (Since 2014)
// Modified: Michael Werski (Since 2014)
// Modified: Katja Blankenheim (Since 2014)
// Modified: Mag Gyver (Since 2016)
// Modified: Rouven Raudzus (Since 2017)

// Last modified: 06.06.2017

#include "KnxTelegram.h"

KnxTelegram::KnxTelegram() {
  clear();
}

void KnxTelegram::clear() {
  for (int i = 0; i < MAX_KNX_TELEGRAM_SIZE; i++) {
    buffer[i] = 0;
  }

  // Control Field, Normal Priority, No Repeat
  buffer[0] = B10111100;

  // Target Group Address, Routing Counter = 6, Length = 1 (= 2 Bytes)
  buffer[5] = B11100001;
}

uint8_t KnxTelegram::getBufferByte(uint8_t index) {
  return buffer[index];
}

void KnxTelegram::setBufferByte(uint8_t index, uint8_t aContent) {
  buffer[index] = aContent;
}

bool KnxTelegram::isRepeated() {
  // Parse Repeat Flag
  if (buffer[0] & B00100000) {
    return false;
  }
  else {
    return true;
  }
}

void KnxTelegram::setRepeated(bool repeat) {
  if (repeat) {
    buffer[0] = buffer[0] & B11011111;
  }
  else {
    buffer[0] = buffer[0] | B00100000;
  }
}

void KnxTelegram::setPriority(KnxPriorityType prio) {
  buffer[0] = buffer[0] & B11110011;
  buffer[0] = buffer[0] | (prio << 2);
}

KnxPriorityType KnxTelegram::getPriority() {
  // Priority
  return (KnxPriorityType) ((buffer[0] & B00001100) >> 2);
}

uint16_t KnxTelegram::getSourceAddress()
{
	return (((uint16_t)buffer[1]) << 8) | buffer[2];
}

void KnxTelegram::setSourceAddress(uint8_t area, uint8_t line, uint8_t member) {
  buffer[1] = ((area & 0x0F) << 4) | (line & 0x0F);	// Source Address
  buffer[2] = (member & 0xFF); // Source Address
}

void KnxTelegram::setSourceAddress(uint16_t aAddress) {
  buffer[1] = (aAddress >> 8) & 0xFF;
  buffer[2] = aAddress & 0x00FF;
}

uint8_t KnxTelegram::getSourceArea() {
  return (buffer[1] >> 4);
}

uint8_t KnxTelegram::getSourceLine() {
  return (buffer[1] & B00001111);
}

uint8_t KnxTelegram::getSourceMember() {
  return buffer[2] & 0xFF;
}

void KnxTelegram::setTargetAddress(uint16_t aAddress, bool aIsGroup)
{
    buffer[3] = (aAddress >> 8) & 0xFF;
    buffer[4] = (aAddress & 0xFF);
    if (aIsGroup)
    {
    	buffer[5] = buffer[5] | B10000000;
    }
    else
    {
    	buffer[5] = buffer[5] & B01111111;
    }
}

void KnxTelegram::setTargetGroupAddress(uint8_t main, uint8_t middle, uint8_t sub) {
  buffer[3] = ((main & 0x1F) << 3) | (middle & 0x07);
  buffer[4] = (sub & 0xFF);
  buffer[5] = buffer[5] | B10000000;
}

void KnxTelegram::setTargetIndividualAddress(uint8_t area, uint8_t line, uint8_t member) {
  buffer[3] = ((area & 0x0F) << 4) | (line & 0x0F);
  buffer[4] = (member & 0xFF);
  buffer[5] = buffer[5] & B01111111;
}


void KnxTelegram::setTargetGroupAddress(uint16_t aAddress) {
    buffer[3] = (aAddress >> 8) & 0xFF;
    buffer[4] = (aAddress & 0xFF);
    buffer[5] = buffer[5] | B10000000;
}

void KnxTelegram::setTargetIndividualAddress(uint16_t aAddress) {
    buffer[3] = (aAddress >> 8) & 0xFF;
    buffer[4] = (aAddress & 0xFF);
    buffer[5] = buffer[5] & B01111111;
}


bool KnxTelegram::isTargetGroup() {
  return buffer[5] & B10000000;
}

uint8_t KnxTelegram::getTargetMainGroup() {
  return ((buffer[3] & B11111000) >> 3);
}

uint8_t KnxTelegram::getTargetMiddleGroup() {
  return (buffer[3] & B00000111);
}

uint8_t KnxTelegram::getTargetSubGroup() {
  return buffer[4];
}

uint16_t KnxTelegram::getTargetGroupAddress()
{
	return (((uint16_t)buffer[3]) << 8) | buffer[4];
}

uint8_t KnxTelegram::getTargetArea() {
  return ((buffer[3] & B11110000) >> 4);
}

uint8_t KnxTelegram::getTargetLine() {
  return (buffer[3] & B00001111);
}

uint8_t KnxTelegram::getTargetMember() {
  return buffer[4];
}

uint16_t KnxTelegram::getTargetAddress()
{
	uint16_t res = (buffer[3] & 0xFF);
	res = res << 8 | (buffer[4] & 0xFF);
	return res;
}

void KnxTelegram::setRoutingCounter(uint8_t counter) {
  buffer[5] = buffer[5] & B10000000;
  buffer[5] = buffer[5] | (counter << 4);
}

uint8_t KnxTelegram::getRoutingCounter() {
  return ((buffer[5] & B01110000) >> 4);
}

void KnxTelegram::setPayloadLength(uint8_t length) {
  buffer[5] = buffer[5] & B11110000;
  buffer[5] = buffer[5] | (length - 1);
}

int KnxTelegram::getPayloadLength() {
  int length = (buffer[5] & B00001111) + 1;
  return length;
}

void KnxTelegram::setCommand(KnxCommandType command) {
  buffer[6] = buffer[6] & B11111100;
  buffer[7] = buffer[7] & B00111111;

  buffer[6] = buffer[6] | (command >> 2); // Command first two bits
  buffer[7] = buffer[7] | (command << 6); // Command last two bits
}

KnxCommandType KnxTelegram::getCommand() {
  return (KnxCommandType) (((buffer[6] & B00000011) << 2) | ((buffer[7] & B11000000) >> 6));
}

void KnxTelegram::setControlData(KnxControlDataType cd) {
  buffer[6] = buffer[6] & B11111100;
  buffer[6] = buffer[6] | cd;
}

KnxControlDataType KnxTelegram::getControlData() {
  return (KnxControlDataType) (buffer[6] & B00000011);
}

KnxCommunicationType KnxTelegram::getCommunicationType() {
  return (KnxCommunicationType) ((buffer[6] & B11000000) >> 6);
}

void KnxTelegram::setCommunicationType(KnxCommunicationType type) {
  buffer[6] = buffer[6] & B00111111;
  buffer[6] = buffer[6] | (type << 6);
}

uint8_t KnxTelegram::getSequenceNumber() {
  return (buffer[6] & B00111100) >> 2;
}

void KnxTelegram::setSequenceNumber(uint8_t number) {
  buffer[6] = buffer[6] & B11000011;
  buffer[6] = buffer[6] | (number << 2);
}

void KnxTelegram::createChecksum() {
	uint8_t checksumPos = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;
  buffer[checksumPos] = calculateChecksum();
}

uint8_t KnxTelegram::getChecksum() {
  uint8_t checksumPos = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;
  return buffer[checksumPos];
}

bool KnxTelegram::verifyChecksum() {
	uint8_t calculatedChecksum = calculateChecksum();
	return (getChecksum() == calculatedChecksum);
}

void KnxTelegram::print(TPUART_SERIAL_CLASS* serial) {
#if defined(TPUART_DEBUG)
  serial->print("Repeated: ");
  serial->println(isRepeated());

  serial->print("Priority: ");
  serial->println(getPriority());

  serial->print("Source: ");
  serial->print(getSourceArea());
  serial->print(".");
  serial->print(getSourceLine());
  serial->print(".");
  serial->println(getSourceMember());

  if (isTargetGroup()) {
    serial->print("Target Group: ");
    serial->print(getTargetMainGroup());
    serial->print("/");
    serial->print(getTargetMiddleGroup());
    serial->print("/");
    serial->println(getTargetSubGroup());
  }
  else {
    serial->print("Target Physical: ");
    serial->print(getTargetArea());
    serial->print(".");
    serial->print(getTargetLine());
    serial->print(".");
    serial->println(getTargetMember());
  }

  serial->print("Routing Counter: ");
  serial->println(getRoutingCounter());

  serial->print("Payload Length: ");
  serial->println(getPayloadLength());

  serial->print("Command: ");
  serial->println(getCommand());

  serial->print("First Data Byte: ");
  serial->println(getFirstDataByte());

  for (int i = 2; i < getPayloadLength(); i++) {
    serial->print("Data Byte ");
    serial->print(i);
    serial->print(": ");
    serial->println(buffer[6 + i], BIN);
  }


  if (verifyChecksum()) {
    serial->println("Checksum matches");
  }
  else {
    serial->println("Checksum mismatch");
    serial->println(getChecksum(), BIN);
    serial->println(calculateChecksum(), BIN);
  }
#endif
}

uint8_t KnxTelegram::calculateChecksum() {
	uint8_t bcc = 0xFF;
	uint8_t size = getPayloadLength() + KNX_TELEGRAM_HEADER_SIZE;

  for (uint8_t i = 0; i < size; i++) {
    bcc ^= buffer[i];
  }

  return bcc;
}

uint8_t KnxTelegram::getTotalLength() {
  return KNX_TELEGRAM_HEADER_SIZE + getPayloadLength() + 1;
}

void KnxTelegram::setFirstDataByte(uint8_t aData) {
  buffer[7] = buffer[7] & B11000000;
  buffer[7] = buffer[7] | aData;
}

uint8_t KnxTelegram::getFirstDataByte() {
  return (buffer[7] & B00111111);
}

bool KnxTelegram::getBool() {
  if (getPayloadLength() != 2) {
    // Wrong payload length
    return 0;
  }

  return (getFirstDataByte() & B00000001);
}

int8_t KnxTelegram::get4BitIntValue() {
  if (getPayloadLength() != 2) {
    // Wrong payload length
    return 0;
  }

  return (getFirstDataByte() & B00001111);
}

bool KnxTelegram::get4BitDirectionValue() {
  if (getPayloadLength() != 2) {
    // Wrong payload length
    return 0;
  }

  return ((getFirstDataByte() & B00001000)) >> 3;
}

uint8_t KnxTelegram::get4BitStepsValue() {
  if (getPayloadLength() != 2) {
    // Wrong payload length
    return 0;
  }

  return (getFirstDataByte() & B00000111);
}

void KnxTelegram::set1ByteIntValue(int8_t value) {
  setPayloadLength(3);
  buffer[8] = (value & 0xFF);
}

void KnxTelegram::set1ByteUIntValue(uint8_t value) {
  setPayloadLength(3);
  buffer[8] = (value & 0xFF);
}

int8_t KnxTelegram::get1ByteIntValue() {
  if (getPayloadLength() != 3) {
    // Wrong payload length
    return 0;
  }

  return (int8_t) (buffer[8] & 0xFF);
}

uint8_t KnxTelegram::get1ByteUIntValue() {
  if (getPayloadLength() != 3) {
    // Wrong payload length
    return 0;
  }

  return (uint8_t) (buffer[8] & 0xFF);
}

void KnxTelegram::set2ByteIntValue(int16_t value) {
  setPayloadLength(4);

  buffer[8] = (value >> 8) & 0xFF;
  buffer[9] = value & 0xFF;
}

int16_t KnxTelegram::get2ByteIntValue() {
  if (getPayloadLength() != 4) {
    // Wrong payload length
    return 0;
  }
  int16_t value = int(buffer[8] << 8) + int(buffer[9]);

  return (value);
}


void KnxTelegram::set2ByteUIntValue(uint16_t value) {
  setPayloadLength(4);

  buffer[8] = (value >> 8) & 0xFF;
  buffer[9] = value & 0xFF;
}

uint16_t KnxTelegram::get2ByteUIntValue() {
  if (getPayloadLength() != 4) {
    // Wrong payload length
    return 0;
  }
  uint16_t value = (((uint16_t)buffer[8]) << 8) + buffer[9];

  return (value);
}



void KnxTelegram::set4ByteIntValue(int32_t value) {
  setPayloadLength(6);

  buffer[8]  = (value  >> 24) & 0xFF;
  buffer[9]  = (value  >> 16) & 0xFF;
  buffer[10] = (value  >> 8) & 0xFF;
  buffer[11] = (value & 0xFF);
}


int32_t KnxTelegram::get4ByteIntValue() {
  if (getPayloadLength() != 6) {
    // Wrong payload length
    return 0;
  }

  int32_t value = buffer[8];
  value = (value << 8) + buffer[9];
  value = (value << 8) + buffer[10];
  value = (value << 8) + buffer[11];

  return (value);
}


void KnxTelegram::set4ByteUIntValue(uint32_t value) {
  setPayloadLength(6);

  buffer[8]  = (value  >> 24) & 0xFF;
  buffer[9]  = (value  >> 16) & 0xFF;
  buffer[10] = (value >> 8) & 0xFF;
  buffer[11] = (value & 0xFF);
}


uint32_t KnxTelegram::get4ByteUIntValue() {
  if (getPayloadLength() != 6) {
    // Wrong payload length
    return 0;
  }
  //TODO: check if this is correct
  uint32_t value = buffer[8];
  value = value << 8 + buffer[9];
  value = value << 8 + buffer[10];
  value = value << 8 + buffer[11];

  return (value);
}


void KnxTelegram::set2ByteFloatValue(float value) {
  setPayloadLength(4);

  float v = value * 100.0f;
  int exponent = 0;
  for (; v < -2048.0f; v /= 2) exponent++;
  for (; v > 2047.0f; v /= 2) exponent++;
//  long m = round(v) & 0x7FF;
  long m = lround(v) & 0x7FF;
  short msb = (short) (exponent << 3 | m >> 8);
  if (value < 0.0f) msb |= 0x80;
  buffer[8] = msb;
  buffer[9] = (uint8_t)m;
}

float KnxTelegram::get2ByteFloatValue() {
  if (getPayloadLength() != 4) {
    // Wrong payload length
    return 0;
  }

  int exponent = (buffer[8] & B01111000) >> 3;
  int mantissa = ((buffer[8] & B00000111) << 8) | (buffer[9]);

  if (buffer[8] & B10000000) {
    return ((-2048 + mantissa) * 0.01) * pow(2.0, exponent); // Thanks to Rouven Raudzus for the note
  }

  return (mantissa * 0.01) * pow(2.0, exponent);
}

void KnxTelegram::set3ByteTime(uint8_t weekday, uint8_t hour, uint8_t minute, uint8_t second) {
  setPayloadLength(5);

  // Move the weekday by 5 bits to the left
  weekday = weekday << 5;

  // Buffer [8] bit 5-7 for weekday, bit 0-4 for hour
  buffer[8] = (weekday & B11100000) + (hour & B00011111);

  // Buffer [9] bit 6-7 empty, bit 0-5 for minutes
  buffer[9] =  minute & B00111111;

  // Buffer [10] bit 6-7 empty, bit 0-5 for seconds
  buffer[10] = second & B00111111;
}

uint8_t KnxTelegram::get3ByteWeekdayValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[8] & B11100000) >> 5;
}

uint8_t KnxTelegram::get3ByteHourValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[8] & B00011111);
}

uint8_t KnxTelegram::get3ByteMinuteValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[9] & B00111111);
}

uint8_t KnxTelegram::get3ByteSecondValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[10] & B00111111);
}

void KnxTelegram::set3ByteDate(uint8_t day, uint8_t month, uint8_t year) {
  setPayloadLength(5);

  // Buffer [8] bit 5-7 empty, bit 0-4 for month days
  buffer[8] = day & B00011111;

  // Buffer [9] bit 4-7 empty, bit 0-3 for months
  buffer[9] =  month & B00001111;

  // Buffer [10] fill with year
  buffer[10] = year;
}

uint8_t KnxTelegram::get3ByteDayValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[8] & B00011111);
}

uint8_t KnxTelegram::get3ByteMonthValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[9] & B00001111);
}

uint8_t KnxTelegram::get3ByteYearValue() {
  if (getPayloadLength() != 5) {
    // Wrong payload length
    return 0;
  }
  return (buffer[10]);
}

void KnxTelegram::set4ByteFloatValue(float value) {
  setPayloadLength(6);

  uint8_t b[4];
  float *f = (float*)(void*) & (b[0]);
  *f = value;

  buffer[8 + 3] = b[0];
  buffer[8 + 2] = b[1];
  buffer[8 + 1] = b[2];
  buffer[8 + 0] = b[3];
}

float KnxTelegram::get4ByteFloatValue() {
  if (getPayloadLength() != 6) {
    // Wrong payload length
    return 0;
  }
  uint8_t b[4];
  b[0] = buffer[8 + 3];
  b[1] = buffer[8 + 2];
  b[2] = buffer[8 + 1];
  b[3] = buffer[8 + 0];
  float *f = (float*)(void*) & (b[0]);
  float  r = *f;
  return r;
}

void KnxTelegram::set14ByteValue(String value) {
    // Define
    char _load[15];

    // Empty/Initialize with space
    for (int i = 0; i < 14; ++i)
    {
        _load[i] = 0;
    }

    setPayloadLength(16);
    // Make out of value the Chararray
    value.toCharArray(_load, 15); // Must be 15 - because it completes with 0
    for (uint8_t i = 0; i < 14; i++)
    {
        buffer[8 + i]  = _load[i];
    }
}

String KnxTelegram::get14ByteValue() {
    if (getPayloadLength() != 16) {
        // Wrong payload length
        return "";
    }
    char _load[15];
    for (uint8_t i = 0; i < 14; i++)
    {
        _load[i] = buffer[8 + i];
    }
    return (_load);
}


uint8_t KnxTelegram::getValue(uint8_t* aBuffer, uint8_t aCount)
{
	if (getPayloadLength() < aCount)
	{
		aCount = getPayloadLength();
	}
	memcpy(aBuffer, buffer+8, aCount);
	return aCount;
}

void KnxTelegram::setValue(uint8_t* aBuffer, uint8_t aSize)
{
	if (aSize > 14)
	{
		// ignore
		return;
	}

	setPayloadLength(aSize+2);
	memcpy(buffer+8, aBuffer, aSize);
}
