# Architecture Overview

## System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Home Assistant                        │
│                     (Climate Control UI)                     │
└────────────────────────┬────────────────────────────────────┘
                         │ ESPHome API
                         │
┌────────────────────────▼────────────────────────────────────┐
│                    ESP32-C3 Device                           │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              NibeGwClimate Component                  │   │
│  │         (Climate Entity - Temperature Control)       │   │
│  └─────────────────────┬────────────────────────────────┘   │
│                        │                                     │
│  ┌─────────────────────▼────────────────────────────────┐   │
│  │            NibeGwComponent                            │   │
│  │                                                        │   │
│  │  ┌───────────────┐        ┌──────────────────────┐  │   │
│  │  │  UDP Handler  │        │   Request Manager    │  │   │
│  │  │  (BSD Sockets)│◄──────►│   (Queues & Cache)   │  │   │
│  │  └───────┬───────┘        └──────────┬───────────┘  │   │
│  │          │                           │               │   │
│  │          │                           ▼               │   │
│  │          │                  ┌────────────────────┐  │   │
│  │          │                  │   NibeGw Protocol  │  │   │
│  │          │                  │   State Machine    │  │   │
│  │          │                  └─────────┬──────────┘  │   │
│  └──────────┼────────────────────────────┼─────────────┘   │
│             │                            │                  │
│  ┌──────────▼────────────┐    ┌──────────▼──────────────┐  │
│  │  Ethernet (DM9051)    │    │   UART (RS-485)         │  │
│  │  lwIP Stack           │    │   Direction Control     │  │
│  └───────────────────────┘    └─────────────────────────┘  │
└──────────┬──────────────────────────────┬──────────────────┘
           │                              │
           │ UDP 9999/10000               │ RS-485 Protocol
           │                              │
┌──────────▼──────────────┐    ┌──────────▼──────────────────┐
│   Network Devices        │    │    Nibe Heat Pump           │
│   (Monitoring/Control)   │    │    (MODBUS40/RMU40)         │
└──────────────────────────┘    └─────────────────────────────┘
```

## Component Communication Flow

### 1. Climate Control (Home Assistant → Heat Pump)

```
Home Assistant
    │
    │ 1. User sets temperature
    │
    ▼
NibeGwClimate
    │
    │ 2. publish_set_point(temp)
    │
    ▼
Request Manager
    │
    │ 3. Queue RMU_WRITE request
    │
    ▼
NibeGw Protocol
    │
    │ 4. Wait for token from heat pump
    │
    ▼
RS-485 UART
    │
    │ 5. Send temperature data
    │
    ▼
Nibe Heat Pump
```

### 2. Temperature Reading (Heat Pump → Home Assistant)

```
Nibe Heat Pump
    │
    │ 1. Sends RMU_DATA_MSG
    │
    ▼
RS-485 UART
    │
    │ 2. Receive and validate checksum
    │
    ▼
NibeGw Protocol
    │
    │ 3. Parse temperature data
    │
    ▼
NibeGwClimate
    │
    │ 4. Update current_temperature
    │
    ▼
ESPHome API
    │
    │ 5. Push state to HA
    │
    ▼
Home Assistant
```

### 3. UDP Forwarding (Network → Heat Pump)

```
Network Device
    │
    │ 1. Send UDP packet (port 10000)
    │
    ▼
Ethernet Interface
    │
    │ 2. lwIP receives packet
    │
    ▼
UDP Handler (recvfrom)
    │
    │ 3. Validate source IP
    │
    ▼
Request Manager
    │
    │ 4. Queue MODBUS request
    │
    ▼
NibeGw Protocol
    │
    │ 5. Wait for token
    │
    ▼
RS-485 UART
    │
    │ 6. Forward to heat pump
    │
    ▼
Nibe Heat Pump
```

### 4. UDP Broadcasting (Heat Pump → Network)

```
Nibe Heat Pump
    │
    │ 1. Sends data message
    │
    ▼
RS-485 UART
    │
    │ 2. Receive complete message
    │
    ▼
NibeGw Protocol
    │
    │ 3. Validate and dedup
    │
    ▼
UDP Handler (sendto)
    │
    │ 4. Send to all targets
    │
    ▼
Ethernet Interface
    │
    │ 5. lwIP transmits packets
    │
    ▼
Network Devices
```

## Data Flow Timing

```
Time →

ESP32 Loop (every ~16ms):
├─ Check network connection status
├─ Poll UDP sockets (non-blocking)
│  ├─ Read socket (port 9999)
│  └─ Write socket (port 10000)
├─ Process received UDP packets
├─ Execute RS-485 state machine
│  ├─ Read UART buffer
│  ├─ Parse protocol messages
│  └─ Send responses/ACKs
└─ Update climate state if needed

Heat Pump Polling (~1s intervals):
├─ Request data (token sent)
├─ ESP32 responds with queued data
└─ Heat pump ACKs or NAKs

Climate Updates (event-driven):
├─ Sensor state change → update temp
├─ User sets target → queue request
└─ Timeout (10s) → invalidate data
```

## Memory Layout

```
ESP32-C3 Memory Map:

