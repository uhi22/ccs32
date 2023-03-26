#ifndef __ESP32_SERIAL_H__
#define __ESP32_SERIAL_H__

#define ESP32SERIAL_DEBUG ::Serial

#include <stdlib.h>
#include <inttypes.h>

#include <driver/uart.h>

class ESP32Serial : public Stream
{
public:
  explicit ESP32Serial(int port);
  virtual ~ESP32Serial();

  int available();
  int begin(unsigned long baud, uint32_t config = SERIAL_8N1, int8_t rxPin=-1, int8_t txPin=-1);
  void end();
  void flush();
  int read(void);
  size_t write(uint8_t c);
  size_t write(const uint8_t *buffer, size_t size);
  size_t print(const char* text);
  int peek(void);
  int checkPort(void);

private:
  uart_port_t m_port;
  QueueHandle_t m_uart_queue;
};

#endif // __ESP32_SERIAL_H__
