#include "cmd.h"
#include "noise.h"
#include <list>
#include <array>
#include <iostream>
#include <random>
#include <sstream>
#include <climits>
#include <cstring>

namespace nti
{
	namespace cmd
	{

		void CommandProcessor::opt_check_(const std::string& name) const noexcept(false)
		{
			if (flag_options_.find(name) == flag_options_.cend())
				throw std::runtime_error("This flag option does not exist");
		}

		void CommandProcessor::v_opt_check_(const std::string& name) const noexcept(false)
		{
			if (valued_options_.find(name) == valued_options_.cend())
				throw std::runtime_error("This valued option does not exist");
		}

		CommandProcessor::CommandProcessor(const std::set<std::string> &valued_options, const std::set<std::string> &flag_options): valued_options_(valued_options), flag_options_(flag_options)
		{
			// validate
			for (auto &v : valued_options_)
				if (flag_options_.find(v) != flag_options_.cend())
					throw std::runtime_error("Dublicated option");
			for (auto &v : flag_options_)
				if (valued_options_.find(v) != valued_options_.cend())
					throw std::runtime_error("Dublicated option");
		}

		std::map<std::string, std::string> CommandProcessor::current_options() const
		{
			return this->current_options_;
		}

		void CommandProcessor::setOpt(const std::string& name) noexcept(false)
		{
			opt_check_(name);
			current_options_[name] = "";
		}

		void CommandProcessor::setOpt(const std::string& name, const std::string& value) noexcept(false)
		{
			v_opt_check_(name);
			current_options_[name] = value;
		}

		bool CommandProcessor::getFlagVal(const std::string& name) const noexcept
		{
			opt_check_(name);
			return isOptSet(name);
		}

		bool CommandProcessor::isOptSet(const std::string& name) const noexcept
		{
			return current_options_.find(name) != current_options_.cend();
		}

		bool CommandProcessor::isOptFlag(const std::string& name) const noexcept(false)
		{
			if (flag_options_.find(name) != flag_options_.cend())
				return true;
			if (valued_options_.find(name) == valued_options_.cend())
				throw std::runtime_error("Option '-"+name+"' does not exist."); // verify existence
			return false;
		}

		

		std::pair<CommandProcessor::TokenType, std::string> CommandProcessor::ProcessToken(const std::string &token) noexcept(false)
		{
			constexpr size_t stepcheck_limit = 1;
			size_t step = 0;
			auto fsm_step = [&step, &stepcheck_limit, &token]() -> bool
				{
					if (token.size() == step)
						throw std::runtime_error("Token '" + token + "' has invalid size for this token type.");
					if (token[step] == '-')
					{
						if (step == stepcheck_limit)
							throw std::runtime_error("Invalid token '" + token + "' - too much '-' chars");
						else
							return true;
					}
					else
						return false;
				};
			// run finite state machine
			while (fsm_step())
				++step;

			auto type = TokenType(step);
			size_t bg = step, ed = 0;
			// post fsm filtering
			if (type == TokenType::OPTION_VALUE)
			{
				// filter out \" chars
				if(token[0] == '\"')
				{
					bg = 1;
					size_t ps = token.find_last_of('\"');
					if (ps == token.npos)
						throw std::runtime_error("Unclosed \" char in token " + token);
					ed = token.size() - ps;
				}
			}
			auto val = std::string(token.begin() + bg, token.end() - ed);
			return std::make_pair<TokenType, std::string>(std::move(type), std::move(val));
		}

		void NTICommandLine::parseArgs_(const char** argv, int argc) noexcept(false)
		{
			struct
			{
				std::set<TokenType> expected_types;
				bool expect_any = {false};
				bool expect_next = {false};
				std::string previous_token;
			} state;
			state.expected_types = {OPTION_NAME};

			std::string last_option = "";
			for (int i = 0; i < argc; ++i)
			{
				auto processed = this->ProcessToken(argv[i]);
				if (!state.expect_any && state.expected_types.find(processed.first) == state.expected_types.cend())
					throw std::runtime_error("Unexpected token: '" + std::string(argv[i]) + "'");
				// state transition and processing
				switch (processed.first)
				{
				case OPTION_NAME:
					{
						bool is_flag = this->isOptFlag(processed.second);
						if (is_flag)
							this->setOpt(processed.second);
						else
						{
							state.expected_types = {OPTION_VALUE};
							last_option = processed.second;
						}
						state.expect_next = !is_flag;
						state.expect_any = is_flag;
					}

					break;
				case OPTION_VALUE:
					state.expect_any = false;
					state.expect_next = false;
					state.expected_types = {OPTION_NAME};
					this->setOpt(last_option, processed.second);
					break;
				}
			}
			// post-process check
			if (state.expect_next)
				throw std::runtime_error("Unexpected end of command-line arguments");

			// validate all options
			for (auto& name_validator : VALIDATORS)
			{
				auto opt = this->current_options_.find(name_validator.first);
				bool has_opt = opt != this->current_options_.cend();
				if (name_validator.second.first != nullptr)
				{
					auto valid = name_validator.second.first(*this, name_validator.first, has_opt);
					if (!valid.first)
						throw std::runtime_error("Validation failed: '" + valid.second + "'");
				}
				if (has_opt && name_validator.second.second != nullptr)
				{
					auto valid = name_validator.second.second(*this, name_validator.first, opt->second);
					if (!valid.first)
						throw std::runtime_error("Validation failed: '" + valid.second + "'");
				}
			}
		}

