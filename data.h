#pragma once
#include <string>
#include "tester.h"
#include <fstream>


namespace nti
{
	class IChannelTesterSerialzer
	{
	public:
		virtual std::string serializeData(const std::vector<UserTestInput> &data) const = 0;
		virtual std::string serializeReport(const nti::TestReport& report, const std::vector<UserTestInput> &generated,
			const std::vector<UserTestInput> &noised, const std::vector<std::string> &decoded) const = 0;

		virtual ~IChannelTesterSerialzer() = default;
	};

	class IChannelTesterParser
	{
	public:
		virtual std::vector<std::string> parseCoderOutput(const std::string &encode_decode) const = 0;
		virtual std::vector<UserTestInput> parseInput(const std::string &input) const = 0;

		virtual ~IChannelTesterParser() = default;
	};

	class NTIChannelTesterParser : public IChannelTesterParser
	{
	public:
		std::vector<std::string> parseCoderOutput(const std::string &encode_decode) const override;
		std::vector<UserTestInput> parseInput(const std::string& input) const override;
	};

	class IChannelTesterWriter
	{
	public:
		virtual void toFile(const std::string &path, const std::string &data) const noexcept(false) = 0;
		virtual std::string fromFile(const std::string &path) const noexcept(false) = 0;

		virtual ~IChannelTesterWriter() = default;

	};

	class NTIChannelTesterWriter : public IChannelTesterWriter
	{

	
	public:
		void toFile(const std::string& path, const std::string& data) const noexcept(false) override;
		std::string fromFile(const std::string& path) const noexcept(false) override;
		
		static std::unique_ptr<std::fstream> SafeOpen(const std::string &path, int open_mode = std::ios_base::out | std::ios_base::in) noexcept(false);
	};


	class NTIChannelTesterSerializer : public IChannelTesterSerialzer
	{
	public:
		std::string serializeData(const std::vector<UserTestInput> &data) const override;
		std::string serializeReport(const nti::TestReport& report, const std::vector<UserTestInput> &generated,
			const std::vector<UserTestInput> &noised, const std::vector<std::string> &decoded) const override;
	};


	
}
