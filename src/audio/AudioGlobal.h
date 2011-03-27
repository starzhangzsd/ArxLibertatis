/*
===========================================================================
ARX FATALIS GPL Source Code
Copyright (C) 1999-2010 Arkane Studios SA, a ZeniMax Media company.

This file is part of the Arx Fatalis GPL Source Code ('Arx Fatalis Source Code'). 

Arx Fatalis Source Code is free software: you can redistribute it and/or modify it under the terms of the GNU General Public 
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

Arx Fatalis Source Code is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied 
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with Arx Fatalis Source Code.  If not, see 
<http://www.gnu.org/licenses/>.

In addition, the Arx Fatalis Source Code is also subject to certain additional terms. You should have received a copy of these 
additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Arx 
Fatalis Source Code. If not, please request a copy in writing from Arkane Studios at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing Arkane Studios, c/o 
ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#ifndef ARX_AUDIO_AUDIOGLOBAL_H
#define ARX_AUDIO_AUDIOGLOBAL_H

#include <cmath>

#include <al.h>
#include <alc.h>

#include "AudioTypes.h"
#include "AudioResource.h"
#include "Stream.h"
#include "Mixer.h"
#include "AudioEnvironment.h"
#include "Sample.h"
#include "Ambient.h"
#include "AudioInstance.h"

namespace ATHENA {
	
	// Common resource memory management
	
	// Internal globals
	

	const aalULong FLAG_ANY_ENV_FX(AAL_FLAG_OBSTRUCTION |
	                               AAL_FLAG_REVERBERATION);
	
	const aalULong FLAG_ANY_3D_FX(AAL_FLAG_POSITION |
	                              AAL_FLAG_VELOCITY |
	                              AAL_FLAG_DIRECTION |
	                              AAL_FLAG_CONE |
	                              AAL_FLAG_FALLOFF |
	                              FLAG_ANY_ENV_FX);
	
	// Audio device interface
	extern ALCdevice *device;
	extern ALCcontext *context;
	extern ALuint primary[1];
	//extern LPDIRECTSOUNDBUFFER primary;
	//extern LPKSPROPERTYSET environment;
	extern aalUBool is_reverb_present;
	extern aalSLong environment_id;
	
	// Global settings
	extern char * sample_path;
	extern char * ambiance_path;
	extern char * environment_path;
	extern aalULong stream_limit_ms;
	extern aalULong stream_limit_bytes;
	extern aalULong session_start;
	extern aalULong session_time;
	extern aalULong global_status;
	extern aalFormat global_format;
	
	// Resources
	extern ResourceList<Mixer> _mixer;
	extern ResourceList<Sample> _sample;
	extern ResourceList<Ambiance> _amb;
	extern ResourceList<Environment> _env;
	extern ResourceList<Instance> _inst;
	
	// Internal functions
	
	// Random number generator
	aalULong Random();
	aalFloat FRandom();
	aalULong InitSeed();
	
	// Conversion
	aalULong UnitsToBytes(const aalULong & v, const aalFormat & format, const aalUnit & unit = AAL_UNIT_MS);
	aalULong BytesToUnits(const aalULong & v, const aalFormat & format, const aalUnit & unit = AAL_UNIT_MS);
	
	inline aalFloat LinearToLogVolume(const aalFloat & volume) {
		return 0.2F * (float)log10(volume) + 1.0F;
	}
	
	// Validity
	inline aalSLong GetSampleID(const aalSLong & id) {
		return id & 0x0000ffff;
	}
	
	inline aalSLong GetInstanceID(const aalSLong & id) {
		return ((id >> 16) & 0x0000ffff);
	}
	
	// Vector operators
	inline aalVector & operator+=(aalVector & dst, const aalVector & src) {
		dst.x += src.x;
		dst.y += src.y;
		dst.z += src.z;
		return dst;
	}
	
	inline aalVector & operator*=(aalVector & dst, const aalVector & src) {
		dst.x *= src.x;
		dst.y *= src.y;
		dst.z *= src.z;
		return dst;
	}
	
	inline aalVector & operator*=(aalVector & dst, const aalFloat & factor) {
		dst.x *= factor;
		dst.y *= factor;
		dst.z *= factor;
		return dst;
	}
	
} // namespace ATHENA

#endif // ARX_AUDIO_AUDIOGLOBAL_H
