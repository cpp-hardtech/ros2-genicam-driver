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

#include "genicam_driver/camera_xml_exporter.h"

// Std
#include <chrono>
#include <fstream>
#include <iostream>
#include <system_error>
#include <thread>

// genicam_driver
#include "genicam_driver/error.h"

namespace genicam_driver
{

//==================================================================================================
CameraXmlExporter::CameraXmlExporter(const rclcpp::NodeOptions& options) :
  GenicamNodeBase("camera_xml_exporter", options),
  xml_file_path_("")
{
    //--- setup parameters
    setupParameters();

    if (get_parameter("output_to_stdout").as_bool())
    {
        if (!get_parameter("xml_file").as_string().empty())
        {
            RCLCPP_ERROR(logger_, "output_to_stdout cannot be used with xml_file.");
            return;
        }
        // Keep discovery and shutdown logs out of the XML document.
        logger_.set_level(rclcpp::Logger::Level::Error);
    }

    //--- open camera device
    CHECK_SUCCESS(discoverAndOpenCameraDevice(), logger_);

    std::string camera_guid_str = GenicamNodeBase::constructCameraGuidStr(p_camera_);
    RCLCPP_INFO(logger_, "Successfully Opened: %s", camera_guid_str.c_str());

    is_initialized_ = true;
}

//==================================================================================================
CameraXmlExporter::~CameraXmlExporter()
{
}

//==================================================================================================
std::string_view CameraXmlExporter::readXmlData() const
{
    if (!is_initialized_)
    {
        RCLCPP_ERROR(logger_, "'%s' is not initialized.", this->get_name());
        return {};
    }

    size_t xml_size = 0;
    const char* xml_data = arv_device_get_genicam_xml(p_device_, &xml_size);
    if (xml_data == nullptr || xml_size == 0)
    {
        RCLCPP_ERROR(logger_, "Camera did not provide GenICam XML.");
        return {};
    }
    return {xml_data, xml_size};
}

//==================================================================================================
[[nodiscard]] bool CameraXmlExporter::exportXmlDataToStdout()
{
    const auto xml_data = readXmlData();
    if (xml_data.empty())
        return false;

    std::cout.write(xml_data.data(), xml_data.size());
    std::cout.flush();
    if (!std::cout)
    {
        std::cerr << "Failed to write GenICam XML to standard output.\n";
        return false;
    }
    return true;
}

//==================================================================================================
[[nodiscard]] bool CameraXmlExporter::exportXmlDataToFile()
{
    const auto xml_data = readXmlData();
    if (xml_data.empty())
        return false;

    /// Path string of output xml file.
    std::string xml_file_str = get_parameter("xml_file").as_string();

    if (xml_file_str.empty())
    {
        std::string tmpFileName = (guid_.empty())
                                    ? constructCameraGuidStr(p_camera_)
                                    : guid_;

        // replace whitespaces in url (coming from GUID) with '_'
        std::replace(tmpFileName.begin(), tmpFileName.end(), ' ', '_');

        xml_file_path_ = std::filesystem::path(tmpFileName + ".xml");
    }
    else
    {
        xml_file_path_ = std::filesystem::path(xml_file_str);
    }

    std::error_code path_error;
    const auto absolute_path = std::filesystem::absolute(xml_file_path_, path_error);
    if (path_error)
    {
        RCLCPP_ERROR(logger_, "Failed to resolve XML output path '%s': %s",
                     xml_file_path_.c_str(), path_error.message().c_str());
        return false;
    }
    xml_file_path_ = absolute_path;

    const auto parent_path = xml_file_path_.parent_path();
    std::filesystem::create_directories(parent_path, path_error);
    if (path_error)
    {
        RCLCPP_ERROR(logger_, "Failed to create XML output directory '%s': %s",
                     parent_path.c_str(), path_error.message().c_str());
        return false;
    }

    const bool output_exists = std::filesystem::exists(xml_file_path_, path_error);
    if (path_error)
    {
        RCLCPP_ERROR(logger_, "Failed to inspect XML output file '%s': %s",
                     xml_file_path_.c_str(), path_error.message().c_str());
        return false;
    }

    std::ofstream fout(xml_file_path_, std::ios::binary);
    if (!fout)
    {
        RCLCPP_ERROR(logger_, "Failed to open XML output file '%s'.", xml_file_path_.c_str());
        return false;
    }
    if (output_exists)
        RCLCPP_WARN(logger_, "Output file already exists and will be overwritten. Path: %s",
                    xml_file_path_.c_str());

    fout.write(xml_data.data(), xml_data.size());
    fout.close();
    if (!fout)
    {
        RCLCPP_ERROR(logger_, "Failed to write GenICam XML to file '%s'.", xml_file_path_.c_str());
        return false;
    }

    RCLCPP_INFO(logger_, "Written GenICam XML to file: %s", xml_file_path_.c_str());

    return true;
}

//==================================================================================================
void CameraXmlExporter::setupParameters()
{
    //--- call method of parent class
    GenicamNodeBase::setupParameters();

    auto xmlFile_desc = rcl_interfaces::msg::ParameterDescriptor{};
    xmlFile_desc.description =
      "Path to XML output file. If omitted, the XML data will be written into file with GUID as "
      "file name.";
    declare_parameter<std::string>("xml_file", "", xmlFile_desc);

    auto stdout_desc = rcl_interfaces::msg::ParameterDescriptor{};
    stdout_desc.description = "Write the XML to standard output. Leave xml_file empty.";
    stdout_desc.read_only = true;
    declare_parameter<bool>("output_to_stdout", false, stdout_desc);
}

}  // end namespace genicam_driver

#include "rclcpp_components/register_node_macro.hpp"
