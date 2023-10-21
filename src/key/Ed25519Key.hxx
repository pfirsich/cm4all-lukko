// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#pragma once

#include "Key.hxx"

#include <array>

class Ed25519Key final : public Key {
	std::array<std::byte, 32> public_key;
	std::array<std::byte, 64> secret_key;

public:
	~Ed25519Key() noexcept override;

	void Generate();

	std::string_view GetAlgorithm() const noexcept override;
	void SerializePublic(SSH::Serializer &s) const override;
	void SerializeKex(SSH::Serializer &s) const override;
	void Sign(SSH::Serializer &s, std::span<const std::byte> src) const override;
};
