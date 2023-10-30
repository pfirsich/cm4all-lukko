// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Sizes.hxx"
#include "util/SpanCast.hxx"

#include <algorithm> // for std::copy()
#include <array>
#include <cassert>
#include <cstddef>
#include <span>

namespace SSH {

struct PacketTooLarge {};

class Serializer {
	std::size_t skip = 0, position = 0;

protected:
	std::array<std::byte, MAX_PACKET_SIZE> buffer;

	constexpr std::size_t size() const noexcept {
		return position - skip;
	}

public:
	constexpr std::span<std::byte> BeginWriteN(std::size_t size) {
		std::size_t max_size = buffer.size() - position;
		if (size > max_size)
			throw PacketTooLarge{};

		return std::span{buffer}.subspan(position, size);
	}

	constexpr void CommitWriteN(std::size_t size) noexcept {
		position += size;
	}

	constexpr std::span<std::byte> WriteN(std::size_t size) {
		auto result = BeginWriteN(size);
		CommitWriteN(size);
		return result;
	}

	template<std::size_t size>
	constexpr std::span<std::byte, size> WriteN() {
		auto result = BeginWriteN(size);
		CommitWriteN(size);
		return result.first<size>();
	}

	/**
	 * Write a number of zero bytes.
	 */
	constexpr void WriteZero(std::size_t size) {
		auto s = WriteN(size);
		std::fill(s.begin(), s.end(), std::byte{});
	}

	constexpr void WriteN(std::span<const std::byte> src) {
		auto dest = WriteN(src.size());
		std::copy(src.begin(), src.end(), dest.begin());
	}

	template<typename T>
	constexpr T &WriteT() {
		return *reinterpret_cast<T *>(WriteN(sizeof(T)).data());
	}

	template<typename T>
	constexpr void WriteT(const T &src) {
		WriteT<T>() = src;
	}

	constexpr void WriteU8(uint8_t value) {
		WriteT(value);
	}

	constexpr void WriteU16(uint_least16_t value) {
		auto dest = FromBytesStrict<uint8_t>(WriteN(2));
		dest[0] = value >> 8;
		dest[1] = value;
	}

	constexpr void WriteU32(uint_least32_t value) {
		auto dest = FromBytesStrict<uint8_t>(WriteN(4));
		dest[0] = value >> 24;
		dest[1] = value >> 16;
		dest[2] = value >> 8;
		dest[3] = value;
	}

	constexpr void WriteBool(bool value) {
		WriteU8(value);
	}

	void WriteString(std::string_view s) {
		WriteU32(s.size());
		WriteN(AsBytes(s));
	}

	constexpr void WriteLengthEncoded(std::span<const std::byte> src) {
		WriteU32(src.size());
		WriteN(src);
	}

	constexpr void WriteBignum2(std::span<const std::byte> src) {
		// skip leading zeroes
		while (!src.empty() && src.front() == std::byte{})
			src = src.subspan(1);

		const bool leading_msb = !src.empty() &&
			(src.front() & std::byte{0x80}) != std::byte{};

		WriteU32(src.size() + leading_msb);

		if (leading_msb)
			// prepend zero, it's not negative
			WriteU8(0);

		WriteN(src);
	}

	constexpr std::size_t Mark() const noexcept {
		return position;
	}

	constexpr void Rewind(std::size_t old_position) noexcept {
		assert(position >= old_position);

		position = old_position;
	}

	constexpr std::span<const std::byte> Since(std::size_t old_position) const noexcept {
		assert(position >= old_position);

		return std::span{buffer}.first(position).subspan(old_position);
	}

	constexpr std::size_t PrepareLength() {
		std::size_t result = position;
		WriteU32(0);
		return result;
	}

	void CommitLength(std::size_t at) noexcept {
		const std::size_t value = position - at - 4;
		uint8_t *dest = reinterpret_cast<uint8_t *>(buffer.data() + at);
		dest[0] = value >> 24;
		dest[1] = value >> 16;
		dest[2] = value >> 8;
		dest[3] = value;
	}

	void InsertNullByte(std::size_t backwards_offset) {
		assert(backwards_offset <= size());

		if (position >= buffer.size())
			throw PacketTooLarge{};

		auto r = std::span{buffer}.first(position).last(backwards_offset);
		*std::prev(std::copy_backward(r.begin(), r.end(), std::next(r.end()))) = std::byte{};
		CommitWriteN(1);
	}

	constexpr void Skip(std::size_t nbytes) noexcept {
		skip += nbytes;
	}

	constexpr std::span<const std::byte> Finish() noexcept {
		return std::span{buffer}.first(position).subspan(skip);
	}
};

} // namespace Mysql
