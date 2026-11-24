// Copyright 2026 Xia Chao
// SPDX-License-Identifier: BSD-3-Clause

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "genicam_driver/conversion_utils.h"

namespace
{
using Image = sensor_msgs::msg::Image;

bool check(const std::string& scenario, const std::string& encoding, uint32_t width,
           uint32_t height, uint32_t input_step, bool big_endian,
           std::initializer_list<uint8_t> bytes, uint32_t expected_step, bool expected_endian,
           std::initializer_list<uint8_t> expected)
{
    auto input = std::make_shared<Image>();
    input->width = width;
    input->height = height;
    input->step = input_step;
    input->is_bigendian = big_endian;
    input->data = bytes;
    Image::SharedPtr output;
    const auto& convert = genicam_driver::CONVERSIONS_DICTIONARY.at(encoding);
    const bool ok = convert(input, output);
    const bool valid = ok && output->step == expected_step && output->width == width &&
        output->height == height && output->is_bigendian == expected_endian &&
        output->data == std::vector<uint8_t>(expected);
    std::cout << scenario << (valid ? " PASS" : " FAIL");
    if (ok)
    {
        std::cout << " step=" << output->step << " big_endian=" << +output->is_bigendian
                  << " data=";
        for (uint8_t byte : output->data)
            std::cout << ' ' << std::hex << static_cast<unsigned>(byte);
        std::cout << std::dec;
    }
    std::cout << '\n';
    return valid;
}
}  // namespace

int main()
{
    bool valid = true;
    valid &= check("big-endian Mono12", "Mono12", 1, 1, 2, true,
                   {0x00, 0xff}, 2, true, {0x0f, 0xf0});
    valid &= check("RGB12 with row padding", "RGB12", 1, 1, 8, false,
                   {0x23, 0x01, 0x56, 0x04, 0x89, 0x07, 0xa1, 0xb2}, 8, false,
                   {0x30, 0x12, 0x60, 0x45, 0x90, 0x78, 0xa1, 0xb2});
    valid &= check("Mono10p row tails", "Mono10p", 5, 2, 7, false,
                   {1, 8, 48, 0, 1, 5, 0, 6, 28, 128, 64, 2, 10, 0}, 10, false,
                   {64, 0, 128, 0, 192, 0, 0, 1, 64, 1, 128, 1, 192, 1, 0, 2, 64, 2, 128, 2});
    valid &= check("Mono12p row tails", "Mono12p", 3, 2, 5, false,
                   {0x23, 0x61, 0x45, 0x89, 0x07, 0xbc, 0xfa, 0xde, 0x21, 0x03}, 6, false,
                   {0x30, 0x12, 0x60, 0x45, 0x90, 0x78, 0xc0, 0xab, 0xf0, 0xde, 0x10, 0x32});
    valid &= check("RGB12p row tails", "RGB12p", 1, 2, 5, false,
                   {0x23, 0x61, 0x45, 0x89, 0x07, 0xbc, 0xfa, 0xde, 0x21, 0x03}, 6, false,
                   {0x30, 0x12, 0x60, 0x45, 0x90, 0x78, 0xc0, 0xab, 0xf0, 0xde, 0x10, 0x32});
    valid &= check("Mono10Packed second sample", "Mono10Packed", 2, 1, 3, false,
                   {0x55, 0x31, 0xaa}, 4, false, {0x40, 0x55, 0xc0, 0xaa});
    return valid ? 0 : 1;
}
