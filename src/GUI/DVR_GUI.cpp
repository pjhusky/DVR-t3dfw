#include "DVR_GUI.h"
#include "gfxAPI/contextOpenGL.h"

//#include "volumeData.h"
//#include "texture.h"
//#include "gfxUtils.h"


#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include <nfd.h>

using namespace GfxAPI;

namespace {
    static float guiScale = 1.0f;
    static auto guiButtonSize() { return ImVec2{ 400, 40 }; }
}

std::vector< const char* > DVR_GUI::rayMarchAlgoNames;

void DVR_GUI::InitGui( const ContextOpenGL& contextOpenGL )
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();

    //ImGui_ImplWin32_EnableDpiAwareness();
    ImGui_ImplGlfw_InitForOpenGL( reinterpret_cast<GLFWwindow*>(contextOpenGL.window()), true );
    ImGui_ImplOpenGL3_Init( "#version 330" );

    DVR_GUI::rayMarchAlgoNames = std::vector< const char* >{
        "eRayMarchAlgo::backfaceCubeRaster",
            "eRayMarchAlgo::fullscreenBoxIsect",
    };
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
        printf( "Open File Dialog\n" );
        nfdchar_t* outPath = NULL;
        nfdresult_t result = NFD_OpenDialog( "dat", NULL, &outPath );

        if (result == NFD_OKAY) {
            puts( "Success!" );
            puts( outPath );

            pGuiUserData->volumeDataUrl = std::string( outPath );
            pGuiUserData->loadFileTrigger = true;
            free( outPath );
        }
        else if (result == NFD_CANCEL) {
            puts( "User pressed cancel." );
        }
        else {
            printf( "Error: %s\n", NFD_GetError() );
        }
    }

    if (ImGui::Button( "Reset Transformations", guiButtonSize() ) && !pGuiUserData->resetTrafos ) {
        printf( "reset trafos button clicked\n" );
        pGuiUserData->resetTrafos = true;
    }

    //if (ImGui::ListBox( "RenderMethod", pGuiUserData->pRayMarchAlgoIdx, &rayMarchAlgoNames[0], rayMarchAlgoNames.size() )) {
    //    printf( "RenderMethod selection - nr. %d, '%s'\n", *(pGuiUserData->pRayMarchAlgoIdx), rayMarchAlgoNames[*(pGuiUserData->pRayMarchAlgoIdx)] );
    //}


    { // combo box
        ImGui::Text( "Raymarch Method:" );

        //const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "PPPP", "QQQQQQQQQQ", "RRR", "SSSS" };
        static const char* current_item = rayMarchAlgoNames[*(pGuiUserData->pRayMarchAlgoIdx)];

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
    }

    ImGui::End();
}

void DVR_GUI::Resize() {
    ImVec2 imguiWindowSize = ImGui::GetWindowSize();
    printf( "ImGui::GetWindowSize(): %f %f\n", imguiWindowSize.x, imguiWindowSize.y );
}

void DVR_GUI::DisplayGui( void* const pUserData )
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    CreateGuiLayout( pUserData );

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void DVR_GUI::DestroyGui()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
