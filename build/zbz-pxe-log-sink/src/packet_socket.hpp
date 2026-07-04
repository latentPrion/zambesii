#pragma once

#include <cerrno>
#include <chrono>
#include <cstring>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace zbz {

inline std::expected<int, std::string> resolveInterfaceIndex(
	std::string_view name)
{
	if (name == "auto") {
		struct ifaddrs* ifap = nullptr;
		if (getifaddrs(&ifap) != 0) {
			return std::unexpected(std::string{"getifaddrs: "}
				+ std::strerror(errno));
		}

		int chosen = -1;
		for (auto* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr == nullptr
				|| ifa->ifa_addr->sa_family != AF_INET) {
				continue;
			}
			const auto* sin = reinterpret_cast<sockaddr_in*>(ifa->ifa_addr);
			const auto addr = ntohl(sin->sin_addr.s_addr);
			if ((addr & 0xFFFFFF00u) == 0x0A2A0000u) { // 10.42.0.0/24
				chosen = static_cast<int>(if_nametoindex(ifa->ifa_name));
				break;
			}
		}
		freeifaddrs(ifap);

		if (chosen < 0) {
			return std::unexpected(
				std::string{"auto: no interface on 10.42.0.0/24"});
		}
		return chosen;
	}

	const auto idx = if_nametoindex(std::string(name).c_str());
	if (idx == 0) {
		return std::unexpected(std::string{"unknown interface: "}
			+ std::string(name));
	}
	return static_cast<int>(idx);
}

class PacketSocket {
public:
	PacketSocket() = default;

	PacketSocket(const PacketSocket&) = delete;
	PacketSocket& operator=(const PacketSocket&) = delete;

	~PacketSocket() { close(); }

	std::expected<void, std::string> open(
		std::string_view iface, std::uint16_t etherType)
	{
		close();

		auto idx = resolveInterfaceIndex(iface);
		if (!idx) {
			return std::unexpected(idx.error());
		}

		const int fd = ::socket(AF_PACKET, SOCK_RAW, htons(etherType));
		if (fd < 0) {
			return std::unexpected(std::string{"socket: "}
				+ std::strerror(errno));
		}

		sockaddr_ll bindAddr{};
		bindAddr.sll_family = AF_PACKET;
		bindAddr.sll_protocol = htons(etherType);
		bindAddr.sll_ifindex = *idx;

		if (::bind(fd, reinterpret_cast<sockaddr*>(&bindAddr),
			sizeof(bindAddr)) != 0) {
			const auto err = std::string{"bind: "} + std::strerror(errno);
			::close(fd);
			return std::unexpected(err);
		}

		fd_ = fd;
		ifaceName_ = (iface == "auto")
			? std::string(if_indextoname(*idx, ifaceBuf_))
			: std::string(iface);
		etherType_ = etherType;
		return {};
	}

	void close()
	{
		if (fd_ >= 0) {
			::close(fd_);
			fd_ = -1;
		}
	}

	bool isOpen() const { return fd_ >= 0; }

	std::expected<std::span<const std::byte>, std::string> recv(
		std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
	{
		if (fd_ < 0) {
			return std::unexpected(std::string{"socket not open"});
		}

		if (timeout.count() >= 0) {
			timeval tv{};
			tv.tv_sec = static_cast<time_t>(timeout.count() / 1000);
			tv.tv_usec = static_cast<suseconds_t>(
				(timeout.count() % 1000) * 1000);
			fd_set rfds;
			FD_ZERO(&rfds);
			FD_SET(fd_, &rfds);
			const int sel = ::select(fd_ + 1, &rfds, nullptr, nullptr, &tv);
			if (sel == 0) {
				return std::unexpected(std::string{"timeout"});
			}
			if (sel < 0) {
				return std::unexpected(std::string{"select: "}
					+ std::strerror(errno));
			}
		}

		sockaddr_ll from{};
		socklen_t fromLen = sizeof(from);
		const auto n = ::recvfrom(
			fd_, buffer_.data(), buffer_.size(), 0,
			reinterpret_cast<sockaddr*>(&from), &fromLen);

		if (n < 0) {
			return std::unexpected(std::string{"recvfrom: "}
				+ std::strerror(errno));
		}

		lastSrcMac_ = formatMac(from.sll_addr, from.sll_halen);
		return std::span<const std::byte>(
			reinterpret_cast<const std::byte*>(buffer_.data()),
			static_cast<std::size_t>(n));
	}

	std::string_view ifaceName() const { return ifaceName_; }
	std::uint16_t etherType() const { return etherType_; }
	std::string_view lastSrcMac() const { return lastSrcMac_; }

private:
	static std::string formatMac(const unsigned char* mac, unsigned char len)
	{
		if (len < 6) {
			return "unknown";
		}
		char out[18];
		std::snprintf(
			out, sizeof(out), "%02x:%02x:%02x:%02x:%02x:%02x",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		return out;
	}

	int fd_{-1};
	std::uint16_t etherType_{0};
	std::string ifaceName_;
	char ifaceBuf_[IF_NAMESIZE]{};
	std::string lastSrcMac_;
	std::vector<std::byte> buffer_{65536};
};

} // namespace zbz
