#include "DVR_GUI.h"

#if 0
    #include "ImGradientiHDR/ImGradientHDR.h"
#endif

#include "gfxAPI/contextOpenGL.h"

#include "../sharedMemIPC.h"

//#include "volumeData.h"
//#include "texture.h"
//#include "gfxUtils.h"


#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "external/libtinyfiledialogs/tinyfiledialogs.h"

#include <nfd.h>

#include <array>
#include <filesystem>

namespace {
#if 0
    ImGradientHDRState gradientState;
    ImGradientHDRTemporaryState temporaryGradientState;
#endif

    static float guiScale = 1.0f;
    static auto guiButtonSize() { return ImVec2{ 400, 40 }; }

    static std::vector< const char* > rayMarchAlgoNames{
        "eRayMarchAlgo::backfaceCubeRaster",
        "eRayMarchAlgo::fullscreenBoxIsect",
    };

    static std::vector< const char* > gradientAlgoNames{
        "Central Differences", 
        "Sobel 3D",
    };

}


void DVR_GUI::InitGui( const GfxAPI::ContextOpenGL& contextOpenGL )
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    //ImGui_ImplWin32_EnableDpiAwareness();
    ImGui_ImplGlfw_InitForOpenGL( reinterpret_cast<GLFWwindow*>(contextOpenGL.window()), true );
    ImGui_ImplOpenGL3_Init( "#version 330" );

#if 0
    gradientState.AddColorMarker( 0.0f, { 0.0f, 1.0f, 0.0f }, 0.8f );
    gradientState.AddColorMarker( 0.2f, { 1.0f, 0.0f, 0.0f }, 0.8f );
    gradientState.AddColorMarker( 0.8f, { 0.0f, 0.0f, 1.0f }, 0.8f );
    gradientState.AddColorMarker( 1.0f, { 0.0f, 1.0f, 0.0f }, 0.8f );
    gradientState.AddAlphaMarker( 0.0f, 1.0f );
    gradientState.AddAlphaMarker( 0.5f, 0.2f );
    gradientState.AddAlphaMarker( 1.0f, 1.0f );
#endif

}

