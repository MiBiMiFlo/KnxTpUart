Arduino EIB/KNX Interface via TP-UART
=====================================


This is a very first version of an interface between Arduino and EIB/KNX using the TP-UART interface and Arduino library.
The library is based on the KnxTpUart library from https://github.com/thorsten-gehrig/arduino-tpuart-knx-user-forum.

Following are the primary changes:
- Addresses can be given as uint16_t (2byte number) instead of String or 3 separate integers.
- Callback for check if telegram is of interest (when listening for telegrams)
- Support for sending user generated telegram
- Optimize for size (FLASH and RAM)
- Support ATTiny MCUs

Last modified 09.02.2021

Hardware
--------
For KNX connection the Siemens BCU 5WG1 117-2AB12 is used in any tests.

-> Please note: When using an additional power supply galvanic separation between ARDUINO and BCU is necessary.

The Library is tested on ARDUINO MEGA, ARDUINO UNO and different ATTiny models (ATTiny1614 and ATTiny3216).

-> During programming (if programming through Serial), the BCU should have no connection to the ARDUINO UNO.



Software (library was tested with IDE 1.0.5-R2 and IDE 1.6.9 by arduino.cc)
---------------------------------------------------------------------------

The Arduino library is found in the directory `KnxTpUart` and can be directly placed in Arduino's library folder. 


Issues, Comments and Suggestions
--------------------------------

If you have any, don't hesitate to contact me or use the issue tracker. You are also invited to improve the code and send me a pull request to reintegrate the changes here.


Code Examples
-------------

Declare and Initialize the KNX connection
-----------------------------------------

To define a KNX connection through standard Serial class use:
<pre>
KnxTpUart knx(&Serial, KNX_IA(1,1,199));
void setup()
{
    Serial.begin(19200, SERIAL_8E1);
}
</pre>

To read KNX telegrams from BUS the following must be called frequently:
<pre>
KnxTpUartSerialEventType eType = knx.serialEvent();
</pre>

This function should either be called in loop or in serialEvent function (of Arduino).

If eType is KNX_TELEGRAM or IRRELEVANT_KNX_TELEGRAM a KNX telegram is available.

Write a message or an answer:
-----------------------------

The API provides two different functions, either use groupWrite\* or groupAnswer\* depending 
if to send a normal write or an answer (to a previous read). The following examples show
both functions, please select the appropriate.

Bool (DPT 1 - 0 or 1)
<pre>
bool value = true;
knx.groupWriteBool(KNX_GA(1,2,3), value);
knx.groupAnswerBool(KNX_GA(1,2,3), value);
</pre>


4 Bit Int (DPT 3)
<pre>
uint8_t value = 7;
knx.groupWrite4BitInt(KNX_GA(1,2,3), value);
knx.groupAnswer4BitInt(KNX_GA(1,2,3), value);
</pre>


4 Bit Dim (DPT 3)
<pre>
bool direction = true;
uint8_t steps = 1;
knx.groupWrite4BitDim(KNX_GA(1,2,3), direction, steps);
knx.groupAnswer4BitDim(KNX_GA(1,2,3), direction, steps);
</pre>


1 Byte int (DTP 5 - 0...255)
<pre>
int8_t value = -5;
knx.groupWrite1ByteInt(KNX_GA(1,2,3), value);
knx.groupAnswer1ByteInt(KNX_GA(1,2,3), value);
</pre>


1 Byte unsigned int (DTP 5 - 0...255)
<pre>
uint8_t value = 255;
knx.groupWrite1ByteInt(KNX_GA(1,2,3), value);
knx.groupAnswer1ByteInt(KNX_GA(1,2,3), value);
</pre>


2 Byte int (DTP 7 - 0…65 535])
<pre>
int16_t value = -5;
knx.groupWrite2ByteInt(KNX_GA(1,2,3), value);
knx.groupAnswer2ByteInt(KNX_GA(1,2,3), value);
</pre>


2 Byte unsigned int (DTP 7 - 0…65 535])
<pre>
uint16_t value = 0xFFFF;
knx.groupWrite2ByteUInt(KNX_GA(1,2,3), value);
knx.groupAnswer2ByteUInt(KNX_GA(1,2,3), value);
</pre>


2 Byte Float (DPT9 - -671 088,64 to 670 760,96 )
<pre>
float value = 250.5;
knx.groupWrite2ByteFloat(KNX_GA(1,2,3), value);
knx.groupAnswer2ByteFloat(KNX_GA(1,2,3), value);
</pre>


