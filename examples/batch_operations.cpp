// examples/batch_operations.cpp
//
// Demonstrates SIMD-accelerated batch byte-swapping and stream
// serialization / deserialization using the byteorder library.
//
// Build & run:
//   cmake --preset debug && cmake --build --preset debug
//   ./build/debug/bin/batch_operations

#include "byteorder/byteorder.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>

using namespace endian;

// ─── batch_byte_swap via convert_batch ──────────────────────────────────────
static void demo_batch_swap()
{
    std::cout << "=== Batch swap (32-bit) ===\n";

    const std::vector<uint32_t> src = {
        0x01020304, 0xDEADBEEF, 0xCAFEBABE, 0x11223344
    };
    std::vector<uint32_t> dst(src.size());

    basic_endian<uint32_t>::convert_batch(src, dst, /*check_alignment=*/false);

    for (size_t i = 0; i < src.size(); ++i)
    {
        std::cout << std::hex
                  << "  0x" << src[i]
                  << " -> 0x" << dst[i] << '\n';
        assert(dst[i] == detail::byte_swap(src[i]));
    }
    std::cout << std::dec << "All assertions passed.\n\n";
}

// ─── buffer_view zero-copy I/O ───────────────────────────────────────────────
static void demo_buffer_view()
{
    std::cout << "=== Zero-copy buffer view ===\n";

    std::vector<std::byte> buf(8, std::byte{0});
    basic_endian<uint64_t>::buffer_view view(buf);

    constexpr uint64_t value = 0x0102030405060708ull;
    view.store_big(value);

    std::cout << "Stored 0x" << std::hex << value
              << " as network (big) order, raw bytes: ";
    for (auto b : buf)
        std::cout << static_cast<int>(b) << ' ';

    const uint64_t back = view.load_big();
    assert(back == value);
    std::cout << "\nLoaded back: 0x" << back << std::dec
              << "  [OK]\n\n";
}

// ─── multi-value stream serialization ────────────────────────────────────────
static void demo_stream_serialization()
{
    std::cout << "=== Stream serialization ===\n";

    std::stringstream ss;

    // Write three different-width values in big-endian order
    basic_endian<uint16_t> v16(0xABCD);
    basic_endian<uint32_t> v32(0xDEADBEEF);
    basic_endian<uint64_t> v64(0xCAFEBABE01020304ull);

    v16.serialize(ss);
    v32.serialize(ss);
    v64.serialize(ss);

    // Read them back
    basic_endian<uint16_t> r16;
    basic_endian<uint32_t> r32;
    basic_endian<uint64_t> r64;

    r16.deserialize(ss);
    r32.deserialize(ss);
    r64.deserialize(ss);

    assert(r16.native() == v16.native());
    assert(r32.native() == v32.native());
    assert(r64.native() == v64.native());

    std::cout << std::hex
              << "  uint16: 0x" << r16.native() << "  [OK]\n"
              << "  uint32: 0x" << r32.native() << "  [OK]\n"
              << "  uint64: 0x" << r64.native() << "  [OK]\n"
              << std::dec;
}

int main()
{
    demo_batch_swap();
    demo_buffer_view();
    demo_stream_serialization();
    std::cout << "\nAll demos completed successfully.\n";
    return 0;
}
