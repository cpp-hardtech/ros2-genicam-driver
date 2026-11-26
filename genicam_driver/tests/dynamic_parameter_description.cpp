// Copyright (c) 2026 Xia Chao
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
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

#include <iostream>
#include <stdexcept>
#include <string>

#include "genicam_driver/camera_driver.h"

namespace
{
class TestCameraDriver : public genicam_driver::CameraDriver
{
  public:
    explicit TestCameraDriver(const std::string& path) :
      CameraDriver("dynamic_parameter_description", rclcpp::NodeOptions().append_parameter_override(
                                                      "dynamic_parameters_yaml_url", path))
    {
        setupParameters();
        p_camera_ = arv_camera_new("Fake_1", nullptr);
        if (!p_camera_)
            throw std::runtime_error("Could not open the Aravis fake camera");
        p_device_ = arv_camera_get_device(p_camera_);
        setupDynamicParameters();
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
        TestCameraDriver driver(argv[1]);
        const auto width              = driver.get_parameter("Width").as_int();
        const auto height             = driver.get_parameter("Height").as_int();
        const auto width_description  = driver.describe_parameter("Width").description;
        const auto height_description = driver.describe_parameter("Height").description;
        std::cout << "Width=" << width << " description='" << width_description
                  << "' Height=" << height << " description='" << height_description << "'\n";
        valid = width == 512 && height == 512 && width_description.empty() &&
                height_description == "Height of the captured image.";
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
    }
    rclcpp::shutdown();
    return valid ? 0 : 1;
}
