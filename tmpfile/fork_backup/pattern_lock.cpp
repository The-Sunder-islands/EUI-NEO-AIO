#include "eui_neo.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace app {
namespace {

constexpr eui::Color kBg{0.06f, 0.06f, 0.08f, 1.0f};
constexpr eui::Color kAccent{0.30f, 0.65f, 1.0f, 1.0f};
constexpr eui::Color kAccentDim{0.15f, 0.40f, 0.70f, 0.6f};
constexpr eui::Color kDotStroke{0.35f, 0.38f, 0.48f, 1.0f};
constexpr eui::Color kTextMuted{0.45f, 0.48f, 0.55f, 1.0f};
constexpr eui::Color kError{0.95f, 0.30f, 0.35f, 1.0f};

// Magnetic snap: the dot "pulls" the line when finger enters this radius
constexpr float kSnapRadius = 38.0f;
// Finger must be within this radius to actually select the dot
constexpr float kHitRadius = 22.0f;
constexpr float kDotOuterRadius = 18.0f;
constexpr float kDotSpacing = 78.0f;

struct PatternLockState {
    bool dragging = false;
    float dpiScale = 1.0f;
    float pointerX = 0, pointerY = 0;      // raw finger position
    float lineEndX = 0, lineEndY = 0;      // where the preview line actually ends
    int snapTarget = -1;                    // dot being hovered (-1 = none)
    std::vector<int> selectedDots;
    bool showResult = false;
    bool resultSuccess = false;
    float resultTimer = 0;
};

PatternLockState state;

struct DotPosition {
    float x, y;
};

DotPosition dotPositions[9];

void computeDotPositions(float cx, float cy) {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            int idx = row * 3 + col;
            dotPositions[idx] = {cx + (col - 1) * kDotSpacing, cy + (row - 1) * kDotSpacing};
        }
    }
}

float distToDot(int dot, float x, float y) {
    float dx = x - dotPositions[dot].x;
    float dy = y - dotPositions[dot].y;
    return std::sqrt(dx * dx + dy * dy);
}

