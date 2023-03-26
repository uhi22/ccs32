#include <Arduino.h>

#include "ESP32Serial.h"

#include <string.h>

#include <driver/uart.h>

 ESP32Serial::ESP32Serial(int port)
 : m_port((uart_port_t)port)
 {
 }

 ESP32Serial::~ESP32Serial()
 {
 }

int ESP32Serial::begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin)
{
#ifdef ESP32SERIAL_DEBUG
  ESP32SERIAL_DEBUG.printf("\nESP32Serial::begin Setting up uart:%d rx:%d/tx:%d at %ld bauds\n", m_port, rxPin, txPin, baud);
#endif
  /* Configure parameters of an UART driver,
   * communication pins and install the driver */
   uart_config_t uart_config = {
     .baud_rate = (int)baud,
     .data_bits = UART_DATA_8_BITS,
     .parity = UART_PARITY_DISABLE,
     .stop_bits = UART_STOP_BITS_1,
     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
   };
   if (uart_param_config(m_port, &uart_config) != ESP_OK)
   {
     return -1;
   }
#ifdef ESP32SERIAL_DEBUG
   ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::begin::uart_param_config ok\n"));
#endif

   //Set UART pins (using UART0 default pins ie no changes.)
   if (uart_set_pin(m_port, txPin, rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
   {
     return -2;
   }
#ifdef ESP32SERIAL_DEBUG
   ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::begin::uart_set_pin ok\n"));
#endif

   //Install UART driver, and get the queue.
   const int uart_buffer_size = (1024 * 2);
   const int uart_queue_size = 20;
   if (uart_driver_install(m_port, uart_buffer_size, uart_buffer_size, uart_queue_size, &m_uart_queue, 0) != ESP_OK)
   {
     return -3;
   }
#ifdef ESP32SERIAL_DEBUG
   ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::begin::uart_driver_install ok\n"));
#endif

   return 0;
}

int ESP32Serial::available()
{
  size_t result;
  if (uart_get_buffered_data_len(m_port, &result) != ESP_OK)
  {
#ifdef ESP32SERIAL_DEBUG
     ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::available::uart_get_buffered_data_len failed\n"));
     return 0;
#endif
  }
  return (int)result;
}

void ESP32Serial::flush()
{
  if (uart_flush(m_port) != ESP_OK)
  {
#ifdef ESP32SERIAL_DEBUG
     ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::flish::uart_flush failed\n"));
#endif
  }
}

void ESP32Serial::end()
{
  if (uart_driver_delete(m_port) != ESP_OK)
  {
#ifdef ESP32SERIAL_DEBUG
     ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::end::uart_driver_delete failed\n"));
#endif
  }
}

int ESP32Serial::read(void)
{
  uint8_t data[1];
  if (available() == 0)
  {
    return -1;
  }
  uart_read_bytes(m_port, &data[0], 1, 100);
  return (int)(data[0]);
}

size_t ESP32Serial::write(uint8_t c)
{
  size_t retsize = uart_write_bytes(m_port, (const char*)&c, 1);
  if (retsize == -1)
  {
#ifdef ESP32SERIAL_DEBUG
    ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::write::uart_write_bytes 1 failed\n"));
#endif
  }
  return retsize;
}

int ESP32Serial::checkPort(void) {
 //ESP32SERIAL_DEBUG.printf(PSTR("Test %d\n"), m_port);
 if (m_port!=1) {  
  /* in our project, the port needs to be 1 (Serial 1). In case the port number was corrupted, report this. */
  ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial ERROR This is the wrong port!!!\n"));
  return -1;
 }
 return 0;
}

size_t ESP32Serial::write(const uint8_t *buffer, size_t size)
{
  size_t retsize;
  ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial in write\n"));
  Serial.flush();
  //retsize = uart_write_bytes(m_port, (const char*)buffer, size);
  ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial in write part 2\n"));
  Serial.flush();
  if (retsize == -1)
  {
#ifdef ESP32SERIAL_DEBUG
    ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::write::uart_write_bytes 2 failed\n"));
#endif
  }
  return retsize;
}

size_t ESP32Serial::print(const char* text)
{
  size_t L;
  //Serial.println("This is ESP32Serial.print");
  L=strlen(text);
  //Serial.println("Len=" + String(L)+ " port=" + String(m_port));
  //Serial.flush();
  //return write((const uint8_t*)text, strlen(text));
  return uart_write_bytes(m_port, text, L);     
}

int ESP32Serial::peek(void)
{
  uint8_t c;
  if(xQueuePeek(m_uart_queue, &c, 0)) {
      return c;
  }
#ifdef ESP32SERIAL_DEBUG
     ESP32SERIAL_DEBUG.printf(PSTR("ESP32Serial::peek failed\n"));
#endif
  return -1;
}
