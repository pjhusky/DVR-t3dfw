#include "DVR_GUI.h"

#include "gfxAPI/contextOpenGL.h"

#include "../sharedMemIPC.h"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>

#include "imGuIZMO.quat/imGuIZMO.quat/imGuIZMOquat.h"

#include "external/libtinyfiledialogs/tinyfiledialogs.h"

#include <nfd.h>

#include <array>
#include <filesystem>

namespace {

    static float guiScale = 1.0f;
    static auto guiButtonSize() { return ImVec2{ 360, 40 }; }

    static const char *const lightingWindowName = "Lighting Menu";
    static const char* const controlsWindowName = "Main Menu";

    static std::vector< const char* > visAlgoNames{
        "Levoy Isosurfaces",
        "F2B Compositing",
        "X-Ray",
        "MRI",
    };

    static std::vector< const char* > rayMarchAlgoNames{
        "Raster Cube Backfaces",
        "Intersect Ray",
    };

    static std::vector< const char* > gradientAlgoNames{
        "Central Differences", 
        "Sobel 3D",
    };

    // https://github.com/libigl/libigl/issues/1300
    static std::string labelPrefix(const char* const label)
    {
        float width = ImGui::CalcItemWidth();

        float x = ImGui::GetCursorPosX();
        ImGui::Text(label); 
        ImGui::SameLine(); 
        ImGui::SetCursorPosX(x + width * 0.5f + ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::SetNextItemWidth(-1);

        std::string labelID = "##";
        labelID += label;

        return labelID;
    }

    static void separatorWithVpadding( float vPadding = 0.125f ) {
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() * vPadding));
        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight() * vPadding));
    }
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

    { // finer tessellation for light-dir arrow
        imguiGizmo::coneSlices = 12;
    }

    ImGui::GetStyle().WindowRounding = 0.0f;
    ImVec4* colors = ImGui::GetStyle().Colors;
    //colors[ImGuiCol_WindowBg] = {134.0f/255.0f, 240.0f/255.0f, 238.0f/255.0f, 1.0f};
    //colors[ImGuiCol_WindowBg] = {10.0f/255.0f, 40.0f/255.0f, 50.0f/255.0f, 0.5f};
    colors[ImGuiCol_WindowBg] = {10.0f/255.0f, 40.0f/255.0f, 50.0f/255.0f, 0.75f};
    //colors[ImGuiCol_WindowBg] = {0.0f, 0.0f, 0.0f, 0.0f};
}

void DVR_GUI::CreateGuiLayout( void* const pUserData, const int32_t fbW, const int32_t fbH )
{
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
    
    //ImGui::Bg

    const ImVec2 imGuiWindowSize = ImGui::GetWindowSize();
    //printf( "   imGuiWindowSize = %f %f\n", imGuiWindowSize.x, imGuiWindowSize.y );
    
    const float imguiWindowDpiScale = ImGui::GetWindowDpiScale();
    //printf( "   imguiWindowDpiScale = %f\n", imguiWindowDpiScale );

    MainMenuGui( pGuiUserData, fbW, fbH );
    LightMenuGui( pGuiUserData, fbW, fbH );
    StatsMenuGui( pGuiUserData, fbW, fbH );
}