┌─────────────────────────────────────┐
│   Flash (Program Storage)           │
│   ~500KB used                        │
├─────────────────────────────────────┤
│   DRAM (Heap + Stack)                │
│   ┌──────────────────────────────┐  │
│   │  NibeGw State Machine        │  │
│   │  ~2KB (buffers + state)      │  │
│   ├──────────────────────────────┤  │
│   │  Request Queues              │  │
│   │  ~5KB (3 requests × ~1.5KB)  │  │
│   ├──────────────────────────────┤  │
│   │  lwIP Network Stack          │  │
│   │  ~20KB (buffers + state)     │  │
│   ├──────────────────────────────┤  │
│   │  ESPHome Framework           │  │
│   │  ~10KB                       │  │
│   ├──────────────────────────────┤  │
│   │  Free Heap                   │  │
│   │  ~250KB available            │  │
│   └──────────────────────────────┘  │
└─────────────────────────────────────┘
```

## Network Packet Format

### UDP Packet Structure
```
┌──────────────────────────────────────┐
│  UDP Header (8 bytes)                │
├──────────────────────────────────────┤
│  Nibe Protocol Data (variable)       │
│  ┌────────────────────────────────┐  │
│  │ 5C 00    │ Start bytes         │  │
│  ├────────────────────────────────┤  │
│  │ ADDR     │ Address (16-bit)    │  │
│  ├────────────────────────────────┤  │
│  │ CMD      │ Command byte        │  │
│  ├────────────────────────────────┤  │
│  │ LEN      │ Data length         │  │
│  ├────────────────────────────────┤  │
│  │ DATA...  │ Payload (0-80 bytes)│  │
│  ├────────────────────────────────┤  │
│  │ CHK      │ XOR Checksum        │  │
│  └────────────────────────────────┘  │
└──────────────────────────────────────┘
```

### RS-485 Frame Structure (Identical)
```
Same format as UDP payload above
Transmitted serially at 9600 baud
Direction control via GPIO19
```

## Thread/Task Model

ESP-IDF uses FreeRTOS, but ESPHome uses cooperative multitasking:

```
┌─────────────────────────────────────────────┐
│  Main Loop (Component::loop())              │
│  ┌───────────────────────────────────────┐  │
│  │  1. Network check                     │  │
│  │  2. UDP socket polling (non-blocking) │  │
│  │  3. UART reading (non-blocking)       │  │
│  │  4. State machine processing          │  │
│  │  5. Yield to other components         │  │
│  └───────────────────────────────────────┘  │
│  ⟲ Repeat ~60 times per second              │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│  Background Tasks (ESP-IDF)                 │
│  ├─ WiFi/Ethernet driver (hardware IRQ)    │
│  ├─ lwIP TCP/IP stack (task)               │
│  ├─ UART driver (DMA + IRQ)                │
│  └─ System housekeeping                    │
└─────────────────────────────────────────────┘
```

## Key Design Decisions

### Why BSD Sockets Instead of AsyncUDP?

| Feature | AsyncUDP | BSD Sockets |
|---------|----------|-------------|
| Framework | Arduino only | ESP-IDF native |
| Callback-based | Yes (complex) | No (simple polling) |
| Ethernet support | Limited | Full |
| Memory overhead | Higher | Lower |
| Portability | Low | High |
| ESP-IDF compatible | No | Yes |

### Why Non-Blocking Sockets?

- ESPHome uses cooperative multitasking
- Blocking calls would stall entire system
- Non-blocking allows other components to run
- Uses `O_NONBLOCK` + `EAGAIN`/`EWOULDBLOCK` pattern

### Why Two Separate UDP Sockets?

- Read socket (9999): Receives from network, forwards to heat pump
- Write socket (10000): Receives write commands from network
- Separation allows different source filtering per direction
- Matches original Arduino implementation for compatibility

## Performance Characteristics

### Latency Breakdown (typical)

```
Component                           Time
───────────────────────────────────────────
UDP packet received                 <1ms
  ├─ Ethernet hardware IRQ          ~50µs
  ├─ lwIP processing                ~200µs
  └─ Application recvfrom()         ~100µs
                                    
Request queuing                     <100µs
  
RS-485 transmission (per byte)      ~1ms
  ├─ Direction pin switching        ~2µs
  ├─ UART TX (9600 baud)            ~1040µs
  └─ Direction pin restore          ~2µs
  
Total end-to-end latency            ~5ms
```

### Throughput Limits

```
RS-485 (9600 baud):                 ~960 bytes/sec
  └─ Practical throughput:          ~800 bytes/sec
                                    (accounting for overhead)

Ethernet (100Mbps):                 12.5 MB/sec
  └─ Practical UDP throughput:      ~10 MB/sec
  
Limiting factor:                    RS-485 (heat pump interface)
```

## Error Handling Strategy

```
┌─────────────────────────────────────────┐
│  Error Detection                        │
├─────────────────────────────────────────┤
│  1. Socket errors → Log + retry         │
│  2. Checksum failures → NAK + retry     │
│  3. Timeouts → Invalidate state         │
│  4. Queue full → Drop oldest            │
│  5. Network loss → Close sockets        │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  Recovery Strategy                      │
├─────────────────────────────────────────┤
│  1. Network reconnect → Reopen sockets  │
│  2. State machine → Auto-reset          │
│  3. Climate data → 10s timeout          │
│  4. Memory → No dynamic allocation      │
└─────────────────────────────────────────┘
```

## Debugging Hooks

```cpp
// Enable in code for detailed logging:
#define ESPHOME_LOG_LEVEL ESPHOME_LOG_LEVEL_VERBOSE

// Key debug points:
ESP_LOGV(TAG, "UDP packet received: %d bytes", n);
ESP_LOGV(TAG, "Socket FD: %d", udp_read_fd_);
ESP_LOGV(TAG, "RS-485 state: %d", state);
ESP_LOGVV(TAG, "Byte: %02X", b);
```

## Extension Points

To add new functionality:

1. **New Protocol Commands**: Add to `NibeGw.h` enums
2. **Additional UDP Targets**: Modify `add_target()` calls
3. **New Climate Entities**: Add to `climate.py` config
4. **Custom Request Handling**: Implement `request_provider_type`
5. **Message Listeners**: Register with `add_listener()`

---

**Next Steps**: Review the code files to see how these architectural concepts are implemented.