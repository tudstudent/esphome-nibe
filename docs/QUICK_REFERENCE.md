# Quick Reference: Arduino → ESP-IDF Migration

## Key Type Changes

| Old (Arduino) | New (Standard C++) |
|---------------|-------------------|
| `byte`        | `uint8_t`         |
| `boolean`     | `bool`            |
| `#include <Arduino.h>` | `#include <cstdint>` |

## UDP Implementation Changes

### Before (AsyncUDP):
```cpp
#include "AsyncUDP.h"

AsyncUDP udp_read_;
AsyncUDP udp_write_;

// In setup:
udp_read_.listen(port);
udp_read_.onPacket([this](AsyncUDPPacket packet) {
    // Handle packet
});

// Send:
udp_read_.writeTo(data, len, &addr, port);

// Cleanup:
udp_read_.close();
```

### After (BSD Sockets):
```cpp
#include "lwip/sockets.h"
#include <fcntl.h>

int udp_read_fd_ = -1;
int udp_write_fd_ = -1;

// In loop() connect section:
udp_read_fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
fcntl(udp_read_fd_, F_SETFL, O_NONBLOCK);
sockaddr_in addr{};
addr.sin_family = AF_INET;
addr.sin_port = htons(port);
addr.sin_addr.s_addr = INADDR_ANY;
bind(udp_read_fd_, (sockaddr*)&addr, sizeof(addr));

// In loop() - poll for packets:
uint8_t buf[MAX_DATA_LEN];
sockaddr_in from{};
socklen_t fromlen = sizeof(from);
int n = recvfrom(udp_read_fd_, buf, sizeof(buf), 0, (sockaddr*)&from, &fromlen);
if (n > 0) {
    // Handle packet
}

// Send:
sockaddr_in to{};
to.sin_family = AF_INET;
to.sin_port = htons(port);
to.sin_addr.s_addr = htonl(ip);
sendto(udp_read_fd_, data, len, 0, (sockaddr*)&to, sizeof(to));

// Cleanup:
if (udp_read_fd_ >= 0) {
    ::close(udp_read_fd_);
    udp_read_fd_ = -1;
}
```

## Build Configuration Changes

### Before (__init__.py):
```python
if CORE.is_esp32:
    cg.add_library("ESP32 Async UDP", None)
if CORE.is_esp8266:
    cg.add_library("ESPAsyncUDP", None)
```

### After (__init__.py):
```python
# No external UDP libraries needed
# ESP-IDF provides lwIP BSD sockets natively
if CORE.is_esp32:
    cg.add_build_flag("-DHARDWARE_SERIAL_WITH_PINS")
```

## Common Patterns

### Converting IP addresses:
```cpp
// AsyncUDP used its own IPAddress format
// BSD sockets use network byte order:

// From sockaddr to ESPHome IPAddress:
uint32_t host_ip = ntohl(from.sin_addr.s_addr);
network::IPAddress ip(host_ip);

// To sockaddr from ESPHome IPAddress:
to.sin_addr.s_addr = htonl(static_cast<uint32_t>(ip));
```

### Non-blocking sockets:
```cpp
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);

// When reading/writing, handle EAGAIN/EWOULDBLOCK:
int n = recvfrom(...);
if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
    // Real error
    ESP_LOGW(TAG, "Socket error: %d", errno);
}
```

## ESPHome Network Abstraction

The component uses ESPHome's network abstraction which works with both WiFi and Ethernet:

```cpp
#include "esphome/components/network/util.h"

// Check if network (WiFi or Ethernet) is connected:
if (network::is_connected()) {
    // Initialize sockets
}
```

## Debugging Tips

### Enable verbose logging in YAML:
```yaml
logger:
  level: VERBOSE
  logs:
    nibegw: VERBOSE
```

### Check socket creation:
```cpp
if (udp_read_fd_ < 0) {
    ESP_LOGE(TAG, "Failed to create socket: errno=%d", errno);
}
```

### Monitor packet flow:
```bash
# On the ESP32:
# Check network interface
esphome logs device.yaml

# On host computer:
# Monitor UDP traffic
tcpdump -i eth0 udp port 9999
```

## Porting Your Own Code

If you have other AsyncUDP code to port:

1. **Replace includes**:
   - Remove `AsyncUDP.h`
   - Add `lwip/sockets.h`, `fcntl.h`

2. **Replace objects**:
   - `AsyncUDP` → `int` (socket file descriptor)
   - Initialize to `-1`

3. **Replace methods**:
   - `.listen(port)` → `socket()` + `bind()`
   - `.onPacket(lambda)` → poll `recvfrom()` in loop
   - `.writeTo()` → `sendto()`
   - `.close()` → `::close()`

4. **Update types**:
   - `byte` → `uint8_t`
   - `boolean` → `bool`

5. **Handle blocking**:
   - Set `O_NONBLOCK` flag
   - Check for `EAGAIN`/`EWOULDBLOCK`

6. **Test thoroughly**:
   - Network connect/disconnect
   - Packet loss scenarios
   - High traffic loads