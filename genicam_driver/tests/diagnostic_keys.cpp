// Copyright (c) 2026 Xia Chao
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "genicam_driver/camera_driver.h"

namespace
{
class TestCameraDriver : public genicam_driver::CameraDriver
{
  public:
    explicit TestCameraDriver(const std::string& path) :
      CameraDriver("diagnostic_keys",
                   rclcpp::NodeOptions()
                     .append_parameter_override("diagnostic_yaml_url", path)
                     .append_parameter_override("diagnostic_publishing_rate", 20.0))
    {
        setupParameters();
        p_camera_ = arv_camera_new("Fake_1", nullptr);
        if (!p_camera_)
            throw std::runtime_error("Could not open the Aravis fake camera");
        p_device_ = arv_camera_get_device(p_camera_);
        setupCameraDiagnosticPublisher();
    }

    ~TestCameraDriver() override
    {
        stopCameraDiagnosticPublisher();
    }

  private:
    bool setTechSpecificTlControlSettings() override
    {
        return true;
    }
    int discoverNumberOfStreams() override
    {
        return 1;
    }
    void tuneArvStream(ArvStream*) const override
    {
    }
    void postFrameProcessingCallback(uint) override
    {
    }
};

bool checkDiagnostics(const std::string& path)
{
    using Message = genicam_driver_msgs::msg::CameraDiagnostics;
    using Entry   = std::pair<std::string, std::string>;
    using namespace std::chrono_literals;

    auto driver = std::make_shared<TestCameraDriver>(path);
    Message::ConstSharedPtr message;
    auto subscription = driver->create_subscription<Message>(
      "~/diagnostics", 1, [&message](Message::ConstSharedPtr received) { message = received; });

    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(driver);
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!message && std::chrono::steady_clock::now() < deadline)
        executor.spin_once(100ms);
    if (!message)
        throw std::runtime_error("Timed out waiting for camera diagnostics");

    std::vector<Entry> actual;
    for (const auto& entry : message->data)
    {
        std::cout << entry.key << '=' << entry.value << '\n';
        actual.emplace_back(entry.key, entry.value);
    }
    const std::vector<Entry> expected = {
      {"image_width", "512"},
      {"Height", "512"},
      {"PixelFormat", "Mono8"},
      {"Width", "512"},
      {"trigger_mode-FrameStart", "Off"},
      {"trigger_mode-AcquisitionStart", "Off"},
    };
    return actual == expected;
}
}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    rclcpp::init(0, nullptr);
    arv_disable_interface("GigEVision");
    arv_disable_interface("USB3Vision");
    arv_enable_interface("Fake");

    bool valid = false;
    try
    {
        valid = checkDiagnostics(argv[1]);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
    }
    rclcpp::shutdown();
    return valid ? 0 : 1;
}
