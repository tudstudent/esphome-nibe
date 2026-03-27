#pragma once

#include "esphome/core/component.h"
#include "esphome/components/number/number.h"

namespace esphome {
namespace nibegw {

class NibeGwComponent;
class NibeGwCoilPoller;

class NibeGwCoilNumber : public number::Number, public Component {
 public:
  void set_gw(NibeGwComponent *gw) { gw_ = gw; }
  void set_poller(NibeGwCoilPoller *poller) { poller_ = poller; }
  void set_address(uint16_t address) { address_ = address; }
  void set_coil_size(uint8_t size) { coil_size_ = size; }
  void set_factor(uint16_t factor) { factor_ = factor; }

  void setup() override;
  void dump_config() override;

 protected:
  void control(float value) override;

  NibeGwComponent *gw_{nullptr};
  NibeGwCoilPoller *poller_{nullptr};
  uint16_t address_{0};
  uint8_t coil_size_{0};
  uint16_t factor_{1};
};

}  // namespace nibegw
}  // namespace esphome
