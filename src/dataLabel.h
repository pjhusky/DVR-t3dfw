#ifndef _SCREEN_LABEL_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
#define _SCREEN_LABEL_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0

#include <vector>
#include <string>
#include <tchar.h>
#include "math/linAlg.h"

struct dataLabel {

    struct alignas(linAlg::vec4_t) sdfRoundRect_t {
        linAlg::vec2_t  startPos;
        linAlg::vec2_t  endPos;
        float           thickness;
        float           cornerRoundness;
        float           reserved[2];
    };
    struct alignas(linAlg::vec4_t) sdfArrow_t {
        linAlg::vec2_t  startPos;
        linAlg::vec2_t  endPos;
        float           shaftThickness;
        float           headThickness;
        float           reserved[2];
    };

    // the round rect (text label) will be placed in 2D only based on some layouting logic
    using roundRectAttribs = sdfRoundRect_t;

    // the arrow, however, not only needs to connect the 2D positions of its starting pos at the round rect and its end pos, but its end position is anchored in the OS of the volume data
    using arrowAttribsSOA = struct {
        std::vector<sdfArrow_t>      screenAttribs;
        std::vector<linAlg::vec3_t>  dataPos3D;
    };
    
    using labelText_t = std::basic_string<TCHAR>;
    
    dataLabel( const labelText_t& labelText, const roundRectAttribs& roundRect, const arrowAttribsSOA& arrowAttribsVector );
    ~dataLabel();

    const labelText_t& labelText() const { return mLabelText; }

    roundRectAttribs& roundRect() { return mRoundRect; }
    const roundRectAttribs& roundRect() const { return mRoundRect; }

    uint32_t numArrows() const { return static_cast<uint32_t>( mArrows.screenAttribs.size() ); }
    std::vector<sdfArrow_t>& arrowScreenAttribs() { return mArrows.screenAttribs; }
    const std::vector<sdfArrow_t>& arrowScreenAttribs() const { return mArrows.screenAttribs; }
    std::vector<linAlg::vec3_t>& arrowDataPos3D() { return mArrows.dataPos3D; }
    const std::vector<linAlg::vec3_t>& arrowDataPos3D() const { return mArrows.dataPos3D; }

private:
    labelText_t                 mLabelText;
    roundRectAttribs            mRoundRect;
    arrowAttribsSOA             mArrows;

};

#endif // _SCREEN_LABEL_H_A5EBD2FE_F21F_40CD_93AC_D2FA7AD211F0
