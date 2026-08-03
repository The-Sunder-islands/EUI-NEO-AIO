#version 450

layout(location = 0) in vec3 aScreenPos;
layout(location = 1) in vec2 aLocalPos;

layout(location = 0) out vec2 vLocalPos;

layout(push_constant) uniform PushConstants {
    vec4 windowSize;         // x=windowWidth, y=windowHeight
    vec4 fillColor;          // rgba
    vec4 strokeColor;        // rgba
    vec4 shapeRect;          // x, y, w, h (or cx, cy, r, 0 for circle)
    vec4 shapeParams;        // x=cornerRadius, y=strokeWidth, z=shapeKind (float)
    vec4 transform;          // m00, m01, tx, ty (2x3 affine)
    vec4 transform2;         // m10, m11, 0, opacity
} pc;

void main() {
    // Apply the canvas-local affine transform to the screen position
    float tx = aScreenPos.x + pc.transform.z;
    float ty = aScreenPos.y + pc.transform.w;
    float rx = pc.transform.x * tx + pc.transform.y * ty;
    float ry = pc.transform2.x * tx + pc.transform2.y * ty;

    vec2 ndc = vec2((rx / pc.windowSize.x) * 2.0 - 1.0,
                    (ry / pc.windowSize.y) * 2.0 - 1.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vLocalPos = aLocalPos;
}