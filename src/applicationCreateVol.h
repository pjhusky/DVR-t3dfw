#ifndef _APPLICATION_CREATE_VOL_H_
#define _APPLICATION_CREATE_VOL_H_

#include "applicationInterface/iApplication.h"

#include "math/linAlg.h"

#include <stdio.h>
#include <vector>
#include <string>
#include <filesystem> // since C++17

struct ApplicationCreateVol : public iApplication {
    ApplicationCreateVol( const GfxAPI::ContextOpenGL& contextOpenGL, const linAlg::i32vec3_t& dim, const std::string& fileUrl )
        : mDim( dim ) {

        std::vector< uint16_t > data;
        const size_t headerSize = 3;
        const size_t numVoxels = mDim[0] * mDim[1] * mDim[2];
        data.resize( numVoxels + headerSize );
        data[0] = mDim[0];
        data[1] = mDim[1];
        data[2] = mDim[2];

        const int32_t minDim = linAlg::minimum( mDim[0], linAlg::minimum( mDim[1], mDim[2] ) );
        const float radius = static_cast<float>(minDim / 2);
        const float radius2 = radius * radius;
        linAlg::i32vec3_t center{ mDim[0] / 2, mDim[1] / 2, mDim[2] / 2 };

        printf( "dim ( %d | %d | %d ), minDim = %d, center ( %d | %d | %d )\n", mDim[0], mDim[1], mDim[2], minDim, center[0], center[1], center[2] );

        for (int32_t z = 0; z < mDim[2]; z++) {
            for (int32_t y = 0; y < mDim[1]; y++) {
                for (int32_t x = 0; x < mDim[0]; x++) {
                    const size_t addr = z * mDim[0] * mDim[1] + y * mDim[0] + x + headerSize;
                    const float dx = static_cast< float >( x - center[0] );
                    const float dy = static_cast< float >( y - center[1] );
                    const float dz = static_cast< float >( z - center[2] );
                    //data[addr] = (x % 2 == 0 && x % 2 == y % 2 && y % 2 == z % 2) ? 4095 : 0; // smallest sphere
                    data[addr] = (dx * dx + dy * dy + dz * dz <= radius2) ? 95 : 0;
                }
            }
        }

        std::filesystem::path filePath{ fileUrl };
        filePath.remove_filename();
        std::filesystem::create_directories( filePath );

        FILE* pFile = fopen( fileUrl.c_str(), "wb" );
        fwrite( data.data(), sizeof( uint16_t ), data.size(), pFile );
        fclose( pFile );
    }

    virtual ~ApplicationCreateVol() {}
    
    virtual Status_t run() override { return Status_t::OK(); }
    
    void setDesiredDim( const linAlg::i32vec3_t& dim ) { mDim = dim; }
    
    private:
    linAlg::i32vec3_t  mDim;
};

#endif // _APPLICATION_CREATE_VOL_H_

