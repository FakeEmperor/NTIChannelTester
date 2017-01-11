#include <memory>
#include <climits>
#include "noise.h"



namespace nti
{
	const float INoiseProducer::NoiseProducerSettings::ERROR_EPS = 0.1;


	NoisedData::NoisedData(const std::string& source, const std::string& noised, float noise_level): source_data(source), noised_data(noised), noise_level(noise_level)
	{

	}

	NTINoiseProducer::NTINoiseProducer(const NoiseProducerSettings& sets): sets_(sets)
	{

	}

	void NTINoiseProducer::set(const NoiseProducerSettings& settings)
	{
		sets_ = settings;
	}

	std::unique_ptr<INoise> NTINoiseProducer::get(const NoiseProducerSettings& settings) const
	{
		return std::make_unique<NTINoise>(settings.noise_level);
	}

	std::unique_ptr<INoise> NTINoiseProducer::get() const
	{
		return std::make_unique<NTINoise>(sets_.noise_level);
	}

	NTINoise::NTINoise(float probability): rs_(rd_()), ds_(0, 1), probability_(probability)
	{

	}

	byte NTINoise::transform(byte chr) const
	{
		if (probability_ == 0)
			return chr;
		if (probability_ == 1)
			return ~chr;
		byte nchar = 0;
		for (auto i = 0; i<CHAR_BIT; ++i, chr <<= 1)
		{
			auto val = ds_(rs_);
			if (val < probability_)
				nchar |= ((chr ^ 0x80) & 0x80) >> 7;
			else
				nchar |= (chr & 0x80) >> 7;
			if (i != CHAR_BIT-1)
				nchar <<= 1;
		}
		return nchar;
	}
}
