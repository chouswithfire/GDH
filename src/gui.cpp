#define IMGUI_DEFINE_MATH_OPERATORS
#include <Geode/Geode.hpp>
#include "gui.hpp"
//#include "console.hpp"
#include "memory.hpp"
#include "hacks.hpp"
#include "hooks.hpp"
#include "replayEngine.hpp"
#include "labels.hpp"
#include "license.hpp"
#include "speedhackAudio.hpp"

bool gui::show = false;
bool gui::inited = false;
bool gui::needRescale = false;
int gui::indexScale = 3;
bool gui::license_accepted = false;
float gui::scale = 1.f;

bool gui::change_keybind = false;
int gui::keybind_key = -2;
int gui::menu_key = GLFW_KEY_TAB;

bool keybind_mode = false;

std::chrono::steady_clock::time_point animationStartTime;
bool isAnimating = false;
bool isFadingIn = false;
int anim_durr = 100;

bool gui::broken_save = false;

bool needUnloadFont = false;
std::string search_text;

void updateCursorState() {
    bool canShowInLevel = true;
    if (auto* playLayer = PlayLayer::get()) {
        canShowInLevel = playLayer->m_hasCompletedLevel ||
                         playLayer->m_isPaused ||
                         GameManager::sharedState()->getGameVariable("0024");
    }
    if (gui::show || canShowInLevel)
        PlatformToolbox::showCursor();
    else
        PlatformToolbox::hideCursor();
}

void animateAlpha(int ms)
{
    ImGuiStyle& style = ImGui::GetStyle();

    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> diff = currentTime - animationStartTime;
    float elapsed = diff.count();

    float time = ms / 1000.0f;
    if (elapsed >= time)
    {
        style.Alpha = isFadingIn ? 1.0f : 0.0f;
        isAnimating = false;

        if (!isFadingIn)
        {
            gui::show = !gui::show;
            hacks::save(hacks::windows, hacks::fileDataPath);
            updateCursorState();
        }

        return;
    }

    float t = elapsed / time;
    float alpha = isFadingIn ? 0.0f + t: 1.0f - t;
    style.Alpha = alpha;
}

