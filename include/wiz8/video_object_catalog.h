#ifndef WIZ8_VIDEO_OBJECT_CATALOG_H
#define WIZ8_VIDEO_OBJECT_CATALOG_H

#pragma pack(push, 1)
struct W8VideoObjectSlot {
    int first_frame;
    short y_offset;
    unsigned char unknown_06[2];
};

struct W8VideoFrame {
    char path[0x30];
    int mode;
    unsigned char loaded;
    unsigned char unknown_35[3];
    unsigned int handle;
};
#pragma pack(pop)

extern "C" {

extern W8VideoObjectSlot g_video_slots_6448c8[494];
extern W8VideoFrame g_video_frames_62c430[1658];

unsigned int GetCatalogVideoObjectHandle(int object, int frame);
short GetCatalogVideoObjectYOffset(int object);
void Function549660(int object, int frame, int image,
                    short* width, short* height);
void Function548F90(int target, int object, int frame, short y,
                    int left, int top, int mode, int flags);
void Function5494F0(int object, int frame, int image,
                    int left, int top, int flags);
void Function549600(int target, int object, int frame, int y,
                    int left, int top, int mode, int flags);
unsigned char Function5497C0(int target, int left, int top,
                             int right, int bottom, int object,
                             int source_x, int source_y);
void Function549090(int object, int frame);

}

#endif
