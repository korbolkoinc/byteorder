#include <benchmark/benchmark.h>
#include "byteorder/byteorder.h"

#include <cstdint>
#include <vector>
#include <array>
#include <random>
#include <algorithm>
#include <numeric>

using namespace endian;

// ============================================================================
// Compile-time byte_swap benchmarks
// ============================================================================

static void BM_ByteSwapConstEval16(benchmark::State& state)
{
    constexpr uint16_t v = 0xABCD;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap_consteval(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapConstEval16);

static void BM_ByteSwapConstEval32(benchmark::State& state)
{
    constexpr uint32_t v = 0xDEADBEEF;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap_consteval(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapConstEval32);

static void BM_ByteSwapConstEval64(benchmark::State& state)
{
    constexpr uint64_t v = 0x0102030405060708ull;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap_consteval(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapConstEval64);

// ============================================================================
// Runtime scalar byte_swap benchmarks
// ============================================================================

static void BM_ByteSwapRuntime16(benchmark::State& state)
{
    const uint16_t v = static_cast<uint16_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapRuntime16)->Arg(0xABCD);

static void BM_ByteSwapRuntime32(benchmark::State& state)
{
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapRuntime32)->Arg(0xDEADBEEF);

static void BM_ByteSwapRuntime64(benchmark::State& state)
{
    const uint64_t v = static_cast<uint64_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapRuntime64)->Arg(0x0102030405060708LL);

static void BM_ByteSwapRuntimeFloat(benchmark::State& state)
{
    const float v = 3.14159f;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapRuntimeFloat);

static void BM_ByteSwapRuntimeDouble(benchmark::State& state)
{
    const double v = 2.718281828459045;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwapRuntimeDouble);

// ============================================================================
// Free function byte_swap benchmarks
// ============================================================================

static void BM_FreeByteSwap32(benchmark::State& state)
{
    const uint32_t v = 0xDEADBEEF;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_FreeByteSwap32);

static void BM_FreeByteSwap64(benchmark::State& state)
{
    const uint64_t v = 0x0102030405060708ull;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_FreeByteSwap64);

// ============================================================================
// basic_endian accessors benchmarks
// ============================================================================

static void BM_EndianNative32(benchmark::State& state)
{
    const basic_endian<uint32_t> e(static_cast<uint32_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.native());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianNative32)->Arg(0xDEADBEEF);

static void BM_EndianNative64(benchmark::State& state)
{
    const basic_endian<uint64_t> e(static_cast<uint64_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.native());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianNative64)->Arg(0x0102030405060708LL);

static void BM_EndianBig32(benchmark::State& state)
{
    const basic_endian<uint32_t> e(static_cast<uint32_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.big());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianBig32)->Arg(0xDEADBEEF);

static void BM_EndianBig64(benchmark::State& state)
{
    const basic_endian<uint64_t> e(static_cast<uint64_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.big());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianBig64)->Arg(0x0102030405060708LL);

static void BM_EndianLittle32(benchmark::State& state)
{
    const basic_endian<uint32_t> e(static_cast<uint32_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.little());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianLittle32)->Arg(0xDEADBEEF);

static void BM_EndianOrder32(benchmark::State& state)
{
    const basic_endian<uint32_t> e(static_cast<uint32_t>(state.range(0)));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(e.order(byte_order::big));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianOrder32)->Arg(0xDEADBEEF);

static void BM_EndianRoundTrip32(benchmark::State& state)
{
    basic_endian<uint32_t> e;
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        e.store_big(v);
        benchmark::DoNotOptimize(e.native());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianRoundTrip32)->Arg(0xCAFEBABE);

static void BM_EndianRoundTrip64(benchmark::State& state)
{
    basic_endian<uint64_t> e;
    const uint64_t v = static_cast<uint64_t>(state.range(0));
    for (auto _ : state)
    {
        e.store_big(v);
        benchmark::DoNotOptimize(e.native());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_EndianRoundTrip64)->Arg(0xCAFEBABE01020304LL);

// ============================================================================
// Network-order helpers benchmarks
// ============================================================================

static void BM_HostToNetwork32(benchmark::State& state)
{
    using no = basic_endian<uint32_t>::network_order;
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(no::host_to_network(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_HostToNetwork32)->Arg(0xDEADBEEF);

static void BM_HostToNetwork64(benchmark::State& state)
{
    using no = basic_endian<uint64_t>::network_order;
    const uint64_t v = static_cast<uint64_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(no::host_to_network(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_HostToNetwork64)->Arg(0x0102030405060708LL);

static void BM_LittleEndianConvert32(benchmark::State& state)
{
    using le = basic_endian<uint32_t>::little_endian_order;
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(le::convert(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_LittleEndianConvert32)->Arg(0xDEADBEEF);

static void BM_BigEndianConvert32(benchmark::State& state)
{
    using be = basic_endian<uint32_t>::big_endian_order;
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(be::convert(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BigEndianConvert32)->Arg(0xDEADBEEF);

// ============================================================================
// Buffer view benchmarks
// ============================================================================

static void BM_BufferViewStoreBig32(benchmark::State& state)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        view.store_big(v);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BufferViewStoreBig32)->Arg(0xDEADBEEF);

static void BM_BufferViewLoadBig32(benchmark::State& state)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);
    view.store_big(0xDEADBEEF);
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(view.load_big());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BufferViewLoadBig32);

static void BM_BufferViewStoreLittle32(benchmark::State& state)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        view.store_little(v);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BufferViewStoreLittle32)->Arg(0xDEADBEEF);

static void BM_BufferViewLoadLittle32(benchmark::State& state)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);
    view.store_little(0xDEADBEEF);
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(view.load_little());
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_BufferViewLoadLittle32);

// ============================================================================
// Batch byte_swap benchmarks — SIMD accelerated
// ============================================================================

static void BM_BatchByteSwap16_Small(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint16_t> src(N, 0xABCD), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint16_t));
}
BENCHMARK(BM_BatchByteSwap16_Small)->Arg(64)->Arg(256);

static void BM_BatchByteSwap16_Large(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint16_t> src(N, 0xABCD), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint16_t));
}
BENCHMARK(BM_BatchByteSwap16_Large)->Arg(1024)->Arg(4096)->Arg(16384);

static void BM_BatchByteSwap32_Small(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N, 0xDEADBEEF), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_BatchByteSwap32_Small)->Arg(64)->Arg(256);

static void BM_BatchByteSwap32_Large(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N, 0xDEADBEEF), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_BatchByteSwap32_Large)->Arg(1024)->Arg(4096)->Arg(16384);

