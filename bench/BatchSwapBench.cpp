#include <benchmark/benchmark.h>
#include "byteorder/byteorder.h"

#include <cstdint>
#include <vector>

using namespace endian;

// ────────────────────────────────────────────────────────────────────────────
// batch_byte_swap — directly from detail
// ────────────────────────────────────────────────────────────────────────────

template <typename T>
static void BM_BatchByteSwapRaw(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<T> src(N), dst(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<T>(i);

    for (auto _ : state)
    {
        detail::batch_byte_swap(src.data(), dst.data(), N, /*check_alignment=*/false);
        benchmark::DoNotOptimize(dst.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(N * sizeof(T)));
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}

BENCHMARK_TEMPLATE(BM_BatchByteSwapRaw, uint16_t)->Range(64, 64 << 10);
BENCHMARK_TEMPLATE(BM_BatchByteSwapRaw, uint32_t)->Range(64, 64 << 10);
BENCHMARK_TEMPLATE(BM_BatchByteSwapRaw, uint64_t)->Range(64, 64 << 10);

// ────────────────────────────────────────────────────────────────────────────
// basic_endian::convert_batch (via contiguous range)
// ────────────────────────────────────────────────────────────────────────────

static void BM_ConvertBatch32(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint32_t> src(N), dst(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint32_t>(i * 0x12345678u);

    for (auto _ : state)
    {
        basic_endian<uint32_t>::convert_batch(src, dst, /*check_alignment=*/false);
        benchmark::DoNotOptimize(dst.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(N * 4));
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_ConvertBatch32)->Range(64, 64 << 10);

static void BM_ConvertBatch64(benchmark::State& state)
{
    const size_t N = static_cast<size_t>(state.range(0));
    std::vector<uint64_t> src(N), dst(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint64_t>(i) * 0xFEDCBA9876543210ull;

    for (auto _ : state)
    {
        basic_endian<uint64_t>::convert_batch(src, dst, /*check_alignment=*/false);
        benchmark::DoNotOptimize(dst.data());
        benchmark::ClobberMemory();
    }

    state.SetBytesProcessed(state.iterations() * static_cast<int64_t>(N * 8));
    state.SetItemsProcessed(state.iterations() * static_cast<int64_t>(N));
}
BENCHMARK(BM_ConvertBatch64)->Range(64, 64 << 10);

BENCHMARK_MAIN();
