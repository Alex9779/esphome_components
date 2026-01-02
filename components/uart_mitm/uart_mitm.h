#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include <vector>

namespace esphome {
namespace serial {

class UARTMITM : public Component {
 public:
  float get_setup_priority() const override { return setup_priority::LATE; }
  void loop() override;
  void dump_config() override;
  void set_uart1(uart::UARTComponent *uart) { this->uart1_ = uart; }
  void set_uart2(uart::UARTComponent *uart) { this->uart2_ = uart; }
  void set_uart1_name(const std::string &name) { this->uart1_name_ = name; }
  void set_uart2_name(const std::string &name) { this->uart2_name_ = name; }
  void add_message_filter(const std::vector<uint8_t> &pattern) { this->message_filters_.push_back(pattern); }
  void add_include_filter(const std::vector<uint8_t> &pattern) { this->include_filters_.push_back(pattern); }
  void add_exclude_filter(const std::vector<uint8_t> &pattern) { this->exclude_filters_.push_back(pattern); }
  void add_packet_header(const std::vector<uint8_t> &header) { this->packet_headers_.push_back(header); }

 protected:
  void log_message_(const char *direction, const std::vector<uint8_t> &message);
  bool should_log_message_(const std::vector<uint8_t> &message);
  void process_byte_(const char *direction, uint8_t byte, std::vector<uint8_t> &buffer);
  bool is_packet_start_(const std::vector<uint8_t> &buffer);
  
  uart::UARTComponent *uart1_;
  uart::UARTComponent *uart2_;
  std::string uart1_name_{"UART1"};
  std::string uart2_name_{"UART2"};
  std::vector<std::vector<uint8_t>> message_filters_;
  std::vector<std::vector<uint8_t>> include_filters_;
  std::vector<std::vector<uint8_t>> exclude_filters_;
  std::vector<std::vector<uint8_t>> packet_headers_;
  std::vector<uint8_t> buffer1_;
  std::vector<uint8_t> buffer2_;
};

}  // namespace serial
}  // namespace esphome
