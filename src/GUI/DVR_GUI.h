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

    struct GuiUserData_t {
        std::string& volumeDataUrl;
        int* pGradientModeIdx;
        SharedMemIPC* pSharedMem;
        int* pRayMarchAlgoIdx;
        bool& loadFileTrigger;
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
    };

    static void InitGui( const GfxAPI::ContextOpenGL& contextOpenGL );

    static void CreateGuiLayout( void* const pUserData );
    static void LightControls( DVR_GUI::GuiUserData_t* const pGuiUserData );

    static void DisplayGui( void* const pUserData, std::vector<bool>& collapsedState );

    static void DestroyGui();

};

#endif // _DVR_GUI_H_3CA9AA94_C3B8_49C7_9142_1B376C6F3B73
