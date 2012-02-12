#include "iOSAudio.h"

#import <AudioToolbox/AudioQueue.h>

#include "../SNES9X/port.h"

#pragma mark Defines

#define SI_AUDIO_BUFFER_COUNT 6
//#define FRAME_SIZE 2048

#pragma mark - External Forward Declarations

//extern void S9xMixSamplesO (signed short *buffer, int sample_count, int sample_offset);
extern bool8 S9xMixSamples (uint8 *buffer, int sample_count);

extern volatile int SI_EmulationPaused;
extern volatile int SI_EmulationRun;
extern volatile int SI_SoundOn;

#pragma mark - Private Structures

typedef struct AQCallbackStruct {
  AudioQueueRef queue;
  UInt32 frameCount;
  AudioQueueBufferRef mBuffers[SI_AUDIO_BUFFER_COUNT];
  AudioStreamBasicDescription mDataFormat;
} AQCallbackStruct;

#pragma mark - Global Variables

//const int SI_IsStereo = 0;
const int SI_IsStereo = 1;
AQCallbackStruct SI_AQCallbackStruct = {0};
uint32_t SI_SoundBufferSizeBytes = 0;
int SI_SoundIsInit = 0;
float SI_AudioVolume = 1.0;
float SI_AQCallbackCount = 0;

#pragma mark - Audio Queue Management

static void AQBufferCallback(
                             void *userdata,
                             AudioQueueRef outQ,
                             AudioQueueBufferRef outQB)
{
	outQB->mAudioDataByteSize = SI_SoundBufferSizeBytes;
  AudioQueueSetParameter(outQ, kAudioQueueParam_Volume, SI_AudioVolume);

	if(SI_EmulationPaused || !SI_EmulationRun || !SI_SoundIsInit)
    memset(outQB->mAudioData, 0, SI_SoundBufferSizeBytes);
	else
  {
    memset(outQB->mAudioData, 0, SI_SoundBufferSizeBytes);
    S9xMixSamples((unsigned char*)outQB->mAudioData, (SI_SoundBufferSizeBytes)/2);
  }

	AudioQueueEnqueueBuffer(outQ, outQB, 0, NULL);
}

int SIOpenSound(int buffersize)
{
  SI_SoundIsInit = 0;
	
  if(SI_AQCallbackStruct.queue != 0)
    AudioQueueDispose(SI_AQCallbackStruct.queue, true);
  
  SI_AQCallbackCount = 0;
  memset(&SI_AQCallbackStruct, 0, sizeof(AQCallbackStruct));
  
  Float64 sampleRate = 22050.0;
  sampleRate = 32000.0;
	SI_SoundBufferSizeBytes = buffersize;
	
  SI_AQCallbackStruct.mDataFormat.mSampleRate = sampleRate;
  SI_AQCallbackStruct.mDataFormat.mFormatID = kAudioFormatLinearPCM;
  SI_AQCallbackStruct.mDataFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  SI_AQCallbackStruct.mDataFormat.mBytesPerPacket    =   4;
  SI_AQCallbackStruct.mDataFormat.mFramesPerPacket   =   SI_IsStereo ? 1 : 2;
  SI_AQCallbackStruct.mDataFormat.mBytesPerFrame     =   SI_IsStereo ? 4 : 2;
  SI_AQCallbackStruct.mDataFormat.mChannelsPerFrame  =   SI_IsStereo ? 2 : 1;
  SI_AQCallbackStruct.mDataFormat.mBitsPerChannel    =   16;
	
	
  /* Pre-buffer before we turn on audio */
  UInt32 err;
  err = AudioQueueNewOutput(&SI_AQCallbackStruct.mDataFormat,
                            AQBufferCallback,
                            NULL,
                            NULL,
                            kCFRunLoopCommonModes,
                            0,
                            &SI_AQCallbackStruct.queue);
	
	for(int i=0; i<SI_AUDIO_BUFFER_COUNT; i++) 
	{
		err = AudioQueueAllocateBuffer(SI_AQCallbackStruct.queue, SI_SoundBufferSizeBytes, &SI_AQCallbackStruct.mBuffers[i]);
    memset(SI_AQCallbackStruct.mBuffers[i]->mAudioData, 0, SI_SoundBufferSizeBytes);
		SI_AQCallbackStruct.mBuffers[i]->mAudioDataByteSize = SI_SoundBufferSizeBytes; //samples_per_frame * 2; //inData->mDataFormat.mBytesPerFrame; //(inData->frameCount * 4 < (sndOutLen) ? inData->frameCount * 4 : (sndOutLen));
		AudioQueueEnqueueBuffer(SI_AQCallbackStruct.queue, SI_AQCallbackStruct.mBuffers[i], 0, NULL);
	}
	
	SI_SoundIsInit = 1;
	err = AudioQueueStart(SI_AQCallbackStruct.queue, NULL);
	
	return 0;
}

void SICloseSound(void)
{
	if( SI_SoundIsInit == 1 )
	{
		AudioQueueDispose(SI_AQCallbackStruct.queue, true);
		SI_SoundIsInit = 0;
	}
}

void SIMuteSound(void)
{
	if( SI_SoundIsInit == 1 )
	{
		SICloseSound();
	}
}

void SIDemuteSound(int buffersize)
{
	if( SI_SoundIsInit == 0 )
	{
		SIOpenSound(buffersize);
	}
}