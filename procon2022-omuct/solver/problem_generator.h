#pragma once

# include <Siv3D.hpp> // OpenSiv3D v0.6.4
#include "wave_int32.h"

namespace Procon33 {

	struct Problem {
		Array<String> waveFileNames;
		Array<int> slideWidth;
	};




	class ProblemGenerator {
	public:

		template<class URBG>
		static Problem randomProblem(
			Array<FilePath> sourceFileNames,
			int superpositionCount,
			int maxSlideWidth,
			URBG& rng
		) {
			int num_wave_file = (int)sourceFileNames.size();
			Print << U"num_wave_file = " << num_wave_file;

			if (superpositionCount > num_wave_file) {
				throw Error(U"there are not enough number of .wav files");
			}

			auto rng_state = rng.serialize();

			Array<int> superpositionSrcIndecies = s3d::Range(0, num_wave_file - 1).asArray().choice(superpositionCount, rng);
			Array<String> superpositionSrcFilenames = superpositionSrcIndecies.map([&](int i) -> String { return sourceFileNames[i]; });

			Array<int> slideWidth = Array<int>(superpositionCount);
			// get random slide_width
			{
				for (auto& w : slideWidth) w = s3d::Random(0, maxSlideWidth, rng);
				int minWidth = maxSlideWidth;
				for (auto a : slideWidth) minWidth = std::min(minWidth, a);
				for (auto& a : slideWidth) a -= minWidth;
			}

			Problem res;
			res.waveFileNames = superpositionSrcFilenames;
			res.slideWidth = slideWidth;
			return res;
		}

		static WaveInt32 waveFromProblem(const Problem& problem) {
			int wave_count = (int)problem.waveFileNames.size();
			if (problem.slideWidth.size() != wave_count) {
				throw Error(U"in ::MixWave : size of slide_width does not match");
			}

			Print << U"MixWave";

			// input from file
			Array<WaveInt32> wave_src;
			for (int i = 0; i < wave_count; i++) {
				wave_src.push_back(WaveInt32::FromWavFile(problem.waveFileNames[i]));
			}

			Print << U"    wave_src.size() = " << wave_src.size();

			WaveInt32 res_wave_arr;

			// determine length of output
			{
				int res_size = 0;
				for (int i = 0; i < wave_count; i++) {
					auto& wave = wave_src[i];
					int length = (int)wave.length() + problem.slideWidth[i];
					res_size = std::max<int>(res_size, length);
				}

				res_wave_arr = WaveInt32(res_size);
			}

			// mix waves
			for (int i = 0; i < wave_count; i++) {
				res_wave_arr.addWave(wave_src[i], problem.slideWidth[i], 1);
			}

			return res_wave_arr;
		}

	};

} // namespace Procon33
