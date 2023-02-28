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
        float frameRate;
        int* pVisAlgoIdx;
        int* pDebugVisModeIdx;
        bool& useEmptySpaceSkipping;
        bool& showLabels;
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
        levoyIsosurface = LEVOY_ISO_SURFACE,
        f2bCompositing  = F2B_COMPOSITE,
        xray            = XRAY,
        mri             = MRI,
    };

    enum class eDebugVisMode: int {
        none            = DEBUG_VIS_NONE,
        bricks          = DEBUG_VIS_BRICKS,
        relativeCost    = DEBUG_VIS_RELCOST,
        stepsSkipped    = DEBUG_VIS_STEPSSKIPPED,
        invStepsSkipped = DEBUG_VIS_INVSTEPSSKIPPED,
    };

    enum class eRayMarchAlgo: int {
        backfaceCubeRaster = 0,
        fullscreenBoxIsect = 1,
    };

    static void InitGui( const GfxAPI::ContextOpenGL& contextOpenGL );


    static void CreateGuiLayout( void* const pUserData, const int32_t fbW, const int32_t fbH );
    static void MainMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH );
    static void LightMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH );
    static void StatsMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH );

    static void DisplayGui( void* const pUserData, std::vector<bool>& collapsedState, const int32_t fbW, const int32_t fbH );

    static void DestroyGui();

};

#endif // _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
