#ifndef _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
#define _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73

#include <vector>
#include <string>
//#include <functional>

struct ContextOpenGL;

//struct VolumeData;
//struct Texture;
//struct ApplicationDVR;

struct DVR_GUI {

    struct GuiUserData_t {
        std::string& volumeDataUrl;
        int* pRayMarchAlgoIdx;
        bool& loadFileTrigger;
        //VolumeData* pVolumeData;
        //Texture& volumeTexture;
        //ApplicationDVR* pAppDVR;
        //std::function<Status_t( const std::string& )> callBack;
        bool& resetTrafos;
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


    static void InitGui( const ContextOpenGL& contextOpenGL );

    static void CreateGuiLayout( void* const pUserData );

    static void DisplayGui( void* const pUserData );

    static void Resize();

    static void DestroyGui();

};

#endif // _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
