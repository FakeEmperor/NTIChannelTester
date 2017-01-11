#pragma once

namespace nti
{
	namespace constants
	{
		
		class FloatingPointEnum {
			float f_;
		protected:
			FloatingPointEnum(float arg) : f_(arg) {}
		public:
			operator float() const { return f_; }
		};

		class NoiseLevelHardness : public FloatingPointEnum
		{
			NoiseLevelHardness(float arg_) : FloatingPointEnum(arg_)
			{
			}
		public:
			static const NoiseLevelHardness  NO_NOISE, EASY, AVERAGE, HARD, VERY_HARD;

		};


	}

}