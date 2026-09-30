# GPB Controller Example

A two-process automotive controller demo using Protocol Buffers as the data model and the RTI Connext Professional 7.7.0 Protocol Buffers extension for typed DDS topics.

## Quick Start

Requires Connext Professional 7.7.0 for `x64Linux4gcc8.5.0`, CMake 3.18+, and a C++11 compiler. CMake fetches RTI's CMake utilities from the `main` branch at configure time; `rtiddsgen` invokes the bundled Protobuf 33.0 compiler and RTI plugins.

```sh
export NDDSHOME=/opt/rti.com/rti_connext_dds-7.7.0
cmake -B build && cmake --build build
```

Run the controller first in one terminal, then the simulator in another. Source the Connext environment in each terminal:

```sh
source /opt/rti.com/rti_connext_dds-7.7.0/resource/scripts/rtisetenv_x64Linux4gcc8.5.0.bash
./build/vehicle_controller
```

```sh
source /opt/rti.com/rti_connext_dds-7.7.0/resource/scripts/rtisetenv_x64Linux4gcc8.5.0.bash
./build/vehicle_simulator
```

The default `cruise` scenario publishes 12 telemetry samples and prints the controller's actuator commands.

## Applications

- `vehicle_controller` subscribes to `VehicleTelemetry`, applies a simple speed-control rule, and publishes `ActuatorCommand`.
- `vehicle_simulator` publishes telemetry and subscribes to actuator commands, simulating the vehicle side of the exchange.

## Behavior and Modes

The controller requests throttle when below the target speed, light braking when above it, and an 80% brake command when the simulated driver presses the brake pedal. This is illustrative logic, not production vehicle-control software.

Pass `brake` to exercise driver braking after the first five samples:

```sh
./build/vehicle_simulator brake
```

## Data Model

[`proto/vehicle_model.proto`](proto/vehicle_model.proto) defines the two topic types. CMake first runs `protoc` with RTI's `idl4` and `connext-cpp` plugins, then invokes `rtiddsgen` through `ConnextDdsCodegen` with `-standard PROTOBUF_CPP`. Generated Protobuf and Connext type support files live under `build/generated`.

## Protobuf and DDS APIs

The generated `automotive::Telemetry` and `automotive::ActuatorCommand` classes are Protocol Buffers messages and DDS topic types. The applications use protobuf setters/getters and `DebugString()`, then pass those same message objects to DDS `DataWriter::write()` and read them from `DataReader::take()`.