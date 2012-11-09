#pragma once

#include <engine-globals.h>
#include <util/audio/PcmFormat.hh>

#if defined(CONFIG_AUDIO_ALSA)
	#include <audio/alsa/config.hh>
#else
	#include <audio/config.hh>
#endif

#if defined CONFIG_AUDIO_OPENSL_ES || defined CONFIG_AUDIO_COREAUDIO || \
	defined CONFIG_AUDIO_SDL || defined CONFIG_AUDIO_ALSA
	#define CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
#endif

namespace Audio
{

struct BufferContext
{
	constexpr BufferContext() { }
	void *data = nullptr;
	uframes frames = 0;
};

static const PcmFormat maxFormat { maxRate, &SampleFormats::s16, 2 };

#if !defined(CONFIG_AUDIO)

static PcmFormat preferredPcmFormat = maxFormat;
static PcmFormat pcmFormat = maxFormat;

static CallResult init() { return OK; }
static CallResult openPcm(const PcmFormat &format) { return UNSUPPORTED_OPERATION; }
static void closePcm() {}
static bool isOpen(){ return 0; }
static void writePcm(uchar *samples, uint framesToWrite) {}
static BufferContext *getPlayBuffer(uint wantedFrames) { return 0; }
static void commitPlayBuffer(BufferContext *buffer, uint frames) {}
static int frameDelay() { return 0; }
static int framesFree() { return 0; }
static void setHintPcmFramesPerWrite(uint frames) { }
static void setHintPcmMaxBuffers(uint buffers) { }
static uint hintPcmMaxBuffers() { return 0; }

#else

extern PcmFormat preferredPcmFormat;
extern PcmFormat pcmFormat; // the currently playing format

CallResult init() ATTRS(cold);
CallResult openPcm(const PcmFormat &format);
void closePcm();
bool isOpen();
void writePcm(uchar *samples, uint framesToWrite);
BufferContext *getPlayBuffer(uint wantedFrames);
void commitPlayBuffer(BufferContext *buffer, uint frames);
int frameDelay();
int framesFree();
void setHintPcmFramesPerWrite(uint frames);
void setHintPcmMaxBuffers(uint buffers);
uint hintPcmMaxBuffers();
void setHintStrictUnderrunCheck(bool on);
bool hintStrictUnderrunCheck();

#endif

// shortcuts
static PcmFormat &pPCM = preferredPcmFormat;

static CallResult openPcm(int rate) { return openPcm({ rate, pPCM.sample, pPCM.channels }); }
static CallResult openPcm(int rate, int channels) { return openPcm({ rate, pPCM.sample, channels }); }
static CallResult openPcm() { return openPcm(pPCM); }
static bool supportsRateNative(int rate) { return rate <= pPCM.rate; }
}
