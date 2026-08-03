#include "core/render/canvas.h"
#include "core/render/render_backend.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stack>

namespace core::render {

namespace {

TransformMatrix multiplyTransform(const TransformMatrix& a, const TransformMatrix& b) {
    TransformMatrix result;
    result.m00 = a.m00 * b.m00 + a.m01 * b.m10;
    result.m01 = a.m00 * b.m01 + a.m01 * b.m11;
    result.tx  = a.m00 * b.tx  + a.m01 * b.ty  + a.tx;
    result.m10 = a.m10 * b.m00 + a.m11 * b.m10;
    result.m11 = a.m10 * b.m01 + a.m11 * b.m11;
    result.ty  = a.m10 * b.tx  + a.m11 * b.ty  + a.ty;
    result.px  = a.px * b.m00 + a.py * b.m10;
    result.py  = a.px * b.m01 + a.py * b.m11;
    result.pw  = a.pw;
    return result;
}

} // namespace

// ============================================================================
// CanvasContext
// ============================================================================

void CanvasContext::fillCircle(float cx, float cy, float radius, const Color& color) {
    Cmd cmd;
    cmd.type = CmdType::FillCircle;
    cmd.data[0] = cx;
    cmd.data[1] = cy;
    cmd.data[2] = radius;
    cmd.color = color;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::fillRect(float x, float y, float w, float h, const Color& color) {
    Cmd cmd;
    cmd.type = CmdType::FillRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    cmd.color = color;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::fillRoundedRect(float x, float y, float w, float h, float radius, const Color& color) {
    Cmd cmd;
    cmd.type = CmdType::FillRoundedRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    cmd.data[4] = radius;
    cmd.color = color;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::strokeCircle(float cx, float cy, float radius, const Color& color, float strokeWidth) {
    Cmd cmd;
    cmd.type = CmdType::StrokeCircle;
    cmd.data[0] = cx;
    cmd.data[1] = cy;
    cmd.data[2] = radius;
    cmd.color = color;
    cmd.strokeWidth = std::max(0.0f, strokeWidth);
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::strokeRect(float x, float y, float w, float h, const Color& color, float strokeWidth) {
    Cmd cmd;
    cmd.type = CmdType::StrokeRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    cmd.color = color;
    cmd.strokeWidth = std::max(0.0f, strokeWidth);
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::strokeRoundedRect(float x, float y, float w, float h, float radius, const Color& color, float strokeWidth) {
    Cmd cmd;
    cmd.type = CmdType::StrokeRoundedRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    cmd.data[4] = radius;
    cmd.color = color;
    cmd.strokeWidth = std::max(0.0f, strokeWidth);
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::drawLine(float x1, float y1, float x2, float y2, const Color& color, float strokeWidth) {
    Cmd cmd;
    cmd.type = CmdType::DrawLine;
    cmd.data[0] = x1;
    cmd.data[1] = y1;
    cmd.data[2] = x2;
    cmd.data[3] = y2;
    cmd.color = color;
    cmd.strokeWidth = std::max(0.0f, strokeWidth);
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::beginPath() {
    buildingPath_ = true;
    Cmd cmd;
    cmd.type = CmdType::BeginPath;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::moveTo(float x, float y) {
    Cmd cmd;
    cmd.type = CmdType::MoveTo;
    cmd.data[0] = x;
    cmd.data[1] = y;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::lineTo(float x, float y) {
    Cmd cmd;
    cmd.type = CmdType::LineTo;
    cmd.data[0] = x;
    cmd.data[1] = y;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::cubicTo(float cx1, float cy1, float cx2, float cy2, float x, float y) {
    Cmd cmd;
    cmd.type = CmdType::CubicTo;
    cmd.data[0] = cx1;
    cmd.data[1] = cy1;
    cmd.data[2] = cx2;
    cmd.data[3] = cy2;
    cmd.data[4] = x;
    cmd.data[5] = y;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::closePath() {
    buildingPath_ = false;
    Cmd cmd;
    cmd.type = CmdType::ClosePath;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::fillPath(const Color& color) {
    Cmd cmd;
    cmd.type = CmdType::FillPath;
    cmd.color = color;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::strokePath(const Color& color, float strokeWidth) {
    Cmd cmd;
    cmd.type = CmdType::StrokePath;
    cmd.color = color;
    cmd.strokeWidth = std::max(0.0f, strokeWidth);
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::drawText(const std::string& text, float x, float y,
                             const Color& color, float fontSize,
                             const std::string& fontFamily) {
    Cmd cmd;
    cmd.type = CmdType::Text;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.color = color;
    cmd.fontSize = std::max(1.0f, fontSize);
    cmd.text = text;
    cmd.fontFamily = fontFamily;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::save() {
    Cmd cmd;
    cmd.type = CmdType::Save;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::restore() {
    Cmd cmd;
    cmd.type = CmdType::Restore;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::translate(float dx, float dy) {
    Cmd cmd;
    cmd.type = CmdType::Translate;
    cmd.data[0] = dx;
    cmd.data[1] = dy;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::rotate(float radians) {
    Cmd cmd;
    cmd.type = CmdType::Rotate;
    cmd.data[0] = radians;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::scale(float sx, float sy) {
    Cmd cmd;
    cmd.type = CmdType::Scale;
    cmd.data[0] = sx;
    cmd.data[1] = sy;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::clipRect(float x, float y, float w, float h) {
    Cmd cmd;
    cmd.type = CmdType::ClipRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    commands_.push_back(cmd);
    dirty_ = true;
}

void CanvasContext::clipRoundedRect(float x, float y, float w, float h, float radius) {
    Cmd cmd;
    cmd.type = CmdType::ClipRoundedRect;
    cmd.data[0] = x;
    cmd.data[1] = y;
    cmd.data[2] = w;
    cmd.data[3] = h;
    cmd.data[4] = radius;
    commands_.push_back(cmd);
    dirty_ = true;
}

// ============================================================================
// CanvasPrimitive
// ============================================================================

CanvasPrimitive::CanvasPrimitive() = default;
CanvasPrimitive::~CanvasPrimitive() = default;

bool CanvasPrimitive::initialize() {
    initialized_ = true;
    return true;
}

void CanvasPrimitive::destroy() {
    initialized_ = false;
    context_.clear();
}

void CanvasPrimitive::setBounds(float x, float y, float w, float h) {
    bounds_ = {x, y, w, h};
}

void CanvasPrimitive::setTransformMatrix(const TransformMatrix& matrix) {
    matrix_ = matrix;
}

void CanvasPrimitive::setOpacity(float value) {
    opacity_ = std::clamp(value, 0.0f, 1.0f);
}

void CanvasPrimitive::setCornerRadius(float value) {
    cornerRadius_ = std::max(0.0f, value);
}

void CanvasPrimitive::setClipToBounds(bool value) {
    clipToBounds_ = value;
}

void CanvasPrimitive::prepare(const std::function<void(CanvasContext&)>& onDraw) {
    if (!onDraw) {
        return;
    }
    context_.clear();
    onDraw(context_);
    context_.markClean();
}

void CanvasPrimitive::render(RenderBackend& backend, int windowWidth, int windowHeight) {
    if (!initialized_ || context_.commands().empty()) {
        return;
    }

    struct State {
        TransformMatrix transform{};
    };

    std::stack<State> savedStates;
    State current;
    current.transform = matrix_;

    std::vector<CanvasDrawCommand> drawCommands;

    for (const auto& cmd : context_.commands()) {
        switch (cmd.type) {
        case CanvasContext::CmdType::Save:
            savedStates.push(current);
            break;
        case CanvasContext::CmdType::Restore:
            if (!savedStates.empty()) {
                current = savedStates.top();
                savedStates.pop();
            }
            break;
        case CanvasContext::CmdType::Translate: {
            TransformMatrix t;
            t.tx = cmd.data[0];
            t.ty = cmd.data[1];
            current.transform = multiplyTransform(current.transform, t);
            break;
        }
        case CanvasContext::CmdType::Rotate: {
            float c = std::cos(cmd.data[0]);
            float s = std::sin(cmd.data[0]);
            TransformMatrix t;
            t.m00 = c;  t.m01 = -s;
            t.m10 = s;  t.m11 = c;
            current.transform = multiplyTransform(current.transform, t);
            break;
        }
        case CanvasContext::CmdType::Scale: {
            TransformMatrix t;
            t.m00 = cmd.data[0];
            t.m11 = cmd.data[1];
            current.transform = multiplyTransform(current.transform, t);
            break;
        }
        case CanvasContext::CmdType::FillCircle: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::FillCircle;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], 0.0f};
            dc.fillColor = cmd.color;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::FillRect: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::FillRect;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]};
            dc.fillColor = cmd.color;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::FillRoundedRect: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::FillRoundedRect;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]};
            dc.fillColor = cmd.color;
            dc.cornerRadius = cmd.data[4];
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::StrokeCircle: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::StrokeCircle;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], 0.0f};
            dc.strokeColor = cmd.color;
            dc.strokeWidth = cmd.strokeWidth;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::StrokeRect: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::StrokeRect;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]};
            dc.strokeColor = cmd.color;
            dc.strokeWidth = cmd.strokeWidth;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::StrokeRoundedRect: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::StrokeRoundedRect;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]};
            dc.strokeColor = cmd.color;
            dc.cornerRadius = cmd.data[4];
            dc.strokeWidth = cmd.strokeWidth;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::DrawLine: {
            CanvasDrawCommand dc;
            dc.kind = CanvasShapeKind::Line;
            dc.rect = {cmd.data[0], cmd.data[1], cmd.data[2], cmd.data[3]};
            dc.strokeColor = cmd.color;
            dc.strokeWidth = cmd.strokeWidth;
            dc.opacity = opacity_;
            dc.transform = current.transform;
            drawCommands.push_back(dc);
            break;
        }
        case CanvasContext::CmdType::FillPath:
        case CanvasContext::CmdType::StrokePath:
        case CanvasContext::CmdType::Text:
        case CanvasContext::CmdType::BeginPath:
        case CanvasContext::CmdType::MoveTo:
        case CanvasContext::CmdType::LineTo:
        case CanvasContext::CmdType::CubicTo:
        case CanvasContext::CmdType::ClosePath:
        case CanvasContext::CmdType::ClipRect:
        case CanvasContext::CmdType::ClipRoundedRect:
            // Handled at runtime level or not yet implemented for GPU path
            break;
        }
    }

    for (const auto& dc : drawCommands) {
        backend.drawCanvasShape(dc, windowWidth, windowHeight);
    }
}

} // namespace core::render