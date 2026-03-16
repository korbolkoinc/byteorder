#include "byteorder/byteorder.h"

#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>
#include <span>
#include <numeric>

using namespace endian;
using namespace endian::literals;

template <std::integral T>
static void print_hex(const char* label, T v)
{
    std::cout << label << ": 0x";
    for (int shift = (sizeof(T) - 1) * 8; shift >= 0; shift -= 8)
        std::cout << std::hex << ((v >> shift) & 0xFF) << " ";
    std::cout << std::dec << '\n';
}

static void demo_scalar()
{
    std::cout << "\n=== Scalar byte-swap ===\n";

    constexpr uint32_t host = 0xDEADBEEF;
    basic_endian<uint32_t> e(host);

    print_hex("host  (native)", e.native());
    print_hex("big   (network)", e.big());
    print_hex("little         ", e.little());

    const uint64_t v64 = 0x0102030405060708ull;
    std::cout << "\nCompile-time byte swap: 0x"
              << std::hex << byte_swap_consteval(v64) << std::dec << '\n';
}

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

    using le = basic_endian<uint32_t>::little_endian_order;
    constexpr uint32_t val = 0x12345678;
    std::cout << "\nLittle-endian convert: 0x"
              << std::hex << le::convert(val) << std::dec << '\n';
}

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

    basic_endian<uint64_t>::const_buffer_view cview(buf);
    std::cout << "Const view loaded: 0x" << std::hex << cview.load_big() << std::dec << '\n';

    view.store_order(byte_order::little, 0xDEADBEEFCAFEBABEull);
    std::cout << "Stored as little, loaded: 0x"
              << std::hex << view.load_order(byte_order::little) << std::dec << '\n';
}

static void demo_literals()
{
    std::cout << "\n=== User-defined literals ===\n";

    auto le_val = 0xCAFEBABE_le;
    auto be_val = 0xCAFEBABE_be;
    auto le32_val = 0xDEAD_le32;
    auto be16_val = 0xBEEF_be16;

    std::cout << "0xCAFEBABE_le native: 0x" << std::hex << le_val.native() << '\n';
    std::cout << "0xCAFEBABE_be native: 0x" << std::hex << be_val.native() << '\n';
    std::cout << "0xDEAD_le32 native: 0x" << le32_val.native() << '\n';
    std::cout << "0xBEEF_be16 native: 0x" << be16_val.native() << std::dec << '\n';
}

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

    std::vector<uint16_t> batch = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    std::stringstream batch_ss;
    basic_endian<uint16_t>::serialize_batch(batch_ss, batch.data(), batch.size());

    std::vector<uint16_t> batch_loaded(batch.size());
    basic_endian<uint16_t>::deserialize_batch(batch_ss, batch_loaded.data(), batch_loaded.size());

    std::cout << "Batch roundtrip: ";
    for (size_t i = 0; i < batch_loaded.size(); ++i)
        std::cout << "0x" << std::hex << batch_loaded[i] << " ";
    std::cout << std::dec << '\n';
}

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

static void demo_batch_operations()
{
    std::cout << "\n=== Batch operations ===\n";

    constexpr size_t N = 1024;
    std::vector<uint32_t> src(N), dst(N);

    std::mt19937_64 rng(42);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint32_t>(rng() & 0xFFFFFFFF);

    auto start = std::chrono::high_resolution_clock::now();
    batch_byte_swap(src.data(), dst.data(), N, false);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Batch swapped " << N << " elements in " << duration << " ns\n";
    std::cout << "Throughput: " << (N * sizeof(uint32_t) * 1000.0 / duration) << " MB/s\n";

    std::cout << "First 8 elements:\n";
    for (size_t i = 0; i < 8 && i < N; ++i)
    {
        std::cout << "  0x" << std::hex << src[i] << " -> 0x" << dst[i] << '\n';
    }
    std::cout << std::dec;

    std::vector<uint64_t> inplace = {0x0102030405060708ull, 0xDEADBEEFCAFEBABEull};
    std::cout << "\nIn-place swap:\n  Before: 0x" << std::hex << inplace[0] << ", 0x" << inplace[1] << '\n';
    batch_byte_swap_inplace(inplace.data(), inplace.size());
    std::cout << "  After:  0x" << inplace[0] << ", 0x" << inplace[1] << std::dec << '\n';
}

