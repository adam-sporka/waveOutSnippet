// waveOutSnippet.cpp by Adam Sporka
// A demonstration of the traditional win32 waveOut sound API.
// This will play a 440 Hz sine wave on the default audio interface.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <math.h>

#include <windows.h>

// The audio will be processed in buffers this many samples long:
#define BUFLEN 512

// This is the sampling frequency in Hz:
#define SAMPLE_RATE 48000

// 4 buffers will be used in total
#define BUFFER_COUNT 4

// Audio device handle
HWAVEOUT hWaveOut;

// Wave headers, one for each buffer
WAVEHDR whdr[BUFFER_COUNT];

// Audio data for of the buffers
unsigned char* buffer[BUFFER_COUNT];

// To know which buffer to process next
int round_robin = 0;

// Global counter of generated samples
int64_t N = 0;

////////////////////////////////////////////////////////////////
void synthesizeBuffer(int count_samples_per_channel, int16_t* buffer)
{
	double A440 = 440.0; // 440 Hz
	double PI = 3.14151926535;

	for (int a = 0; a < count_samples_per_channel; a++)
	{
		// Calculate the next value of the sine wave sample.
		double value = 0.1 * sin(N * A440 * 2 * PI / (double)SAMPLE_RATE);

		// Convert to 16-bit value
		int16_t v16bit = static_cast<int16_t>(value * 32767);

		// Write the value to the buffer. The buffer has two channels.
		// Copy the same value to both.
		*buffer = v16bit;
		buffer++;
		*buffer = v16bit;
		buffer++;

		// Increment the global sample counter.
		N++;
	}
}

////////////////////////////////////////////////////////////////
void allocateAndClearBuffers()
{
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		// Allocate & clear the buffer
		buffer[i] = new unsigned char[BUFLEN * 2 * 2];
		memset(buffer[i], 0, BUFLEN * 2 * 2);

		// Fill out the header
		whdr[i].lpData = (char*)buffer[i];
		whdr[i].dwBufferLength = BUFLEN * 2 * 2;
		whdr[i].dwBytesRecorded = 0;
		whdr[i].dwUser = 0;
		whdr[i].dwFlags = 0;
		whdr[i].dwLoops = 0;
	}
}

////////////////////////////////////////////////////////////////
void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg) {

	case WOM_OPEN:
		// When the audio device is opened
		allocateAndClearBuffers();
		break;

	case WOM_DONE:
		// When the audio device needs more data
		synthesizeBuffer(BUFLEN, (int16_t*)(buffer[round_robin % BUFFER_COUNT]));
		waveOutWrite(hWaveOut, &whdr[round_robin % BUFFER_COUNT], sizeof(whdr[round_robin % BUFFER_COUNT]));
		round_robin++;
		break;

	case WOM_CLOSE:
		// No need to implement
		break;

	default:
		break;
	}
}

////////////////////////////////////////////////////////////////
bool openAudio()
{
	DWORD dummy;

	// Open the sound card
	WAVEFORMATEX waveformatex;
	waveformatex.wFormatTag = WAVE_FORMAT_PCM;
	waveformatex.nChannels = 2; // Stereo
	waveformatex.nSamplesPerSec = SAMPLE_RATE;
	waveformatex.nAvgBytesPerSec = SAMPLE_RATE * 2 * 2; // 2 channels, 2 bytes per each sample
	waveformatex.nBlockAlign = 2 * 2; // 2 channels, 2 bytes per 1 sample in each channel
	waveformatex.wBitsPerSample = 16;
	waveformatex.cbSize = 0;

	auto result = waveOutOpen(&hWaveOut, WAVE_MAPPER, &waveformatex, (DWORD_PTR)waveOutProc, (DWORD_PTR)&dummy, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
	if (result != MMSYSERR_NOERROR)
	{
		return false;
	}

	// Pre-buffer with silence
	for (int i = 0; i < BUFFER_COUNT; i++)
	{
		waveOutPrepareHeader(hWaveOut, &whdr[i], sizeof(whdr[i]));
		waveOutWrite(hWaveOut, &whdr[i], sizeof(whdr[i]));
	}

	return true;
}

////////////////////////////////////////////////////////////////
int main()
{
	bool success = openAudio();

	if (!success)
	{
		printf("Unable to open the audio interface. Terminating.\n");
		return 1;
	}

	printf("You should hear a sine wave now. Press Escape to quit.");

	char key;
	do {
 		key = _getch();
	}
	while (key != 27);
	return 0;
}
