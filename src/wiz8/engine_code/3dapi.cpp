#include "wiz8/gameplay_boundaries.h"
#include "wiz8/sr_api.h"

/*
 * Engine Code\3dapi.cpp.
 *
 * The thin layer the rest of the engine calls the renderer through. Most of
 * what is here forwards straight on, which is what makes the file a layer
 * rather than an implementation.
 */

class srNode {
public:
    ~srNode();
};

extern void SetRendererReady(void);
extern void Function44FAF0(int argument);
extern void Function44F5F0(int a, int b, int c, int d, int e);
extern void SetHeapFree(void* block);
extern unsigned char g_renderer_ready_00607d7c;

/* Note that the renderer is up. Eight bytes and no branch. */
// FUNCTION: WIZ8 0x00451010
void MarkRendererReady(void)
{
    g_renderer_ready_00607d7c = 1;
}

/* Two forwarders that pass their arguments through unchanged. */
// FUNCTION: WIZ8 0x00451140
void Forward44FAF0(int argument)
{
    Function44FAF0(argument);
}

// FUNCTION: WIZ8 0x00451110
void Forward44F5F0(int a, int b, int c, int d, int e)
{
    Function44F5F0(a, b, c, d, e);
}

/* Report a failed assertion with no message of its own, so the expression and
   the site are all the caller has to give. */
// FUNCTION: WIZ8 0x00450780
void ReportAssertion(const char* expression, const char* source_path, long line)
{
    srAssertFail(expression, source_path, line, 0);
}

/* An srNode's scalar deleting destructor. It releases through the renderer's
   own heap rather than the CRT's, which is what makes it the renderer's node
   rather than one of ours. */
// FUNCTION: WIZ8 0x0044F3D0
srNode* __fastcall srNode_ScalarDeletingDestructor(
    srNode* self, int /* unused edx */, unsigned char flags)
{
    self->~srNode();
    if ((flags & 1) != 0) {
        SetHeapFree(self);
    }
    return self;
}
