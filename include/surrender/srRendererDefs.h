#pragma once

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
    enum e_clip {};
    /* OpenGL vertex-array setup uses GL_FLOAT (0x1406) for this value.
       Wizardry passes it for float[] position and texcoord arrays. */
    enum e_type { TYPE_FLOAT = 1 };
    enum e_vertexArray {};
    enum e_indexType {};
    struct VertexArrayInfo;
};
