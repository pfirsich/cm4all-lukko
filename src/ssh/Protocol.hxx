// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "util/ByteOrder.hxx"

#include <cstdint>
#include <cstddef>

namespace SSH {

struct PacketHeader {
	PackedBE32 length;
};

constexpr std::size_t
Padding(std::size_t size) noexcept
{
	/* minimum packet size is 16 bytes (see RFC 4253 section 6),
	   and since the padding is at least 4 bytes, we need to check
	   only for sizes up to 12 here */
	if (size <= 12)
		return 16 - size;

	return 11 - ((size - 5) & 0x7);
}

/**
 * @see RFC 4253 section 12
 */
enum class MessageNumber : uint8_t {
	DISCONNECT = 1,
	IGNORE = 2,
	UNIMPLEMENTED = 3,
	DEBUG = 4,
	SERVICE_REQUEST = 5,
	SERVICE_ACCEPT = 6,
	EXT_INFO = 7,
	NEWCOMPRESS = 8,

	KEXINIT = 20,
	NEWKEYS = 21,

	ECDH_KEX_INIT = 30,
	ECDH_KEX_INIT_REPLY = 31,

	USERAUTH_REQUEST = 50,
	USERAUTH_FAILURE = 51,
	USERAUTH_SUCCESS = 52,
	USERAUTH_BANNER = 53,

	USERAUTH_INFO_REQUEST = 60,
	USERAUTH_INFO_RESPONSE = 61,

	GLOBAL_REQUEST = 80,
	REQUEST_SUCCESS = 81,
	REQUEST_FAILURE = 82,

	CHANNEL_OPEN = 90,
	CHANNEL_OPEN_CONFIRMATION = 91,
	CHANNEL_OPEN_FAILURE = 92,
	CHANNEL_WINDOW_ADJUST = 93,
	CHANNEL_DATA = 94,
	CHANNEL_EXTENDED_DATA = 95,
	CHANNEL_EOF = 96,
	CHANNEL_CLOSE = 97,
	CHANNEL_REQUEST = 98,
	CHANNEL_SUCCESS = 99,
	CHANNEL_FAILURE = 100,
};

static constexpr std::size_t MAX_PACKET_SIZE = 35000;

static constexpr std::size_t KEX_COOKIE_SIZE = 16;

enum class DisconnectReasonCode : uint32_t {
	HOST_NOT_ALLOWED_TO_CONNECT = 1,
	PROTOCOL_ERROR = 2,
	KEY_EXCHANGE_FAILED = 3,
	RESERVED = 4,
	MAC_ERROR = 5,
	COMPRESSION_ERROR = 6,
	SERVICE_NOT_AVAILABLE = 7,
	PROTOCOL_VERSION_NOT_SUPPORTED = 8,
	HOST_KEY_NOT_VERIFIABLE = 9,
	CONNECTION_LOST = 10,
	BY_APPLICATION = 11,
	TOO_MANY_CONNECTIONS = 12,
	AUTH_CANCELLED_BY_USER = 13,
	NO_MORE_AUTH_METHODS_AVAILABLE = 14,
	ILLEGAL_USER_NAME = 15,
};

enum class ChannelOpenFailureReasonCode : uint32_t {
	ADMINISTRATIVELY_PROHIBITED = 1,
	CONNECT_FAILED = 2,
	UNKNOWN_CHANNEL_TYPE = 3,
	RESOURCE_SHORTAGE = 4,
};

enum class ChannelExtendedDataType : uint32_t {
	STDERR = 1,
};

} // namespace SSH