void DVR_GUI::CreateGuiLayout( void* const pUserData )
{
    ImGui::Begin( "Controls" );

    GuiUserData_t* const pGuiUserData = reinterpret_cast<GuiUserData_t* const>(pUserData);

    auto& io = ImGui::GetIO();
    // printf( "   io.DisplaySize = %f %f\n", io.DisplaySize.x, io.DisplaySize.y );
    
    const float imguiDpiScale = ImGui::GetMainViewport()->DpiScale;
    // printf( "   imguiDpiScale = %f\n", imguiDpiScale );

    const ImVec2 imguiMainViewportSize = ImGui::GetMainViewport()->Size;
    // printf( "   imguiMainViewportSize = %f %f\n", imguiMainViewportSize.x, imguiMainViewportSize.y );

    // printf( "   io.FontGlobalScale = %f\n", io.FontGlobalScale );

    io.FontAllowUserScaling = true;
    //ImGui::SetWindowFontScale( 2.0f );
    //io.FontGlobalScale = 2.0f;
    io.FontGlobalScale = imguiDpiScale;

    // printf( "   io.FontGlobalScale = %f\n", io.FontGlobalScale );
    


    const ImVec2 imGuiWindowSize = ImGui::GetWindowSize();
    //printf( "   imGuiWindowSize = %f %f\n", imGuiWindowSize.x, imGuiWindowSize.y );
    
    const float imguiWindowDpiScale = ImGui::GetWindowDpiScale();
    //printf( "   imguiWindowDpiScale = %f\n", imguiWindowDpiScale );
 

    ImGui::Text( "Loaded .dat file:\n%s", pGuiUserData->volumeDataUrl.c_str() );

    if ( ImGui::Button( "Load .dat File", guiButtonSize() ) && !pGuiUserData->loadFileTrigger ) {
        printf( "Open DAT File Dialog\n" );

        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog( "dat", NULL, &outPath );

        if (result == NFD_OKAY) {
            puts( "Success!" );
            puts( outPath );
            pGuiUserData->loadFileTrigger = true;
            pGuiUserData->volumeDataUrl = std::string( outPath );
            free( outPath );
        }
        else if (result == NFD_CANCEL) {
            puts( "User pressed cancel." );
        }
        else {
            printf( "Error: %s\n", NFD_GetError() );
        }
    }

    ImGui::Text( "%d", (pGuiUserData->dim)[0] );
    ImGui::SameLine();
    ImGui::Text( "x %d x", (pGuiUserData->dim)[1] );
    ImGui::SameLine();
    ImGui::Text( "%d", (pGuiUserData->dim)[2] );

    if (ImGui::Button( "Reset Transformations", guiButtonSize() ) && !pGuiUserData->resetTrafos ) {
        printf( "reset trafos button clicked\n" );
        pGuiUserData->resetTrafos = true;
    }

    //if (ImGui::ListBox( "RenderMethod", pGuiUserData->pRayMarchAlgoIdx, &rayMarchAlgoNames[0], rayMarchAlgoNames.size() )) {
    //    printf( "RenderMethod selection - nr. %d, '%s'\n", *(pGuiUserData->pRayMarchAlgoIdx), rayMarchAlgoNames[*(pGuiUserData->pRayMarchAlgoIdx)] );
    //}

    if (*(pGuiUserData->pGradientModeIdx) >= 0) {
        ImGui::Text( "Gradient Calculation Method:" );
        static const char* current_item = gradientAlgoNames[*(pGuiUserData->pGradientModeIdx)];

        for (int32_t i = 0; i < gradientAlgoNames.size(); i++) {
            ImGui::RadioButton( gradientAlgoNames[i], pGuiUserData->pGradientModeIdx, i );
        }
    }


    { // combo box
        ImGui::Text( "Raymarch Method:" );

        //const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "PPPP", "QQQQQQQQQQ", "RRR", "SSSS" };
        static const char* current_item = rayMarchAlgoNames[*(pGuiUserData->pRayMarchAlgoIdx)];

    #if 0
        if (ImGui::BeginCombo( "##combo", current_item )) { // The second parameter is the label previewed before opening the combo.
            //for (int n = 0; n < IM_ARRAYSIZE( &rayMarchAlgoNames[0] ); n++) {
            for (int n = 0; n < rayMarchAlgoNames.size(); n++) {
                bool is_selected = (current_item == rayMarchAlgoNames[n]); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable( rayMarchAlgoNames[n], is_selected )) {
                    current_item = rayMarchAlgoNames[n];
                    *(pGuiUserData->pRayMarchAlgoIdx) = n;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
                }
            }
            ImGui::EndCombo();
        }
    #else
        for (int32_t i = 0; i < rayMarchAlgoNames.size(); i++) {
            ImGui::RadioButton( rayMarchAlgoNames[i], pGuiUserData->pRayMarchAlgoIdx, i );
        }
    #endif

        ImGui::SliderFloat( "Iso value", &pGuiUserData->surfaceIsoAndThickness[0], 0.0f, 1.0f );
        ImGui::SameLine();
        //ImGui::ColorEdit3( "", std::array<float, 3>{1.0f, 0.2f, 0.0f}.data(), ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs );
        ImGui::ColorEdit3( "", pGuiUserData->surfaceIsoColor.data(), ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs );
        //ImGui::ColorPicker3( "", std::array<float, 3>{0.0f, 0.2f, 1.0f}.data(), ImGuiColorEditFlags_NoPicker );

        ImGui::SliderFloat( "Thickness", &pGuiUserData->surfaceIsoAndThickness[1], 0.0f, 50.0f / 4096.0f, "%.5f" );
    }

    pGuiUserData->editTransferFunction = ImGui::Button( "Edit Transfer Function", guiButtonSize() );

    // load TF
    
    if ( ImGui::Button( "Load .tf File", guiButtonSize() ) && pGuiUserData->pSharedMem->get( "loadTF" ) == " " /*&& !pGuiUserData->loadFileTrigger*/ ) {
        printf( "Open TF File Dialog\n" );

        
        std::filesystem::path volumeDataUrl = pGuiUserData->volumeDataUrl;
        std::filesystem::path volumeDataUrlTfExtenstion = ( volumeDataUrl.empty() ) ? "" : volumeDataUrl.replace_extension( "tf" );
        const char* filter{ "*.tf" };
        const char* fileDesc = "*.tf Transfer Function"; // "Transfer Function File";
        const char* pTfFileUrl = tinyfd_openFileDialog( "Load TF File", volumeDataUrlTfExtenstion.string().c_str(), 1, &filter, fileDesc, 0 );

        if (pTfFileUrl == nullptr) {
            printf( "Load TF canceled or there was an error\n" );
        }
        else {
            std::string tfFileUrl = pTfFileUrl;
            pGuiUserData->pSharedMem->put( "loadTF", tfFileUrl );
            //pGuiUserData->editTransferFunction = true;
        }
    }

    // save TF
    if ( ImGui::Button( "Save .tf File", guiButtonSize() ) && pGuiUserData->pSharedMem->get( "saveTF" ) == " " /*&& !pGuiUserData->loadFileTrigger*/ ) {
        printf( "Save TF File Dialog\n" );

        std::filesystem::path volumeDataUrl = pGuiUserData->volumeDataUrl;
        std::filesystem::path volumeDataUrlTfExtenstion = ( volumeDataUrl.empty() ) ? "" : volumeDataUrl.replace_extension( "tf" );
        const char* filter{ "*.tf" };
        const char* fileDesc = "*.tf Transfer Function"; // "Transfer Function File";
        const char* pTfFileUrl = tinyfd_saveFileDialog( "Save TF File", volumeDataUrlTfExtenstion.string().c_str(), 1, &filter, fileDesc );

        if (pTfFileUrl == nullptr) {
            printf( "Save TF canceled or there was an error\n" );
        } else {
            std::string tfFileUrl = pTfFileUrl;
            pGuiUserData->pSharedMem->put( std::string{ "saveTF" }, tfFileUrl );
        }
    }

