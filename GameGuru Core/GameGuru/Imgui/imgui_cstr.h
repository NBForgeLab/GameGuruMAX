#pragma once

#include "imgui.h"
#include "../Include/cStr.h"
#include <algorithm>
#include <array>

namespace ImGui
{
    // Bridge bounded editor fields to owning strings. ImGui updates the scratch
    // buffer on every edit even when EnterReturnsTrue only reports submission.
    template<size_t Capacity>
    bool InputTextBounded(const char* label, cStr& value, ImGuiInputTextFlags flags = 0)
    {
        static_assert(Capacity > 0, "Text input needs space for its terminator");
        std::array<char, Capacity> buffer{};
        const size_t length = (std::min)(value.size(), Capacity - 1);
        std::copy_n(value.c_str(), length, buffer.data());
        const bool submitted = InputText(label, buffer.data(), buffer.size(), flags);
        value = buffer.data();
        return submitted;
    }
}
