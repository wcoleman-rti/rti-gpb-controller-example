#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include <dds/dds.hpp>

#include "vehicle_model.hpp"

int main(int argc, char **argv)
{
    const std::string scenario = argc > 1 ? argv[1] : "cruise";
    if (scenario != "cruise" && scenario != "brake") {
        std::cerr << "Usage: vehicle_simulator [cruise|brake]\n";
        return 2;
    }

    dds::domain::DomainParticipant participant(0);
    dds::topic::Topic<automotive::Telemetry> telemetry_topic(participant, "VehicleTelemetry");
    dds::topic::Topic<automotive::ActuatorCommand> command_topic(participant, "ActuatorCommand");
    dds::pub::DataWriter<automotive::Telemetry> telemetry_writer(telemetry_topic);
    dds::sub::DataReader<automotive::ActuatorCommand> command_reader(command_topic);

    std::cout << "[simulator] scenario: " << scenario << '\n';
    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (std::uint64_t sequence = 0; sequence < 12; ++sequence) {
        automotive::Telemetry telemetry;
        telemetry.set_sample_id(sequence);
        const double speed = scenario == "brake" && sequence >= 5
            ? std::max(0.0, 12.5 - (sequence - 5) * 2.0)
            : sequence * 2.0;
        telemetry.set_speed_mps(speed);
        telemetry.set_target_speed_mps(20.0);
        telemetry.set_driver_brake_pressed(
            scenario == "brake" && sequence >= 5);

        if (sequence == 0) {
            std::cout << "[protobuf API] built Telemetry:\n"
                      << telemetry.DebugString();
        }
        telemetry_writer.write(telemetry);
        std::cout << "[DDS API] wrote Telemetry " << sequence << ": speed="
                  << speed << " m/s target=20.0 m/s"
                  << (telemetry.driver_brake_pressed() ? " brake pedal pressed" : "")
                  << '\n';

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        for (const auto &sample : command_reader.take()) {
            if (sample.info().valid()) {
                if (sample.data().sample_id() == 0) {
                    std::cout << "[protobuf API] received ActuatorCommand:\n"
                              << sample.data().DebugString();
                }
                std::cout << "[DDS API] received command "
                          << sample.data().sample_id() << ": throttle="
                          << sample.data().throttle_percent() << "% brake="
                          << sample.data().brake_percent() << "% ("
                          << sample.data().reason() << ")\n";
            }
        }
    }

    return 0;
}