#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

#include <csignal>

#include "packet_socket.hpp"
#include "protocol.hpp"
#include "session_writer.hpp"

namespace {

struct Options {
	std::string interface{"auto"};
	std::filesystem::path logDir{"/var/log/zbz-kernel"};
	std::uint16_t etherType{zbz::protocol::kDefaultEtherType};
	std::chrono::seconds idleGap{30};
	bool raw{false};
	std::chrono::milliseconds reopenDelay{1500};
};

std::atomic<bool> gStop{false};

void onSignal(int) { gStop.store(true); }

void usage(const char* prog)
{
	std::cerr
		<< "Usage: " << prog << " [options]\n"
		<< "  --interface NAME   Interface name or 'auto' (default: auto)\n"
		<< "  --log-dir PATH     Session log directory "
		   "(default: /var/log/zbz-kernel)\n"
		<< "  --ethertype HEX    EtherType filter (default: 0x02B2)\n"
		<< "  --idle-gap SECS    New session after idle gap; 0 disables "
		   "(default: 30)\n"
		<< "  --raw              Append LOG payloads without timestamps\n"
		<< "  --reopen-delay MS  Delay before socket reopen (default: 1500)\n"
		<< "  -h, --help         Show this help\n";
}

std::expected<std::uint16_t, std::string> parseEtherType(std::string_view s)
{
	std::string str(s);
	if (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0) {
		str = str.substr(2);
	}

	char* end = nullptr;
	const auto val = std::strtoul(str.c_str(), &end, 16);
	if (end == str.c_str() || *end != '\0' || val > 0xFFFFu) {
		return std::unexpected(std::string{"invalid ethertype: "}
			+ std::string(s));
	}
	return static_cast<std::uint16_t>(val);
}

std::expected<Options, std::string> parseArgs(int argc, char** argv)
{
	Options opt;
	for (int i = 1; i < argc; ++i) {
		const std::string_view arg(argv[i]);
		auto needValue = [&](const char* flag) -> std::expected<std::string_view, std::string> {
			if (i + 1 >= argc) {
				return std::unexpected(
					std::string{"missing value for "} + flag);
			}
			return std::string_view{argv[++i]};
		};

		if (arg == "-h" || arg == "--help") {
			usage(argv[0]);
			std::exit(0);
		}
		if (arg == "--interface") {
			auto v = needValue("--interface");
			if (!v) { return std::unexpected(v.error()); }
			opt.interface = std::string(*v);
		} else if (arg == "--log-dir") {
			auto v = needValue("--log-dir");
			if (!v) { return std::unexpected(v.error()); }
			opt.logDir = std::filesystem::path(*v);
		} else if (arg == "--ethertype") {
			auto v = needValue("--ethertype");
			if (!v) { return std::unexpected(v.error()); }
			auto et = parseEtherType(*v);
			if (!et) { return std::unexpected(et.error()); }
			opt.etherType = *et;
		} else if (arg == "--idle-gap") {
			auto v = needValue("--idle-gap");
			if (!v) { return std::unexpected(v.error()); }
			opt.idleGap = std::chrono::seconds(
				std::strtol(std::string(*v).c_str(), nullptr, 10));
		} else if (arg == "--raw") {
			opt.raw = true;
		} else if (arg == "--reopen-delay") {
			auto v = needValue("--reopen-delay");
			if (!v) { return std::unexpected(v.error()); }
			opt.reopenDelay = std::chrono::milliseconds(
				std::strtol(std::string(*v).c_str(), nullptr, 10));
		} else {
			return std::unexpected(
				std::string{"unknown argument: "} + std::string(arg));
		}
	}
	return opt;
}

std::string_view payloadAsString(std::span<const std::byte> payload)
{
	return {reinterpret_cast<const char*>(payload.data()), payload.size()};
}

void handleFrame(
	zbz::SessionWriter& writer,
	const Options& opt,
	std::span<const std::byte> frame,
	std::string_view srcMac)
{
	if (frame.size() < zbz::protocol::kEthernetHeaderLen) {
		return;
	}

	const auto payload = frame.subspan(zbz::protocol::kEthernetHeaderLen);
	const auto parsed = zbz::protocol::parse(payload);
	if (!parsed) {
		std::cerr << "zbz-pxe-log-sink: drop frame: "
			<< parsed.error() << '\n';
		return;
	}

	writer.onIdleGap(opt.idleGap);

	switch (parsed->type) {
	case zbz::protocol::MessageType::SessionStart:
		writer.onSessionStart(payloadAsString(parsed->payload), srcMac);
		break;
	case zbz::protocol::MessageType::Log:
		writer.onLog(parsed->payload, srcMac);
		break;
	}
}

int run(const Options& opt)
{
	zbz::PacketSocket sock;
	zbz::SessionWriter writer(opt.logDir, opt.raw);

	while (!gStop.load()) {
		if (!sock.isOpen()) {
			auto opened = sock.open(opt.interface, opt.etherType);
			if (!opened) {
				std::cerr << "zbz-pxe-log-sink: "
					<< opened.error() << "; retrying...\n";
				std::this_thread::sleep_for(opt.reopenDelay);
				continue;
			}
			std::cerr << "zbz-pxe-log-sink: listening on "
				<< sock.ifaceName()
				<< " ethertype 0x"
				<< std::hex << opt.etherType << std::dec << '\n';
		}

		auto frame = sock.recv(std::chrono::milliseconds{1000});
		if (!frame) {
			if (frame.error() == "timeout") {
				continue;
			}
			std::cerr << "zbz-pxe-log-sink: " << frame.error()
				<< "; reopening socket\n";
			sock.close();
			std::this_thread::sleep_for(opt.reopenDelay);
			continue;
		}

		handleFrame(writer, opt, *frame, sock.lastSrcMac());
	}

	writer.flush();
	return 0;
}

} // namespace

int main(int argc, char** argv)
{
	std::signal(SIGINT, onSignal);
	std::signal(SIGTERM, onSignal);

	const auto opt = parseArgs(argc, argv);
	if (!opt) {
		std::cerr << opt.error() << '\n';
		usage(argv[0]);
		return 1;
	}

	return run(*opt);
}
