#include <stddef.h>
#include <stdint.h>
#include "ts_24aa1025.h"
#include "ts_tis.h"

static uint32_t wall_clock_time;

uint8_t process_remote_access_wait_for_serial(uint32_t timeout)
{
  if (Serial.available())
    return 1;
  timeout += (uint32_t)millis();
  while ((uint32_t)millis() < timeout)
    if (Serial.available())
      return 1;
  return 0;
}

uint8_t process_remote_access()
{
  static uint8_t buffer[0x80];
  uint8_t length = 0;
  if (Serial && Serial.available())
  {
    buffer[0] = (uint8_t)Serial.read();
    if (buffer[0] == 0x3F)
    {
      Serial.write(0x4B);
      return 0;
    }
    else if (buffer[0] == 0x24)
    {
      while (length != 2 && process_remote_access_wait_for_serial(0x100))
        buffer[length++] = (uint8_t)Serial.read();
      if (length != 2)
        return 0;
      uint16_t address = (uint16_t)(buffer[0] & 0xFE) | ((uint16_t)buffer[1] << 8);
      Serial.write(0x4B);
      if (!(buffer[0] & 1))
      {
        ts_read_eeprom_page(buffer, address);
        for (const uint8_t* read = (const uint8_t*)buffer, * read_end = read + 0x80; read != read_end; ++read)
          Serial.write(*read);
        return 0;
      }
      else
      {
        for (uint8_t* write = (uint8_t*)buffer, * write_end = write + 0x80; write != write_end; ++write)
        {
          if (!process_remote_access_wait_for_serial(0x100))
            return 0;
          *write = (uint8_t)Serial.read();
        }
        ts_write_eeprom_page(buffer, address);
        return 1;
      }
    }
    else if (buffer[0] == 0x2A)
    {
      Serial.write(0x4B);
      Serial.write((uint8_t)wall_clock_time);
      Serial.write((uint8_t)(wall_clock_time >> 8));
      Serial.write((uint8_t)(wall_clock_time >> 16));
      Serial.write((uint8_t)(wall_clock_time >> 24));
      return 0;
    }
    else if (buffer[0] == 0x23)
    {
      while (length != 4 && process_remote_access_wait_for_serial(0x100))
        buffer[length++] = (uint8_t)Serial.read();
      if (length != 4)
        return 0;
      Serial.write(0x4B);
      wall_clock_time = (uint32_t)buffer[0] | ((uint32_t)buffer[1] << 8) | ((uint32_t)buffer[2] << 16) | ((uint32_t)buffer[3] << 24);
      ts_save_wall_clock_time(wall_clock_time);
      return 1;
    }
  }
  return 0;
}

void setup()
{
  ts_initialize_eeprom();
  wall_clock_time = ts_load_wall_clock_time();
  Serial.begin(9600);
}

void loop()
{
  process_remote_access();
}
