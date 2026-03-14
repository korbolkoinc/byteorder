#include <gtest/gtest.h>
#include "byteorder/byteorder.h"

#include <array>
#include <cstdint>
#include <sstream>
#include <vector>

using namespace endian;

// ============================================================================
// detail::batch_byte_swap
// ============================================================================

TEST(BatchByteSwap, Width16AllElements)
{
    constexpr size_t N = 8;
    std::array<uint16_t, N> src = {0x0102, 0x0304, 0x0506, 0x0708,
                                   0x090A, 0x0B0C, 0x0D0E, 0x0F10};
    std::array<uint16_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, Width32AllElements)
{
    constexpr size_t N = 4;
    std::array<uint32_t, N> src = {0xDEADBEEF, 0xCAFEBABE, 0x01020304, 0xFFFFFFFF};
    std::array<uint32_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, Width64AllElements)
{
    constexpr size_t N = 4;
    std::array<uint64_t, N> src = {
        0x0102030405060708ull, 0xDEADBEEFCAFEBABEull,
        0x0ull,                ~0ull
    };
    std::array<uint64_t, N> dst{};

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i])) << "mismatch at index " << i;
}

TEST(BatchByteSwap, LargeBatch32)
{
    constexpr size_t N = 128;
    std::vector<uint32_t> src(N), dst(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint32_t>(i * 0x01020304u);

    detail::batch_byte_swap(src.data(), dst.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

TEST(BatchByteSwap, RoundTrip)
{
    constexpr size_t N = 32;
    std::vector<uint64_t> src(N), tmp(N), result(N);
    for (size_t i = 0; i < N; ++i)
        src[i] = static_cast<uint64_t>(i) * 0xFEDCBA9876543210ull;

    detail::batch_byte_swap(src.data(),    tmp.data(),    N, false);
    detail::batch_byte_swap(tmp.data(),    result.data(), N, false);

    for (size_t i = 0; i < N; ++i)
        EXPECT_EQ(result[i], src[i]);
}

TEST(BatchByteSwap, SingleElement)
{
    uint32_t src = 0xABCD1234, dst = 0;
    detail::batch_byte_swap(&src, &dst, 1, false);
    EXPECT_EQ(dst, detail::byte_swap(src));
}

TEST(BatchByteSwap, ZeroElements)
{
    uint32_t dummy = 0xDEAD;
    EXPECT_NO_THROW(detail::batch_byte_swap(&dummy, &dummy, 0, false));
}

// ============================================================================
// basic_endian::convert_batch (via ContainerRange)
// ============================================================================

TEST(ConvertBatch, StdVector32)
{
    std::vector<uint32_t> src = {0x11223344, 0xAABBCCDD, 0x01020304};
    std::vector<uint32_t> dst(src.size());

    basic_endian<uint32_t>::convert_batch(src, dst, false);

    for (size_t i = 0; i < src.size(); ++i)
        EXPECT_EQ(dst[i], detail::byte_swap(src[i]));
}

// ============================================================================
// buffer_view
// ============================================================================

TEST(BufferView, StoreBigLoadBig)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);

    view.store_big(0x01020304);
    EXPECT_EQ(view.load_big(), uint32_t{0x01020304});
}

TEST(BufferView, StoreLittleLoadLittle)
{
    std::array<std::byte, 4> buf{};
    basic_endian<uint32_t>::buffer_view view(buf);

    view.store_little(0x01020304);
    EXPECT_EQ(view.load_little(), uint32_t{0x01020304});
}

TEST(BufferView, StoreNativeLoadNative)
{
    std::array<std::byte, 8> buf{};
    basic_endian<uint64_t>::buffer_view view(buf);

    const uint64_t val = 0xDEADBEEFCAFEBABEull;
    view.store_native(val);
    EXPECT_EQ(view.load_native(), val);
}

TEST(BufferView, BigEndianByteLayout)
{
    std::array<std::byte, 2> buf{};
    basic_endian<uint16_t>::buffer_view view(buf);

    view.store_big(0x1234);

    EXPECT_EQ(buf[0], std::byte{0x12});
    EXPECT_EQ(buf[1], std::byte{0x34});
}

TEST(BufferView, LittleEndianByteLayout)
{
    std::array<std::byte, 2> buf{};
    basic_endian<uint16_t>::buffer_view view(buf);

    view.store_little(0x1234);

    EXPECT_EQ(buf[0], std::byte{0x34});
    EXPECT_EQ(buf[1], std::byte{0x12});
}

TEST(BufferView, ThrowsOnSmallBuffer)
{
    std::array<std::byte, 2> buf{};
    EXPECT_THROW(
        (basic_endian<uint64_t>::buffer_view{buf}),
        std::out_of_range);
}

// ============================================================================
// network_order helpers
// ============================================================================

TEST(NetworkOrder, HostToNetworkRoundtrip16)
{
    using no = basic_endian<uint16_t>::network_order;
    constexpr uint16_t value = 0x1234;
    EXPECT_EQ(no::network_to_host(no::host_to_network(value)), value);
}

TEST(NetworkOrder, HostToNetworkRoundtrip32)
{
    using no = basic_endian<uint32_t>::network_order;
    constexpr uint32_t value = 0xDEADBEEF;
    EXPECT_EQ(no::network_to_host(no::host_to_network(value)), value);
}

TEST(NetworkOrder, ByteOrderIsAlwaysBig)
{
    EXPECT_EQ(basic_endian<uint32_t>::network_order::byte_order, std::endian::big);
}

// ============================================================================
// Stream serialization / deserialization
// ============================================================================

TEST(Serialization, RoundTripUint16)
{
    basic_endian<uint16_t> orig(0xBEEF), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, RoundTripUint32)
{
    basic_endian<uint32_t> orig(0xDEADBEEF), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, RoundTripUint64)
{
    basic_endian<uint64_t> orig(0xCAFEBABEDEAD1234ull), loaded;
    std::stringstream ss;
    orig.serialize(ss);
    loaded.deserialize(ss);
    EXPECT_EQ(loaded.native(), orig.native());
}

TEST(Serialization, AlwaysWritesBigEndianBytes)
{
    basic_endian<uint16_t> e(0x0102);
    std::stringstream ss;
    e.serialize(ss);
    const std::string bytes = ss.str();
    EXPECT_EQ(static_cast<unsigned char>(bytes[0]), 0x01u);
    EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 0x02u);
}

// ============================================================================
// memory-mapped I/O (write_mmio / read_mmio)
// ============================================================================

TEST(MMIO, WriteAndReadBack32)
{
    uint32_t storage = 0;
    basic_endian<uint32_t>::write_mmio(&storage, 0x11223344);
    const uint32_t back = basic_endian<uint32_t>::read_mmio(&storage);
    EXPECT_EQ(back, uint32_t{0x11223344});
}

TEST(MMIO, WriteAndReadBack64)
{
    uint64_t storage = 0;
    basic_endian<uint64_t>::write_mmio(&storage, 0xDEADBEEFCAFEBABEull);
    const uint64_t back = basic_endian<uint64_t>::read_mmio(&storage);
    EXPECT_EQ(back, uint64_t{0xDEADBEEFCAFEBABEull});
}
