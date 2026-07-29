#include "surrender/srTypeRegistry.h"
#include "surrender/srCore.h"
#include "wiz8/vector.h"
#include "wiz8/wiz8_windows.h"

#include <ostream>

/* The constructor at 0x004CF020 proves a 0x18-byte srClass prefix followed by
   two ordinary five-element growable vectors.  The prefix bytes are owned by
   the imported srClass constructor; the explicit tail padding keeps the first
   vector at its reviewed +0x18 offset without restating those hidden fields. */
class W8MusicPlaylist : public srClass {
public:
    W8MusicPlaylist();
    virtual ~W8MusicPlaylist();

    virtual const char* getClassName() const { return "stScript"; }
    virtual unsigned long getClassID() const { return 0x1000d; }
    virtual srRegistry::ClassNode* getClassNode() const
    { return classNode(); }
    virtual void dump(std::ostream&) {}
    virtual void verify(srRuntimeClass::e_verify mode) { srClass::verify(mode); }
    virtual srClass* vInstance() { return new W8MusicPlaylist(); }

private:
    static srRegistry::ClassNode* classNode();

    unsigned char srclass_tail_04[0x14];
    W8GrowableVector<void*> entries_18;
    W8GrowableVector<void*> entries_28;
};

typedef char W8MusicPlaylist_must_be_0x38[
    sizeof(W8MusicPlaylist) == 0x38 ? 1 : -1];

srRegistry::ClassNode* W8MusicPlaylist::classNode()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x1000d);
    if (!node) {
        node = registry->registerClass(
            "stScript", srClass::sGetClassNode(), 0x1000d, 1);
    }
    return node;
}

W8MusicPlaylist::W8MusicPlaylist()
    : entries_18(5), entries_28(5)
{
    srCore.getRegistry()->registerInstance(classNode(), this);
}

W8MusicPlaylist::~W8MusicPlaylist()
{
    srCore.getRegistry()->unregisterInstance(classNode(), this);
}

W8MusicPlaylist* g_music_playlist_65ba74;
unsigned int g_music_playlist_tick_65ba78;
int g_music_state_60aae8;
int g_music_state_60aaec;
int g_music_state_60aaf0;
extern "C" unsigned char g_flag_650e50;

/* Builds the named playlist object before the screen loop begins. */
// FUNCTION: WIZ8 0x0048F940
extern "C" unsigned char InitializeMusicPlaylist(void)
{
    g_music_playlist_65ba74 = new W8MusicPlaylist();
    if (g_music_playlist_65ba74) {
        g_music_playlist_65ba74->setName("Music Playlist");
    }
    g_music_playlist_tick_65ba78 = GetTickCount();
    g_music_state_60aae8 = 0;
    g_music_state_60aaec = 0;
    g_music_state_60aaf0 = 0;
    return g_music_playlist_65ba74 != 0;
}

/* The visible menu does not require a music stream, and the retail routine is
   a no-op when the Miles subsystem did not open.  Preserve that startup path
   without inventing the playlist parser; the enabled-audio path remains a
   separate recovery. */
extern "C" unsigned char Function48FC10(const char*, int, int)
{
    if (!g_flag_650e50) {
        return 1;
    }
    return 0;
}
