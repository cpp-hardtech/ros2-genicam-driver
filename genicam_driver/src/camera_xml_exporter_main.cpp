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

#include <exception>

// ROS
#include <rclcpp/rclcpp.hpp>

// genicam_driver
#include "genicam_driver/camera_xml_exporter.h"

int main(int argc, char** argv)
{
    bool is_successful = false;
    try
    {
        rclcpp::init(argc, argv);
        auto p_node = std::make_shared<genicam_driver::CameraXmlExporter>();
        if (p_node->isInitialized())
        {
            const bool use_stdout = p_node->get_parameter("output_to_stdout").as_bool();
            is_successful = use_stdout ? p_node->exportXmlDataToStdout()
                                       : p_node->exportXmlDataToFile();
        }
    }
    catch (const std::exception& error)
    {
        RCLCPP_ERROR(rclcpp::get_logger("camera_xml_exporter"), "Failed to export GenICam XML: %s",
                     error.what());
    }

    if (rclcpp::ok())
        rclcpp::shutdown();

    return is_successful ? EXIT_SUCCESS : EXIT_FAILURE;
}
