#include "tester.h"
#include <algorithm>
#include "utils.h"
#include "tests/catch.hpp"

namespace nti
{
	std::string TestReport::reasonToString() const
	{
		// ReSharper disable once CppDefaultCaseNotHandledInSwitchStatement
		// ReSharper disable once CppIncompleteSwitchStatement
		switch (this->fail_reason)
		{
		case FailReason::ENCODE_SPEED_LOW:
			return "Encoding speed of '" + std::to_string(this->mean_encode_speed) + "' is too low. Consider using a different algorithms or less encoding characters per input character.";
		case FailReason::DECODE_FAILURE_RATE_HIGH:
			return "Decode failure of '" + std::to_string(1 - this->mean_decode_success_rate) + "' rate is too high. Check your algorithm for errors and make sure you are adapting to your noise level.";
		case FailReason::DECODE_FAILURE_MANY_ERRORS:
			return "Decoding algorithm failed more than maximum number of times. Check your algorithm for errors. Check your algorithm for errors and make sure you are adapting to your noise level.";
		}
		return "";
	}

	const std::string
		IChannelTester::MODE_DECODE_STR = "decode",
		IChannelTester::MODE_ENCODE_STR = "encode";
	const size_t NTIChannelTester::THRESHOLD_FAILS = 2;
	const float 
		NTIChannelTester::THRESHOLD_CALC_SPEED = 0.2f, 
		NTIChannelTester::THRESHOLD_SUCCESS_RATE = 0.8f;

	std::vector<UserTestInput> NTIChannelTester::generateNoisedInputs(const std::vector<UserTestInput>& inputs, const std::vector<std::string> &encoded) const
	{
		std::vector<UserTestInput> ret;

		for (size_t i = 0, s = inputs.size(); i<s; ++i)
		{
			std::string new_str = encoded[i];

			NTINoiseProducer noise_producer(INoiseProducer::NoiseProducerSettings(inputs[i].noise_level));
			auto noise = noise_producer.get();
			std::transform(new_str.begin(), new_str.end(), new_str.begin(), [&noise](char val)
			{
				auto nval = char(noise->transform(val));
				while (nval == '\r' || nval == '\n')
					nval = char(noise->transform(val));
				return nval;
			});
			ret.emplace_back(UserTestInput{ MODE_DECODE_STR, inputs[i].noise_level, new_str });
		}
		return ret;
	}

	const NoisedData* NTIChannelTester::setAlgoEncodeResponse(const std::string& source, const std::string& response, float noise_level)
	{
		noised_responses_[source] = std::make_unique<NoisedData>(source, response, noise_level);
		// update speed
		auto ns = noised_responses_.size();
		auto this_speed = static_cast<float>(source.size()) / response.size() / ns;
		if (noised_responses_.size() == 1)
			calc_speed_ = this_speed;
		else
			calc_speed_ = calc_speed_ * (ns-1) / ns + this_speed;
		return noised_responses_[source].get();

	}

	void NTIChannelTester::setAlgoDecodeResponse(const NoisedData* noised_data, const std::string& response)
	{
		decode_responses_[noised_data] = response;
		// update failed tests if failed or remove from failed if updated
		if (response == noised_data->source_data)
		{
			auto it = failed_tests_.find(noised_data);
			if (it != failed_tests_.cend()) failed_tests_.erase(it);
		}
		else
			this->failed_tests_.emplace(noised_data);
		calc_success_rate_ = this->num_success_tests() / static_cast<float>(this->num_finished_tests());
	}

	float NTIChannelTester::success_rate() const
	{
		return calc_success_rate_;
	}

	float NTIChannelTester::speed() const
	{
		return calc_speed_;
	}

	size_t NTIChannelTester::num_tests() const
	{
		return this->noised_responses_.size();
	}

	size_t NTIChannelTester::num_finished_tests() const
	{
		return this->decode_responses_.size();
	}

