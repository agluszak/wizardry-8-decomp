// VC6 loop, table, fill/copy and bitfield lowering facts used when deciding
// whether a retail body came from a loop, a descriptor table or straight-line
// source. Compile under the first-party profile:
//   cl /nologo /c /DWIN32 /DNDEBUG /GX /GR- /MD /O2 /G6 /FAs loop_shape_probe.cpp
//
// Observed with the pinned SP5 compiler:
// - None of the tested counted loops is unrolled, including a 4-trip trivial store loop
//   (PlainLoop); static const descriptor tables are not folded into
//   immediates (TableLoop loads every field from the table). A loop of
//   `new T(...)` keeps a single EH state, while the straight-line form gives
//   each allocation its own state (Unrolled). Retail push-immediate sequences
//   with incrementing EH states support straight-line source in the reviewed
//   families. These probes do not establish a rule for every possible loop.
// - Store-constant loops of any spelling (dword, byte, pointer walk, char
//   array, -1 fill) become `rep stosd` exactly like memset/ZeroMemory/FillMemory
//   (ZeroDword, ZeroBytes, FillInts, ZeroMem). Copy loops are NOT
//   idiom-recognised and stay element loops (CopyBytes); `rep movsd` in retail
//   is memcpy or aggregate assignment.
// - Tests of two bitfields in one byte merge into `test byte, mask`
//   (EitherBit); a run of constant bitfield stores covering a byte folds into a
//   single byte store (InitBits -> `mov byte, 0x40` + `and next, 0xfe`).
// - `GetAt(GetCount() - 1)` keeps its `count - 1 < count` bounds test
//   (TopChecked); an unchecked `data[count - 1]` is a different accessor.

#include <string.h>

struct Panel;
struct Ctl {
    Ctl(Panel* p, int id, int l, int t, int r, int b, int img, int a, int f0, int f1, int f2,
        int f3, int f4);
    virtual ~Ctl();
    int pad[40];
};
struct Desc {
    int id;
    short l, t, r, b;
    int img;
    int f0, f1, f2, f3, f4;
};
static const Desc g_desc[4] = {
    {1, 10, 11, 12, 13, 0x98, 0, 2, 1, 4, 3},
    {2, 20, 21, 22, 23, 0x98, 5, 7, 6, 9, 8},
    {3, 30, 31, 32, 33, 0x98, 10, 11, 12, 13, 14},
    {4, 40, 41, 42, 43, 0x98, 15, 16, 17, 18, 19},
};
Ctl* g_ctl[4];
int g_vals[4];

void TableLoop(Panel* p)
{
    for (int i = 0; i < 4; i++)
        g_ctl[i] = new Ctl(p, g_desc[i].id, g_desc[i].l, g_desc[i].t, g_desc[i].r, g_desc[i].b,
                           g_desc[i].img, 0, g_desc[i].f0, g_desc[i].f1, g_desc[i].f2, g_desc[i].f3,
                           g_desc[i].f4);
}
void Unrolled(Panel* p)
{
    g_ctl[0] = new Ctl(p, 1, 10, 11, 12, 13, 0x98, 0, 0, 2, 1, 4, 3);
    g_ctl[1] = new Ctl(p, 2, 20, 21, 22, 23, 0x98, 0, 5, 7, 6, 9, 8);
}
void PlainLoop()
{
    for (int i = 0; i < 4; i++)
        g_vals[i] = g_desc[i].id + 1;
}

struct Rec {
    char text[0x3c];
};
void Use(void*);
void ZeroDword()
{
    Rec r;
    for (unsigned long i = 0; i < sizeof(r) / 4; ++i)
        ((unsigned long*)&r)[i] = 0;
    Use(&r);
}
void ZeroBytes()
{
    Rec r;
    for (unsigned long i = 0; i < sizeof(r); ++i)
        ((char*)&r)[i] = 0;
    Use(&r);
}
void ZeroMem()
{
    Rec r;
    memset(&r, 0, sizeof(r));
    Use(&r);
}
void FillInts(int* d, int n)
{
    for (int i = 0; i < n; ++i)
        d[i] = -1;
}
void CopyBytes(char* d, const char* s)
{
    for (int i = 0; i < 0x400; ++i)
        d[i] = s[i];
}

struct Bits {
    int vp;
    char type;
    unsigned char b0 : 1, b1 : 1, b2 : 1, b3 : 1, b4 : 1, b5 : 1, b6 : 1, b7 : 1;
    unsigned char b8 : 1, b9 : 1;
    short item;
};
int EitherBit(Bits* d)
{
    return d->b0 || d->b2;
}
void InitBits(Bits* d)
{
    d->b0 = 0;
    d->b1 = 0;
    d->b2 = 0;
    d->b3 = 0;
    d->b4 = 0;
    d->b5 = 0;
    d->b6 = 1;
    d->b7 = 0;
    d->b8 = 0;
    d->item = -1;
}

template <class T> class Vec {
public:
    virtual ~Vec();
    int GetCount() const
    {
        return count;
    }
    T* GetAt(int position)
    {
        if (position < count)
            return data + position;
        return data;
    }
    int count;
    int capacity;
    T* data;
};
struct Head {
    int id;
};
int TopChecked(Vec<Head*>* v)
{
    return (*v->GetAt(v->GetCount() - 1))->id;
}
int TopUnchecked(Vec<Head*>* v)
{
    return v->data[v->GetCount() - 1]->id;
}
