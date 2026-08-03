#include "eui_neo.h"

namespace app {

const DslAppConfig& dslAppConfig() {
    static const DslAppConfig config = DslAppConfig{}
        .title("Find Package Consumer")
        .pageId("find_package_consumer")
        .windowSize(320, 240)
        .iconPath("");
    return config;
}

void compose(eui::Ui& ui, const eui::Screen& screen) {
    ui.text("label")
        .size(screen.width, screen.height)
        .text("find_package")
        .build();
}

} // namespace app

int main() {
    eui::Color color{1.0f, 0.0f, 0.0f, 1.0f};
    return color.r > 0.5f ? 0 : 1;
}