static void BM_BatchByteSwap64_Small(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> src(N, 0xCAFEBABE01020304ull), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint64_t));
}
BENCHMARK(BM_BatchByteSwap64_Small)->Arg(64)->Arg(256);

static void BM_BatchByteSwap64_Large(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> src(N, 0xCAFEBABE01020304ull), dst(N);
    
    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint64_t));
}
BENCHMARK(BM_BatchByteSwap64_Large)->Arg(1024)->Arg(4096)->Arg(16384);

// ============================================================================
// In-place batch byte_swap benchmarks
// ============================================================================

static void BM_BatchByteSwapInplace32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> data(N, 0xDEADBEEF);
    
    for (auto _ : state)
    {
        auto data_copy = data;
        detail::batch_byte_swap_inplace(data_copy.data(), N);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_BatchByteSwapInplace32)->RangeMultiplier(4)->Range(64, 16384);

static void BM_BatchByteSwapInplace64(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> data(N, 0xCAFEBABE01020304ull);
    
    for (auto _ : state)
    {
        auto data_copy = data;
        detail::batch_byte_swap_inplace(data_copy.data(), N);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint64_t));
}
BENCHMARK(BM_BatchByteSwapInplace64)->RangeMultiplier(4)->Range(64, 16384);

// ============================================================================
// Prefetch batch benchmarks
// ============================================================================

static void BM_BatchByteSwapPrefetch32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N, 0xDEADBEEF), dst(N);
    
    for (auto _ : state)
    {
        batch_byte_swap_prefetch(src.data(), dst.data(), N, 16);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_BatchByteSwapPrefetch32)->RangeMultiplier(4)->Range(256, 16384);

static void BM_BatchByteSwapPrefetch64(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> src(N, 0xCAFEBABE01020304ull), dst(N);
    
    for (auto _ : state)
    {
        batch_byte_swap_prefetch(src.data(), dst.data(), N, 16);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint64_t));
}
BENCHMARK(BM_BatchByteSwapPrefetch64)->RangeMultiplier(4)->Range(256, 16384);

// ============================================================================
// Non-temporal store batch benchmarks
// ============================================================================

static void BM_BatchByteSwapNT32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N, 0xDEADBEEF), dst(N);
    
    for (auto _ : state)
    {
        batch_byte_swap_nt(dst.data(), src.data(), N);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_BatchByteSwapNT32)->RangeMultiplier(4)->Range(256, 16384);

static void BM_BatchByteSwapNT64(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> src(N, 0xCAFEBABE01020304ull), dst(N);
    
    for (auto _ : state)
    {
        batch_byte_swap_nt(dst.data(), src.data(), N);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint64_t));
}
BENCHMARK(BM_BatchByteSwapNT64)->RangeMultiplier(4)->Range(256, 16384);

// ============================================================================
// convert_batch benchmarks
// ============================================================================

