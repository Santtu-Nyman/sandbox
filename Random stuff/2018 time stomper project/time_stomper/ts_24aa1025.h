#ifndef TS_24AA1025_H
#define TS_24AA1025_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define TIS_FAKE_EEPROM
#include <Windows.h>

static HANDLE ts_fake_eeprom;

void ts_initialize_eeprom()
{
	HANDLE new_fake_eeprom = CreateFileW(L"TS_FAKE_EEPROM.dat", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_ALWAYS, 0, 0);
	if (new_fake_eeprom == INVALID_HANDLE_VALUE || SetFilePointer(new_fake_eeprom, 0x10000, 0, FILE_BEGIN) != 0x10000 || !SetEndOfFile(new_fake_eeprom))
		ExitProcess((UINT)GetLastError());
	ts_fake_eeprom = new_fake_eeprom;
}

void ts_read_eeprom_page(void* buffer, uint16_t offset)
{
	if (offset & 0x7F)
		ExitProcess((UINT)ERROR_INVALID_PARAMETER);
	if (SetFilePointer(ts_fake_eeprom, (LONG)offset, 0, FILE_BEGIN) != (DWORD)offset)
		ExitProcess((UINT)GetLastError());
	for (DWORD file_read = 0, read_result; file_read != 0x80;)
		if (ReadFile(ts_fake_eeprom, (LPVOID)((UINT_PTR)buffer + (UINT_PTR)file_read), 0x80 - file_read, &read_result, 0))
			file_read += read_result;
		else
			ExitProcess((UINT)GetLastError());
}

void ts_write_eeprom_page(const void* buffer, uint16_t offset)
{
	if (offset & 0x7F)
		ExitProcess((UINT)ERROR_INVALID_PARAMETER);
	if (SetFilePointer(ts_fake_eeprom, (LONG)offset, 0, FILE_BEGIN) != (DWORD)offset)
		ExitProcess((UINT)GetLastError());
	for (DWORD file_written = 0, write_result; file_written != 0x80;)
		if (WriteFile(ts_fake_eeprom, (LPCVOID)((UINT_PTR)buffer + (UINT_PTR)file_written), 0x80 - file_written, &write_result, 0))
			file_written += write_result;
		else
			ExitProcess((UINT)GetLastError());
}

#else
	
#ifdef __AVR__

#define TS_SCL_PIN 2
#define TS_SDA_PIN 3
#define TS_SCL_DELAY 4

#else
	
#define TS_SCL_PIN PB8
#define TS_SDA_PIN PB9
#define TS_SCL_DELAY 4

#endif

void ts_i2c_initialize()
{
	pinMode(TS_SCL_PIN, OUTPUT);
	pinMode(TS_SDA_PIN, OUTPUT);
	digitalWrite(TS_SCL_PIN, HIGH);
	digitalWrite(TS_SDA_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
}

void ts_i2c_start()
{
	digitalWrite(TS_SDA_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
}

void ts_i2c_stop()
{
	digitalWrite(TS_SCL_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SDA_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
}

void ts_i2c_repeat_start()
{
	digitalWrite(TS_SDA_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SDA_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
}

uint8_t ts_i2c_write(uint8_t data)
{
	uint8_t tmp = 8;
	while (tmp--)
	{
		digitalWrite(TS_SDA_PIN, (data >> tmp) & 1);
		delayMicroseconds(TS_SCL_DELAY);
		digitalWrite(TS_SCL_PIN, HIGH);
		delayMicroseconds(TS_SCL_DELAY);
		digitalWrite(TS_SCL_PIN, LOW);
		delayMicroseconds(TS_SCL_DELAY);
	}
	pinMode(TS_SDA_PIN, INPUT_PULLUP);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	tmp = (uint8_t)digitalRead(TS_SDA_PIN);
	digitalWrite(TS_SCL_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
	pinMode(TS_SDA_PIN, OUTPUT);
	digitalWrite(TS_SDA_PIN, LOW);
	return tmp;
}

uint8_t ts_i2c_read(uint8_t ack)
{
	uint8_t data = 0;
	pinMode(TS_SDA_PIN, INPUT_PULLUP);
	for (uint8_t count = 8; count--;)
	{
		delayMicroseconds(TS_SCL_DELAY);
		digitalWrite(TS_SCL_PIN, HIGH);
		delayMicroseconds(TS_SCL_DELAY);
		data |= ((uint8_t)digitalRead(TS_SDA_PIN) << count);
		digitalWrite(TS_SCL_PIN, LOW);
		delayMicroseconds(TS_SCL_DELAY);
	}
	pinMode(TS_SDA_PIN, OUTPUT);
	digitalWrite(TS_SDA_PIN, ack ? LOW : HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, HIGH);
	delayMicroseconds(TS_SCL_DELAY);
	digitalWrite(TS_SCL_PIN, LOW);
	digitalWrite(TS_SDA_PIN, LOW);
	delayMicroseconds(TS_SCL_DELAY);
	return data;
}

void ts_read_eeprom_page(void* buffer, uint16_t address)
{
	ts_i2c_start();
	ts_i2c_write(0xA0);
	ts_i2c_write((uint8_t)(address >> 8));
	ts_i2c_write((uint8_t)address);
	ts_i2c_repeat_start();
	ts_i2c_write(0xA1);
	for (void* last_address = (void*)((uintptr_t)buffer + 0x7F); buffer != last_address; buffer = (void*)((uintptr_t)buffer + 1))
		*(uint8_t*)buffer = ts_i2c_read(1);
	*(uint8_t*)buffer = ts_i2c_read(0);
	ts_i2c_stop();
}

void ts_write_eeprom_page(const void* buffer, uint16_t address)
{
	ts_i2c_start();
	ts_i2c_write(0xA0);
	ts_i2c_write((uint8_t)(address >> 8));
	ts_i2c_write((uint8_t)address);
	for (const void* buffer_end = (const void*)((uintptr_t)buffer + 0x80); buffer != buffer_end; buffer = (const void*)((uintptr_t)buffer + 1))
		ts_i2c_write(*(uint8_t*)buffer);
	ts_i2c_stop();
}

void ts_initialize_eeprom()
{
  ts_i2c_initialize();
}

#endif

#ifdef __cplusplus
}
#endif

#endif
