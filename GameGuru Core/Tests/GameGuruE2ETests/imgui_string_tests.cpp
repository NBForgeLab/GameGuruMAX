#include "doctest.h"
#include "../../GameGuru/Imgui/imgui_cstr.h"

namespace
{
    class EditorContext
    {
    public:
        EditorContext()
        {
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = nullptr;
            io.DisplaySize = ImVec2(640, 480);
            io.DeltaTime = 1.0f / 60.0f;
            unsigned char* pixels = nullptr;
            int width = 0, height = 0;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        }
        ~EditorContext() { ImGui::DestroyContext(); }

        void Frame(cStr& text, bool focus = false)
        {
            ImGui::NewFrame();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(600, 300));
            ImGui::Begin("String editor");
            if (focus) ImGui::SetKeyboardFocusHere();
            ImGui::InputTextBounded<250>("Texture name", text, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::End();
            ImGui::EndFrame();
        }
    };
}

TEST_CASE("Production_editor_text_input_owns_growth_before_enter")
{
    EditorContext context;
    cStr name;
    context.Frame(name, true);
    context.Frame(name);
    const std::string entered(200, 'a');
    ImGui::GetIO().AddInputCharactersUTF8(entered.c_str());
    context.Frame(name);
    CHECK(name.str() == entered);
    CHECK(name.Len() == 200);
}

TEST_CASE("Production_editor_text_input_preserves_field_capacity")
{
    EditorContext context;
    cStr name;
    context.Frame(name, true);
    context.Frame(name);
    const std::string entered(400, 'b');
    ImGui::GetIO().AddInputCharactersUTF8(entered.c_str());
    context.Frame(name);
    CHECK(name.Len() == 249);
    CHECK(name.str() == std::string(249, 'b'));
}
