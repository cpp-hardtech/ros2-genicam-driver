<p align="center">
  <img src=".github/branding/logo.png" alt="ROS 2 GenICam Driver logo" width="160">
</p>

<h1 align="center">ROS 2 GenICam Driver</h1>

<p align="center"><strong>A ROS 2 driver for industrial GigE Vision and USB3 Vision cameras.</strong></p>

ROS 2 GenICam Driver connects GenICam-compatible industrial cameras to ROS 2 through the
Aravis library. It discovers devices on the network or bus, exposes camera features as ROS 2
parameters, and publishes images and camera information for downstream perception,
inspection, and calibration nodes.

Supported transports are GigE Vision and USB3 Vision. A single node can drive several
streams, selected camera features can be re-exposed as dynamically changeable ROS 2
parameters, and a configurable set of device features can be published as diagnostics.

## Executables

| Executable | Purpose |
| --- | --- |
| `camera_finder` | Enumerate reachable devices and print their identifiers. |
| `camera_driver_gv` | Drive one or more GigE Vision cameras. |
| `camera_driver_uv` | Drive one or more USB3 Vision cameras. |
| `camera_xml_exporter` | Write the GenICam feature description of a device to an XML file. |

## Requirements

- ROS 2 on Ubuntu. The CI matrix covers Humble, Iron, Jazzy, Kilted, and Lyrical.
- C++17 compiler and CMake 3.16 or newer.
- Aravis 0.8 or newer, including its development headers.
- The GenICam runtime that ships with your camera vendor, if the camera requires one.

## Build

The driver and its message package live in this repository and are built together.

```bash
source /opt/ros/jazzy/setup.bash
sudo apt install libaravis-dev

mkdir -p camera_ws/src
cd camera_ws
git clone https://github.com/cpp-hardtech/ros2-genicam-driver.git src/ros2-genicam-driver

rosdep update
rosdep install --from-paths src --ignore-src -y
colcon build --symlink-install --packages-up-to genicam_driver
source install/setup.bash
```

On a fresh ROS 2 installation, run `sudo rosdep init` once before `rosdep update`.

## Quick start

### 1. Find the device

Attach the camera and enumerate the reachable devices:

```bash
ros2 run genicam_driver camera_finder
```

The finder prints a device identifier for each camera. Use that value as `guid` below. For
GigE Vision cameras the device IP address is also accepted.

Set `transport` to `GigEVision` or `USB3Vision` to probe a single transport. The default
`all` discovers cameras on all enabled transports. For scripts, set `output_format` to
`yaml` to write a YAML sequence to standard output, with the device ID, address, vendor,
model, serial number, and protocol for each camera. An empty discovery writes `[]`.

```bash
ros2 run genicam_driver camera_finder --ros-args -p transport:=GigEVision -p output_format:=yaml
```

### 2. Start the driver

```bash
ros2 run genicam_driver camera_driver_gv --ros-args -p "guid:=<device-identifier>"
```

For a USB3 Vision camera use `camera_driver_uv` instead.

### 3. Consume the stream

In a second terminal, with the workspace sourced:

```bash
ros2 topic list
ros2 topic hz /camera_driver_gv/image_raw
```

The launch files under `genicam_driver/launch/` provide working starting points for both
transports:

- `camera_driver_gv_example.launch.py`
- `camera_driver_uv_example.launch.py`
- `camera_xml_exporter_example.launch.py`

## Topics and services

### Published topics

| Topic | Type | Notes |
| --- | --- | --- |
| `<node>/image_raw` | `sensor_msgs/msg/Image` | Per stream. |
| `<node>/camera_info` | `sensor_msgs/msg/CameraInfo` | Per stream, next to the corresponding image topic. |
| `<node>/<stream>/image_raw` | `sensor_msgs/msg/Image` | Used instead when several streams are configured. |
| `<node>/<stream>/camera_info` | `sensor_msgs/msg/CameraInfo` | Same naming rule as above. |
| `~/diagnostics` | `genicam_driver_msgs/msg/CameraDiagnostics` | Only when diagnostic features are configured. |

