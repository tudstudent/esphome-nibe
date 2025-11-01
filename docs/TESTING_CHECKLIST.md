# Testing Checklist for ESP-IDF Migration

## Pre-Deployment Testing

### 1. Compilation Tests
- [ ] Code compiles without errors with ESP-IDF framework
- [ ] No warnings about deprecated functions
- [ ] All required libraries/dependencies resolved
- [ ] Binary size is reasonable (check flash usage)
- [ ] RAM usage is acceptable (check heap free)

### 2. Basic Network Tests
- [ ] ESP32-C3 connects to Ethernet on boot
- [ ] IP address assigned correctly (DHCP or static)
- [ ] Network remains stable over time (24h test)
- [ ] Handles network disconnection gracefully
- [ ] Reconnects automatically after network restore
- [ ] No memory leaks during connect/disconnect cycles

### 3. UDP Socket Tests
- [ ] Read socket binds to port 9999 successfully
- [ ] Write socket binds to port 10000 successfully
- [ ] Both sockets are non-blocking
- [ ] Socket creation error handling works
- [ ] Sockets close cleanly on network disconnect
- [ ] No file descriptor leaks

### 4. UDP Receive Tests
- [ ] Device receives UDP packets on read port
- [ ] Device receives UDP packets on write port
- [ ] Packets from authorized IPs are processed
- [ ] Packets from unauthorized IPs are rejected (if source filter enabled)
- [ ] Handles packet loss gracefully
- [ ] Handles malformed packets without crashing
- [ ] Can receive back-to-back packets
- [ ] No packet buffer overflows

### 5. UDP Send Tests
- [ ] Device sends packets to all configured targets
- [ ] Packets reach destination (verify with tcpdump/Wireshark)
- [ ] Send errors are logged appropriately
- [ ] Handles send failures gracefully (e.g., network congestion)
- [ ] No memory leaks on repeated sends

### 6. RS-485 Communication Tests
- [ ] UART configuration correct (baud rate, pins)
- [ ] Direction pin (dir_pin) switches correctly
- [ ] Can send data to heat pump
- [ ] Can receive data from heat pump
- [ ] Checksums calculated correctly
- [ ] ACK/NAK sent appropriately
- [ ] State machine handles all message types
- [ ] No data corruption on wire

### 7. Nibe Protocol Tests
- [ ] Device responds to MODBUS40 read requests
- [ ] Device responds to MODBUS40 write requests
- [ ] Device responds to RMU40 requests
- [ ] Device handles accessory information requests
- [ ] Token-based communication works
- [ ] Message deduplication works correctly
- [ ] Request queueing functions properly

### 8. Climate Component Tests
- [ ] Climate entity appears in Home Assistant
- [ ] Current temperature updates correctly
- [ ] Target temperature can be set
- [ ] Mode changes work (AUTO, HEAT_COOL)
- [ ] State persists across reboots
- [ ] Timeouts trigger correctly (no data/sensor)
- [ ] Sensor callback updates temperature

## Load Testing

### 9. Performance Tests
- [ ] CPU usage under normal load < 50%
- [ ] CPU usage under high UDP traffic < 80%
- [ ] Heap memory doesn't continuously decrease
- [ ] No stack overflows under load
- [ ] Loop execution time remains consistent
- [ ] High-frequency loop requester works correctly

### 10. Stress Tests
- [ ] Handles 100+ UDP packets per second
- [ ] Handles RS-485 message bursts
- [ ] No crashes after 48 hours continuous operation
- [ ] No performance degradation over time
- [ ] Recovers from temporary network failures
- [ ] Handles heat pump communication errors

## Integration Testing

### 11. Home Assistant Integration
- [ ] Device discovered by ESPHome
- [ ] Climate entity controllable from HA
- [ ] Temperature updates appear in HA
- [ ] Mode changes reflected in HA
- [ ] Logs accessible from HA
- [ ] OTA updates work

### 12. Multi-Target Tests
- [ ] Packets sent to multiple UDP targets
- [ ] All targets receive correct data
- [ ] Failure of one target doesn't affect others
- [ ] Can add/remove targets via config update

