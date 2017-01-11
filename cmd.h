#pragma once
#include <string>
#include <map>
#include <functional>
#include <set>
#include <locale>
#include <vector>
#include "utils.h"
#include "data.h"

namespace nti {
	class NTIChannelTesterParser;
}

namespace nti
{
	namespace cmd
	{
		class CommandProcessor
		{
		protected:
			std::set<std::string> valued_options_, flag_options_;

			std::map<std::string, std::string> current_options_;

			void opt_check_(const std::string &name) const noexcept(false);
			void v_opt_check_(const std::string &name) const noexcept(false);

			

		public:
			enum TokenType
			{
				OPTION_VALUE = 0,
				OPTION_NAME = 1,
			};

			template <typename DesiredType>
			static DesiredType Parse(const std::string &val) noexcept(false)
			{
				throw std::runtime_error("Unknown type to parse");
			}

			CommandProcessor(const std::set<std::string> &valued_options, const std::set<std::string> &flag_options);

			virtual ~CommandProcessor() = default;

			std::map<std::string, std::string> current_options() const;


			virtual void setOpt(const std::string &name) noexcept(false);
			virtual void setOpt(const std::string &name, const std::string& value) noexcept(false);

			template <typename DesiredType>
			DesiredType getOpt(const std::string &name) const noexcept(false)
			{
				v_opt_check_(name);
				auto val = current_options_.at(name);
				return Parse<DesiredType>(val);
			}
			
			
			bool getFlagVal(const std::string &name) const noexcept;
			bool isOptSet(const std::string &name) const noexcept;
			bool isOptFlag(const std::string &name) const noexcept(false);

			static std::pair<TokenType, std::string> ProcessToken(const std::string& token) noexcept(false);
		};

		// well, no if-constexpr and return/auto in MSVC, touche
		/// <fast_patch>
		template <>
		inline std::string CommandProcessor::Parse<std::basic_string<char>>(const std::string& val)
		{
			return val;
		}

		template <>
		inline int CommandProcessor::Parse<int>(const std::string& val) noexcept(false)
		{
			return std::stoi(val);
		}

		template <>
		inline float CommandProcessor::Parse<float>(const std::string& val) noexcept(false)
		{
			return std::stof(val);
		}

		template <>
		inline bool CommandProcessor::Parse<bool>(const std::string& val) noexcept(false)
		{
			auto icompare = [](const unsigned char a, const unsigned char b) -> bool
			{
				return tolower(a) == tolower(b);
			};
			if (std::equal(val.begin(), val.end(), std::begin("false")))
				return false;
			if (std::equal(val.begin(), val.end(), std::begin("true")))
				return true;
			throw std::runtime_error("Conversion to bool failed");
		}

		template <>
		inline std::vector<float> CommandProcessor::Parse<std::vector<float>>(const std::string& val) noexcept(false)
		{
			auto vs = split(val, ",");
			std::vector<float> ret;
			for (auto& v : vs)
				ret.push_back(Parse<float>(v));
			return ret;
		}

		template <>
		inline std::vector<int> CommandProcessor::Parse<std::vector<int>>(const std::string& val) noexcept(false)
		{
			auto vs = split(val, ",");
			std::vector<int> ret;
			for (auto& v : vs)
				ret.push_back(Parse<int>(v));
			return ret;
		}

		// </fast_patch>
		


		class NTICommandLine : protected CommandProcessor
		{
			private:
			enum class Mode { SEND, CHECK, GENERATE, UNKNOWN };
			Mode mode_ = { Mode::UNKNOWN };
			NTIChannelTesterSerializer serializer_;
			NTIChannelTesterWriter writer_;
			NTIChannelTesterParser parser_;
			mutable NTIChannelTester tester_;


		protected:

			void parseArgs_(const char** argv, int argc) noexcept(false);
			
			void doAddNoise_() const;
			void doCheckDecode_() const;
			void doGenerateSource_() const;

			public:
			static const std::string HELP_TEXT;

			struct Flags {
				static const std::string MODE_CHECK_DECODE, MODE_SEND_DATA, MODE_GENERATE_DATA;
			};
			struct Values
			{
				static const std::string
					PARAM_SOURCE_MAXSIZE,
					PARAM_NUM_SOURCES,
					PARAM_NOISE_LEVELS,

					INPUT_ENCODED_DATA,
					INPUT_DECODED_DATA,
					INOUT_NOISED_DATA,
					OUTPUT_REPORT,
					INOUT_SOURCE_DATA;
						
			};

			template <typename Arg>
			using ValidatorFn = std::function<std::pair<bool, std::string>(const NTICommandLine &, const std::string &, Arg)> ;

			// { <PRESENCE VALIDATION/NULLPTR (HAS/OR NOT) -> { VALID/INVALID, ERR_MSG } >, <VALIDATING_FUNCTION/NULLPTR -> { VALID/INVALID, ERR_MSG }>  }
			typedef std::pair<ValidatorFn<bool>, ValidatorFn<const std::string &>> ValidatorEntry;

			static const std::set<std::string> VALUED_OPTS, FLAG_OPTS;

			static const std::map<std::string, ValidatorEntry> VALIDATORS;

			NTICommandLine();


			void start(const char** argv, size_t argc);


			using CommandProcessor::isOptFlag;
			using CommandProcessor::getFlagVal;
			using CommandProcessor::isOptSet;
			using CommandProcessor::getOpt;
			using CommandProcessor::setOpt;

			using CommandProcessor::Parse;
		};

	}

}
