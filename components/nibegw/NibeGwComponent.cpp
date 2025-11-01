#include "NibeGwComponent.h"

namespace esphome {
namespace nibegw {

NibeGwComponent::NibeGwComponent(esphome::GPIOPin *dir_pin) {
  gw_ = new NibeGw(this, dir_pin);
  gw_->setCallback(
      std::bind(&NibeGwComponent::callback_msg_received, this, std::placeholders::_1, std::placeholders::_2),
      std::bind(&NibeGwComponent::callback_msg_token_received, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3));
}

static request_data_type dedup(const uint8_t *data, int len, uint8_t val) {
  request_data_type message;
  uint8_t value = ~val;
  for (int i = 5; i < len - 1; i++) {
    if (data[i] == val && value == val) {
      value = ~val;
      continue;
    }
    value = data[i];
    message.push_back(value);
  }
  return message;
}

void NibeGwComponent::callback_msg_received(const uint8_t *data, int len) {
  {
    request_key_type key{static_cast<uint16_t>(data[2] | (data[1] << 8)), data[3]};
    const auto &it = message_listener_.find(key);
    if (it != message_listener_.end()) {
      it->second(dedup(data, len, STARTBYTE_MASTER));
    }
  }

  if (!is_connected_) {
    return;
  }

  // Send to all UDP targets
  for (auto target = udp_targets_.begin(); target != udp_targets_.end(); target++) {
    if (udp_read_fd_ < 0) {
      ESP_LOGW(TAG, "UDP read socket not available");
      continue;
    }

    sockaddr_in to{};
    to.sin_family = AF_INET;
    to.sin_port = htons(std::get<1>(*target));

    // Convert IPAddress to sockaddr format using inet_pton
    const auto &ip = std::get<0>(*target);
    if (inet_pton(AF_INET, ip.str().c_str(), &to.sin_addr) <= 0) {
      ESP_LOGW(TAG, "Invalid IP address: %s", ip.str().c_str());
      continue;
    }

    int result = sendto(udp_read_fd_, data, len, 0, (sockaddr *) &to, sizeof(to));
    if (result < 0) {
      ESP_LOGW(TAG, "UDP sendto failed to %s:%d, error: %d", ip.str().c_str(), std::get<1>(*target), errno);
    }
  }
}

bool NibeGwComponent::is_source_ip_allowed(const network::IPAddress &ip) {
  if (udp_source_ip_.empty()) {
    return true;
  }
  return std::find(udp_source_ip_.begin(), udp_source_ip_.end(), ip) != udp_source_ip_.end();
}

void NibeGwComponent::handle_udp_packet(const uint8_t *data, int len, const network::IPAddress &from_ip,
                                        uint8_t address, uint8_t token) {
  if (!is_connected_) {
    return;
  }

  if (len == 0) {
    return;
  }

  ESP_LOGV(TAG, "UDP Packet token data of %d bytes received", len);

  if (len > MAX_DATA_LEN) {
    ESP_LOGE(TAG, "UDP Packet too large: %d", len);
    return;
  }

  if (!is_source_ip_allowed(from_ip)) {
    ESP_LOGW(TAG, "UDP Packet from unauthorized IP ignored: %s", from_ip.str().c_str());
    return;
  }

  request_data_type request;
  request.assign(data, data + len);
  add_queued_request(address, token, std::move(request));
}

static int copy_request(const request_data_type &request, uint8_t *data) {
  auto len = std::min(request.size(), (size_t) MAX_DATA_LEN);
  std::copy_n(request.begin(), len, data);
  return len;
}

int NibeGwComponent::callback_msg_token_received(uint16_t address, uint8_t command, uint8_t *data) {
  request_key_type key{address, command};

  {
    const auto &it = requests_.find(key);
    if (it != requests_.end()) {
      auto &queue = it->second;
      if (!queue.empty()) {
        auto len = copy_request(queue.front(), data);
        queue.pop();
        ESP_LOGD(TAG, "Response to address: 0x%x token: 0x%x bytes: %d", std::get<0>(key), std::get<1>(key), len);
        return len;
      }
    }
  }

  {
    const auto &it = requests_provider_.find(key);
    if (it != requests_provider_.end()) {
      auto len = copy_request(it->second(), data);
      ESP_LOGD(TAG, "Response to address: 0x%x token: 0x%x bytes: %d", std::get<0>(key), std::get<1>(key), len);
      return len;
    }
  }

  return 0;
}

void NibeGwComponent::setup() {
  ESP_LOGI(TAG, "Starting up");
  gw_->connect();
}

void NibeGwComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "NibeGw");
  for (auto target = udp_targets_.begin(); target != udp_targets_.end(); target++) {
    ESP_LOGCONFIG(TAG, " Target: %s:%d", std::get<0>(*target).str().c_str(), std::get<1>(*target));
  }
  for (auto address = udp_source_ip_.begin(); address != udp_source_ip_.end(); address++) {
    ESP_LOGCONFIG(TAG, " Source: %s", address->str().c_str());
  }
  ESP_LOGCONFIG(TAG, " Read Port: %d", udp_read_port_);
  ESP_LOGCONFIG(TAG, " Write Port: %d", udp_write_port_);
}

