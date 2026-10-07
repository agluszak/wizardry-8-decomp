#pragma once

#include "srFlags.h"

class srRendererDefs {
public:
    enum e_primitive {
        PRIMITIVE_POINTS = 0,
        PRIMITIVE_LINE_STRIP = 1,
        PRIMITIVE_LINES = 2,
        PRIMITIVE_TRIANGLE_STRIP = 3,
        PRIMITIVE_TRIANGLE_FAN = 4,
        PRIMITIVE_TRIANGLES = 5
    };
    /* Clip-mask bit indices: the six frustum planes (X pair, Y pair, then near/far); bits 6+ are
       user planes from pushClipPlane. */
    enum e_clip {
        CLIP_LEFT = 0,
        CLIP_RIGHT = 1,
        CLIP_BOTTOM = 2,
        CLIP_TOP = 3,
        CLIP_NEAR = 4,
        CLIP_FAR = 5
    };
    enum { FRUSTUM_CLIP_MASK = 0x3fu, FIRST_USER_CLIP_PLANE = 6 };

    enum e_type { TYPE_FLOAT = 1 };
    /* DD vertex-array slot indices; VertexArrayInfo's mask sets bit 1<<slot for each live stream. */
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
    /* Vertex-stream state handed to srDD::setVertexArrayInfo before each draw. */
    struct VertexArrayInfo {
        srFlags<e_vertexArray> mask;
        unsigned long count;
        srFlags<e_clip> clip;
        long components[6];
        e_type types[6];
        unsigned long strides[6];
        const void* arrays[6];
    };
};
