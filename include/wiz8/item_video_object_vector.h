#ifndef WIZ8_ITEM_VIDEO_OBJECT_VECTOR_H
#define WIZ8_ITEM_VIDEO_OBJECT_VECTOR_H

struct W8ItemVideoObjectEntry {
    unsigned char initialized;
    unsigned char padding_01[3];
    int video_object;

    W8ItemVideoObjectEntry();
    ~W8ItemVideoObjectEntry();
};

class W8ItemVideoObjectCache {
public:
    W8ItemVideoObjectCache() : data(0), capacity(0), loaded_count(0) {}
    ~W8ItemVideoObjectCache()
    {
        if (data) {
            delete[] data;
            data = 0;
        }
    }
    void Initialize(int capacity);
    void Clear();
    int GetOrCreateVideoObject(int item_id);

    /* The methods share this receiver, including the counter at +8.
       A nested srArray identity is unproved: retail teardown leaves capacity
       untouched, unlike the canonical srArray destructor. */
    W8ItemVideoObjectEntry* data;
    int capacity;
    int loaded_count;
};

extern W8ItemVideoObjectCache g_item_video_objects_68ec68;

#endif
