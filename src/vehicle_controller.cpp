#include <algorithm>
#include <iomanip>
#include <iostream>
#include <type_traits>

#include <dds/dds.hpp>
#include <google/protobuf/message.h>

#include "vehicle_model.hpp"

static_assert(
    std::is_base_of<google::protobuf::Message, automotive::Telemetry>::value,
    "Telemetry must remain a Protocol Buffers message");
static_assert(
    std::is_base_of<google::protobuf::Message, automotive::ActuatorCommand>::value,
    "ActuatorCommand must remain a Protocol Buffers message");

int main()
{
    dds::domain::DomainParticipant participant(0);
    dds::topic::Topic<automotive::Telemetry> telemetry_topic(participant, "VehicleTelemetry");
    dds::topic::Topic<automotive::ActuatorCommand> command_topic(participant, "ActuatorCommand");
    dds::sub::DataReader<automotive::Telemetry> telemetry_reader(telemetry_topic);
    dds::pub::DataWriter<automotive::ActuatorCommand> command_writer(command_topic);

    std::size_t processed = 0;
    dds::sub::cond::ReadCondition telemetry_condition(
        telemetry_reader,
        dds::sub::status::DataState::any(),
        [&]() {
            for (const auto &sample : telemetry_reader.take()) {
                if (!sample.info().valid()) {
                    continue;
                }

                const automotive::Telemetry &telemetry = sample.data();
                if (processed == 0) {
                    std::cout << "[protobuf API] received Telemetry:\n"
                              << telemetry.DebugString();
                }
                automotive::ActuatorCommand command;
                command.set_sample_id(telemetry.sample_id());

                if (telemetry.driver_brake_pressed()) {
                    command.set_brake_percent(80.0);
                    command.set_reason("driver_braking");
                } else {
                    const double error =
                        telemetry.target_speed_mps() - telemetry.speed_mps();
                    if (error > 0.5) {
                        command.set_throttle_percent(
                            std::min(60.0, error * 5.0));
                        command.set_reason("accelerating");
                    } else if (error < -0.5) {
                        command.set_brake_percent(
                            std::min(40.0, -error * 4.0));
                        command.set_reason("slowing_down");
                    } else {
                        command.set_reason("at_target_speed");
                    }
                }

                if (processed == 0) {
                    std::cout << "[protobuf API] built ActuatorCommand:\n"
                              << command.DebugString();
                }
                command_writer.write(command);
                ++processed;
                std::cout << "[DDS API] wrote ActuatorCommand " << command.sample_id()
                          << ": throttle=" << std::fixed << std::setprecision(1)
                          << command.throttle_percent() << "% brake="
                          << command.brake_percent() << "% ("
                          << command.reason() << ")\n";
            }
        });

    dds::core::cond::WaitSet waitset;
    waitset += telemetry_condition;
    std::cout << "[controller] waiting for vehicle telemetry\n";
    while (processed < 12) {
        waitset.dispatch(dds::core::Duration(1));
    }

    return 0;
}