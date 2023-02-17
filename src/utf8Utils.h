#ifndef _UTF8_UTILS_H_B35F2D75_E81F_413B_9BD4_174FD20A21F3
#define _UTF8_UTILS_H_B35F2D75_E81F_413B_9BD4_174FD20A21F3

#include "../utfcpp/source/utf8.h" // solve C++17 UTF deprecation

namespace utf8Utils {
    // https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
    static inline std::string utf8_encode( const std::wstring& wStr )
    {
    #if 0
        return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes( wStr );
    #else
        std::string utf8line;

        if (wStr.empty())
            return utf8line;

    #ifdef _MSC_VER
        utf8::utf16to8( wStr.begin(), wStr.end(), std::back_inserter( utf8line ) );
    #else
        utf8::utf32to8( wStr.begin(), wStr.end(), std::back_inserter( utf8line ) );
    #endif
        return utf8line;
    #endif
    }

    // https://codingtidbit.com/2020/02/09/c17-codecvt_utf8-is-deprecated/
    static inline std::wstring utf8_decode( const std::string& sStr )
    {
    #if 0
        return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes( sStr );
    #else
        std::wstring wide_line;

        if (sStr.empty())
            return wide_line;

    #ifdef _MSC_VER
        utf8::utf8to16( sStr.begin(), sStr.end(), std::back_inserter( wide_line ) );
    #else
        utf8::utf8to32( sStr.begin(), sStr.end(), std::back_inserter( wide_line ) );
    #endif
        return wide_line;
    #endif
    }

}

#endif // _UTF8_UTILS_H_B35F2D75_E81F_413B_9BD4_174FD20A21F3
