#ifndef _APPLICATION_DVR_COMMON_H_2CCB6D07_079B_4B55_B75E_459260FE322C
#define _APPLICATION_DVR_COMMON_H_2CCB6D07_079B_4B55_B75E_459260FE322C

#include "gfxAPI/texture.h"
#include "math/linAlg.h"


struct ApplicationDVR_common {
	static GfxAPI::Texture::Desc_t densityColorsTexDesc() {
		return GfxAPI::Texture::Desc_t{
			.texDim = linAlg::i32vec3_t{ 1024, 1, 0 },
			.numChannels = 4,
			.channelType = GfxAPI::eChannelType::i8,
			.semantics = GfxAPI::eSemantics::color,
			.isMipMapped = false,
		};
	}
};

#endif // _APPLICATION_DVR_COMMON_H_2CCB6D07_079B_4B55_B75E_459260FE322C
