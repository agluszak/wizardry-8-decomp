#ifndef WIZ8_VIDEO_OBJECT_CATALOG_H
#define WIZ8_VIDEO_OBJECT_CATALOG_H

extern "C" {

unsigned int GetCatalogVideoObjectHandle(int object, int frame);
short GetCatalogVideoObjectYOffset(int object);
void Function549660(int object, int frame, int image,
                    short* width, short* height);

}

#endif
