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
#include <array>
#include <chrono>
#include <random>
#include <iomanip>

using namespace endian;

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

static void demo_large_batch()
{
    std::cout << "=== Large batch operation (SIMD accelerated) ===\n";

    constexpr size_t N = 65536;
    std::vector<uint64_t> src(N), dst(N);

    std::mt19937_64 rng(42);
    for (size_t i = 0; i < N; ++i)
        src[i] = rng();

    auto start = std::chrono::high_resolution_clock::now();
    batch_byte_swap(src.data(), dst.data(), N, false);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double throughput = (N * sizeof(uint64_t) * 1000.0) / duration;

    std::cout << "  Processed " << N << " elements (" 
              << (N * sizeof(uint64_t) / 1024.0) << " KB)\n";
    std::cout << "  Time: " << duration << " µs\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2) 
              << throughput << " MB/s\n";

    assert(dst[0] == detail::byte_swap(src[0]));
    assert(dst[N - 1] == detail::byte_swap(src[N - 1]));
    std::cout << "  Verification: PASSED\n\n";
}

static void demo_inplace_swap()
{
    std::cout << "=== In-place batch swap ===\n";

    std::array<uint16_t, 8> data = {
        0x0102, 0x0304, 0x0506, 0x0708,
        0x090A, 0x0B0C, 0x0D0E, 0x0F10
    };

    std::cout << "  Before: ";
    for (auto v : data)
        std::cout << std::hex << v << " ";
    std::cout << "\n";

    batch_byte_swap_inplace(data.data(), data.size());

    std::cout << "  After:  ";
    for (auto v : data)
        std::cout << v << " ";
    std::cout << std::dec << "\n\n";
}

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

static void demo_endian_span()
{
    std::cout << "=== endian_span operations ===\n";

    std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF, 0xCAFEBABE, 0x01020304};
    endian_span<uint32_t, byte_order::native> span(data);

    std::cout << "  Access via proxy:\n";
    for (size_t i = 0; i < span.size(); ++i)
    {
        std::cout << "    [" << i << "] native: 0x" << std::hex 
                  << span[i].native() << '\n';
    }

    std::cout << "  Iteration:\n    ";
    for (const auto& elem : span)
    {
        std::cout << "0x" << elem.big() << " ";
    }
    std::cout << std::dec << "\n\n";
}

static void demo_prefetch_batch()
{
    std::cout << "=== Prefetch-optimized batch swap ===\n";

    constexpr size_t N = 32768;
    std::vector<uint64_t> src(N), dst(N);

    std::mt19937_64 rng(123);
    for (size_t i = 0; i < N; ++i)
        src[i] = rng();

    auto start = std::chrono::high_resolution_clock::now();
    batch_byte_swap_prefetch(src.data(), dst.data(), N, 16);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    double throughput = (N * sizeof(uint64_t) * 1000.0) / duration;

    std::cout << "  Processed " << N << " elements with prefetch\n";
    std::cout << "  Time: " << duration << " µs\n";
    std::cout << "  Throughput: " << std::fixed << std::setprecision(2)
              << throughput << " MB/s\n\n";
}

static void demo_stream_serialization()
{
    std::cout << "=== Stream serialization ===\n";

    std::stringstream ss;

    basic_endian<uint16_t> v16(0xABCD);
    basic_endian<uint32_t> v32(0xDEADBEEF);
    basic_endian<uint64_t> v64(0xCAFEBABE01020304ull);

    v16.serialize(ss);
    v32.serialize(ss);
    v64.serialize(ss);

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

static void demo_network_order()
{
    std::cout << "=== Network byte order helpers ===\n";

    using network = basic_endian<uint32_t>::network_order;
    using host_order = basic_endian<uint32_t>::big_endian_order;

    constexpr uint32_t ip_addr = 0xC0A80101; // 192.168.1.1

    const uint32_t network_order = network::host_to_network(ip_addr);
    const uint32_t back = network::network_to_host(network_order);

    std::cout << std::hex;
    std::cout << "  Host:       0x" << ip_addr << '\n';
    std::cout << "  Network:    0x" << network_order << '\n';
    std::cout << "  Back to host: 0x" << back << '\n';
    std::cout << std::dec;
    std::cout << "  Roundtrip:  " << (back == ip_addr ? "PASSED" : "FAILED") << "\n\n";
}

int main()
{
    demo_batch_swap();
    demo_large_batch();
    demo_inplace_swap();
    demo_buffer_view();
    demo_endian_span();
    demo_prefetch_batch();
    demo_stream_serialization();
    demo_network_order();

    std::cout << "\nAll demos completed successfully.\n";
    return 0;
}
