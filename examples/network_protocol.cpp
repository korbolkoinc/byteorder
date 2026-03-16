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
#include <vector>
#include <string>

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
static constexpr size_t   HEADER_SIZE   = 8;

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

static void demo_basic_framing()
{
    std::cout << "=== Basic Frame Encoding/Decoding ===\n\n";

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

    std::cout << "\nRoundtrip " << (ok ? "PASSED" : "FAILED") << "\n\n";
}

static void demo_buffer_view_encoding()
{
    std::cout << "=== Buffer View Encoding ===\n\n";

    std::array<std::byte, 8> buf{};
    basic_endian<uint16_t>::buffer_view magic_view(std::span(buf).first(2));
    basic_endian<uint16_t>::buffer_view version_view(std::span(buf).subspan(2, 2));
    basic_endian<uint32_t>::buffer_view length_view(std::span(buf).last(4));

    magic_view.store_big(PROTO_MAGIC);
    version_view.store_big(PROTO_VERSION);
    length_view.store_big(1024);

    std::cout << "Encoded header: ";
    for (auto b : buf)
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << ' ';
    std::cout << std::dec << "\n\n";

    const uint16_t magic_read = magic_view.load_big();
    const uint16_t version_read = version_view.load_big();
    const uint32_t length_read = length_view.load_big();

    std::cout << "Decoded:\n";
    std::cout << "  magic:   0x" << std::hex << magic_read << '\n';
    std::cout << "  version: 0x" << version_read << '\n';
    std::cout << "  length:  " << length_read << std::dec << "\n\n";
}

static void demo_batch_header_encoding()
{
    std::cout << "=== Batch Header Encoding ===\n\n";

    constexpr size_t NUM_FRAMES = 16;
    std::vector<FrameHeader> headers(NUM_FRAMES);

    for (size_t i = 0; i < NUM_FRAMES; ++i)
    {
        headers[i] = {
            PROTO_MAGIC,
            PROTO_VERSION,
            static_cast<uint32_t>((i + 1) * 100)
        };
    }

    std::vector<std::byte> wire_buffer(NUM_FRAMES * HEADER_SIZE);

    for (size_t i = 0; i < NUM_FRAMES; ++i)
    {
        auto frame_span = std::span(wire_buffer).subspan(i * HEADER_SIZE, HEADER_SIZE);
        basic_endian<uint16_t>::buffer_view magic_view(frame_span.first(2));
        basic_endian<uint16_t>::buffer_view version_view(frame_span.subspan(2, 2));
        basic_endian<uint32_t>::buffer_view length_view(frame_span.last(4));

        magic_view.store_big(headers[i].magic);
        version_view.store_big(headers[i].version);
        length_view.store_big(headers[i].length);
    }

    std::cout << "Encoded " << NUM_FRAMES << " frames (" 
              << (NUM_FRAMES * HEADER_SIZE) << " bytes)\n\n";

    std::cout << "First 3 frames:\n";
    for (size_t i = 0; i < 3 && i < NUM_FRAMES; ++i)
    {
        auto frame_span = std::span(wire_buffer).subspan(i * HEADER_SIZE, HEADER_SIZE);
        FrameHeader decoded = decode_header(std::span<const std::byte, HEADER_SIZE>(
            reinterpret_cast<const std::byte*>(frame_span.data()), HEADER_SIZE));

        std::cout << "  Frame " << i << ": magic=0x" << std::hex << decoded.magic
                  << ", version=0x" << decoded.version
                  << ", length=" << decoded.length << std::dec << '\n';
    }
    std::cout << '\n';
}

static void demo_endian_span_protocol()
{
    std::cout << "=== endian_span for Protocol Arrays ===\n\n";

    std::vector<uint32_t> ip_addresses = {
        0xC0A80101, // 192.168.1.1
        0xC0A80102, // 192.168.1.2
        0x0A000001, // 10.0.0.1
        0xAC100001  // 172.16.0.1
    };

    endian_span<uint32_t, byte_order::network> span(ip_addresses);

    std::cout << "Host order IP addresses:\n";
    for (size_t i = 0; i < span.size(); ++i)
    {
        std::cout << "  [" << i << "] 0x" << std::hex << ip_addresses[i] << '\n';
    }

    std::cout << "\nNetwork order (big-endian) view:\n";
    for (size_t i = 0; i < span.size(); ++i)
    {
        std::cout << "  [" << i << "] 0x" << span[i].big() << '\n';
    }
    std::cout << std::dec << '\n';
}

int main()
{
    demo_basic_framing();
    demo_buffer_view_encoding();
    demo_batch_header_encoding();
    demo_endian_span_protocol();

    std::cout << "\nAll protocol demos completed successfully.\n";
    return 0;
}