### 13. Source IP Filtering Tests
- [ ] Empty source list allows all IPs
- [ ] Populated source list blocks unauthorized IPs
- [ ] Log messages indicate blocked IPs
- [ ] Authorized IPs work correctly

## Edge Cases

### 14. Error Condition Tests
- [ ] Handles socket creation failure
- [ ] Handles bind() failure (port in use)
- [ ] Handles sendto() failure
- [ ] Handles recvfrom() errors
- [ ] Handles Ethernet cable unplug
- [ ] Handles router/switch reboot
- [ ] Handles IP address changes
- [ ] Handles DNS failures (if using hostnames)

### 15. Configuration Tests
- [ ] Works with minimal config
- [ ] Works with full config (all options)
- [ ] Invalid config rejected at compile time
- [ ] Config changes applied on OTA update
- [ ] Multiple climate entities work (S1-S4)

## Comparison Testing

### 16. Feature Parity Tests
Compare with original Arduino implementation:
- [ ] Same UDP behavior
- [ ] Same RS-485 timing
- [ ] Same protocol responses
- [ ] Same Home Assistant functionality
- [ ] Performance equal or better

## Documentation Verification

### 17. Documentation Tests
- [ ] README instructions work
- [ ] Example YAML compiles and runs
- [ ] Migration guide is accurate
- [ ] Pin assignments correct for your board
- [ ] Troubleshooting section helpful

## Final Validation

### 18. Real-World Testing
- [ ] Controls actual heat pump successfully
- [ ] Temperature readings accurate
- [ ] Set point changes take effect
- [ ] No unexpected heat pump errors
- [ ] Works alongside other ESPHome components
- [ ] No interference with other network devices

## Sign-Off

Date: ____________

Tested by: ____________

### Critical Issues Found:
```
[List any blocking issues]
```

### Minor Issues Found:
```
[List any non-blocking issues]
```

### Recommended Actions:
```
[Any follow-up needed before production deployment]
```

### Production Ready: YES / NO

---

## Quick Test Commands

### Monitor ESP32 logs:
```bash
esphome logs nibegw.yaml
```

### Monitor UDP traffic on host:
```bash
# Incoming to ESP32
tcpdump -i eth0 -n dst 192.168.1.xxx and udp

# Outgoing from ESP32  
tcpdump -i eth0 -n src 192.168.1.xxx and udp

# Specific ports
tcpdump -i eth0 -n udp port 9999 or udp port 10000
```

### Test UDP send to ESP32:
```bash
echo "test data" | nc -u 192.168.1.xxx 10000
```

### Check socket status on ESP32:
```bash
# Via ESPHome API if you add debug sensors
# Or monitor logs for bind success/failure messages
```

### Memory monitoring:
```yaml
# Add to ESPHome config for testing:
sensor:
  - platform: template
    name: "Free Heap"
    lambda: return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    update_interval: 10s
```

### CPU monitoring:
```yaml
sensor:
  - platform: template
    name: "Loop Time"  
    lambda: return millis();
    update_interval: 1s
```

## Automated Test Script Template

```python
#!/usr/bin/env python3
"""
Basic automated test for NibeGW ESP-IDF port
"""
import socket
import time

ESP_IP = "192.168.1.xxx"
READ_PORT = 9999
WRITE_PORT = 10000

def test_udp_send():
    """Test sending UDP packet to ESP32"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    test_data = b"\x5C\x00\x20\x69\x00\x49"
    sock.sendto(test_data, (ESP_IP, WRITE_PORT))
    print("✓ Sent test packet")
    sock.close()

def test_udp_receive():
    """Test receiving UDP packet from ESP32"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("", READ_PORT))
    sock.settimeout(5.0)
    try:
        data, addr = sock.recvfrom(1024)
        print(f"✓ Received {len(data)} bytes from {addr}")
        return True
    except socket.timeout:
        print("✗ No data received within timeout")
        return False
    finally:
        sock.close()

if __name__ == "__main__":
    print("Testing NibeGW UDP communication...")
    test_udp_send()
    time.sleep(1)
    test_udp_receive()
```

Run with: `python3 test_nibegw.py`