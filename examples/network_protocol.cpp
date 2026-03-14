// examples/network_protocol.cpp
//
// Demonstrates using byteorder for network-protocol framing.
// Builds a minimal binary frame (magic + version + payload length)
// in big-endian (network) byte order, then parses it back.
//
// Build & run:
//   cmake --preset debug && cmake --build --preset debug
//   ./build/debug/bin/network_protocol

#include "byteorder/byteorder.h"
#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <span>

using namespace endian;

// ─── Wire format ────────────────────────────────────────────────────────────
//   Offset │ Size │ Field
//   -------+------+--------------
//     0    │  2   │ magic   (0xCAFE)
//     2    │  2   │ version (0x0001)
//     4    │  4   │ length  (payload bytes)
// ─────────────────────────────────────────────────────────────────────────────
struct FrameHeader
{
    uint16_t magic;
    uint16_t version;
    uint32_t length;
};

static constexpr uint16_t PROTO_MAGIC   = 0xCAFE;
static constexpr uint16_t PROTO_VERSION = 0x0001;
static constexpr size_t   HEADER_SIZE   = 8; // 2+2+4

// Serialize a FrameHeader into 8 bytes, big-endian
static std::array<std::byte, HEADER_SIZE> encode_header(const FrameHeader& hdr)
{
    std::array<std::byte, HEADER_SIZE> buf{};

    using no16 = basic_endian<uint16_t>::network_order;
    using no32 = basic_endian<uint32_t>::network_order;

    const uint16_t magic_be   = no16::host_to_network(hdr.magic);
    const uint16_t version_be = no16::host_to_network(hdr.version);
    const uint32_t length_be  = no32::host_to_network(hdr.length);

    std::memcpy(buf.data() + 0, &magic_be,   2);
    std::memcpy(buf.data() + 2, &version_be, 2);
    std::memcpy(buf.data() + 4, &length_be,  4);

    return buf;
}

// Parse a FrameHeader from 8 big-endian bytes
static FrameHeader decode_header(std::span<const std::byte, HEADER_SIZE> buf)
{
    using no16 = basic_endian<uint16_t>::network_order;
    using no32 = basic_endian<uint32_t>::network_order;

    uint16_t magic_be, version_be;
    uint32_t length_be;

    std::memcpy(&magic_be,   buf.data() + 0, 2);
    std::memcpy(&version_be, buf.data() + 2, 2);
    std::memcpy(&length_be,  buf.data() + 4, 4);

    return {
        no16::network_to_host(magic_be),
        no16::network_to_host(version_be),
        no32::network_to_host(length_be)
    };
}

static void print_bytes(const char* label, std::span<const std::byte> data)
{
    std::cout << label << ": ";
    for (auto b : data)
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << ' ';
    std::cout << std::dec << '\n';
}

int main()
{
    std::cout << "=== Network Protocol Frame Demo ===\n\n";

    const std::string payload = "Hello, network!";
    FrameHeader hdr{PROTO_MAGIC, PROTO_VERSION, static_cast<uint32_t>(payload.size())};

    const auto wire = encode_header(hdr);
    print_bytes("Encoded header (hex)", wire);

    const FrameHeader decoded = decode_header(wire);

    std::cout << '\n';
    std::cout << "magic   : 0x" << std::hex << decoded.magic   << '\n';
    std::cout << "version : 0x" << std::hex << decoded.version << '\n';
    std::cout << "length  : "   << std::dec << decoded.length  << " bytes\n";

    const bool ok =
        decoded.magic   == PROTO_MAGIC   &&
        decoded.version == PROTO_VERSION &&
        decoded.length  == payload.size();

    std::cout << "\nRoundtrip " << (ok ? "PASSED" : "FAILED") << '\n';
    return ok ? 0 : 1;
}
