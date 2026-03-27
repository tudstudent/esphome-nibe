#include "NibeGwCoilNumber.h"
#include "NibeGwCoilPoller.h"
#include "NibeGwComponent.h"
#include "NibeGw.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nibegw {

static const char *TAG = "nibegw.number";

void NibeGwCoilNumber::setup() {
  // Register as listener so we get state updates when the pump confirms the value
  poller_->register_coil(address_, static_cast<CoilSize>(coil_size_), factor_, poll_group_,
                         [this](float value) { this->publish_state(value); });
}

void NibeGwCoilNumber::dump_config() {
  LOG_NUMBER("", "NibeGW Coil Number", this);
  ESP_LOGCONFIG(TAG, "  Address: %u", address_);
  ESP_LOGCONFIG(TAG, "  Size: %u", coil_size_);
  ESP_LOGCONFIG(TAG, "  Factor: %u", factor_);
}

void NibeGwCoilNumber::control(float value) {
  // Encode the value and build a write request packet
  int32_t raw;
  if (factor_ > 1) {
    raw = (int32_t) roundf(value * (float) factor_);
  } else {
    raw = (int32_t) roundf(value);
  }

  // Build payload: address (2 bytes LE) + value (2 or 4 bytes LE)
  std::vector<uint8_t> payload;
  payload.push_back(address_ & 0xFF);
  payload.push_back((address_ >> 8) & 0xFF);

  switch (static_cast<CoilSize>(coil_size_)) {
    case COIL_SIZE_U8:
    case COIL_SIZE_S8:
    case COIL_SIZE_U16:
    case COIL_SIZE_S16:
      payload.push_back(raw & 0xFF);
      payload.push_back((raw >> 8) & 0xFF);
      break;
    case COIL_SIZE_U32:
    case COIL_SIZE_S32:
      payload.push_back(raw & 0xFF);
      payload.push_back((raw >> 8) & 0xFF);
      payload.push_back((raw >> 16) & 0xFF);
      payload.push_back((raw >> 24) & 0xFF);
      break;
  }

  // Build the packet: start, token, len, payload, checksum
  request_data_type packet;
  packet.push_back(STARTBYTE_SLAVE);
  packet.push_back(WRITE_TOKEN);
  packet.push_back(payload.size());
  for (auto b : payload) {
    packet.push_back(b);
  }

  uint8_t checksum = 0;
  for (auto b : packet) {
    checksum ^= b;
  }
  if (checksum == 0x5C) {
    checksum = 0xC5;
  }
  packet.push_back(checksum);

  ESP_LOGD(TAG, "Writing coil %u = %.2f (raw: %d)", address_, value, (int) raw);
  gw_->add_queued_request(MODBUS40, WRITE_TOKEN, std::move(packet));
}

}  // namespace nibegw
}  // namespace esphome