void NibeGwComponent::loop() {
  // Handle network connection state
  if (network::is_connected() && !is_connected_) {
    ESP_LOGI(TAG, "Connecting network ports.");

    // Create and bind read socket
    udp_read_fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_read_fd_ >= 0) {
      // Set non-blocking
      int flags = fcntl(udp_read_fd_, F_GETFL, 0);
      fcntl(udp_read_fd_, F_SETFL, flags | O_NONBLOCK);

      // Bind to read port
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(udp_read_port_);
      addr.sin_addr.s_addr = INADDR_ANY;

      if (bind(udp_read_fd_, (sockaddr *) &addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind read socket to port %d, error: %d", udp_read_port_, errno);
        ::close(udp_read_fd_);
        udp_read_fd_ = -1;
      } else {
        ESP_LOGI(TAG, "UDP read socket bound to port %d", udp_read_port_);
      }
    } else {
      ESP_LOGE(TAG, "Failed to create read socket, error: %d", errno);
    }

    // Create write socket (not bound to specific port)
    udp_write_fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_write_fd_ >= 0) {
      // Set non-blocking
      int flags = fcntl(udp_write_fd_, F_GETFL, 0);
      fcntl(udp_write_fd_, F_SETFL, flags | O_NONBLOCK);

      // Bind to write port
      sockaddr_in addr{};
      addr.sin_family = AF_INET;
      addr.sin_port = htons(udp_write_port_);
      addr.sin_addr.s_addr = INADDR_ANY;

      if (bind(udp_write_fd_, (sockaddr *) &addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind write socket to port %d, error: %d", udp_write_port_, errno);
        ::close(udp_write_fd_);
        udp_write_fd_ = -1;
      } else {
        ESP_LOGI(TAG, "UDP write socket bound to port %d", udp_write_port_);
      }
    } else {
      ESP_LOGE(TAG, "Failed to create write socket, error: %d", errno);
    }

    is_connected_ = true;
  }

  if (!network::is_connected() && is_connected_) {
    ESP_LOGI(TAG, "Disconnecting network ports.");
    if (udp_read_fd_ >= 0) {
      ::close(udp_read_fd_);
      udp_read_fd_ = -1;
    }
    if (udp_write_fd_ >= 0) {
      ::close(udp_write_fd_);
      udp_write_fd_ = -1;
    }
    is_connected_ = false;
  }

  // Poll UDP read socket for incoming packets
  if (is_connected_ && udp_read_fd_ >= 0) {
    uint8_t buf[MAX_DATA_LEN];
    sockaddr_in from{};
    socklen_t fromlen = sizeof(from);

    int n = recvfrom(udp_read_fd_, buf, sizeof(buf), 0, (sockaddr *) &from, &fromlen);
    if (n > 0) {
      uint32_t host_ip = ntohl(from.sin_addr.s_addr);
      // Construct IPAddress from individual octets
      uint8_t octet1 = (host_ip >> 24) & 0xFF;
      uint8_t octet2 = (host_ip >> 16) & 0xFF;
      uint8_t octet3 = (host_ip >> 8) & 0xFF;
      uint8_t octet4 = host_ip & 0xFF;
      network::IPAddress ip(octet1, octet2, octet3, octet4);

      ESP_LOGV(TAG, "Received UDP packet from %s, %d bytes", ip.str().c_str(), n);
      handle_udp_packet(buf, n, ip, MODBUS40, READ_TOKEN);
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      ESP_LOGW(TAG, "recvfrom error on read socket: %d", errno);
    }
  }

  // Poll UDP write socket for incoming packets
  if (is_connected_ && udp_write_fd_ >= 0) {
    uint8_t buf[MAX_DATA_LEN];
    sockaddr_in from{};
    socklen_t fromlen = sizeof(from);

    int n = recvfrom(udp_write_fd_, buf, sizeof(buf), 0, (sockaddr *) &from, &fromlen);
    if (n > 0) {
      uint32_t host_ip = ntohl(from.sin_addr.s_addr);
      // Construct IPAddress from individual octets
      uint8_t octet1 = (host_ip >> 24) & 0xFF;
      uint8_t octet2 = (host_ip >> 16) & 0xFF;
      uint8_t octet3 = (host_ip >> 8) & 0xFF;
      uint8_t octet4 = host_ip & 0xFF;
      network::IPAddress ip(octet1, octet2, octet3, octet4);

      ESP_LOGV(TAG, "Received UDP packet from %s, %d bytes", ip.str().c_str(), n);
      handle_udp_packet(buf, n, ip, MODBUS40, WRITE_TOKEN);
    } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
      ESP_LOGW(TAG, "recvfrom error on write socket: %d", errno);
    }
  }

  // Handle high frequency loop requirement
  if (gw_->messageStillOnProgress()) {
    high_freq_.start();
  } else {
    high_freq_.stop();
  }

  gw_->loop();
}

}  // namespace nibegw
}  // namespace esphome