#if 0
    int32_t gradientID = 0;
    
    const bool isMarkerShown = true;
    ImGradientHDR( gradientID, gradientState, temporaryGradientState, isMarkerShown );

    //static float col1[3] = { 1.0f, 0.0f, 0.2f };
    //ImGui::ColorEdit3( "color 1", col1, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions /*| ImGuiColorEditFlags_NoSmallPreview*/ );

    if (temporaryGradientState.selectedIndex >= 0) {
        
        ImGui::ColorEdit3( "color", 
            gradientState.GetColorMarker( temporaryGradientState.selectedIndex )->Color.data(), 
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoOptions /*| ImGuiColorEditFlags_NoSmallPreview*/ );
    }
#endif
    //ImGui::PlotHistogram
    //static float col2[4] = { 0.4f, 0.7f, 0.0f, 0.5f };
    ////ImGui::SameLine(); 
    //ImGui::ColorEdit4( "color 2", col2 );

    //static float colRGBA[3] = { 1.0f, 0.0f, 0.2f };
    //ImGui::ColorPicker4( "Color Picker RGBA", colRGBA );

    ImGui::End();
}

void DVR_GUI::Resize() {
    ImVec2 imguiWindowSize = ImGui::GetWindowSize();
    printf( "ImGui::GetWindowSize(): %f %f\n", imguiWindowSize.x, imguiWindowSize.y );
}

void DVR_GUI::DisplayGui( void* const pUserData ) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    GuiUserData_t* const pGuiUserData = reinterpret_cast<GuiUserData_t* const>(pUserData);
    pGuiUserData->wantsToCaptureMouse = ImGui::GetIO().WantCaptureMouse;

    CreateGuiLayout( pUserData );

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void DVR_GUI::DestroyGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