		void NTICommandLine::doAddNoise_() const
		{
			// Noises encoded data - noise, source data
			auto encoded_data = writer_.fromFile(getOpt<std::string>(Values::INPUT_ENCODED_DATA));
			auto input_data = writer_.fromFile(getOpt<std::string>(Values::INOUT_SOURCE_DATA));

			auto input = parser_.parseInput(input_data);
			auto encoded = parser_.parseCoderOutput(encoded_data);
			if (input.size() != encoded.size())
				throw std::runtime_error("[Send()] Data mismatch: number of lines in encoded data (" + std::to_string(encoded.size()) + ") not equal to lines in source data ("+std::to_string(input.size())+")");

			auto out = tester_.generateNoisedInputs(input, encoded);
			auto noised_serialized = serializer_.serializeData(out);
			writer_.toFile(getOpt<std::string>(Values::INOUT_NOISED_DATA), noised_serialized);

			std::cout << out.size() << " noised inputs have successfully generated!";

		}

		void NTICommandLine::doCheckDecode_() const
		{
			// Noises encoded data - noise, source data
			auto noised_data = writer_.fromFile(getOpt<std::string>(Values::INOUT_NOISED_DATA));
			auto source_data = writer_.fromFile(getOpt<std::string>(Values::INOUT_SOURCE_DATA));
			auto decoded_data = writer_.fromFile(getOpt<std::string>(Values::INPUT_DECODED_DATA));
			auto encoded_data = writer_.fromFile(getOpt<std::string>(Values::INPUT_ENCODED_DATA));

			auto source = parser_.parseInput(source_data);
			auto decoded = parser_.parseCoderOutput(decoded_data);
			auto noised = parser_.parseInput(noised_data);
			auto encoded = parser_.parseCoderOutput(encoded_data);
			
			std::vector<UserTestInput> out;

			if (source.size() != noised.size() || source.size() != decoded.size() || source.size() != encoded.size() )
				throw std::runtime_error("[Decode()] Data mismatch: numbers of lines in files are in consistent. \
										  Decoded: "+ std::to_string(decoded.size()) + 
											", noised: "+ std::to_string(noised.size())+
											", source: "+std::to_string(source.size())+ 
											", encoded: "+std::to_string(encoded.size()));

			for (size_t i = 0, s = source.size(); i<s; ++i)
			{
				auto token = tester_.setAlgoEncodeResponse(source[i].input, encoded[i], {});
				tester_.setAlgoDecodeResponse(token, decoded[i]);
				
			}
			auto rep = tester_.generateReport(decoded);

			writer_.toFile(getOpt<std::string>(Values::OUTPUT_REPORT), serializer_.serializeReport(rep, source, noised, decoded));
			std::cout << "Test report has successfully generated!";

		}

		
		

		void NTICommandLine::doGenerateSource_() const
		{
			size_t number_tests = this->getOpt<int>(Values::PARAM_NUM_SOURCES);
			size_t max_length = this->getOpt<int>(Values::PARAM_SOURCE_MAXSIZE);
			auto noise_levels = this->getOpt<std::vector<float>>(Values::PARAM_NOISE_LEVELS);
			std::vector<UserTestInput> vals;
			
			

			for (auto nlevel : noise_levels)
			{
				auto t = tester_.generateInputs(number_tests, nlevel, max_length);
				vals.insert(vals.end(), t.begin(), t.end());
			}
				

			auto serialized = this->serializer_.serializeData(vals);
			this->writer_.toFile(this->getOpt<std::string>(Values::INOUT_SOURCE_DATA), serialized);

			std::cout << vals.size() << " source inputs have successfully generated!";
		}

