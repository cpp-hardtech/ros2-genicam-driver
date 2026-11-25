// Copyright (c) 2024 Fraunhofer IOSB and contributors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the Fraunhofer IOSB nor the names of its
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

#include <arv.h>
#include <yaml-cpp/yaml.h>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include <rclcpp/rclcpp.hpp>

#include "genicam_driver/genicam_node_base.h"

namespace
{

bool selectTransport(const std::string& transport)
{
    bool available = false;
    const auto interface_count = arv_get_n_interfaces();
    for (unsigned int i = 0; i < interface_count; ++i)
    {
        const char* interface_id = arv_get_interface_id(i);
        if (transport == interface_id)
        {
            arv_enable_interface(interface_id);
            available = true;
        }
        else
        {
            arv_disable_interface(interface_id);
        }
    }
    return available;
}

const char* valueOrEmpty(const char* value)
{
    return value != nullptr ? value : "";
}

bool writeInventory()
{
    arv_update_device_list();
    const auto device_count = arv_get_n_devices();

    YAML::Emitter output;
    output.SetStringFormat(YAML::DoubleQuoted);
    output << YAML::BeginSeq;
    for (unsigned int i = 0; i < device_count; ++i)
    {
        output << YAML::BeginMap
               << YAML::Key << "id" << YAML::Value << valueOrEmpty(arv_get_device_id(i))
               << YAML::Key << "address" << YAML::Value << valueOrEmpty(arv_get_device_address(i))
               << YAML::Key << "vendor" << YAML::Value << valueOrEmpty(arv_get_device_vendor(i))
               << YAML::Key << "model" << YAML::Value << valueOrEmpty(arv_get_device_model(i))
               << YAML::Key << "serial_number" << YAML::Value
               << valueOrEmpty(arv_get_device_serial_nbr(i))
               << YAML::Key << "protocol" << YAML::Value << valueOrEmpty(arv_get_device_protocol(i))
               << YAML::EndMap;
    }
    output << YAML::EndSeq;
    if (!output.good())
        return false;

    std::cout << output.c_str() << '\n' << std::flush;
    return static_cast<bool>(std::cout);
}

int runFinder()
{
    genicam_driver::GenicamNodeBase node("camera_finder");
    auto logger = node.get_logger();
    const auto output_format = node.declare_parameter<std::string>("output_format", "text");
    const auto transport = node.declare_parameter<std::string>("transport", "all");
    if (output_format != "text" && output_format != "yaml")
    {
        RCLCPP_ERROR(logger, "Unknown output_format '%s'. Expected text or yaml.",
                     output_format.c_str());
        return EXIT_FAILURE;
    }
    if (transport != "all" && transport != "GigEVision" && transport != "USB3Vision")
    {
        RCLCPP_ERROR(logger, "Unknown transport '%s'. Expected all, GigEVision, or USB3Vision.",
                     transport.c_str());
        return EXIT_FAILURE;
    }
    if (transport != "all" && !selectTransport(transport))
    {
        RCLCPP_ERROR(logger, "Aravis does not support the requested transport '%s'.",
                     transport.c_str());
        return EXIT_FAILURE;
    }
    if (output_format == "text")
    {
        RCLCPP_DEBUG(logger, "Discovering cameras with transport '%s'.", transport.c_str());
        node.listAvailableCameraDevices();
        return EXIT_SUCCESS;
    }

    // Keep the node's shutdown log out of the YAML document.
    logger.set_level(rclcpp::Logger::Level::Warn);
    if (!writeInventory())
    {
        std::cerr << "Failed to write the YAML camera inventory.\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

}  // namespace

int main(int argc, char** argv)
{
    int status = EXIT_FAILURE;
    try
    {
        rclcpp::init(argc, argv);
        status = runFinder();
    }
    catch (const std::exception& error)
    {
        RCLCPP_ERROR(rclcpp::get_logger("camera_finder"), "Camera discovery failed: %s",
                     error.what());
    }

    if (rclcpp::ok())
        rclcpp::shutdown();
    return status;
}
