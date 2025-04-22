#pragma once
#include <precompiled.h>

class CinematicAudio
{
public:
	virtual ~CinematicAudio();
	virtual void InitAudio( void* audioContext ) = 0;
	virtual void PlayAudio( uint8_t* data, int size ) = 0;
	virtual void ResetAudio() = 0;
	virtual void ShutdownAudio() = 0;
};
