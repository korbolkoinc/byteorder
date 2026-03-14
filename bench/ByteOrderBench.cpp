#include <benchmark/benchmark.h>
#include "byteorder/byteorder.h"

#include <cstdint>

using namespace endian;

// ────────────────────────────────────────────────────────────────────────────
// Scalar byte_swap
// ────────────────────────────────────────────────────────────────────────────

static void BM_ByteSwap16(benchmark::State& state)
{
    const uint16_t v = static_cast<uint16_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwap16)->Arg(0xABCD);

static void BM_ByteSwap32(benchmark::State& state)
{
    const uint32_t v = static_cast<uint32_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwap32)->Arg(0xDEADBEEF);

static void BM_ByteSwap64(benchmark::State& state)
{
    const uint64_t v = static_cast<uint64_t>(state.range(0));
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(detail::byte_swap(v));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(BM_ByteSwap64)->Arg(0x0102030405060708LL);

// ────────────────────────────────────────────────────────────────────────────
// basic_endian accessors (host → big, host → little, round-trip)
// ────────────────────────────────────────────────────────────────────────────

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

// ────────────────────────────────────────────────────────────────────────────
// Network-order helpers
// ────────────────────────────────────────────────────────────────────────────

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

BENCHMARK_MAIN();