	size_t NTIChannelTester::num_success_tests() const
	{
		return num_finished_tests() - num_failed_tests();
	}

	size_t NTIChannelTester::num_failed_tests() const
	{
		return failed_tests_.size();
	}

	float NTIChannelTester::find_least_successfull_rate_() const
	{
		std::map<float, size_t> noise_to_fails;
		for (auto fail : failed_tests_)
			noise_to_fails[fail->noise_level]++;

		using pt = decltype(noise_to_fails)::value_type;
		auto m = std::max_element(noise_to_fails.begin(), noise_to_fails.end(), [](const pt &a, const pt &b) { return a.second < b.second; });
		return m == noise_to_fails.end() ? -1 : m->first;
	}

	std::pair<bool, TestReport::FailReason> NTIChannelTester::verify_passed_() const
	{
		auto reason = TestReport::FailReason::NONE;
		bool has_passed = calc_speed_ >= THRESHOLD_CALC_SPEED && calc_success_rate_ >= THRESHOLD_SUCCESS_RATE;
		auto has_failed = [](const std::string &source, const std::string &decoded)
		{
			size_t n = 0;
			for (size_t i = 0, ss = source.size(), ds = source.size(); i < ss && i < ds; ++i)
				if (source[i] != decoded[i])
					if (++n > THRESHOLD_FAILS)
						return true;
			return false;
		};

		if (has_passed)
			// check last condition - no errors more than
			for (auto fail : failed_tests_)
			{
				const auto &dr = decode_responses_.at(fail);
				if (has_failed(fail->source_data, dr))
				{
					has_passed = false;
					reason = TestReport::FailReason::DECODE_FAILURE_MANY_ERRORS;
					break;
				}
			}
		else
		{
			// check fail reason between speed or anything
			if (calc_speed_ < THRESHOLD_CALC_SPEED)
				reason = TestReport::FailReason::ENCODE_SPEED_LOW;
			else
				reason = TestReport::FailReason::DECODE_FAILURE_RATE_HIGH;
		}
		return std::make_pair<bool, TestReport::FailReason>(std::move(has_passed), std::move(reason));
	}

	TestReport NTIChannelTester::generateReport(std::vector<std::string> decoded_for_ordering) const
	{
		TestReport ret;
		ret.num_success = num_success_tests();
		ret.mean_encode_speed = calc_speed_;
		ret.mean_decode_success_rate = calc_success_rate_;
		auto r = verify_passed_(); 
		ret.has_passed = r.first; ret.fail_reason = r.second;
		ret.least_successful_error_rate = find_least_successfull_rate_();
		for (auto fail : failed_tests_)
		{
			const auto &dr = decode_responses_.at(fail);
			auto pos = std::find(decoded_for_ordering.begin(), decoded_for_ordering.end(), dr);
			if (pos == decoded_for_ordering.end())
				throw std::runtime_error("[generateReport] Couldn't find decoded response!");

			ret.failed_tests.push_back(pos - decoded_for_ordering.begin());
		}

		return ret;
	}


	std::vector<std::pair<NoisedData, std::string>> NTIChannelTester::failed() const
	{
		std::vector<std::pair<NoisedData, std::string>> ret;
		for (auto fail : failed_tests_)
		{
			const auto &dr = decode_responses_.at(fail);
			ret.push_back(std::pair<NoisedData, std::string>(NoisedData(*fail), dr));
		}
		return ret;
	}

	std::vector<UserTestInput> NTIChannelTester::generateInputs(size_t num_inputs, float noise_level, size_t max_length) const
	{
		std::vector<UserTestInput> ret;
		std::random_device rd;
		std::mt19937 rng(rd());
		std::uniform_int_distribution<size_t> uid(1, max_length);

		for (size_t i = 0; i<num_inputs; ++i)
		{
			size_t len = uid(rng);
			ret.emplace_back(UserTestInput{ MODE_ENCODE_STR, noise_level, generate_alnum_str(len) });
		}
		return ret;
	}


}
