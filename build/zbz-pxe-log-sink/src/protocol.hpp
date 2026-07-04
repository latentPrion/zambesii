#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>

namespace zbz::protocol {

inline constexpr std::uint8_t kMagic0 = 0x5A;
inline constexpr std::uint8_t kMagic1 = 0x62;
inline constexpr std::uint8_t kVersion = 0x01;
inline constexpr std::uint16_t kDefaultEtherType = 0x02B2;
inline constexpr std::size_t kEthernetHeaderLen = 14;
inline constexpr std::size_t kFrameHeaderLen = 4;

enum class MessageType : std::uint8_t {
	SessionStart = 0x01,
	Log = 0x02,
};

struct ParsedFrame {
	MessageType type{};
	std::span<const std::byte> payload{};
};

inline std::expected<ParsedFrame, std::string_view> parse(
	std::span<const std::byte> ethernetPayload)
{
	if (ethernetPayload.size() < kFrameHeaderLen) {
		return std::unexpected(std::string_view{"frame too short"});
	}

	const auto* bytes = reinterpret_cast<const std::uint8_t*>(
		ethernetPayload.data());

	if (bytes[0] != kMagic0 || bytes[1] != kMagic1) {
		return std::unexpected(std::string_view{"bad magic"});
	}
	if (bytes[2] != kVersion) {
		return std::unexpected(std::string_view{"unsupported version"});
	}

	const auto type = static_cast<MessageType>(bytes[3]);
	if (type != MessageType::SessionStart && type != MessageType::Log) {
		return std::unexpected(std::string_view{"unknown type"});
	}

	return ParsedFrame{
		type,
		ethernetPayload.subspan(kFrameHeaderLen),
	};
}

} // namespace zbz::protocol