void DVR_GUI::MainMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH )
{
    ImGui::SetNextWindowPos( ImVec2{ static_cast<float>(fbW-215*ImGui::GetWindowDpiScale()), 20.0f }, ImGuiCond_FirstUseEver );

    if (ImGui::Begin( controlsWindowName, nullptr, ImGuiWindowFlags_AlwaysAutoResize )) {
        //ImGui::Text( "Loaded .dat file:\n%s", pGuiUserData->volumeDataUrl.c_str() );
        ImGui::Text( "Loaded .dat file:\n%s", std::filesystem::path( pGuiUserData->volumeDataUrl ).filename().string().c_str() );

        if (ImGui::Button( "Load .dat File", guiButtonSize() ) && !pGuiUserData->loadFileTrigger) {

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

        separatorWithVpadding();

        if (ImGui::Button( "Reset Transformations", guiButtonSize() ) && !pGuiUserData->resetTrafos) {
            printf( "reset trafos button clicked\n" );
            pGuiUserData->resetTrafos = true;
        }

        if (*(pGuiUserData->pGradientModeIdx) >= 0) {
            separatorWithVpadding();

            ImGui::Text( "Gradient Calculation Method" );
            static const char* current_item = gradientAlgoNames[*(pGuiUserData->pGradientModeIdx)];

            for (int32_t i = 0; i < gradientAlgoNames.size(); i++) {
                ImGui::RadioButton( gradientAlgoNames[i], pGuiUserData->pGradientModeIdx, i );
            }
        }


        { // combo box
            separatorWithVpadding();

            ImGui::Text( "Volume Intersection Method" );

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

            //LightMenuGui( pGuiUserData );

            separatorWithVpadding();
            ImGui::Text( "Render Algorithm" );

            for (int32_t i = 0; i < visAlgoNames.size(); i++) {
                ImGui::RadioButton( visAlgoNames[i], pGuiUserData->pVisAlgoIdx, i );
            }

            if ( *pGuiUserData->pVisAlgoIdx == static_cast<int>(DVR_GUI::eVisAlgo::levoyIsosurface) ) {
                const float capturedWidth = guiButtonSize().x - ImGui::CalcTextSize( "Iso Value", NULL, true ).x - ImGui::GetCursorPosX();

                ImGui::BeginGroup();
                ImGui::SetNextItemWidth( capturedWidth );
                ImGui::SliderFloat( "Iso Value", &pGuiUserData->surfaceIsoAndThickness[0], 0.0f, 1.0f );
                ImGui::SameLine();
                ImGui::ColorEdit3( "", pGuiUserData->surfaceIsoColor.data(), ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoInputs );
                ImGui::EndGroup();
                ImGui::SetNextItemWidth( capturedWidth );
                ImGui::SliderFloat( "Thickness", &pGuiUserData->surfaceIsoAndThickness[1], 0.0f, 100.0f / 4096.0f, "%.5f" );
            }
        }

        separatorWithVpadding();

        pGuiUserData->editTransferFunction = ImGui::Button( "Edit Transfer Function", guiButtonSize() );

        // load TF

        if (ImGui::Button( "Load .tf File", guiButtonSize() ) && pGuiUserData->pSharedMem->get( "loadTF" ) == " " /*&& !pGuiUserData->loadFileTrigger*/) {
            printf( "Open TF File Dialog\n" );


            std::filesystem::path volumeDataUrl = pGuiUserData->volumeDataUrl;
            std::filesystem::path volumeDataUrlTfExtenstion = (volumeDataUrl.empty()) ? "" : volumeDataUrl.replace_extension( "tf" );
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
        if (ImGui::Button( "Save .tf File", guiButtonSize() ) && pGuiUserData->pSharedMem->get( "saveTF" ) == " " /*&& !pGuiUserData->loadFileTrigger*/) {
            printf( "Save TF File Dialog\n" );

            std::filesystem::path volumeDataUrl = pGuiUserData->volumeDataUrl;
            std::filesystem::path volumeDataUrlTfExtenstion = (volumeDataUrl.empty()) ? "" : volumeDataUrl.replace_extension( "tf" );
            const char* filter{ "*.tf" };
            const char* fileDesc = "*.tf Transfer Function"; // "Transfer Function File";
            const char* pTfFileUrl = tinyfd_saveFileDialog( "Save TF File", volumeDataUrlTfExtenstion.string().c_str(), 1, &filter, fileDesc );

            if (pTfFileUrl == nullptr) {
                printf( "Save TF canceled or there was an error\n" );
            }
            else {
                std::string tfFileUrl = pTfFileUrl;
                pGuiUserData->pSharedMem->put( std::string{ "saveTF" }, tfFileUrl );
            }
        }

    }
    ImGui::End();
}

void DVR_GUI::LightMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH ) {
    //separatorWithVpadding();
    ImGui::SetNextWindowPos( ImVec2{ 10.0f, static_cast<float>(fbH-120*ImGui::GetWindowDpiScale()) }, ImGuiCond_FirstUseEver );
    if (ImGui::Begin( lightingWindowName, nullptr, ImGuiWindowFlags_AlwaysAutoResize )) {

        //    static bool firstRunDofGuiParams = true;
        //    if (firstRunDofGuiParams) { ImGui::SetNextItemOpen( true ); firstRunDofGuiParams = false; }
        //    if (ImGui::TreeNode( "Light Parameters" )) {

        //const std::array<float, 4> dir3{1.0f,0.0f,0.0f,0.0f};
        static vgm::Vec3* dir3 = reinterpret_cast<vgm::Vec3*>( pGuiUserData->lightParams.lightDir.data() );
        *dir3 = vgm::normalize( *dir3 );
        //vgm::Vec3 tmpDir3 = dir3;
        //const float widgetSize = 180;
        //const float widget3dSize = ImGui::GetTextLineHeight() * 5.5f;
        const float widget3dSize = ImGui::GetTextLineHeight() * 7.0f;

        //ImGui::GetTextLineHeightWithSpacing();
        //ImGui::SetNextItemWidth( 420 );

        const float step = 0.01f;
        //ImGui::DragFloat3( "Light Dir", &tmpDir3.x, step, ImGuiSliderFlags_NoInput );
        //ImGui::DragFloat3( labelPrefix( "Light Dir" ).c_str(), &dir3.x, step, ImGuiSliderFlags_NoInput );
        if (ImGui::DragFloat3( "Direction", &dir3->x, step, -1.0f, 1.0f, "%.2f", ImGuiSliderFlags_NoInput )) {
            pGuiUserData->lightParamsChanged = true;
        }
        //const auto capturedWidth = ImGui::GetItemRectSize().x;

        ImGui::SameLine();
        //const auto capturedWidth = ImGui::GetWindowContentRegionWidth();

        //ImFont::DisplayOffset.y = 0;

        // ImGui::gizmo3D( std::string( "##" ).append( "light Direction" ).c_str(), tmpDir3, widgetSize, imguiGizmo::modeDirection );
        // if (!std::isinf(tmpDir3.x) && !std::isnan(tmpDir3.x) &&
        //     !std::isinf(tmpDir3.y) && !std::isnan(tmpDir3.y) &&
        //     !std::isinf(tmpDir3.z) && !std::isnan(tmpDir3.z) ) { dir3 = tmpDir3; }
        // else { printf("ouch!!!\n"); }
        //const auto capturedWidth2 = ImGui::CalcItemWidth();
        if (ImGui::gizmo3D( std::string( "##" ).append( "Direction" ).c_str(), *dir3, widget3dSize, imguiGizmo::modeDirection )) {
            pGuiUserData->lightParamsChanged = true;
        }
        //const auto capturedWidth2 = ImGui::GetItemRectSize().x;


        auto guiCursorPosY = ImGui::GetCursorPosY();
        auto guiCursorPosYOff = guiCursorPosY;
        //guiCursorPosYOff -= ImGui::GetTextLineHeight()*4.2f;
        guiCursorPosYOff -= ImGui::GetTextLineHeight() * 5.7f;
        ImGui::SetCursorPosY( guiCursorPosYOff );

        //static std::array<float, 3> lightColor{ .5f, 0.5f, 0.5f };
        //ImGui::ColorEdit3( "Diffuse Color", diffColor.data(), ImGuiColorEditFlags_NoPicker );
        //ImGui::ColorEdit3( "Diffuse Color", diffColor.data(), ImGuiColorEditFlags_NoInputs );
        //ImGui::ColorEdit3( "Ambient ", diffColor.data() );
        //ImGui::ColorEdit3( "Diffuse ", diffColor.data() );
        //ImGui::ColorEdit3( "Specular", diffColor.data() );

        if (ImGui::ColorEdit3( "Color", pGuiUserData->lightParams.lightColor.data() )) {
            pGuiUserData->lightParamsChanged = true;
        }

        guiCursorPosY = ImGui::GetCursorPosY();
        guiCursorPosYOff = guiCursorPosY;
        guiCursorPosYOff += ImGui::GetTextLineHeight() * 0.2f;
        ImGui::SetCursorPosY( guiCursorPosYOff );

        //ImGui::SetNextItemWidth( capturedWidth - capturedWidth2 );
        //ImGui::SetNextItemWidth( capturedWidth );
        //static bool firstRunDofGuiParams = true;
        //if ( firstRunDofGuiParams ) { ImGui::SetNextItemOpen(true); firstRunDofGuiParams = false; }        
        //if (ImGui::TreeNode( "Intensities" )) {


        const float capturedWidth = guiButtonSize().x - ImGui::CalcTextSize( "Ambient ", NULL, true ).x - ImGui::GetCursorPosX();
        //ImGui::BeginGroup();
        //ImGui::Text( "Intensities" );

        //ImGui::SetNextItemWidth( capturedWidth - capturedWidth2 );
        ImGui::SetNextItemWidth( capturedWidth );
        //ImGui::SliderFloat( labelPrefix( "Ambient" ).c_str(), &ambient, 0.0f, 1.0f, "%.2f" );
        if (ImGui::SliderFloat( "Ambient ", &pGuiUserData->lightParams.ambient, 0.0f, 1.0f, "%.2f" )) {
            pGuiUserData->lightParamsChanged = true;
        }

        ImGui::SetNextItemWidth( capturedWidth );
        if (ImGui::SliderFloat( "Diffuse ", &pGuiUserData->lightParams.diffuse, 0.0f, 1.0f, "%.2f" )) {
            pGuiUserData->lightParamsChanged = true;
        }

        ImGui::SetNextItemWidth( capturedWidth );
        if (ImGui::SliderFloat( "Specular", &pGuiUserData->lightParams.specular, 0.0f, 1.0f, "%.2f" )) {
            pGuiUserData->lightParamsChanged = true;
        }
        //ImGui::EndGroup();
        //ImGui::TreePop();
        //}

        //ImGui::TreePop();
        //}
    }
    ImGui::End();
}

void DVR_GUI::StatsMenuGui( DVR_GUI::GuiUserData_t* const pGuiUserData, const int32_t fbW, const int32_t fbH ) {
    ImGui::SetNextWindowPos( ImVec2{ 10.0f, 20.0f }, ImGuiCond_FirstUseEver );
    if (ImGui::Begin( "Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize )) {
        ImGui::Text( "FPS: %6.2f", pGuiUserData->frameRate );
    }
    ImGui::End();
}

void DVR_GUI::DisplayGui( void* const pUserData, std::vector<bool>& collapsedState, const int32_t fbW, const int32_t fbH ) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    GuiUserData_t* const pGuiUserData = reinterpret_cast<GuiUserData_t* const>(pUserData);
    pGuiUserData->wantsToCaptureMouse = ImGui::GetIO().WantCaptureMouse;

    //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1,0,0,0));

    CreateGuiLayout( pUserData, fbW, fbH );

    //ImGui::PopStyleColor(1);

    const auto afterCollapsedControls = ImGui::FindWindowByName( controlsWindowName )->Collapsed;
    collapsedState.push_back(afterCollapsedControls);
    const auto afterCollapsedLighting = ImGui::FindWindowByName( lightingWindowName )->Collapsed;
    collapsedState.push_back(afterCollapsedLighting);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void DVR_GUI::DestroyGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