3 Byte Time (DTP 10)
<pre>
uint8_t weekday = 2;
uint8_t hour    = 10;
uint8_t minute  = 56;
uint8_t second  = 44;
knx.groupWrite3ByteTime(KNX_GA(1,2,3), weekday, hour, minute, second);
knx.groupAnswer3ByteTime(KNX_GA(1,2,3), weekday, hour, minute, second);
</pre>


3 Byte Date (DTP 11)
<pre>
uint8_t day   = 2;
uint8_t month = 10;
uint8_t year  = 21;
knx.groupWrite3ByteDate(KNX_GA(1,2,3),day, month, year);
knx.groupAnswer3ByteDate(KNX_GA(1,2,3), day, month, year);
</pre>


4 byte Float (DTP 14 - -2147483648 to 2147483647) 
<pre>
float value 433.152
knx.groupWrite4ByteFloat(KNX_GA(1,2,3),value);
knx.groupAnswer4ByteFloat(KNX_GA(1,2,3), value);
</pre>


14 Byte Text (DTP 16)
<pre>
String value = "Hello World";
knx.groupWrite14ByteText(KNX_GA(1,2,3), value);
knx.groupAnswer14ByteText(KNX_GA(1,2,3), value);
</pre>


Request a value:
--------------------------------------------

The following code can be used to request a value for a GA:
<pre>
knx.groupRead(KNX_GA(1,2,3));
</pre>


Evaluation telegrams:
-------------------------------

The following code shows how to receive a telegram:
<pre>
KnxTpUartSerialEventType eType = knx.serialEvent();
// A telegram is flagged as IRRELEVANT_KNX_TELEGRAM if it's target is not of interest.
if (eType == KNX_TELEGRAM || eType == IRRELEVANT_KNX_TELEGRAM)
{
    KnxTelegram* telegram = knx.getReceivedTelegram();
}
</pre>


Bool (DPT 1 - 0 or 1)
<pre>
bool value = telegram->getBool();
</pre>


4 Bit Int (DPT 3)
<pre>
uint8_t value = telegram->get4BitIntValue();
</pre>


Bool (DPT 3)
<pre>
bool value = telegram->get4BitDirectionValue();
</pre>


1 byte (DPT 3)
<pre>
uint8_t value = telegram->get4BitStepsValue();
</pre>


1 Byte int (DTP 5 - 0...255)
<pre>
int8_t value = telegram->get1ByteIntValue();
</pre>


1 Byte unsigned int (DTP 5 - 0...255)
<pre>
uint8_t value = telegram->get1ByteUIntValue();
</pre>


2 Byte int (DTP 7 - 0…65 535])
<pre>
int16_t value = telegram->get2ByteIntValue();
</pre>


2 Byte unsigned int (DTP 7 - 0…65 535])
<pre>
uint16_t value = telegram->get2ByteIntValue();
</pre>


2 Byte Float (DPT9 - -671 088,64 to 670 760,96 )
<pre>
float value = telegram->get2ByteFloatValue();
</pre>


3 Byte Time (DTP 10)
<pre>
uint8_t value = telegram->get3ByteWeekdayValue();
uint8_t value = telegram->get3ByteHourValue();
uint8_t value = telegram->get3ByteMinuteValue();
uint8_t value = telegram->get3ByteSecondValue();
</pre>


3 Byte Date (DTP 11)
<pre>
uint8_t value = telegram->get3ByteDayValue();
uint8_t value = telegram->get3ByteMonthValue();
uint8_t value = telegram->get3ByteYearValue();
</pre>


4 byte Float (DTP 14 - -2147483648 to 2147483647)
<pre>
float value = telegram->get4ByteFloatValue();
</pre>


14 Byte Text (DTP 16)
<pre>
String value = telegram->get14ByteValue();
</pre>

References
----------
The following links can be helpfull
- https://www.hqs.sbt.siemens.com/cps_product_data/gamma-b2b/PCBA_UP117-12_datasheet.pdf
- https://www.hqs.sbt.siemens.com/cps_product_data/gamma-b2b/TPUART2_technical-data.pdf
- https://sar.informatik.hu-berlin.de/research/publications/SAR-PR-2017-01/SAR-PR-2017-01_.pdf
- https://github.com/franckmarini/KnxDevice
