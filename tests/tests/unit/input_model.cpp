#include "components/input_model.h"

#include <iostream>
#include <string>

int main() {
    using Model = components::input_detail::InputModel;

    Model::InputState state;
    state.text = "first line\nsecond line";
    state.textRevision = 1;

    constexpr float inset = 12.0f;
    constexpr float fontSize = 17.0f;
    constexpr float width = 360.0f;
    constexpr float height = 116.0f;
    const float viewportWidth = width - inset * 2.0f;
    const float viewportHeight = height - inset * 2.0f;
    const Model::InputLayout layout = Model::InputLayout::build(
        state,
        viewportWidth,
        viewportHeight,
        width,
        inset,
        inset,
        fontSize,
        "monospace",
        fontSize,
        true);

    const core::Rect bounds{100.0f, 200.0f, width, height};
    const int firstLineCursor = layout.cursorFromPointer(
        bounds.x + inset,
        bounds.y + inset + fontSize * 0.5f,
        bounds,
        width,
        inset);
    const int secondLineCursor = layout.cursorFromPointer(
        bounds.x + inset,
        bounds.y + inset + fontSize * 1.5f,
        bounds,
        width,
        inset);

    if (firstLineCursor >= 10) {
        std::cerr << "First line click resolved past newline: " << firstLineCursor << "\n";
        return 1;
    }
    if (secondLineCursor < 11) {
        std::cerr << "Second line click did not resolve to second line: " << secondLineCursor << "\n";
        return 1;
    }

    return 0;
}