		NTICommandLine::NTICommandLine(): CommandProcessor(VALUED_OPTS, FLAG_OPTS)
		{
		}

		void NTICommandLine::start(const char** argv, size_t argc)
		{
			// check if help needed
			if (argc == 0)
			{
				std::cout << HELP_TEXT; return;
			}
			for (auto a : std::list<const char*>(argv, argv+argc))
			{
				if (std::strcmp(a, "-help") == 0 || std::strcmp(a, "-h") == 0)
				{
					std::cout << HELP_TEXT; return;
				}
			}
			// anyway, proceed as normal
			parseArgs_(argv, argc);
			mode_ = getFlagVal(Flags::MODE_SEND_DATA) ? Mode::SEND : getFlagVal(Flags::MODE_GENERATE_DATA) ? Mode::GENERATE : Mode::CHECK; // get mode
			switch (mode_)
			{
			case Mode::SEND:
				doAddNoise_();
				break;
			case Mode::CHECK:
				doCheckDecode_();
				break;
			case Mode::GENERATE:
				doGenerateSource_();
				break;
			case Mode::UNKNOWN:
				throw std::runtime_error("Unknown mode to run");
				break;
			}
		}

		////////

		const std::string NTICommandLine::HELP_TEXT = 
"Usage <mode> [parameters...]\r\n\
Modes available:\r\n\
  -g - generates 'num_sources'*|'noise_levels'| datasets for an encoding algorithm in \"encode\" mode.\r\n\
\t Parameters are: -max_source_size <int> -noise_levels <array[float][0..1]> -num_sources <int> -io_sources <str>\r\n\
\t Program generates a new dataset for \"encoding\" mode. For each noise level num_sources of tests will be generated.\r\n\
  -s - generates dataset for an encoding algorithm in \"decode\" mode.\r\n\
\t Parameters are: -io_noised <str> -in_encoded <str> -io_sources <str> \r\n\
\t Program generates a new dataset for \"decoding\" mode using coder's output and source data.\r\n\
  -d - evaluates the accuracy and other parameters of both encoding and decoding algorithms.\r\n\
\t Parameters are: -in_encoded <str> -in_decoded <str> -io_noised <str> -io_source <str> -out_report <str> \r\n\
\t Program performs accuracy test, evaluates speed and other algorithm's parameters and writes them into 'out_report' file. \r\n\
\r\n\
Note: data type '<array[type]>' is a coma (',') delimeted list of values of given type. \
E.g.: <array[float[0..1]]> - 0,0.2,0.4,0.8,0.9,1\r\n\
"
		;


		const std::string
			NTICommandLine::Values::PARAM_NOISE_LEVELS = "noise_levels",
			NTICommandLine::Values::PARAM_SOURCE_MAXSIZE = "max_source_size",
			NTICommandLine::Values::PARAM_NUM_SOURCES = "num_sources",
			//NTICommandLine::Values::PARAM_DIFFICULTIES = "difficulties",

			NTICommandLine::Values::INPUT_ENCODED_DATA = "in_encoded",
			NTICommandLine::Values::INPUT_DECODED_DATA = "in_decoded",
			NTICommandLine::Values::INOUT_NOISED_DATA = "io_noised",
			NTICommandLine::Values::OUTPUT_REPORT = "out_report",
			NTICommandLine::Values::INOUT_SOURCE_DATA = "io_source";

		const std::string
			NTICommandLine::Flags::MODE_CHECK_DECODE = "d",
			NTICommandLine::Flags::MODE_SEND_DATA = "s",
			NTICommandLine::Flags::MODE_GENERATE_DATA = "g";
			

		const std::set<std::string>
			NTICommandLine::VALUED_OPTS = {
				Values::PARAM_NOISE_LEVELS ,
				//Values::PARAM_DIFFICULTIES ,
				Values::PARAM_SOURCE_MAXSIZE,
				Values::PARAM_NUM_SOURCES,
				
				Values::INPUT_ENCODED_DATA ,
				Values::INOUT_NOISED_DATA  ,
				Values::INPUT_DECODED_DATA ,
				Values::OUTPUT_REPORT	   ,
				Values::INOUT_SOURCE_DATA
		}, 
		NTICommandLine::FLAG_OPTS = {
			Flags::MODE_CHECK_DECODE,
			Flags::MODE_SEND_DATA,
			Flags::MODE_GENERATE_DATA
		};

