#pragma once

#include "srFlags.h"

class srRendererDefs {
public:
    /* OpenGL srDD maps these to glDrawArrays modes (POINTS, LINE_STRIP, LINES,
       TRIANGLE_STRIP, TRIANGLE_FAN, TRIANGLES). DirectX7 keeps the same SR
       values as the draw-helper primitive key (value 5 is the triangle-list
       merge case; value 3 is the four-vertex strip Wizardry draws). */
    enum e_primitive {
        PRIMITIVE_POINTS = 0,
        PRIMITIVE_LINE_STRIP = 1,
        PRIMITIVE_LINES = 2,
        PRIMITIVE_TRIANGLE_STRIP = 3,
        PRIMITIVE_TRIANGLE_FAN = 4,
        PRIMITIVE_TRIANGLES = 5
    };
    /* Bit indices into srGERD's VertexArrayInfo clip_08 mask / getClipMask.
       applyClipPlaneChanges fills the six frustum planes at GERD+0x1418 in this
       order (X pair, Y pair, then constant Z near/far). Bits 6+ are extra user
       planes from pushClipPlane. Wizardry's 2D path writes the six-bit mask 0x3f. */
    enum e_clip {
        CLIP_LEFT = 0,
        CLIP_RIGHT = 1,
        CLIP_BOTTOM = 2,
        CLIP_TOP = 3,
        CLIP_NEAR = 4,
        CLIP_FAR = 5
    };
    /* OpenGL vertex-array setup uses GL_FLOAT (0x1406) for this value.
       Wizardry passes it for float[] position and texcoord arrays. */
    enum e_type { TYPE_FLOAT = 1 };
    /* DD vertex-array slot indices consumed by srGERD::setDataPtr;
       VertexArrayInfo mask_00 sets bit 1<<slot for each live stream. Slot 3
       carries the specular record's w component alone when a batch has
       specular alpha without a specular stream. */
    enum e_vertexArray {
        VERTEX_ARRAY_POSITIONS = 0,
        VERTEX_ARRAY_DIFFUSE = 1,
        VERTEX_ARRAY_SPECULAR = 2,
        VERTEX_ARRAY_SPECULAR_ALPHA = 3,
        VERTEX_ARRAY_TEXCOORD0 = 4,
        VERTEX_ARRAY_TEXCOORD1 = 5
    };
    /* drawElements passes 2 for the renderer's 32-bit index triples. */
    enum e_indexType { INDEX_ULONG = 2 };
    /* The vertex-stream state srGERD embeds at +0x21c4 and hands to
       srDD::setVertexArrayInfo before each draw: live-slot mask, vertex count,
       clip mask, then the per-slot component/type/stride/pointer arrays. */
    struct VertexArrayInfo {
        srFlags<e_vertexArray> mask_00;
        unsigned long count_04;
        srFlags<e_clip> clip_08;
        long components_0c[6];
        e_type types_24[6];
        unsigned long strides_3c[6];
        const void* arrays_54[6];
    };
};
