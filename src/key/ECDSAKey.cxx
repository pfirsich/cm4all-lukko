// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "ECDSAKey.hxx"
#include "ssh/Serializer.hxx"
#include "openssl/SerializeEVP.hxx"
#include "openssl/Sign.hxx"
#include "openssl/Verify.hxx"
#include "lib/openssl/Key.hxx"

using std::string_view_literals::operator""sv;

ECDSAKey::ECDSAKey(Generate)
	:key(GenerateEcKey())
{
}

std::string_view
ECDSAKey::GetAlgorithm() const noexcept
{
	return "ecdsa-sha2-nistp256"sv;
}

void
ECDSAKey::SerializePublic(SSH::Serializer &s) const
{
	constexpr auto ecdsa_curve_id = "nistp256"sv;

	s.WriteString(GetAlgorithm());
	s.WriteString(ecdsa_curve_id);

	const auto key_length = s.PrepareLength();
	SerializePublicKey(s, *key);
	s.CommitLength(key_length);
}

bool
ECDSAKey::Verify(std::span<const std::byte> message,
	       std::span<const std::byte> signature) const
{
	// TODO do we a special ECDSA verifier?
	return VerifyGeneric(*key, DigestAlgorithm::SHA256, message, signature);
}

void
ECDSAKey::Sign(SSH::Serializer &s, std::span<const std::byte> src) const
{
	constexpr DigestAlgorithm hash_alg = DigestAlgorithm::SHA256; // TODO

	SignECDSA(s, *key, hash_alg, src);

}
