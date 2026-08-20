#ifndef WIZ8_VIDEO_OBJECT_CATALOG_H
#define WIZ8_VIDEO_OBJECT_CATALOG_H

extern "C" {

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

}

#endif