int findSnapTarget(float x, float y) {
    int best = -1;
    float bestDist = kSnapRadius;
    for (int i = 0; i < 9; ++i) {
        if (std::find(state.selectedDots.begin(), state.selectedDots.end(), i) != state.selectedDots.end()) {
            continue; // already selected
        }
        float d = distToDot(i, x, y);
        if (d < bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

bool isDotSelected(int dot) {
    return std::find(state.selectedDots.begin(), state.selectedDots.end(), dot) != state.selectedDots.end();
}

void updateLineEndpoint() {
    if (state.snapTarget >= 0) {
        // Magnetic snap: line end pulls toward the dot
        float dx = dotPositions[state.snapTarget].x - state.pointerX;
        float dy = dotPositions[state.snapTarget].y - state.pointerY;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 1.0f) {
            state.lineEndX = dotPositions[state.snapTarget].x;
            state.lineEndY = dotPositions[state.snapTarget].y;
        } else {
            // Smooth interpolation: line end is partway between finger and dot
            float t = std::min(1.0f, (kSnapRadius - dist) / (kSnapRadius - kHitRadius));
            t = t * t; // ease-in
            state.lineEndX = state.pointerX + dx * t;
            state.lineEndY = state.pointerY + dy * t;
        }
    } else {
        state.lineEndX = state.pointerX;
        state.lineEndY = state.pointerY;
    }
}

void drawPatternLock(eui::CanvasContext& ctx, float w, float h) {
    const float cx = w * 0.5f;
    const float cy = h * 0.5f - 30.0f;

    computeDotPositions(cx, cy);

    // Background
    ctx.fillRect(0, 0, w, h, kBg);

    // Title
    ctx.drawText("Pattern Lock", 20, 14, kTextMuted, 14.0f);

    auto accentColor = [&]() -> eui::Color {
        if (state.showResult) {
            return state.resultSuccess ? kAccent : kError;
        }
        return state.dragging ? kAccent : kAccentDim;
    };

    // Draw connecting lines between selected dots
    if (!state.selectedDots.empty()) {
        for (size_t i = 0; i + 1 < state.selectedDots.size(); ++i) {
            int a = state.selectedDots[i];
            int b = state.selectedDots[i + 1];
            ctx.drawLine(dotPositions[a].x, dotPositions[a].y,
                         dotPositions[b].x, dotPositions[b].y,
                         accentColor(), 5.0f);
        }

        // Preview line from last selected dot to lineEnd position
        if (state.dragging && !state.showResult) {
            int last = state.selectedDots.back();
            ctx.drawLine(dotPositions[last].x, dotPositions[last].y,
                         state.lineEndX, state.lineEndY,
                         {0.30f, 0.65f, 1.0f, 0.45f}, 5.0f);
        }
    }

    // Draw dots
    for (int i = 0; i < 9; ++i) {
        bool selected = isDotSelected(i);
        bool isSnapTarget = (state.dragging && state.snapTarget == i);

        // Outer ring
        float ringWidth = selected ? 3.0f : 2.5f;
        eui::Color ringColor = selected ? accentColor() : kDotStroke;

        if (isSnapTarget && !selected) {
            // Magnetic hover: expanding ring
            float dist = distToDot(i, state.pointerX, state.pointerY);
            float t = 1.0f - std::max(0.0f, (dist - kHitRadius) / (kSnapRadius - kHitRadius));
            float extraRadius = t * 10.0f;
            ringColor = {0.30f, 0.65f, 1.0f, 0.5f + t * 0.5f};
            ringWidth = 2.5f + t * 1.5f;
            ctx.strokeCircle(dotPositions[i].x, dotPositions[i].y,
                             kDotOuterRadius + extraRadius, ringColor, ringWidth);
            // Glow under the ring
            ctx.fillCircle(dotPositions[i].x, dotPositions[i].y,
                           kDotOuterRadius + extraRadius,
                           {0.30f, 0.65f, 1.0f, 0.08f + t * 0.12f});
        } else {
            ctx.strokeCircle(dotPositions[i].x, dotPositions[i].y,
                             kDotOuterRadius, ringColor, ringWidth);
        }

        // Dot fill
        if (selected) {
            ctx.fillCircle(dotPositions[i].x, dotPositions[i].y, 6.0f, accentColor());
            ctx.fillCircle(dotPositions[i].x, dotPositions[i].y, 11.0f,
                           {0.30f, 0.65f, 1.0f, 0.25f});
        } else if (isSnapTarget) {
            // Hover fill: dot center grows slightly
            float dist = distToDot(i, state.pointerX, state.pointerY);
            float t = 1.0f - std::max(0.0f, (dist - kHitRadius) / (kSnapRadius - kHitRadius));
            float innerR = 4.5f + t * 5.0f;
            ctx.fillCircle(dotPositions[i].x, dotPositions[i].y, innerR,
                           {0.30f, 0.65f, 1.0f, 0.3f + t * 0.7f});
        } else {
            ctx.fillCircle(dotPositions[i].x, dotPositions[i].y, 4.5f, kDotStroke);
        }
    }

    // Result text
    if (state.showResult) {
        const char* msg = state.resultSuccess ? "Pattern recorded!" : "Too short! (min 4 dots)";
        eui::Color msgColor = state.resultSuccess ? kAccent : kError;
        ctx.drawText(msg, 20, h - 50, msgColor, 14.0f);
    }

    // Hint
    ctx.drawText("Draw a pattern connecting dots", 20, h - 24, kTextMuted, 12.0f);
}

} // namespace

const DslAppConfig& dslAppConfig() {
    static DslAppConfig config = DslAppConfig{}
        .title("Pattern Lock")
        .pageId("pattern_lock")
        .clearColor(kBg)
        .windowSize(420, 520)
        .fps(60.0);
    return config;
}

void compose(eui::Ui& ui, const eui::Screen& screen) {
    ui.canvas("lock")
        .size(screen.width, screen.height)
        .onPress([&, sw = screen.width](const eui::PointerEvent& event, const eui::Rect& bounds) {
            state.dpiScale = bounds.width / sw;
            state.dragging = true;
            state.selectedDots.clear();
            state.snapTarget = -1;
            state.showResult = false;

            float lx = event.x / state.dpiScale;
            float ly = event.y / state.dpiScale;
            state.pointerX = lx;
            state.pointerY = ly;

            // Check if pressing on a dot
            for (int i = 0; i < 9; ++i) {
                if (distToDot(i, lx, ly) < kHitRadius) {
                    state.selectedDots.push_back(i);
                    state.snapTarget = -1;
                    break;
                }
            }
            updateLineEndpoint();
        })
        .onDrag([&](const eui::DragEvent& event) {
            if (!state.dragging) return;
            float lx = event.x / state.dpiScale;
            float ly = event.y / state.dpiScale;
            state.pointerX = lx;
            state.pointerY = ly;

            // Find snap target (magnetic hover)
            state.snapTarget = findSnapTarget(lx, ly);

            // Select dot if finger is within hit radius
            if (state.snapTarget >= 0) {
                int dot = state.snapTarget;
                float d = distToDot(dot, lx, ly);
                if (d < kHitRadius) {
                    // Check if this dot was previously selected (backtrack)
                    auto it = std::find(state.selectedDots.begin(), state.selectedDots.end(), dot);
                    if (it == state.selectedDots.end()) {
                        state.selectedDots.push_back(dot);
                    } else if (state.selectedDots.size() >= 2 && state.selectedDots.back() != dot) {
                        // Backtrack: remove dots after this one
                        state.selectedDots.erase(it + 1, state.selectedDots.end());
                    }
                }
            }

            updateLineEndpoint();
        })
        .onRelease([&](const eui::PointerEvent&, const eui::Rect&) {
            if (!state.dragging) return;
            state.dragging = false;
            state.snapTarget = -1;

            if (state.selectedDots.size() >= 4) {
                state.showResult = true;
                state.resultSuccess = true;
                state.resultTimer = 1.5f;
            } else if (!state.selectedDots.empty()) {
                state.showResult = true;
                state.resultSuccess = false;
                state.resultTimer = 1.5f;
            }
        })
        .onFrame([&](float dt) {
            if (state.showResult) {
                state.resultTimer -= dt;
                if (state.resultTimer <= 0) {
                    state.showResult = false;
                    state.selectedDots.clear();
                }
            }
        })
        .onDraw([w = screen.width, h = screen.height](eui::CanvasContext& ctx) {
            drawPatternLock(ctx, w, h);
        });
}

} // namespace app