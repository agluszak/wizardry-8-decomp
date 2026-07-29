#include "wiz8/gameplay_boundaries.h"
#include "wiz8/render_state.h"

/*
 * Sets the viewport and rebuilds the camera view plane to match it.
 *
 * The srCamera and srGERD entry points are declared to produce exactly the
 * decorated names Wiz8.exe imports, taken from its import table: two
 * setViewPlane overloads, the const getViewPlane, and srGERD::flush. The
 * dllimport is not decoration - without it VC6 emits a direct call to the
 * thunk where the canonical has an indirect call through the import table.
 */

class __declspec(dllimport) srCamera {
public:
    /* Four doubles: left, bottom, right, top. The body only establishes that
       the third minus the first is a width and the fourth minus the second a
       height, which is what the two differences it takes are used for. */
    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    void setViewPlane(double width, double height);
    void setViewPlane(const Rect& plane, double depth);
    void getViewPlane(Rect& plane, double& depth) const;
};

class __declspec(dllimport) srGERD {
public:
    void flush();
};

extern "C" {

extern unsigned char g_flush_pending_603c3a;
extern W8World* g_world_00659ab4;
extern unsigned char g_flag_652da4;
extern const float g_scale_x_5ebb1c;
extern const float g_scale_y_5ebb20;
extern const float g_one_5ebc30;
extern const double g_plane_5ebcf8;
extern const double g_plane_5ebd00;
extern const double g_plane_5ebf48;
extern const double g_plane_5ebf4c;

extern void Function450080(srCamera* camera, int flag);

// FUNCTION: WIZ8 0x00426250
void SetViewport(int left, int top, int right, int bottom)
{
    float fractional_left;
    float fractional_top;
    float fractional_right;
    float fractional_bottom;
    srCamera::Rect view;
    srCamera::Rect plane;
    double depth;

    if (g_gerd_659634 != 0 && g_flush_pending_603c3a) {
        g_gerd_659634->flush();
    }
    fractional_left = (float)left * g_scale_x_5ebb1c;
    g_viewport_right_6595f0 = right + 1;
    g_viewport_left_6595e8 = left;
    g_viewport_bottom_6595f4 = bottom + 1;
    fractional_top = (float)top * g_scale_y_5ebb20;
    g_viewport_top_6595ec = top;
    fractional_right = (float)g_viewport_right_6595f0 * g_scale_x_5ebb1c;
    fractional_bottom = (float)g_viewport_bottom_6595f4 * g_scale_y_5ebb20;

    if (g_world_00659ab4 != 0 && g_world_00659ab4->camera != 0) {
        g_world_00659ab4->camera->setViewPlane(
            g_plane_5ebd00 * g_plane_5ebcf8 * g_plane_5ebf48,
            g_plane_5ebd00 * g_plane_5ebcf8 * g_plane_5ebf4c);
        g_world_00659ab4->camera->getViewPlane(view, depth);

        plane.left = (double)fractional_left * (view.right - view.left) + view.left;
        plane.right = (double)fractional_right * (view.right - view.left) + view.left;
        plane.bottom = (double)((g_one_5ebc30 - fractional_bottom)
                                * (float)(view.top - view.bottom) + (float)view.bottom);
        plane.top = (double)((g_one_5ebc30 - fractional_top)
                             * (float)(view.top - view.bottom) + (float)view.bottom);

        g_world_00659ab4->camera->setViewPlane(plane, 1.0);
        if (g_world_659ab8 != 0) {
            g_world_659ab8->camera->setViewPlane(plane, 1.0);
        }
        if (g_flag_652da4) {
            Function450080(g_world_00659ab4->camera, 1);
        }
    }
}

}