void License() {
    if (!gui::inited) {
        ImGui::SetNextWindowSize({590 * gui::scale, 390 * gui::scale});
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));  
        gui::inited = true;
    }    

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20, 20});
    ImGui::Begin("Welcome", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::Text(gui::broken_save ? "Looks like the save file was corrupted" : "Welcome to GDH.");
    ImGui::PopFont();

    ImGui::Text(gui::broken_save ? "GDH settings were reset to prevent a crash" : "Please read the license agreement.");

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10, 10});
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImColor(40, 45, 51).Value);
    ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImColor(40, 45, 51).Value);
    ImGui::BeginChild("##LicenseChild", {0, ImGui::GetContentRegionAvail().y - 40 * gui::scale}, true);
    ImGui::Text("%s", license.c_str());
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static bool agree = false;
    ImGui::Checkbox("I agree to the license terms", &agree, gui::scale);
    
    ImGui::SetCursorPos({ImGui::GetWindowSize().x - 170 * gui::scale, ImGui::GetWindowSize().y - 45 * gui::scale});
    if (ImGui::Button(agree ? "Agree" : "Disagree", {150 * gui::scale, 30 * gui::scale})) {
        if (agree) {
            gui::license_accepted = true;
        } else {
            gui::Toggle();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

std::vector<std::string> stretchedWindows;
void gui::RenderMain() {   
    speedhackAudio::update();

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);
    imgui_popup::render();
    ImGui::PopStyleVar();
    
    if (isAnimating) {
        animateAlpha(anim_durr);
    }
    
    if (!gui::show) return;

    if (!gui::license_accepted) {
        License();
        return;
    }

    for (auto& win : hacks::windows) {
        std::string windowName = win.name;
        if (std::find(stretchedWindows.begin(), stretchedWindows.end(), windowName) == stretchedWindows.end())
        {
            ImVec2 windowSize = ImVec2(win.w, win.h);
            ImVec2 windowPos = ImVec2(win.x, win.y);

            if (gui::needRescale) {
                windowSize = ImVec2(win.orig_w * gui::scale, win.orig_h * gui::scale);
                windowPos = ImVec2(win.orig_x * gui::scale, win.orig_y * gui::scale);
            }

            ImGui::SetNextWindowSize(windowSize);
            ImGui::SetNextWindowPos(windowPos);

            stretchedWindows.push_back(windowName);
        }   

        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[2]);
        ImGui::Begin(windowName.c_str(), 0, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoNavInputs);
        ImGui::PopFont();

        if (!ImGui::IsWindowCollapsed()) {
            auto size = ImGui::GetWindowSize();
            auto pos = ImGui::GetWindowPos();

            win.w = size.x;
            win.h = size.y;
            win.x = pos.x;
            win.y = pos.y;
        }


        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);      

        if (windowName == "Framerate") {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - (35 + 5) * gui::scale);
            ImGui::DragFloat("##speedhack_value", &hacks::speed_value, 0.01f, 0, FLT_MAX, "Speed: %.2fx");

            ImGui::SameLine();
            ImGui::Checkbox("##speedhack_enabled", &hacks::speed_enabled, gui::scale);

            ImGui::Checkbox("Speedhack Audio", &hacks::speedhack_audio, gui::scale);

        }
        else if (windowName == "Replay Engine") {
            engine.render();
        }
        else if (windowName == "Labels") {
            ImGui::Checkbox("Custom Text", &labels::custom_text_enabled, gui::scale);

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputText("##Custom Text2", &labels::custom_text);
            
            ImGui::Checkbox("Time (24H)", &labels::time24_enabled, gui::scale);

            ImGui::SameLine();

            ImGui::Checkbox("Time (12H)", &labels::time12_enabled, gui::scale);            
            ImGui::Checkbox("Noclip Accuracy", &labels::noclip_accuracy_enabled, gui::scale);

            ImGui::Checkbox("CPS Counter", &labels::cps_counter_enabled, gui::scale);

            ImGui::Checkbox("Death Counter", &labels::death_enabled, gui::scale);
            
            const char *e[] = {"Top Left", "Bottom Left", "Top Right", "Bottom Right"};
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::Combo("##Labels Position", &labels::pos, e, 4, 4);
        }
        else if (windowName == "GDH Settings") {
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::InputTextWithHint("##Search", "Search:", &search_text);

            if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsItemActive())
                ImGui::SetWindowFocus(NULL);

            
            const char* items[] = {"25%", "50%", "75%", "100%", "125%", "150%", "175%", "200%", "250%", "300%", "400%"};
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::Combo("##Menu scale", &gui::indexScale, items, IM_ARRAYSIZE(items))) {
                gui::scale = float(atof(items[gui::indexScale])) / 100.0f;    
                gui::needRescale = true;
                ImGuiCocos::get().reload();
            }

            ImGui::Checkbox("Keybind Mode", &keybind_mode, gui::scale);

            if (keybind_mode) {
                ImGui::SameLine();
                if (ImGui::Button("More", {ImGui::GetContentRegionAvail().x, NULL})) {
                    ImGui::OpenPopup("Keybinds");
                }

                if (ImGui::BeginPopupModal("Keybinds", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                    auto renderKeyButton = [&](const std::string& label, int& key) {
                        std::string keyStr = label + hacks::GetKeyName(key);
                        if (key == -1) {
                            keyStr = "Press any key...";
                            if (gui::keybind_key != -2) {
                                key = gui::keybind_key;
                                gui::keybind_key = -2;
                            }
                        }
                        if (ImGui::Button(keyStr.c_str(), {400 * gui::scale, 0})) {
                            key = -1;
                            change_keybind = true;
                        }
                    };

                    ImGui::Text("Tip: To disable the bind, bind it to backspace");
                    ImGui::Separator();
                    ImGui::Spacing();

                    renderKeyButton("Menu Key: ", gui::menu_key);
                    renderKeyButton("Speedhack Key: ", hacks::speed_key);
                    renderKeyButton("Disable/Playback Macro: ", hacks::playback_key);
                    renderKeyButton("Startpos Switcher Left: ", startpos_switcher::left_key);
                    renderKeyButton("Startpos Switcher Right: ", startpos_switcher::right_key);
                    renderKeyButton("Enable Frame Advance + Next Frame: ", hacks::frame_advance_key);
                    renderKeyButton("Disable Frame Advance: ", hacks::frame_advance_disable_key);

                    if (ImGui::Button("Close", {ImGui::GetContentRegionAvail().x, 0})) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
        }
        else {        
            for (auto& hck : win.hacks) {
                std::string search_name = hck.name;
                std::string search_item = search_text;
                
                std::transform(search_item.begin(), search_item.end(), search_item.begin(), ::tolower);
                std::transform(search_name.begin(), search_name.end(), search_name.begin(), ::tolower);

                bool founded = search_item.empty() ? true : (search_name.find(search_item) != std::string::npos);
                ImGui::PushStyleColor(ImGuiCol_Text, founded ? ImColor(255, 255, 255).Value : ImColor(64, 64, 64).Value);

                if (!keybind_mode) {
                    if (ImGui::Checkbox(hck.name.c_str(), &hck.enabled, gui::scale)) {
                        if (hck.name == "Unlock Items") { hacks::unlock_items = hck.enabled; }
                        else if (hck.name == "Noclip") { hacks::nolcip_enabled = hck.enabled; }
                        else if (hck.name == "Auto Sond Download") { hacks::auto_song_download = hck.enabled; }
                        else if (hck.name == "Jump Hack") { hacks::jump_hack = hck.enabled; }
                        else if (hck.name == "Ignore ESC") { hacks::ignore_esc = hck.enabled; }
                        else if (hck.name == "Startpos Switcher") { hacks::startpos_switcher = hck.enabled; }
                        else if (hck.name == "Reset Camera") { hacks::startpos_switcher_reset_camera = hck.enabled; }
                        else if (hck.name == "Instant Complete") { hacks::instant_complate = hck.enabled; }
                        else if (hck.name == "RGB Icons") { hacks::rgb_icons = hck.enabled; }
                        else if (hck.name == "Wave Trail Size") { hacks::wave_trail = hck.enabled; }
                        else if (hck.name == "Random Seed") { hacks::random_seed_enabled = hck.enabled; }
                        else if (hck.name == "Show Hitboxes") { 
                            hacks::show_hitboxes = hck.enabled;
                            auto pl = PlayLayer::get();
                            if (pl && !hck.enabled && !(pl->m_isPracticeMode && GameManager::get()->getGameVariable("0166"))) {
                                pl->m_debugDrawNode->setVisible(false);
                            }
                        }
                        else if (hck.name == "Hide Pause Menu") { 
                            auto pl = PlayLayer::get();
                            hacks::hide_menu = hck.enabled; 
                            if (pl && pl->m_isPaused && hooks::pauseLayer != nullptr)
                                hooks::pauseLayer->setVisible(!hck.enabled);
                        }
                        else if (hck.name == "Auto Pickup Coins") { hacks::auto_pickup_coins = hck.enabled; }
                        else if (hck.name == "Respawn Time") { hacks::respawn_time_enabled = hck.enabled; }
                        else if (hck.name == "Restart Level") { 
                            auto pl = PlayLayer::get();
                            if (pl)
                                pl->resetLevel();
                        }
                        else if (hck.name == "Practice Mode") { 
                            auto pl = PlayLayer::get();
                            if (pl)
                                pl->togglePracticeMode(!pl->m_isPracticeMode);
                        }
                        else {
                            for (auto& opc : hck.opcodes) {
                                std::string bytesStr = hck.enabled ? opc.on : opc.off;
                                if (opc.address != 0) {
                                    uintptr_t base = (uintptr_t)GetModuleHandleA(0);
                                    if (!opc.module.empty())
                                    {
                                        base = (uintptr_t)GetModuleHandleA(opc.module.c_str());
                                    }
                                    memory::WriteBytes(base + opc.address, bytesStr);
                                }
                            }
                        }
                    }
                }
                else {
                    std::string keybind_with_hack = hck.name + ": " + hacks::GetKeyName(hck.keybind);
                    if (hck.keybind == -1) {
                        keybind_with_hack = "Press any key...";
                        if (gui::keybind_key != -2) {
                            hck.keybind = gui::keybind_key;
                            gui::keybind_key = -2;
                        }
                    }

                    if (ImGui::Button(keybind_with_hack.c_str(), {ImGui::GetContentRegionAvail().x, NULL})) {
                        hck.keybind = (hck.keybind == -1) ? 0 : -1;
                        change_keybind = true;
                    }
                }

                ImGui::PopStyleColor();

                if (ImGui::IsItemHovered() && !hck.desc.empty())
                    ImGui::SetTooltip("%s", hck.desc.c_str());
                
                if (hck.name == "RGB Icons") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##rainbowSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Rainbow Icon Settings");
                    }

                    if (ImGui::BeginPopupModal("Rainbow Icon Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::DragFloat("##riconBrightness", &hacks::ricon_brightness, 0.01f, 0.0f, 1.0f, "Brightness: %.2f");
                        ImGui::DragFloat("##riconSaturation", &hacks::ricon_saturation, 0.01f, 0.0f, 1.0f, "Saturation: %.2f");

                        ImGui::DragFloat("##riconCoef", &hacks::ricon_coef, 0.01f, 0.0f, 10.0f, "Speed Coefficent: %.2f");
                        ImGui::DragFloat("##riconShift", &hacks::ricon_shift, 0.01f, 0.0f, 1.0f, "Secondary Color Shift: %.2f");

                        if (ImGui::Button("Close", {ImGui::GetContentRegionAvail().x, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (hck.name == "Respawn Time") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##respawnTimeSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Respawn Time Settings");
                    }

                    if (ImGui::BeginPopupModal("Respawn Time Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::DragFloat("##respawnTime", &hacks::respawn_time_value, 0.01f, 0.0f, FLT_MAX, "Respawn Time: %.2f");

                        if (ImGui::Button("Close", {ImGui::GetContentRegionAvail().x, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (hck.name == "Show Hitboxes") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("#hitboxesSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Hitboxes Settings");
                    }

                    if (ImGui::BeginPopupModal("Hitboxes Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Checkbox("Draw Trail", &hacks::draw_trail, gui::scale);
                        ImGui::DragInt("##trail_length", &hacks::trail_length, 1, 0, INT_MAX, "Trail Length: %i");
                        //ImGui::Checkbox("Show Hitboxes on Death", &hacks::show_hitboxes_on_death);
                        if (ImGui::Button("Close", {ImGui::GetContentRegionAvail().x, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (hck.name == "Wave Trail Size") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##waveTrailSizeSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Wave Trail Size Settings");
                    }

                    if (ImGui::BeginPopupModal("Wave Trail Size Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::DragFloat("##waveTrailSize", &hacks::wave_trail_size, 0.01f, 0.0f, FLT_MAX, "Wave Trail Size: %.2f");

                        if (ImGui::Button("Close", {ImGui::GetContentRegionAvail().x, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (hck.name == "Noclip") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##noclipSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Noclip Settings");
                    }

                    if (ImGui::BeginPopupModal("Noclip Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::Checkbox("Player 1", &hacks::noclip_p1, gui::scale);
                        ImGui::SameLine();
                        ImGui::Checkbox("Player 2", &hacks::noclip_p2, gui::scale);

                        ImGui::Checkbox("Tint on death", &hacks::tint_on_death, gui::scale);

                        if (ImGui::Button("Close", {400 * gui::scale, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }

                if (hck.name == "Random Seed") {
                    ImGui::SameLine();
                    if (ImGui::ArrowButton("##randomSeedSettings", ImGuiDir_Right)) {
                        ImGui::OpenPopup("Random Seed Settings");
                    }

                    if (ImGui::BeginPopupModal("Random Seed Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::DragInt("##seed", &hacks::seed_value, 1, 0, INT_MAX, "Seed Value: %i");

                        if (ImGui::Button("Close", {400 * gui::scale, NULL})) {
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
            }
        }

        ImGui::PopFont();
        ImGui::End();
    }
}

void gui::toggleKeybinds(int key) {
    auto pl = PlayLayer::get();
    if (!pl) return;

    if (engine.mode == state::record || engine.mode == state::play) {
        if (key == hacks::frame_advance_key) {
            if (!engine.frame_advance)
                imgui_popup::add_popup("Frame Advance enabled");

            engine.frame_advance = true;
            engine.next_frame = true;
        } 
        else if (key == hacks::frame_advance_disable_key) {
            if (engine.frame_advance)
                imgui_popup::add_popup("Frame Advance disabled");

            engine.frame_advance = false;
        }
    }

    for (auto& win : hacks::windows) {
        for (auto& hck : win.hacks) {
            if (hck.keybind == key && key != 0) {                
                hck.enabled = !hck.enabled;

                if (hck.name == "Unlock Items") { hacks::unlock_items = hck.enabled; }
                else if (hck.name == "Noclip") { hacks::nolcip_enabled = hck.enabled; }
                else if (hck.name == "Auto Sond Download") { hacks::auto_song_download = hck.enabled; }
                else if (hck.name == "Jump Hack") { hacks::jump_hack = hck.enabled; }
                else if (hck.name == "Ignore ESC") { hacks::ignore_esc = hck.enabled; }
                else if (hck.name == "Startpos Switcher") { hacks::startpos_switcher = hck.enabled; }
                else if (hck.name == "Reset Camera") { hacks::startpos_switcher_reset_camera = hck.enabled; }
                else if (hck.name == "Instant Complete") { hacks::instant_complate = hck.enabled; }
                else if (hck.name == "RGB Icons") { hacks::rgb_icons = hck.enabled; }
                else if (hck.name == "Wave Trail Size") { hacks::wave_trail = hck.enabled; }
                else if (hck.name == "Random Seed") { hacks::random_seed_enabled = hck.enabled; }
                else if (hck.name == "Show Hitboxes") { 
                    hacks::show_hitboxes = hck.enabled;
                    auto pl = PlayLayer::get();
                    if (pl && !hck.enabled && !(pl->m_isPracticeMode && GameManager::get()->getGameVariable("0166"))) {
                        pl->m_debugDrawNode->setVisible(false);
                    }
                }
                else if (hck.name == "Hide Pause Menu") { 
                    auto pl = PlayLayer::get();
                    hacks::hide_menu = hck.enabled; 
                    if (pl && pl->m_isPaused && hooks::pauseLayer != nullptr)
                        hooks::pauseLayer->setVisible(!hck.enabled);
                }
                else if (hck.name == "Auto Pickup Coins") { hacks::auto_pickup_coins = hck.enabled; }
                else if (hck.name == "Respawn Time") { hacks::respawn_time_enabled = hck.enabled; }
                else if (hck.name == "Restart Level") { 
                    auto pl = PlayLayer::get();
                    if (pl)
                        pl->resetLevel();
                }
                else if (hck.name == "Practice Mode") { 
                    auto pl = PlayLayer::get();
                    if (pl)
                        pl->togglePracticeMode(!pl->m_isPracticeMode);
                }
                else {                    
                    for (auto& opc : hck.opcodes) {
                        std::string bytesStr = hck.enabled ? opc.on : opc.off;
                        if (opc.address != 0) {
                            uintptr_t base = (uintptr_t)GetModuleHandleA(0);
                            if (!opc.module.empty())
                            {
                                base = (uintptr_t)GetModuleHandleA(opc.module.c_str());
                            }
                            memory::WriteBytes(base + opc.address, bytesStr);
                        }
                    }
                }
            }
        }
    }

    hacks::save(hacks::windows, hacks::fileDataPath);
}

void gui::Toggle() {
    if (!isAnimating)
    {
        isAnimating = true;
        isFadingIn = !isFadingIn;
        animationStartTime = std::chrono::steady_clock::now();

        if (isFadingIn)
        {
            gui::show = !gui::show;  
            updateCursorState();        
        }
    }
}

void gui::Unload() {
    stretchedWindows.clear();
}

void CustomRoundedProgressBar(float fraction, ImVec2 pos, ImVec2 size, float rounding, ImU32 bgColor, ImU32 fillColor) {
    ImGui::GetWindowDrawList()->AddRectFilled(
        pos,
        pos + size,
        bgColor,
        rounding
    );

    ImVec2 fillSize = ImVec2(size.x * fraction, size.y);
    ImGui::GetWindowDrawList()->AddRectFilled(
        pos,
        pos + fillSize,
        fillColor,
        rounding
    );
}
namespace imgui_popup {
    std::vector<popup_message> messages;
    void add_popup(std::string caption) {
        messages.push_back({caption, ImGui::GetTime() + popupDuration});
    }

    void render() {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float currentY = 10.0f;
        float elapsedTime = 0.0f;

        for (auto& message : messages) {
            if (ImGui::GetTime() > message.expiry_time) {
                continue;
            }

            std::string windowName = "PopupWindow##" + std::to_string(reinterpret_cast<std::uintptr_t>(&message));

            ImGuiStyle& style = ImGui::GetStyle();

            ImGui::SetNextWindowPos(ImVec2(displaySize.x - ((ImGui::CalcTextSize(message.caption.c_str()).x + style.WindowPadding.x * 2) * gui::scale) - 10.0f, currentY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2((ImGui::CalcTextSize(message.caption.c_str()).x + style.WindowPadding.x * 2) * gui::scale, (40.0f * gui::scale)), ImGuiCond_Always);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
            ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
            ImGui::PopStyleVar();

            float timeRemaining = message.expiry_time - ImGui::GetTime();
            float progress = timeRemaining / popupDuration;
            CustomRoundedProgressBar(progress, ImGui::GetWindowPos(), ImGui::GetWindowSize(), ImGui::GetStyle().WindowRounding, ImColor(27, 27, 29, 250), ImColor(47, 47, 49, 250));

            std::string popupName = "Popup##" + std::to_string(reinterpret_cast<std::uintptr_t>(&message));

            ImGui::BeginChild(popupName.c_str(), {0, 0}, ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);       
            ImGui::Text("%s", message.caption.c_str());
            ImGui::EndChild();
            
            ImGui::End();

            currentY += (40.0f * gui::scale) + (10.0f * gui::scale);
        }

        messages.erase(std::remove_if(messages.begin(), messages.end(), [](const popup_message& msg) {
            return ImGui::GetTime() > msg.expiry_time;
        }), messages.end());
    }
}