		template<const std::string &...modes>
        std::pair<bool, std::string> __check_must_be_in(const NTICommandLine &cmd, const std::string &name, bool has)
		{
			std::string err_msg = "'" + name + "' must be set";
			bool has_mode = false;
		    
			for (const auto &m : {modes...})
			{
				has_mode = cmd.getFlagVal(m);
				if (has_mode) break;
			}
			return std::make_pair<bool, std::string>(!has_mode || has_mode && has , std::move(err_msg));
		};
		
		auto __check_mode(const NTICommandLine &cmd, const std::string &name, bool has)
		{
			static const auto& err_msg = "Either one option must be selected: -" + NTICommandLine::Flags::MODE_SEND_DATA +
										 ", -" + NTICommandLine::Flags::MODE_GENERATE_DATA + 
										 " or -" + NTICommandLine::Flags::MODE_CHECK_DECODE;
			return std::make_pair<bool, std::string>(
				cmd.isOptSet(NTICommandLine::Flags::MODE_SEND_DATA) + 
				cmd.isOptSet(NTICommandLine::Flags::MODE_CHECK_DECODE) + 
				cmd.isOptSet(NTICommandLine::Flags::MODE_GENERATE_DATA) == 1, 
				std::string(err_msg));
		};
		auto __check_noise_dif(const NTICommandLine &cmd, const std::string &name, bool has)
		{
			static const auto& err_msg = "Either one option must be selected: -" + /*NTICommandLine::Values::PARAM_DIFFICULTIES + " or -" + */NTICommandLine::Values::PARAM_NOISE_LEVELS;
			auto check_result = 0;
			auto mode_ok = __check_must_be_in<NTICommandLine::Flags::MODE_GENERATE_DATA>(cmd, name, has);
			if (!mode_ok.first)
				return mode_ok;

			return std::make_pair<bool, std::string>(
				/*!cmd.isOptSet(NTICommandLine::Values::PARAM_DIFFICULTIES)*/ true, 
				std::string(err_msg));
		};

		template <int min, int max>
		std::pair<bool, std::string> __value_check_int_range(const NTICommandLine &cmd, const std::string &name, const std::string &val)
		{
			auto parsed = CommandProcessor::Parse<int>(val);
			return std::make_pair<bool, std::string>(parsed >= min && parsed <= max, "Value of '-" + name + "' must lie in intervals [" + std::to_string(min) + ", " + std::to_string(max) + "]");
		}

		const std::map<std::string, NTICommandLine::ValidatorEntry> NTICommandLine::VALIDATORS = {
				{ Flags::MODE_CHECK_DECODE, { __check_mode, nullptr } },

				{ Flags::MODE_SEND_DATA, { __check_mode, nullptr } },
                { Flags::MODE_GENERATE_DATA, { __check_mode, nullptr } },
                { Values::PARAM_NOISE_LEVELS,
                {	__check_noise_dif,
                    [](const NTICommandLine &cmd, const std::string &name, const std::string &val)
                    {
                        auto parsed = Parse<std::vector<float>>(val);
                        bool success = true;
                        for (auto p : parsed)
                            if (p < 0 || p > 1 || fabs(p-0.5f) < INoiseProducer::NoiseProducerSettings::ERROR_EPS)
                            {
                                success = false;
                                break;
                            }
                        return std::make_pair<bool, std::string>(std::move(success), "Noise must lie in [0, 1] interval and must not be close to a 0.5 value");
                    }
                }
                },
                 { Values::INOUT_NOISED_DATA,  {  __check_must_be_in <Flags::MODE_CHECK_DECODE, Flags::MODE_SEND_DATA> , nullptr } },
                 { Values::INPUT_ENCODED_DATA, { __check_must_be_in<Flags::MODE_SEND_DATA, Flags::MODE_CHECK_DECODE>, nullptr } },
                 { Values::INPUT_DECODED_DATA, { __check_must_be_in<Flags::MODE_CHECK_DECODE>, nullptr } },
                 { Values::INOUT_SOURCE_DATA, { __check_must_be_in<Flags::MODE_GENERATE_DATA, Flags::MODE_SEND_DATA, Flags::MODE_CHECK_DECODE>, nullptr } },
                 { Values::OUTPUT_REPORT,	  { __check_must_be_in<Flags::MODE_CHECK_DECODE>, nullptr } },
                 { Values::PARAM_SOURCE_MAXSIZE,{ __check_must_be_in<Flags::MODE_GENERATE_DATA>, __value_check_int_range<1, 4'000'000>  } },
                 { Values::PARAM_NUM_SOURCES,  { __check_must_be_in<Flags::MODE_GENERATE_DATA>, __value_check_int_range<1, INT_MAX> } },/**/
		};
		// END OF VALIDATORS
		
	}

}
