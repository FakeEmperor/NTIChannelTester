#pragma once
#include "noise.h"
#include <string>
#include <map>
#include <set>

namespace nti
{

	struct UserTestInput
	{
		std::string mode;
		float noise_level; // it is either computed or given by user
		std::string input;
	};

	struct TestReport
	{
		enum class FailReason { ENCODE_SPEED_LOW, DECODE_FAILURE_RATE_HIGH, NONE,DECODE_FAILURE_MANY_ERRORS};

		// directly from the tester
		FailReason fail_reason = { FailReason::NONE };
		size_t num_success; 
		bool has_passed;
		std::vector<size_t> failed_tests;
		// mean values
		float mean_decode_success_rate;
		float mean_encode_speed;
		// integrated over tests
		float least_successful_error_rate; // -1 - N/A


		std::string reasonToString() const;
	};


	class IChannelTester
	{
	public:
		static const std::string MODE_ENCODE_STR, MODE_DECODE_STR;

		virtual std::vector<UserTestInput> generateInputs(size_t num_tests, float noise_level, size_t max_length) const = 0;
		virtual std::vector<UserTestInput> generateNoisedInputs(const std::vector<UserTestInput>& inputs, const std::vector<std::string>& encoded) const = 0;

		virtual const NoisedData * setAlgoEncodeResponse(const std::string& source, const std::string& response, float noise_level) = 0;
		virtual void setAlgoDecodeResponse(const NoisedData *noised_data, const std::string &response) = 0;

		virtual float success_rate() const = 0;
		virtual float speed() const = 0;
		virtual std::vector<std::pair<NoisedData, std::string>> failed() const = 0;

		virtual TestReport generateReport(std::vector<std::string> decoded_for_ordering) const = 0;

		virtual ~IChannelTester() = default;
	};



	class NTIChannelTester : public IChannelTester
	{
		std::map<const NoisedData*, std::string> decode_responses_;
		std::map<std::string, std::unique_ptr<NoisedData>> noised_responses_;

		std::set<const NoisedData*> failed_tests_;

		float calc_speed_ = { 0 };
		float calc_success_rate_ = { 0 };

		std::pair<bool, TestReport::FailReason>  verify_passed_() const;
	public:
		static const size_t THRESHOLD_FAILS;
		static const float THRESHOLD_CALC_SPEED, THRESHOLD_SUCCESS_RATE;


		std::vector<UserTestInput> generateInputs(size_t num_inputs, float noise_level, size_t max_length) const override;
		std::vector<UserTestInput> generateNoisedInputs(const std::vector<UserTestInput>& inputs, const std::vector<std::string>& encoded) const override;

		const NoisedData * setAlgoEncodeResponse(const std::string& source, const std::string& response, float noise_level) override;
		void setAlgoDecodeResponse(const NoisedData *noised_data, const std::string &response) override;


		float success_rate() const override;
		float speed() const override;

		size_t num_tests() const;
		size_t num_finished_tests() const;
		size_t num_success_tests() const;
		size_t num_failed_tests() const;
		float find_least_successfull_rate_() const;

		TestReport generateReport(std::vector<std::string> decoded_for_ordering) const;
		std::vector<std::pair<NoisedData, std::string>> failed() const override;

		~NTIChannelTester() override = default;
	};

}


