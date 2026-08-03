#pragma once

#include "core/render/render_types.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <stack>
#include <string>
#include <utility>
#include <vector>

namespace core::render {

class RenderBackend;

// ============================================================================
// CanvasContext – immediate-mode drawing API (user-facing)
// ============================================================================

class CanvasContext {
public:
    // --- filled shapes ---
    void fillCircle(float cx, float cy, float radius, const Color& color);
    void fillRect(float x, float y, float w, float h, const Color& color);
    void fillRoundedRect(float x, float y, float w, float h, float radius, const Color& color);

    // --- stroked shapes ---
    void strokeCircle(float cx, float cy, float radius, const Color& color, float strokeWidth = 1.0f);
    void strokeRect(float x, float y, float w, float h, const Color& color, float strokeWidth = 1.0f);
    void strokeRoundedRect(float x, float y, float w, float h, float radius, const Color& color, float strokeWidth = 1.0f);
    void drawLine(float x1, float y1, float x2, float y2, const Color& color, float strokeWidth = 1.0f);

    // --- free-form path ---
    void beginPath();
    void moveTo(float x, float y);
    void lineTo(float x, float y);
    void cubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y);
    void closePath();
    void fillPath(const Color& color);
    void strokePath(const Color& color, float strokeWidth = 1.0f);

    // --- text ---
    void drawText(const std::string& text, float x, float y,
                  const Color& color, float fontSize = 16.0f,
                  const std::string& fontFamily = "");

    // --- transform stack ---
    void save();
    void restore();
    void translate(float dx, float dy);
    void rotate(float radians);
    void scale(float sx, float sy);

    // --- clipping ---
    void clipRect(float x, float y, float w, float h);
    void clipRoundedRect(float x, float y, float w, float h, float radius);

    // --- dirty management (for per-frame optimization) ---
    void markDirty() { dirty_ = true; }
    bool isDirty() const { return dirty_; }

private:
    friend class CanvasPrimitive;

    enum class CmdType : std::uint8_t {
        FillCircle, FillRect, FillRoundedRect, FillPath,
        StrokeCircle, StrokeRect, StrokeRoundedRect, StrokePath,
        DrawLine,
        Text,
        Save, Restore, Translate, Rotate, Scale,
        ClipRect, ClipRoundedRect,
        BeginPath, MoveTo, LineTo, CubicTo, ClosePath
    };

    struct Cmd {
        CmdType type = CmdType::FillRect;
        float data[8] = {};
        Color color{1.0f, 1.0f, 1.0f, 1.0f};
        float strokeWidth = 1.0f;
        std::string text;
        std::string fontFamily;
        float fontSize = 16.0f;
    };

    std::vector<Cmd> commands_;
    bool dirty_ = true;

    void markClean() { dirty_ = false; }
    void clear() { commands_.clear(); }
    const std::vector<Cmd>& commands() const { return commands_; }

    // path building state
    bool buildingPath_ = false;
};

// ============================================================================
// CanvasDrawCommand – single draw call ready for the GPU backend
// ============================================================================

enum class CanvasShapeKind : std::uint8_t {
    FillCircle = 0,
    FillRect = 1,
    FillRoundedRect = 2,
    StrokeCircle = 3,
    StrokeRect = 4,
    StrokeRoundedRect = 5,
    Line = 6,
    FillPath = 7,
    StrokePath = 8
};

struct CanvasDrawCommand {
    CanvasShapeKind kind = CanvasShapeKind::FillRect;
    Rect rect{};                               // shape bounding box in canvas-local coords
    Color fillColor{1.0f, 1.0f, 1.0f, 1.0f};
    Color strokeColor{1.0f, 1.0f, 1.0f, 1.0f};
    float cornerRadius = 0.0f;
    float strokeWidth = 1.0f;
    // For line: rect.x,y = start, rect.width,height = end
    // For circle: rect.x,y = center, rect.width = radius, rect.height unused
    TransformMatrix transform{};               // accumulated canvas-local transform
    float opacity = 1.0f;
};

// ============================================================================
// CanvasPrimitive – retained primitive that owns the texture & issues GPU work
// ============================================================================

class CanvasPrimitive {
public:
    CanvasPrimitive();
    ~CanvasPrimitive();

    bool initialize();
    void destroy();
    void setBounds(float x, float y, float w, float h);
    void setTransformMatrix(const TransformMatrix& matrix);
    void setOpacity(float value);
    void setCornerRadius(float value);
    void setClipToBounds(bool value);

    CanvasContext& context() { return context_; }

    // Invoke the DSL onDraw callback to populate commands, then render.
    void prepare(const std::function<void(CanvasContext&)>& onDraw);
    void render(RenderBackend& backend, int windowWidth, int windowHeight);

private:
    CanvasContext context_;
    Rect bounds_{};
    TransformMatrix matrix_{};
    float opacity_ = 1.0f;
    float cornerRadius_ = 0.0f;
    bool clipToBounds_ = true;
    bool initialized_ = false;
};

} // namespace core::render