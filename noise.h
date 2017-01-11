#pragma once
#include <vector>
#include <memory>
#include <random>

namespace nti {

	typedef unsigned char byte;
	typedef std::vector<byte> data;
	typedef std::unique_ptr<data> data_ref;

	class IChecksumProvider
	{
	public:
		virtual ~IChecksumProvider() = default;

		virtual data_ref get(const data *data) const = 0;

	};

	class INoise
	{
	public:
		virtual ~INoise() = default;
		virtual byte transform(byte chr) const = 0;
	};

	class INoiseProducer;

	struct NoisedData
	{
		std::string source_data;
		std::string noised_data;
		float noise_level;

		NoisedData(const std::string& source, const std::string& noised, float noise_level);
	};

	class INoiseProducer
	{
	public:
		struct NoiseProducerSettings
		{
			static const float ERROR_EPS;
			float noise_level;

			explicit NoiseProducerSettings(float noise_level) : noise_level(noise_level)
			{
				if (noise_level < 0 || noise_level > 1 || fabs(noise_level-0.5) < ERROR_EPS )
					throw std::runtime_error("Noise level must be lesser than 1");
			}
		};

		typedef NoiseProducerSettings Settings;


		virtual void set(const NoiseProducerSettings &settings) = 0;

		virtual std::unique_ptr<INoise> get() const = 0;
		virtual std::unique_ptr<INoise> get(const NoiseProducerSettings &settings) const = 0;

		virtual ~INoiseProducer() = default;
	};

	class NTINoiseProducer : public INoiseProducer
	{
		NoiseProducerSettings sets_;
	public:
		explicit NTINoiseProducer(const NoiseProducerSettings& sets);

		void set(const NoiseProducerSettings& settings) override;
		std::unique_ptr<INoise> get() const override;
		std::unique_ptr<INoise> get(const NoiseProducerSettings &settings) const override;

	};

	class NTINoise : public INoise
	{
		std::random_device rd_;
		mutable std::mt19937 rs_;
		mutable std::uniform_real_distribution<float> ds_;
		float probability_;
	public:
		NTINoise(float probability);

		byte transform(byte chr) const override;
	};

}
