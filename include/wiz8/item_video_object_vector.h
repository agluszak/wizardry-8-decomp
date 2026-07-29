#ifndef WIZ8_ITEM_VIDEO_OBJECT_VECTOR_H
#define WIZ8_ITEM_VIDEO_OBJECT_VECTOR_H

struct W8ItemVideoObjectEntry {
    unsigned char initialized;
    unsigned char padding_01[3];
    int video_object;

    W8ItemVideoObjectEntry();
    ~W8ItemVideoObjectEntry();
};

class W8ItemVideoObjectVector {
public:
    void Initialize(int capacity);
    void Clear();
    int GetOrCreateVideoObject(int item_id);

    W8ItemVideoObjectEntry* data;
    int capacity;
    int count;
};

extern W8ItemVideoObjectVector g_item_video_objects_68ec68;

#endif