### Services

| Service | Type | Notes |
| --- | --- | --- |
| `~/calculate_white_balance_once` | `genicam_driver_msgs/srv/CalculateWhiteBalance` | Runs a one-shot white balance calculation on the device. |
| `~/trigger` | `std_srvs/srv/Trigger` | Requests a software trigger when `software_trigger` is enabled and acquisition is active. |

## Parameters

| Parameter | Type | Default | Purpose |
| --- | --- | --- | --- |
| `guid` | string | — | Device identifier reported by `camera_finder`, or a GigE Vision IP address. |
| `stream_names` | string[] | `[]` | Stream names used in the topic namespace when several streams are driven. |
| `camera_info_urls` | string[] | `[]` | Calibration paths or URLs, one per stream. Relative paths use the node's working directory. An omitted or blank entry tries the default file and allows uncalibrated streaming if it is unavailable. |
| `frame_id` | string | `""` | Coordinate frame attached to published images and camera info. |
| `dynamic_parameters_yaml_url` | string | `""` | YAML file listing device features to expose as dynamic ROS 2 parameters. |
| `diagnostic_yaml_url` | string | `""` | YAML file listing device features to publish as diagnostics. |
| `diagnostic_publishing_rate` | double | `0.1` | Diagnostics publish rate in Hz. |
| `verbose` | bool | `false` | Enables additional logging from the driver. |
| `timestamp_offset` | double | `0.0` | Constant offset applied to image timestamps. |
| `acquisition_always_on` | bool | `false` | Starts acquisition even without subscribers. ROS releases with publisher match events otherwise start and stop acquisition with subscriber demand. |
| `software_trigger` | bool | `false` | Configures software-triggered acquisition on a camera that supports it. |
| `xml_file` | string | `""` | Output path, `camera_xml_exporter` only. |
| `output_to_stdout` | bool | `false` | Write XML to standard output, `camera_xml_exporter` only. Leave `xml_file` empty in this mode. |

## Configuration files

- `genicam_driver/config/dynamic_parameters_example.yaml` — which device features become
  runtime-changeable ROS 2 parameters. Each entry may include a `Description` for the
  ROS 2 parameter descriptor.
- `genicam_driver/config/camera_diagnostics_example.yaml` — which device features are
  published on `~/diagnostics` and under what names.

Feature names and accepted values are defined by the device and can be listed with
`camera_xml_exporter`, which writes the GenICam XML description of the camera:

```bash
ros2 run genicam_driver camera_xml_exporter --ros-args \
  -p "guid:=<device-identifier>" -p "xml_file:=$PWD/camera.xml"
```

For use in a pipeline, set `output_to_stdout` to `true` and leave `xml_file` empty.

```bash
ros2 run genicam_driver camera_xml_exporter --ros-args \
  -p "guid:=<device-identifier>" -p output_to_stdout:=true > camera.xml
```

## Multi-stream nodes

Set `stream_names` to one name per stream. The names form the topic namespace, so a node with
`stream_names: ["left", "right"]` publishes `<node>/left/image_raw` and
`<node>/right/image_raw`. Leave the parameter empty to keep the topics directly under the
node name.

`CameraInfo` publishes the configured binning and the captured region in full-resolution
sensor coordinates. Calibrated dimensions continue to describe the calibration image.

## Software triggering

Start the driver with `-p software_trigger:=true`, subscribe to the image stream, then request a
capture. Set `-p acquisition_always_on:=true` if the caller needs to trigger the camera without
a connected image or camera info subscriber.

```bash
ros2 service call /camera_driver_gv/trigger std_srvs/srv/Trigger '{}'
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Distributed under the BSD 3-Clause License. The original copyright notice and the full
license text are retained in [LICENSE](LICENSE).