static void BM_ConvertBatch32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N, 0xDEADBEEF), dst(N);
    
    for (auto _ : state)
    {
        basic_endian<uint32_t>::convert_batch(src, dst, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_ConvertBatch32)->RangeMultiplier(4)->Range(64, 16384);

static void BM_ConvertBatchInplace32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> data(N, 0xDEADBEEF);
    
    for (auto _ : state)
    {
        auto data_copy = data;
        basic_endian<uint32_t>::convert_batch_inplace(data_copy, false);
        benchmark::ClobberMemory();
    }
    state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(N) * sizeof(uint32_t));
}
BENCHMARK(BM_ConvertBatchInplace32)->RangeMultiplier(4)->Range(64, 16384);

// ============================================================================
// endian_span benchmarks
// ============================================================================

static void BM_EndianSpanAccess32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> data(N, 0xDEADBEEF);
    endian_span<uint32_t> span(data);
    
    size_t idx = 0;
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(span[idx].native());
        idx = (idx + 1) % N;
    }
}
BENCHMARK(BM_EndianSpanAccess32)->RangeMultiplier(4)->Range(64, 4096);

static void BM_EndianSpanIteration32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> data(N, 0xDEADBEEF);
    endian_span<uint32_t> span(data);
    
    for (auto _ : state)
    {
        uint64_t sum = 0;
        for (const auto& elem : span)
        {
            sum += elem.native();
        }
        benchmark::DoNotOptimize(sum);
    }
}
BENCHMARK(BM_EndianSpanIteration32)->RangeMultiplier(4)->Range(64, 4096);

// ============================================================================
// Cache-optimized storage benchmarks
// ============================================================================

static void BM_CacheLinePaddedStore(benchmark::State& state)
{
    detail::cache_line_padded<uint64_t> padded;
    const uint64_t v = 0xCAFEBABE01020304ull;
    
    for (auto _ : state)
    {
        padded.value = v;
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_CacheLinePaddedStore);

static void BM_CacheLinePaddedLoad(benchmark::State& state)
{
    detail::cache_line_padded<uint64_t> padded(0xCAFEBABE01020304ull);
    
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(padded.value);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_CacheLinePaddedLoad);

// ============================================================================
// atomic_padded benchmarks
// ============================================================================

static void BM_AtomicPaddedStore(benchmark::State& state)
{
    detail::atomic_padded<uint64_t> atomic;
    const uint64_t v = 0xCAFEBABE01020304ull;
    
    for (auto _ : state)
    {
        atomic.store(v, std::memory_order_relaxed);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_AtomicPaddedStore);

static void BM_AtomicPaddedLoad(benchmark::State& state)
{
    detail::atomic_padded<uint64_t> atomic(0xCAFEBABE01020304ull);
    
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(atomic.load(std::memory_order_relaxed));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_AtomicPaddedLoad);

// ============================================================================
// MMIO benchmarks
// ============================================================================

static void BM_MMIOWrite32(benchmark::State& state)
{
    uint32_t storage = 0;
    const uint32_t v = 0xDEADBEEF;
    
    for (auto _ : state)
    {
        basic_endian<uint32_t>::write_mmio(&storage, v, std::memory_order_relaxed);
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MMIOWrite32);

static void BM_MMIORead32(benchmark::State& state)
{
    uint32_t storage = 0xDEADBEEF;
    
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(basic_endian<uint32_t>::read_mmio(&storage, std::memory_order_relaxed));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_MMIORead32);

// ============================================================================
// Serialization benchmarks
// ============================================================================

static void BM_SerializeUint32(benchmark::State& state)
{
    basic_endian<uint32_t> e(0xDEADBEEF);
    std::vector<char> buffer(1024);
    
    for (auto _ : state)
    {
        std::stringstream ss;
        for (int i = 0; i < 100; ++i)
        {
            e.serialize(ss);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * 100);
}
BENCHMARK(BM_SerializeUint32);

static void BM_DeserializeUint32(benchmark::State& state)
{
    basic_endian<uint32_t> e(0xDEADBEEF);
    std::stringstream ss;
    for (int i = 0; i < 100; ++i)
        e.serialize(ss);
    
    for (auto _ : state)
    {
        ss.seekg(0);
        basic_endian<uint32_t> loaded;
        for (int i = 0; i < 100; ++i)
        {
            loaded.deserialize(ss);
        }
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(int64_t(state.iterations()) * 100);
}
BENCHMARK(BM_DeserializeUint32);

// ============================================================================
// Comparison with std::byteswap (C++23)
// ============================================================================

#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L

static void BM_StdByteSwap32(benchmark::State& state)
{
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(std::byteswap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_StdByteSwap32)->Arg(0xDEADBEEF);

static void BM_StdByteSwap64(benchmark::State& state)
{
    const uint64_t v = static_cast<uint64_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(std::byteswap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_StdByteSwap64)->Arg(0x0102030405060708LL);

#endif

BENCHMARK_MAIN();
