#include "NibeGwCoilPoller.h"
#include "NibeGw.h"
#include "esphome/core/log.h"

#ifdef USE_MQTT
#include "esphome/components/mqtt/mqtt_client.h"
#endif

namespace esphome {
namespace nibegw {

static const uint8_t MAX_COILS_PER_REQUEST = 20;

void NibeGwCoilPoller::register_coil(uint16_t address, CoilSize size, uint16_t factor,
                                     std::function<void(float)> callback) {
  size_t idx = coils_.size();
  coils_.push_back({address, size, factor, std::move(callback)});
  coil_index_[address] = idx;
}

void NibeGwCoilPoller::setup() {
  // Set up the history buffer capacity
  if (buffer_mode_ == BUFFER_MODE_HISTORY && buffer_size_bytes_ > 0) {
    size_t capacity = buffer_size_bytes_ / sizeof(BufferEntry);
    if (capacity > 0) {
      history_buffer_.resize(capacity);
    }
  }

  // Register as listener for MODBUS40 READ_TOKEN responses
  gw_->add_listener(MODBUS40, READ_TOKEN,
                     [this](const request_data_type &data) { this->on_data_received(data); });

  ESP_LOGI(TAG, "Coil poller set up with %zu coils, poll interval %u ms", coils_.size(), poll_interval_ms_);
}

void NibeGwCoilPoller::loop() {
  uint32_t now = millis();

  // Check MQTT connection state for buffer flush
  bool connected = is_mqtt_connected();
  if (connected && !was_connected_) {
    ESP_LOGI(TAG, "MQTT reconnected, flushing buffer");
    flush_buffer();
  }
  was_connected_ = connected;

  // Poll on interval
  if (coils_.empty()) {
    return;
  }

  if (now - last_poll_ >= poll_interval_ms_) {
    last_poll_ = now;
    auto request = build_poll_request();
    if (!request.empty()) {
      gw_->add_queued_request(MODBUS40, READ_TOKEN, std::move(request));
    }
  }
}

void NibeGwCoilPoller::dump_config() {
  ESP_LOGCONFIG(TAG, "NibeGW Coil Poller:");
  ESP_LOGCONFIG(TAG, "  Poll interval: %u ms", poll_interval_ms_);
  ESP_LOGCONFIG(TAG, "  Buffer mode: %s",
                buffer_mode_ == BUFFER_MODE_OFF        ? "off"
                : buffer_mode_ == BUFFER_MODE_LATEST_ONLY ? "latest_only"
                                                          : "history");
  if (buffer_mode_ == BUFFER_MODE_HISTORY) {
    ESP_LOGCONFIG(TAG, "  Buffer size: %zu bytes (%zu entries)", buffer_size_bytes_,
                  history_buffer_.size());
  }
  ESP_LOGCONFIG(TAG, "  Registered coils: %zu", coils_.size());
  for (auto &coil : coils_) {
    ESP_LOGCONFIG(TAG, "    Address: %u, Size: %u, Factor: %u", coil.address, coil.size, coil.factor);
  }
}

request_data_type NibeGwCoilPoller::build_poll_request() {
  // Build a slave response packet with coil addresses to read
  // Format: C0 69 len [addr_lo addr_hi ...] checksum
  size_t count = std::min((size_t) MAX_COILS_PER_REQUEST, coils_.size());

  std::vector<uint8_t> payload;
  for (size_t i = 0; i < count; i++) {
    size_t idx = (poll_offset_ + i) % coils_.size();
    uint16_t addr = coils_[idx].address;
    payload.push_back(addr & 0xFF);
    payload.push_back((addr >> 8) & 0xFF);
  }

  // Advance round-robin offset for next poll
  poll_offset_ = (poll_offset_ + count) % coils_.size();

  // Build the packet: start, token, len, payload, checksum
  request_data_type packet;
  packet.push_back(STARTBYTE_SLAVE);
  packet.push_back(READ_TOKEN);
  packet.push_back(payload.size());
  for (auto b : payload) {
    packet.push_back(b);
  }

  // Calculate checksum (XOR of all bytes)
  uint8_t checksum = 0;
  for (auto b : packet) {
    checksum ^= b;
  }
  if (checksum == 0x5C) {
    checksum = 0xC5;
  }
  packet.push_back(checksum);

  return packet;
}

void NibeGwCoilPoller::on_data_received(const request_data_type &data) {
  // Data is the deduped message body (after start byte removal)
  // Format: [addr_lo, addr_hi, value_b0, value_b1, ...] repeated for each coil
  size_t offset = 0;
  while (offset + 2 <= data.size()) {
    uint16_t address = data[offset] | (data[offset + 1] << 8);
    offset += 2;

    auto it = coil_index_.find(address);
    if (it == coil_index_.end()) {
      // Unknown coil - skip 2 bytes (assume u16 default)
      offset += 2;
      continue;
    }

    auto &coil = coils_[it->second];
    uint8_t bytes_needed = coil_data_bytes(coil.size);

    if (offset + bytes_needed > data.size()) {
      ESP_LOGW(TAG, "Truncated data for coil %u", address);
      break;
    }

    float value = decode_coil_value(&data[offset], coil.size, coil.factor);
    offset += bytes_needed;

    ESP_LOGD(TAG, "Coil %u = %.2f", address, value);

    // Dispatch to sensor
    coil.callback(value);

    // Buffer the value
    buffer_value(address, value);
  }
}

float NibeGwCoilPoller::decode_coil_value(const uint8_t *data, CoilSize size, uint16_t factor) {
  float raw;
  switch (size) {
    case COIL_SIZE_U8:
      raw = (float) data[0];
      break;
    case COIL_SIZE_S8:
      raw = (float) (int8_t) data[0];
      break;
    case COIL_SIZE_U16:
      raw = (float) (uint16_t)(data[0] | (data[1] << 8));
      break;
    case COIL_SIZE_S16:
      raw = (float) (int16_t)(data[0] | (data[1] << 8));
      break;
    case COIL_SIZE_U32:
      raw = (float) (uint32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
      break;
    case COIL_SIZE_S32:
      raw = (float) (int32_t)(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
      break;
    default:
      return NAN;
  }
  if (factor > 1) {
    return raw / (float) factor;
  }
  return raw;
}

uint8_t NibeGwCoilPoller::coil_data_bytes(CoilSize size) {
  switch (size) {
    case COIL_SIZE_U8:
    case COIL_SIZE_S8:
      return 2;  // Nibe pads u8/s8 to 2 bytes in the protocol
    case COIL_SIZE_U16:
    case COIL_SIZE_S16:
      return 2;
    case COIL_SIZE_U32:
    case COIL_SIZE_S32:
      return 4;
    default:
      return 2;
  }
}

void NibeGwCoilPoller::buffer_value(uint16_t address, float value) {
  if (buffer_mode_ == BUFFER_MODE_OFF) {
    return;
  }

  if (buffer_mode_ == BUFFER_MODE_LATEST_ONLY) {
    latest_buffer_[address] = value;
    return;
  }

  // BUFFER_MODE_HISTORY
  if (history_buffer_.empty()) {
    return;
  }

  size_t capacity = history_buffer_.size();
  size_t write_idx = (history_head_ + history_count_) % capacity;
  history_buffer_[write_idx] = {millis(), address, value};
  if (history_count_ < capacity) {
    history_count_++;
  } else {
    // Buffer full, advance head (evict oldest)
    history_head_ = (history_head_ + 1) % capacity;
  }
}

void NibeGwCoilPoller::flush_buffer() {
  if (buffer_mode_ == BUFFER_MODE_LATEST_ONLY) {
    for (auto &[address, value] : latest_buffer_) {
      auto it = coil_index_.find(address);
      if (it != coil_index_.end()) {
        coils_[it->second].callback(value);
      }
    }
    latest_buffer_.clear();
    return;
  }

  if (buffer_mode_ == BUFFER_MODE_HISTORY) {
    ESP_LOGI(TAG, "Flushing %zu buffered entries", history_count_);
    for (size_t i = 0; i < history_count_; i++) {
      size_t idx = (history_head_ + i) % history_buffer_.size();
      auto &entry = history_buffer_[idx];
      auto it = coil_index_.find(entry.address);
      if (it != coil_index_.end()) {
        coils_[it->second].callback(entry.value);
      }
    }
    history_count_ = 0;
    history_head_ = 0;
    return;
  }
}

bool NibeGwCoilPoller::is_mqtt_connected() {
#ifdef USE_MQTT
  if (mqtt::global_mqtt_client != nullptr) {
    return mqtt::global_mqtt_client->is_connected();
  }
#endif
  return true;  // If no MQTT, consider always "connected" (no buffering needed)
}

}  // namespace nibegw
}  // namespace esphome
