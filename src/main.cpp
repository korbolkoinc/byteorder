// src/main.cpp  —  byteorder library demo / smoke-test
#include "byteorder/byteorder.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <sstream>

using namespace endian;
using namespace endian::literals;

// ────────────────────────────────────────────────────────────────────────────
// Helper: print a value in hex
// ────────────────────────────────────────────────────────────────────────────
template <std::integral T>
static void print_hex(const char* label, T v)
{
    std::cout << label << ": 0x";
    for (int shift = (sizeof(T) - 1) * 8; shift >= 0; shift -= 8)
        std::cout << std::hex << ((v >> shift) & 0xFF) << " ";
    std::cout << std::dec << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
// 1. Scalar byte-swap
// ────────────────────────────────────────────────────────────────────────────
static void demo_scalar()
{
    std::cout << "\n=== Scalar byte-swap ===\n";

    constexpr uint32_t host = 0xDEADBEEF;
    basic_endian<uint32_t> e(host);

    print_hex("host  (native)", e.native());
    print_hex("big   (network)", e.big());
    print_hex("little         ", e.little());
}

// ────────────────────────────────────────────────────────────────────────────
// 2. Network-order helpers
// ────────────────────────────────────────────────────────────────────────────
static void demo_network()
{
    std::cout << "\n=== Network byte order ===\n";
    using be = basic_endian<uint16_t>::network_order;

    constexpr uint16_t port_host    = 8080;
    const     uint16_t port_network = be::host_to_network(port_host);
    const     uint16_t port_back    = be::network_to_host(port_network);

    std::cout << "Host port   : " << port_host    << '\n';
    print_hex("Network port", port_network);
    std::cout << "Decoded back: " << port_back     << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
// 3. Buffer (zero-copy) view
// ────────────────────────────────────────────────────────────────────────────
static void demo_buffer()
{
    std::cout << "\n=== Buffer view ===\n";

    std::array<std::byte, 8> buf{};
    basic_endian<uint64_t>::buffer_view view(buf);

    constexpr uint64_t value = 0x0102030405060708ull;
    view.store_big(value);

    std::cout << "Stored as big-endian, raw bytes: ";
    for (auto b : buf)
        std::cout << std::hex << static_cast<int>(b) << " ";
    std::cout << std::dec << '\n';

    std::cout << "Loaded as big: 0x" << std::hex << view.load_big() << std::dec << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
// 4. User-defined literals
// ────────────────────────────────────────────────────────────────────────────
static void demo_literals()
{
    std::cout << "\n=== User-defined literals ===\n";

    auto le_val = 0xCAFEBABEull_le;
    auto be_val = 0xCAFEBABEull_be;

    std::cout << "0xCAFEBABE_le native: 0x" << std::hex << le_val.native() << '\n';
    std::cout << "0xCAFEBABE_be native: 0x" << std::hex << be_val.native() << std::dec << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
// 5. Stream serialization
// ────────────────────────────────────────────────────────────────────────────
static void demo_serialization()
{
    std::cout << "\n=== Stream serialization ===\n";

    basic_endian<uint32_t> orig(0xABCD1234);

    std::stringstream ss;
    orig.serialize(ss);

    basic_endian<uint32_t> loaded;
    loaded.deserialize(ss);

    std::cout << "Original : 0x" << std::hex << orig.native()   << '\n';
    std::cout << "Roundtrip: 0x" << std::hex << loaded.native() << std::dec << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
// 6. Version
// ────────────────────────────────────────────────────────────────────────────
static void demo_version()
{
    std::cout << "\n=== Library version ===\n";
    std::cout << "endian::version " << version::str << "  ("
              << version::major << '.' << version::minor << '.' << version::patch
              << ")\n";
    std::cout << "Host byte order: "
              << (std::endian::native == std::endian::little ? "little-endian" : "big-endian")
              << '\n';
}

// ────────────────────────────────────────────────────────────────────────────
int main()
{
    demo_version();
    demo_scalar();
    demo_network();
    demo_buffer();
    demo_literals();
    demo_serialization();

    std::cout << "\nAll demos completed successfully.\n";
    return 0;
}
