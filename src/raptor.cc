#include "raptor.h"

#include <chrono>
#include <sstream>
#include <string>
#include <thread>

#include "base_drone.h"
#include "timer.h"
#include "util.h"

namespace drone {

raptor::raptor(std::string const& conn_url)
    : base_drone(conn_url), _climqtt(drone::SERVER_ADDRESS, drone::CLIENT_ID) {
  _gimbal.reset(new mavsdk::Gimbal{_mavsdk_system});

  mavsdk::Gimbal::Result gimbal_result =
      _gimbal->take_control(mavsdk::Gimbal::ControlMode::Primary);
  if (gimbal_result != mavsdk::Gimbal::Result::Success) {
    std::cerr << "Could not take gimbal control: " << gimbal_result << '\n';
  } else {
    _gimbal->set_pitch_and_yaw(-30.0f, 0.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }

  _climqtt.connect();
}

void raptor::move2() {
  state_ = STATE::SEARCH;
  _climqtt.subscribe(drone::mqtt::topics::TELEMETRY_RESPONSE_TOPIC);

  while (target_count_) {
    if (state_ == STATE::SEARCH) {
      state_search();
    } else if (state_ == STATE::LOCK) {
      state_lock();
    }
    if (state_ == STATE::INIT) {
      land();
    }
  }
}

void raptor::publish_telemetry() {
  _climqtt.publish(
      drone::mqtt::topics::TELEMETRY_TOPIC,
      std::move(drone::util::get_string(
          ' ', id_, position_.lat_deg_, position_.lon_deg_, position_.rel_alt_,
          attitude_.roll_deg_, attitude_.pitch_deg_, attitude_.yaw_deg_,
          speed_m_s_, battery_remaning_percent_, 1, 0, 0, 0, 0, 0, 0, 0, 0,
          0)));
}

mavsdk::Action::Result raptor::do_land() {
  _climqtt.publish(drone::mqtt::topics::LAND_TOPIC,
                   std::move(std::to_string(id_)));
  auto resp = base_drone::do_land();
  return resp;
}

void raptor::state_search() {
  using namespace std::chrono_literals;

  publish_telemetry();
  auto opt_msg = _climqtt.consume();

  if (opt_msg.has_value()) {
    auto msg = opt_msg.value();
    if (msg.first != mqtt::topics::TELEMETRY_RESPONSE_TOPIC) {
      state_ = STATE::LOCK;
      return;
    }
    auto out = drone::util::split(msg.second, ',');

    double dest_lat = std::stod(out[5]);
    double dest_lon = std::stod(out[6]);

    if (!request_process_image_ &&
        drone::util::haversine(position_.lat_deg_, position_.lon_deg_, dest_lat,
                               dest_lon) < 10) {
      request_process_image_ = true;
      _climqtt.publish(drone::mqtt::topics::PROCESS_IMAGE,
                       std::move(std::to_string(id_)));
      _climqtt.subscribe("im/coord");
    }

    auto dest_heading = drone::util::bearing(
        position_.lat_deg_, position_.lon_deg_, dest_lat, dest_lon);
    debug_print("Dest heading(bearing): ", dest_heading);
    debug_print("Heading: ", _heading);
    debug_print("dest_heading - _heading", dest_heading - _heading);
    debug_print("******************************************************");

    move({.forward = 3,
          .down = (position_.rel_alt_ - std::stof(out[7]) - 1),
          .yaw = static_cast<float>(dest_heading - _heading)});

    std::this_thread::sleep_for(30ms);
  } else {
    std::this_thread::sleep_for(500ms);
  }
}

constexpr static auto imcoor = "im/coord";

void raptor::state_lock() {
  _climqtt.subscribe("im/coord");
  debug_print("STATE: LOCK");
  if (request_process_image_) {
    request_process_image_ = false;
    _climqtt.publish(drone::mqtt::topics::PROCESS_IMAGE,
                     std::move(std::to_string(id_ + 1)));
    debug_print("Process image message sent!");
  }

  debug_print("Try to consume message");
  auto opt_msg = _climqtt.consume();
  if (opt_msg.has_value()) {
    auto msg = opt_msg.value();
    std::cout << "Message: " << msg.first << " " << msg.second << "\n";
    if (msg.first != imcoor) {
      debug_print("Setting state to SEARCH");
    }
    auto out = drone::util::split(msg.second, ',');
    if (out.size() != 2) {
      debug_print("Response size is lower than 2, invalid message");
      debug_print("Response size:", out.size());
      state_ = STATE::SEARCH;
      return;
    }
    debug_print("im/coord value[0]: ", out[0]);
    debug_print("im/coord value[1]: ", out[1]);
    move(
        {.forward = 1 + (-std::stof(out[1])), .yaw = 40 * (std::stof(out[0]))});
  } else {
    debug_print("No message received in 500msec, settin state to SEARCH");
    state_ = STATE::SEARCH;
  }
  //_climqtt.publish(mqtt::topics::LOCK, out[4]);
  // targetCount--;
}

}  // namespace drone