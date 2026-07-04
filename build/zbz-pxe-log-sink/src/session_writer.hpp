#pragma once

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "protocol.hpp"

namespace zbz {

inline std::string formatLocalTime(
	const std::tm& tm, std::string_view pattern)
{
	std::string buf(64, '\0');
	const auto n = std::strftime(
		buf.data(), buf.size(), std::string(pattern).c_str(), &tm);
	if (n == 0) {
		return "unknown-time";
	}
	buf.resize(n);
	return buf;
}

class SessionWriter {
public:
	explicit SessionWriter(std::filesystem::path logDir, bool rawMode)
	: logDir_(std::move(logDir)), rawMode_(rawMode)
	{
		std::filesystem::create_directories(logDir_);
	}

	void flush()
	{
		if (out_.is_open()) {
			out_.flush();
		}
	}

	void onIdleGap(std::chrono::seconds gap)
	{
		if (gap.count() <= 0 || !lastFrameTime_.has_value()) {
			return;
		}
		const auto now = std::chrono::system_clock::now();
		if (now - *lastFrameTime_ >= gap) {
			startNewSession("idle-gap", "unknown");
		}
	}

	void onSessionStart(
		std::string_view metadata, std::string_view srcMac)
	{
		startNewSession(metadata, srcMac);
	}

	void onLog(std::span<const std::byte> payload, std::string_view srcMac)
	{
		ensureSession(srcMac);
		writePayload(payload);
		lastFrameTime_ = std::chrono::system_clock::now();
	}

private:
	void ensureSession(std::string_view srcMac)
	{
		if (!out_.is_open()) {
			startNewSession("", srcMac);
		}
	}

	void startNewSession(std::string_view metadata, std::string_view srcMac)
	{
		if (out_.is_open()) {
			out_.flush();
			out_.close();
		}

		const auto now = std::chrono::system_clock::now();
		const auto tt = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
		localtime_r(&tt, &tm);

		std::string path;
		const auto stampCompact = formatLocalTime(tm, "%Y%m%dT%H%M%S");
		for (int suffix = 0; suffix < 1000; ++suffix) {
			path = std::format(
				"{}/zbz-kernel-{}", logDir_.string(), stampCompact);
			if (suffix > 0) {
				path += std::format("-{:03d}", suffix);
			}
			path += ".log";

			if (!std::filesystem::exists(path)) {
				break;
			}
		}

		out_.open(path, std::ios::out | std::ios::app);
		if (!out_.is_open()) {
			std::fprintf(
				stderr, "zbz-pxe-log-sink: failed to open %s\n",
				path.c_str());
			return;
		}

		currentPath_ = path;
		const auto stamp = formatLocalTime(tm, "%Y-%m-%dT%H:%M:%S");
		out_ << "# SESSION_START " << stamp
			<< " src=" << srcMac;
		if (!metadata.empty()) {
			out_ << " meta=" << metadata;
		}
		out_ << '\n';
		out_.flush();
		lastFrameTime_ = std::chrono::system_clock::now();

		std::fprintf(
			stderr, "zbz-pxe-log-sink: new session %s\n", path.c_str());
	}

	void writePayload(std::span<const std::byte> payload)
	{
		if (!out_.is_open() || payload.empty()) {
			return;
		}

		if (!rawMode_) {
			const auto now = std::chrono::system_clock::now();
			const auto tt = std::chrono::system_clock::to_time_t(now);
			std::tm tm{};
			localtime_r(&tt, &tm);
			out_ << formatLocalTime(tm, "%Y-%m-%dT%H:%M:%S ") ;
		}

		out_.write(
			reinterpret_cast<const char*>(payload.data()),
			static_cast<std::streamsize>(payload.size()));
		out_.flush();
	}

	std::filesystem::path logDir_;
	bool rawMode_{false};
	std::ofstream out_;
	std::string currentPath_;
	std::optional<std::chrono::system_clock::time_point> lastFrameTime_;
};

} // namespace zbz