static void demo_endian_span()
{
    std::cout << "\n=== endian_span ===\n";

    std::vector<uint32_t> data = {0x12345678, 0xDEADBEEF, 0xCAFEBABE};
    endian_span<uint32_t, byte_order::native> span(data);

    std::cout << "Span size: " << span.size() << '\n';
    std::cout << "Elements via proxy:\n";
    for (size_t i = 0; i < span.size(); ++i)
    {
        std::cout << "  [" << i << "] native: 0x" << std::hex << span[i].native() << '\n';
    }
    std::cout << std::dec;

    const_endian_span<uint32_t> cspan(data);
    std::cout << "Const span first element: 0x" << std::hex << cspan[0].native() << std::dec << '\n';
}

static void demo_cache_optimized()
{
    std::cout << "\n=== Cache-optimized storage ===\n";

    detail::cache_line_padded<uint32_t> padded_value(0xDEADBEEF);
    std::cout << "Cache-line padded value: 0x" << std::hex << *padded_value << std::dec << '\n';
    std::cout << "Storage alignment: " << alignof(decltype(padded_value)) << " bytes\n";

    detail::atomic_padded<uint64_t> atomic_val(0xCAFEBABE01020304ull);
    std::cout << "Atomic padded value: 0x" << std::hex << atomic_val.load() << std::dec << '\n';
}

static void demo_prefetch()
{
    std::cout << "\n=== Prefetch batch operations ===\n";

    constexpr size_t N = 4096;
    std::vector<uint64_t> src(N), dst(N);

    std::mt19937_64 rng(123);
    for (size_t i = 0; i < N; ++i)
        src[i] = rng();

    auto start = std::chrono::high_resolution_clock::now();
    batch_byte_swap_prefetch(src.data(), dst.data(), N, 16);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Prefetch batch swapped " << N << " elements in " << duration << " ns\n";
    std::cout << "Throughput: " << (N * sizeof(uint64_t) * 1000.0 / duration) << " MB/s\n";
}

static void demo_alignment_utils()
{
    std::cout << "\n=== Alignment utilities ===\n";

    int* ptr = reinterpret_cast<int*>(0x12345678);
    std::cout << "Original pointer: " << ptr << '\n';
    std::cout << "Aligned up to 64: " << detail::align_up(ptr, 64) << '\n';
    std::cout << "Is 8-aligned: " << detail::is_aligned(ptr, 8) << '\n';
    std::cout << "Is 4-aligned: " << detail::is_aligned(ptr, 4) << '\n';

    std::cout << "\nPower of two checks:\n";
    std::cout << "  64 is power of 2: " << detail::is_power_of_two(64) << '\n';
    std::cout << "  63 is power of 2: " << detail::is_power_of_two(63) << '\n';
}

static void demo_float_support()
{
    std::cout << "\n=== Floating-point support ===\n";

    basic_endian<float> fval(3.14159f);
    std::cout << "Float native: " << fval.native() << '\n';
    std::cout << "Float big: " << fval.big() << '\n';

    basic_endian<double> dval(2.718281828459045);
    std::cout << "Double native: " << dval.native() << '\n';
    std::cout << "Double big: " << dval.big() << '\n';

    le_float32 le_f(1.414f);
    std::cout << "Little-endian float: " << le_f.native() << '\n';

    be_float64 be_d(1.732);
    std::cout << "Big-endian double: " << be_d.native() << '\n';
}

static void demo_byte_order_enum()
{
    std::cout << "\n=== byte_order enum support ===\n";

    basic_endian<uint32_t> val(0x12345678);
    
    std::cout << "Native order value: 0x" << std::hex << val.order(byte_order::native) << '\n';
    std::cout << "Little order value: 0x" << val.order(byte_order::little) << '\n';
    std::cout << "Big order value:    0x" << val.order(byte_order::big) << '\n';
    std::cout << "Network order value: 0x" << val.order(byte_order::network) << std::dec << '\n';

    basic_endian<uint16_t> port(8080);
    port.store_order(byte_order::network, 443);
    std::cout << "\nStored 443 as network order, loaded: " << port.native() << '\n';
}

int main()
{
    demo_version();
    demo_scalar();
    demo_network();
    demo_buffer();
    demo_literals();
    demo_serialization();
    demo_batch_operations();
    demo_endian_span();
    demo_cache_optimized();
    demo_prefetch();
    demo_alignment_utils();
    demo_float_support();
    demo_byte_order_enum();

    std::cout << "\n=== All demos completed successfully ===\n";
    return 0;
}
