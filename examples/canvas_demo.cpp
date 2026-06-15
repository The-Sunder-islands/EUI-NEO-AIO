#include "eui_neo.h"

#include <cmath>
#include <functional>
#include <string>

namespace app {
namespace {

constexpr eui::Color kBg{0.08f, 0.08f, 0.10f, 1.0f};
constexpr eui::Color kAccent{0.30f, 0.60f, 1.0f, 1.0f};
constexpr eui::Color kRed{0.95f, 0.30f, 0.35f, 1.0f};
constexpr eui::Color kGreen{0.30f, 0.85f, 0.45f, 1.0f};
constexpr eui::Color kYellow{0.95f, 0.80f, 0.20f, 1.0f};
constexpr eui::Color kOrange{0.95f, 0.55f, 0.15f, 1.0f};
constexpr eui::Color kPurple{0.65f, 0.40f, 0.95f, 1.0f};
constexpr eui::Color kMuted{0.45f, 0.45f, 0.50f, 1.0f};

float tick = 0;

void drawDemo(eui::CanvasContext& ctx, float w, float h) {
    // Background
    ctx.fillRect(0, 0, w, h, kBg);

    // --- Basic shapes (top-left quadrant) ---
    ctx.fillRect(20, 20, 60, 40, kAccent);
    ctx.strokeRect(100, 20, 60, 40, kGreen, 2.0f);
    ctx.fillCircle(50, 110, 22, kRed);
    ctx.strokeCircle(50, 110, 22, kYellow, 2.5f);
    ctx.fillCircle(140, 110, 22, kOrange);
    ctx.strokeCircle(140, 110, 22, kGreen, 2.0f);
    ctx.fillRoundedRect(20, 155, 110, 35, 10.0f, kPurple);

    // Lines
    ctx.drawLine(20, 210, 180, 210, kAccent, 3.0f);
    ctx.drawLine(20, 220, 120, 240, kYellow, 2.0f);
    ctx.drawLine(20, 230, 80, 270, kRed, 1.5f);

    // --- Pattern Lock Grid (right side) ---
    const float cx = 380;
    const float cy = 130;
    const float spacing = 42.0f;

    for (int row = -1; row <= 1; ++row) {
        for (int col = -1; col <= 1; ++col) {
            float x = cx + col * spacing;
            float y = cy + row * spacing;
            ctx.strokeCircle(x, y, 15, kMuted, 2.5f);
            ctx.fillCircle(x, y, 3, kAccent);
        }
    }

    // --- Transform (right side, bottom) ---
    ctx.save();
    ctx.translate(380, 310);
    for (int i = 0; i < 8; ++i) {
        ctx.rotate(0.785f);
        ctx.strokeRect(-18, -18, 36, 36, kRed, 1.0f + i * 0.3f);
    }
    ctx.restore();

    // --- Animation (bottom-left) ---
    // Pulsing circle
    float pulse = 0.5f + 0.5f * std::sin(tick * 2.0f);
    float radius = 10.0f + pulse * 10.0f;
    ctx.fillCircle(50, 340, radius, {1.0f, 1.0f, 1.0f, 0.25f + pulse * 0.55f});
    ctx.strokeCircle(50, 340, radius, kAccent, 2.0f);

    // Rotating line
    ctx.save();
    ctx.translate(120, 340);
    ctx.rotate(tick);
    ctx.drawLine(0, 0, 35, 0, kGreen, 3.0f);
    ctx.restore();

    // Bouncing ball
    float bounceY = 340.0f + std::abs(std::sin(tick * 3.5f)) * 40.0f;
    ctx.fillCircle(190, bounceY, 10, kOrange);
}

} // namespace

const DslAppConfig& dslAppConfig() {
    static DslAppConfig config = DslAppConfig{}
        .title("EUI-NEO Canvas Demo")
        .pageId("canvas_demo")
        .clearColor(kBg)
        .windowSize(480, 430)
        .fps(60.0);
    return config;
}

void compose(eui::Ui& ui, const eui::Screen& screen) {
    tick += 0.016f;

    ui.canvas("main")
        .size(screen.width, screen.height)
        .onDraw([&](eui::CanvasContext& ctx) {
            drawDemo(ctx, screen.width, screen.height);
        });
}

} // namespace app