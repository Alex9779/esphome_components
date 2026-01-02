#include "uart_mitm.h"
#include "esphome/core/log.h"

namespace esphome {
namespace serial {

static const char *const TAG = "uart_mitm";

bool UARTMITM::should_log_message_(const std::vector<uint8_t> &message) {
  // Check exclude filters first - if message matches any exclude pattern, don't log it
  for (const auto &pattern : this->exclude_filters_) {
    if (message.size() >= pattern.size()) {
      bool match = true;
      for (size_t i = 0; i < pattern.size(); i++) {
        if (message[i] != pattern[i]) {
          match = false;
          break;
        }
      }
      if (match) {
        return false;  // Excluded
      }
    }
  }
  
  // If include filters exist, only log messages that match at least one include pattern
  if (!this->include_filters_.empty()) {
    for (const auto &pattern : this->include_filters_) {
      if (message.size() >= pattern.size()) {
        bool match = true;
        for (size_t i = 0; i < pattern.size(); i++) {
          if (message[i] != pattern[i]) {
            match = false;
            break;
          }
        }
        if (match) {
          return true;  // Included
        }
      }
    }
    return false;  // Include filters exist but message didn't match any
  }
  
  // Legacy filter support (deprecated - use include_filters instead)
  if (!this->message_filters_.empty()) {
    for (const auto &pattern : this->message_filters_) {
      if (message.size() >= pattern.size()) {
        bool match = true;
        for (size_t i = 0; i < pattern.size(); i++) {
          if (message[i] != pattern[i]) {
            match = false;
            break;
          }
        }
        if (match) {
          return true;
        }
      }
    }
    return false;
  }
  
  // No filters, log everything
  return true;
}

void UARTMITM::log_message_(const char *direction, const std::vector<uint8_t> &message) {
  if (message.empty()) {
    return;
  }
  
  // Log all messages when no filters, or only matching messages when filters exist
  if (!should_log_message_(message)) {
    return;
  }
  
  std::string hex_str;
  for (size_t i = 0; i < message.size(); i++) {
    char buf[4];
    snprintf(buf, sizeof(buf), "%02X", message[i]);
    hex_str += buf;
    if (i < message.size() - 1) {
      hex_str += ":";
    }
  }
  
  // Use configured uart names
  const char *direction_str = (direction[4] == '1') 
    ? (this->uart1_name_ + "->" + this->uart2_name_).c_str()
    : (this->uart2_name_ + "->" + this->uart1_name_).c_str();
  ESP_LOGD(TAG, "%s: %s", direction_str, hex_str.c_str());
}

bool UARTMITM::is_packet_start_(const std::vector<uint8_t> &buffer) {
  if (this->packet_headers_.empty()) {
    return false;  // No headers defined
  }
  
  // Check if buffer starts with any defined header
  for (const auto &header : this->packet_headers_) {
    if (buffer.size() >= header.size()) {
      bool match = true;
      for (size_t i = 0; i < header.size(); i++) {
        if (buffer[i] != header[i]) {
          match = false;
          break;
        }
      }
      if (match) {
        return true;
      }
    }
  }
  return false;
}

void UARTMITM::process_byte_(const char *direction, uint8_t byte, std::vector<uint8_t> &buffer) {
  // Add the byte first
  buffer.push_back(byte);
  
  // If we have packet headers defined, check if we've started a new packet
  if (!this->packet_headers_.empty() && buffer.size() > 1) {
    // Check if the buffer (starting from the last byte going backwards) matches a header
    // This means we found a new packet start within the buffer
    for (const auto &header : this->packet_headers_) {
      if (buffer.size() > header.size()) {
        // Check if header appears at the end of buffer (indicating start of new packet)
        size_t check_pos = buffer.size() - header.size();
        bool match = true;
        for (size_t i = 0; i < header.size(); i++) {
          if (buffer[check_pos + i] != header[i]) {
            match = false;
            break;
          }
        }
        if (match) {
          // Found a new packet header, log everything before it
          std::vector<uint8_t> prev_packet(buffer.begin(), buffer.begin() + check_pos);
          if (!prev_packet.empty()) {
            log_message_(direction, prev_packet);
          }
          // Keep only the new packet header in buffer
          buffer = std::vector<uint8_t>(buffer.begin() + check_pos, buffer.end());
          return;
        }
      }
    }
  }
}

void UARTMITM::loop() {
  uint8_t c;
  
  while (this->uart1_->available()) {
    this->uart1_->read_byte(&c);
    this->uart2_->write_byte(c);
    this->process_byte_("UART1->UART2", c, buffer1_);
  }
  while (this->uart2_->available()) {
    this->uart2_->read_byte(&c);
    this->uart1_->write_byte(c);
    this->process_byte_("UART2->UART1", c, buffer2_);
  }
}

void UARTMITM::dump_config() {
  ESP_LOGCONFIG(TAG, "UART MITM");
  ESP_LOGCONFIG(TAG, "  %s <-> %s", this->uart1_name_.c_str(), this->uart2_name_.c_str());
  if (!this->packet_headers_.empty()) {
    ESP_LOGCONFIG(TAG, "  Packet Headers: %d patterns", this->packet_headers_.size());
    for (size_t i = 0; i < this->packet_headers_.size(); i++) {
      std::string hex_str;
      for (size_t j = 0; j < this->packet_headers_[i].size(); j++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", this->packet_headers_[i][j]);
        hex_str += buf;
        if (j < this->packet_headers_[i].size() - 1) {
          hex_str += ":";
        }
      }
      ESP_LOGCONFIG(TAG, "    Header %d: %s", i + 1, hex_str.c_str());
    }
  }
  if (!this->message_filters_.empty()) {
    ESP_LOGCONFIG(TAG, "  Message Filters (deprecated): %d patterns", this->message_filters_.size());
    for (size_t i = 0; i < this->message_filters_.size(); i++) {
      std::string hex_str;
      for (size_t j = 0; j < this->message_filters_[i].size(); j++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", this->message_filters_[i][j]);
        hex_str += buf;
        if (j < this->message_filters_[i].size() - 1) {
          hex_str += ":";
        }
      }
      ESP_LOGCONFIG(TAG, "    Pattern %d: %s", i + 1, hex_str.c_str());
    }
  }
  if (!this->include_filters_.empty()) {
    ESP_LOGCONFIG(TAG, "  Include Filters: %d patterns", this->include_filters_.size());
    for (size_t i = 0; i < this->include_filters_.size(); i++) {
      std::string hex_str;
      for (size_t j = 0; j < this->include_filters_[i].size(); j++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", this->include_filters_[i][j]);
        hex_str += buf;
        if (j < this->include_filters_[i].size() - 1) {
          hex_str += ":";
        }
      }
      ESP_LOGCONFIG(TAG, "    Pattern %d: %s", i + 1, hex_str.c_str());
    }
  }
  if (!this->exclude_filters_.empty()) {
    ESP_LOGCONFIG(TAG, "  Exclude Filters: %d patterns", this->exclude_filters_.size());
    for (size_t i = 0; i < this->exclude_filters_.size(); i++) {
      std::string hex_str;
      for (size_t j = 0; j < this->exclude_filters_[i].size(); j++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", this->exclude_filters_[i][j]);
        hex_str += buf;
        if (j < this->exclude_filters_[i].size() - 1) {
          hex_str += ":";
        }
      }
      ESP_LOGCONFIG(TAG, "    Pattern %d: %s", i + 1, hex_str.c_str());
    }
  }
}

}  // namespace serial
}  // namespace esphome
