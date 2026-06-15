#version 450

layout(location = 0) in vec2 vLocalPos;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    vec4 windowSize;         // x=windowWidth, y=windowHeight
    vec4 fillColor;          // rgba
    vec4 strokeColor;        // rgba
    vec4 shapeRect;          // x, y, w, h (or cx, cy, r, 0 for circle)
    vec4 shapeParams;        // x=cornerRadius, y=strokeWidth, z=shapeKind (float)
    vec4 transform;          // m00, m01, tx, ty
    vec4 transform2;         // m10, m11, 0, opacity
} pc;

// Shape kinds (must match CanvasShapeKind enum)
#define KIND_FILL_CIRCLE        0.0
#define KIND_FILL_RECT          1.0
#define KIND_FILL_ROUNDED_RECT  2.0
#define KIND_STROKE_CIRCLE      3.0
#define KIND_STROKE_RECT        4.0
#define KIND_STROKE_ROUNDED_RECT 5.0
#define KIND_LINE               6.0

float roundedBoxDistance(vec2 point, vec2 halfSize, float radius) {
    vec2 cornerVector = abs(point) - halfSize + vec2(radius);
    return length(max(cornerVector, 0.0)) + min(max(cornerVector.x, cornerVector.y), 0.0) - radius;
}

float circleDistance(vec2 point, vec2 center, float radius) {
    return length(point - center) - radius;
}

float lineDistance(vec2 point, vec2 a, vec2 b) {
    vec2 pa = point - a;
    vec2 ba = b - a;
    float h = clamp(dot(pa, ba) / max(dot(ba, ba), 0.0001), 0.0, 1.0);
    return length(pa - ba * h);
}

void main() {
    float shapeKind = pc.shapeParams.z;
    float cornerRadius = pc.shapeParams.x;
    float strokeWidth = pc.shapeParams.y;
    float opacity = pc.transform2.w;

    float dist = 0.0;
    bool isStroke = false;
    vec4 baseColor = pc.fillColor;

    if (shapeKind == KIND_FILL_CIRCLE || shapeKind == KIND_STROKE_CIRCLE) {
        // shapeRect.xy = center, shapeRect.z = radius
        vec2 center = pc.shapeRect.xy;
        float radius = pc.shapeRect.z;
        dist = circleDistance(vLocalPos, center, radius);
        isStroke = (shapeKind == KIND_STROKE_CIRCLE);
    } else if (shapeKind == KIND_FILL_RECT || shapeKind == KIND_STROKE_RECT) {
        vec2 halfSize = pc.shapeRect.zw * 0.5;
        vec2 center = pc.shapeRect.xy + halfSize;
        dist = roundedBoxDistance(vLocalPos - center, halfSize, 0.0);
        isStroke = (shapeKind == KIND_STROKE_RECT);
    } else if (shapeKind == KIND_FILL_ROUNDED_RECT || shapeKind == KIND_STROKE_ROUNDED_RECT) {
        vec2 halfSize = pc.shapeRect.zw * 0.5;
        vec2 center = pc.shapeRect.xy + halfSize;
        dist = roundedBoxDistance(vLocalPos - center, halfSize, cornerRadius);
        isStroke = (shapeKind == KIND_STROKE_ROUNDED_RECT);
    } else if (shapeKind == KIND_LINE) {
        // shapeRect.xy = start, shapeRect.zw = end
        vec2 a = pc.shapeRect.xy;
        vec2 b = pc.shapeRect.zw;
        dist = lineDistance(vLocalPos, a, b);
        isStroke = true;
    }

    if (isStroke) {
        baseColor = pc.strokeColor;
        float halfStroke = strokeWidth * 0.5;
        float edgeWidth = max(fwidth(dist), 0.5);
        float alpha = 1.0 - smoothstep(-halfStroke - edgeWidth, -halfStroke + edgeWidth, dist);
        // Also apply the outer edge for strokes
        float outerAlpha = smoothstep(halfStroke - edgeWidth, halfStroke + edgeWidth, dist);
        alpha *= outerAlpha;
        if (alpha <= 0.0) {
            discard;
        }
        outColor = vec4(baseColor.rgb, baseColor.a * alpha * opacity);
    } else {
        float edgeWidth = max(fwidth(dist), 0.5);
        float alpha = 1.0 - smoothstep(-edgeWidth, edgeWidth, dist);
        if (alpha <= 0.0) {
            discard;
        }
        outColor = vec4(baseColor.rgb, baseColor.a * alpha * opacity);
    }
}