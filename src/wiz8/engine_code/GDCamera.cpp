#include "wiz8/engine_code/GDCamera.h"

#include "surrender/srNode.h"
#include "wiz8/engine_code/registry_classes.h"

extern const double g_camera_view_factor_005ec300;
extern const double g_camera_view_factor_005ec538;
extern const double g_camera_view_factor_005ec568;

/* Creates the game camera when a parent is supplied, otherwise installs or
   updates a caller-provided camera. Both paths finish by applying the game
   camera owner's current rotation. */
// FUNCTION: WIZ8 0x00476440
W8Camera005EBE14* GDCamera::Method00476440(
    srNode* parent, W8Camera005EBE14* camera)
{
    if (parent != 0) {
        srVector3T<double> position;

        g_game_camera_65a0fc = new W8Camera005EBE14(parent);
        g_game_camera_65a0fc->setName("Sirtech Camera");
        position.x = m_position_08c.x;
        position.y = m_position_08c.y;
        position.z = m_position_08c.z;
        g_game_camera_65a0fc->setLocation(position);
        g_game_camera_65a0fc->setClipRange(250.0, 75000.0);
        g_game_camera_65a0fc->setRotation(0.0, 0.0, 0.0);
        g_game_camera_65a0fc->setEnvironmentRange(0.0f, 1.0f);
        double view = g_camera_view_factor_005ec538
                      * g_camera_view_factor_005ec300
                      * g_camera_view_factor_005ec568;
        g_game_camera_65a0fc->setViewPlane(view, view);
    } else {
        srVector3T<double> position;

        if (camera == 0) {
            if (g_game_camera_65a0fc == 0) {
                return 0;
            }
        } else {
            g_game_camera_65a0fc = camera;
        }
        position.x = m_position_08c.x;
        position.y = m_position_08c.y;
        position.z = m_position_08c.z;
        g_game_camera_65a0fc->setLocation(position);
        g_game_camera_65a0fc->setRotation(0.0, 0.0, 0.0);
    }

    m_flag_088 = 0;
    m_flag_089 = 0;
    g_game_camera_65a0fc->setRotation(m_matrix_054);
    return g_game_camera_65a0fc;
}
