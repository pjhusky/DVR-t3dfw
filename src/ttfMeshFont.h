#ifndef _TTF_MESH_FONT_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
#define _TTF_MESH_FONT_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0

#include "external/ttf2mesh/ttf2mesh.h"
#include "gfxUtils.h"

#include <map>
#include <filesystem>
#include <tchar.h>

struct ttfMeshFont {

    ttfMeshFont( const std::filesystem::path& fileUrl );
    ~ttfMeshFont();

    void renderText2d( const float x, const float y, const TCHAR* pText );
    void renderText3d( const float x, const float y, const TCHAR* pText );

    void setAspectRatios( const linAlg::vec2_t& xyRatios ) { mRatiosXY = xyRatios; }

private:

    struct glyphData_t {
        ttf_glyph_t*                pGlyph;
        ttf_mesh_t*                 pMesh2d;
        ttf_mesh3d_t*               pMesh3d;
        gfxUtils::bufferHandles_t   bufferHandle2d;
        gfxUtils::bufferHandles_t   bufferHandle3d;
    };

    const glyphData_t* chooseGlyph( TCHAR symbol );

    linAlg::vec2_t                 mRatiosXY;

    ttf_t*                         mpFont;
    std::map< TCHAR, glyphData_t > mGlyphs;
};

#endif // _TTF_MESH_FONT_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
