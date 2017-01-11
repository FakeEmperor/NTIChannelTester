#include "data.h"
#include <chrono>
#include "utils.h"
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <cstring>
#include <cerrno>

namespace nti
{
	
	std::string NTIChannelTesterSerializer::serializeData(const std::vector<UserTestInput>& data) const
	{
		std::stringstream ss;
		for (const auto &d : data)
			ss << d.mode << " " << d.noise_level << " " << d.input << "\r\n";
		return ss.str();
	}

	std::vector<std::string> NTIChannelTesterParser::parseCoderOutput(const std::string& encode_decode) const
	{
		return split(encode_decode, "\r\n");
	}

	std::vector<UserTestInput> NTIChannelTesterParser::parseInput(const std::string& input) const
	{
		auto inputs = split(input, "\r\n");
		std::vector<UserTestInput> res;
		for (const auto &in : inputs)
		{
			std::istringstream iss(in);
			UserTestInput ui;
			iss >> ui.mode >> ui.noise_level >> std::ws;
			size_t pos = iss.tellg();
			ui.input = in.substr(pos);
			res.push_back(ui);
		}
		return res;
	}


	void NTIChannelTesterWriter::toFile(const std::string& path, const std::string& data) const noexcept(false)
	{
		auto file = SafeOpen(path, std::fstream::out | std::fstream::binary);
		*file << data;
		if (file->fail())
			throw std::runtime_error(std::string("Failed to write to '" + path + "': ") + strerror(errno));
	}

	std::string NTIChannelTesterWriter::fromFile(const std::string& path) const noexcept(false)
	{
		auto file = SafeOpen(path, std::fstream::in | std::ios_base::binary);

		std::string data((std::istreambuf_iterator<char>(*file)),
			std::istreambuf_iterator<char>());
		if (file->fail())
			throw std::runtime_error("Failed to read from '" + path + "': " + strerror(errno));
		return data;
	}

	std::unique_ptr<std::fstream> NTIChannelTesterWriter::SafeOpen(const std::string& path, int open_mode) noexcept(false)
	{
		auto file = std::make_unique<std::fstream>(path, (std::ios_base::openmode)(open_mode));
		if (file->fail())
			throw std::runtime_error("Failed to open file '" + path + "': " + strerror(errno));
		return file;
	}


	std::string NTIChannelTesterSerializer::serializeReport(
		const nti::TestReport& report, const std::vector<UserTestInput> &generated,
		const std::vector<UserTestInput> &noised, const std::vector<std::string> &decoded) const
	{
		std::stringstream ss;
		auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		ss << "Report generated at: " << std::ctime(&t) << std::endl
			<< "[ GENERAL ] " << std::endl
			<< "\tOverall (is passed?): " << std::boolalpha << report.has_passed << std::endl;
		if (!report.has_passed)
			ss << "\tReason to failure: " << report.reasonToString() << std::endl;
		ss
			<< "\tTests total: " << report.num_success + report.failed_tests.size() << std::endl
			<< "\tTests passed: " << report.num_success << std::endl
			<< "\tTests failed: " << report.failed_tests.size() << std::endl
			<< "[ STATISTICS ]" << std::endl
			<< "\tOverall decode success rate: " << report.mean_decode_success_rate << std::endl
			<< "\tOverall encode speed rate: " << report.mean_encode_speed << std::endl
			<< "\tError rate of most errors: " << report.least_successful_error_rate << std::endl;

		if (report.failed_tests.size() > 0)
		{
			ss << "[ FOR DEBUG ]" << std::endl
				<< "\tFailed tests: " << std::endl
				<< "(Below are the tests which decoder failed to pass)" << std::endl;

			auto get_err_positions = [](const UserTestInput &gen, const std::string &dec, size_t glue_threshold = 0) {
				std::vector<std::pair<size_t, size_t>> vec;
				for (size_t i = 0, sg = gen.input.size(), sd = dec.size(), glue = 0, tstart = -1, tend = -1; i<sg && i<sd; ++i, ++glue)
				{
					if (tstart != -1 && tend != -1 && glue > glue_threshold)
					{
						vec.push_back(std::make_pair(tstart, tend));
						tstart = tend = -1;
					}
					if (dec[i] != gen.input[i])
					{
						glue = 0; // reset glue
						if (tstart != -1)
							tend = i;
						else
							tstart = i;
					}
					else
						tend = i;
				}
				return vec;
			};
			auto print_transform = [&ss](const std::string str, const std::string &delim, const std::vector<std::pair<size_t, size_t>> &err_positions)
			{
				size_t prev = 0;
				for (auto &ep : err_positions)
				{
					ss << str.substr(prev, ep.first - prev) << delim << str.substr(ep.first, ep.second + 1 - ep.first) << delim;
					prev = ep.second;
				}
				if (prev < str.size()) // something left? print it
					ss << str.substr(prev);
			};
			for (auto i : report.failed_tests)
			{
				ss << "TEST #" << i + 1 << ". Noise level:" << generated[i].noise_level << std::endl;
				auto errs = get_err_positions(generated[i], decoded[i], 3);
				ss << "GENERATED: "; print_transform(generated[i].input, "   ", errs); ss << std::endl;
				ss << "NOISED:    "; print_transform(noised[i].input, "   ", errs); ss << std::endl;
				ss << "DECODED:   "; print_transform(decoded[i], "   ", errs); ss << std::endl;

				ss << std::endl;
			}
		}

		ss
			<< std::endl << std::endl;

		return ss.str();
	}
}