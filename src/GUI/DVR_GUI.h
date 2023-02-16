#ifndef _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
#define _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73

#include <array>
#include <vector>
#include <string>

namespace GfxAPI {
    struct ContextOpenGL;
}

struct SharedMemIPC;

struct DVR_GUI {

#if 1
    #include "src/shaders/dvrCommonDefines.h.glsl"
#else
    struct LightParameters {
        std::array<float, 4> lightDir;
        std::array<float, 4> lightColor;
        float ambient;
        float diffuse;
        float specular;
    };
#endif

    struct GuiUserData_t {
        std::string& volumeDataUrl;
        int* pGradientModeIdx;
        SharedMemIPC* pSharedMem;
        int* pVisAlgoIdx;
        int* pRayMarchAlgoIdx;
        bool& loadFileTrigger;
        bool& resetTrafos;
        bool& wantsToCaptureMouse;
        bool& editTransferFunction;
        std::array<int, 3>& dim;
        std::array<float, 2>& surfaceIsoAndThickness;
        std::array<float, 3>   surfaceIsoColor;
        LightParameters& lightParams;
        bool& lightParamsChanged;
    };

    enum class eVisAlgo: int {
        levoyIsosurface = 0,
        f2bCompositing  = 1,
        xray            = 2,
        mri             = 3,
    };

    enum class eRayMarchAlgo: int {
        backfaceCubeRaster = 0,
        fullscreenBoxIsect = 1,
    };

    static void InitGui( const GfxAPI::ContextOpenGL& contextOpenGL );


    static void CreateGuiLayout( void* const pUserData );
    static void MainMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData );
    static void LightMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData );

    static void DisplayGui( void* const pUserData, std::vector<bool>& collapsedState );

    static void DestroyGui();

};

#endif // _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
