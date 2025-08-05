#pragma once

#include "../signalsmith-stretch.h"
#include "./stop-denormals.h"

struct StretchVocalUp {

	void configure(float sampleRate, size_t maxBufferLength) {
		stretch.configure(1, sampleRate*0.06, sampleRate*0.015, true);
		floatBufferIn.resize(maxBufferLength);
		floatBufferOut.resize(maxBufferLength);
		limiterSlew = 1/(0.05*sampleRate + 1);

		stretch.setTransposeSemitones(12, 8000/sampleRate);
		stretch.setFormantSemitones(3, true);
		stretch.setFormantBase(100/sampleRate);
	}
	void reset() {
		stretch.reset();
		limiterGain = 1;
	}
	
	// In-place processing is supported
	void process(int16_t *buffer, size_t length) {
		process(buffer, buffer, length);
	}

	void process(int16_t *inBuffer, int16_t *outBuffer, size_t length) {
		StopDenormals scoped;

		// 16-bit to float
		for (size_t i = 0; i < length; ++i) {
			floatBufferIn[i] = inBuffer[i];
		}

		stretch.process(&floatBufferIn, length, &floatBufferOut, length);
		
		// float to 16-bit, with a basic limiter
		for (size_t i = 0; i < length; ++i) {
			float x = floatBufferOut[i];
			limiterGain += (1 - limiterGain)*limiterSlew;
			float y = x*limiterGain, absY = std::abs(y);
			if (absY > maxOutput) {
				limiterGain = maxOutput/absY;
				y = x*limiterGain;
			}
			outBuffer[i] = y;
		}
	}

private:
	std::vector<float> floatBufferIn, floatBufferOut;
	static constexpr float maxOutput = 32760;
	float limiterGain = 1, limiterSlew = 1;
	signalsmith::stretch::SignalsmithStretch<float> stretch;
};
