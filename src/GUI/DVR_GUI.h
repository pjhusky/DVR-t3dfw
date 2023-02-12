#ifndef _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
#define _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73

#include <array>
#include <vector>
#include <string>
//#include <functional>

namespace GfxAPI {
    struct ContextOpenGL;
}

//struct VolumeData;
//struct Texture;
//struct ApplicationDVR;
//struct ApplicationTransferFunction;
struct SharedMemIPC;

struct DVR_GUI {

    struct GuiUserData_t {
        std::string& volumeDataUrl;
        //std::string& tfUrl;
        SharedMemIPC* pSharedMem;
        int* pRayMarchAlgoIdx;
        bool& loadFileTrigger;
        //VolumeData* pVolumeData;
        //Texture& volumeTexture;
        //ApplicationDVR* pAppDVR;
        //ApplicationTransferFunction* pAppTF;
        //std::function<Status_t( const std::string& )> callBack;
        bool& resetTrafos;
        bool& wantsToCaptureMouse;
        bool& editTransferFunction;
        std::array<int, 3>& dim;
        std::array<float, 2>& surfaceIsoAndThickness;
        std::array<float, 3>   surfaceIsoColor;
    };

    enum class eRayMarchAlgo: int {
        backfaceCubeRaster = 0,
        fullscreenBoxIsect = 1,
        //numEnums,
    };
    //std::vector < std::pair<eRayMarchAlgo, std::string> > rayMarchAlgoRefl{
    //    std::make_pair( eRayMarchAlgo::backfaceCubeRaster , "eRayMarchAlgo::backfaceCubeRaster" ),
    //    std::make_pair( eRayMarchAlgo::fullscreenBoxIsect , "eRayMarchAlgo::fullscreenBoxIsect" ),
    //};

    static std::vector< const char* > rayMarchAlgoNames;
    //int rayMarchAlgoIdx;


    static void InitGui( const GfxAPI::ContextOpenGL& contextOpenGL );

    static void CreateGuiLayout( void* const pUserData );

    static void DisplayGui( void* const pUserData );

    static void Resize();

    static void DestroyGui();

};

#endif // _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
