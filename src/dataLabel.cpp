#include "dataLabel.h"

#include "ttfMeshFont.h"

dataLabel::dataLabel( const dataLabel::labelText_t& labelText, const dataLabel::roundRectAttribs& roundRect, const dataLabel::arrowAttribsSOA& arrowAttribsVector )
    : mLabelText( labelText )
    , mRoundRect( roundRect )
    , mArrows( arrowAttribsVector ) {

}

dataLabel::~dataLabel() {
}

