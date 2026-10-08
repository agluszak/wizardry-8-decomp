/* State probes for SurRender contracts that the vector-processor cases do
   not reach: the class registry name index, the driver list records GERD
   copies from a device, and the camera's model-view matrix.

   All probes run after the exported srInit(). Device-facing probes drive a
   real srGERD through a runner-owned srDD whose records follow srDD.h; the
   srDD layout facts they rely on are checked against retail instructions
   (tests/differential/README.md). Output is values and logical identities
   only. */

#include <excpt.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "surrender/srCamera.h"
#include "surrender/srCore.h"
#include "surrender/srDD.h"
#include "surrender/srGERD.h"
#include "surrender/srTexture.h"
#include "surrender/srTypeRegistry.h"

int probeSelected(const char* name);
int probeListOnly();

static int g_initialized;

static int ensureInit()
{
    if (!g_initialized) {
        g_initialized = srInit() != 0 ? 1 : -1;
    }
    return g_initialized > 0;
}

static int filterProbe(unsigned long code, unsigned long* stored)
{
    *stored = code;
    return EXCEPTION_EXECUTE_HANDLER;
}

/* ------------------------------------------------------------------------ */
/* Registry name index: initial capacity 4, growth on the fifth insert,
   shrinking as instances are unregistered. */

/* findByName only returns instances whose getClassNode() derives from the
   requested node, so probes report the probe registry's node once they are
   named (naming refreshes the global registry through getClassNode()). */
static srRegistry::ClassNode* g_probe_node;

class RegistryProbe : public srRuntimeClass {
public:
    RegistryProbe() : in_probe_registry(0) {}
    virtual srRegistry::ClassNode* getClassNode() const
    {
        return in_probe_registry ? g_probe_node : srRuntimeClass::sGetClassNode();
    }
    int in_probe_registry;
};

enum { REGISTRY_PROBES = 40 };

static void reportLookups(srRegistry* registry, srRegistry::ClassNode* node, RegistryProbe** probes,
                          const char names[][16], int first_live, int live_end, int total,
                          const char* step)
{
    int index;
    int found = 0;
    int wrong = 0;
    int first_missing = -1;
    int stale = 0;
    for (index = 0; index < total; ++index) {
        srRuntimeClass* result = registry->find(node, names[index], 0);
        int live = index >= first_live && index < live_end;
        if (live) {
            if (result == probes[index]) {
                ++found;
            } else {
                if (first_missing < 0) {
                    first_missing = index;
                }
                if (result != 0) {
                    ++wrong;
                }
            }
        } else if (result != 0) {
            ++stale;
        }
    }
    printf("%s live %d found %d wrong %d first-missing %d stale %d\n", step, live_end - first_live,
           found, wrong, first_missing, stale);
}

static void registryProbe()
{
    static char names[REGISTRY_PROBES][16];
    RegistryProbe* probes[REGISTRY_PROBES];
    srRegistry* registry;
    srRegistry::ClassNode* node;
    int index;
    char step[64];

    registry = new srRegistry;
    node = registry->registerClass("srDiffTestProbe", registry->getRootClass(), 0x7ff00001, 1);
    g_probe_node = node;
    for (index = 0; index < REGISTRY_PROBES; ++index) {
        sprintf(names[index], "probe%02d", index);
        probes[index] = new RegistryProbe;
        probes[index]->setName(names[index]);
        probes[index]->in_probe_registry = 1;
    }
    for (index = 0; index < REGISTRY_PROBES; ++index) {
        registry->registerInstance(node, probes[index]);
        sprintf(step, "insert %02d", index);
        reportLookups(registry, node, probes, names, 0, index + 1, REGISTRY_PROBES, step);
    }
    printf("instances %ld\n", registry->getNumberOfInstances(node, 1));
    for (index = 0; index < REGISTRY_PROBES; ++index) {
        registry->unregisterInstance(node, probes[index]);
        sprintf(step, "remove %02d", index);
        reportLookups(registry, node, probes, names, index + 1, REGISTRY_PROBES, REGISTRY_PROBES,
                      step);
    }
    printf("instances %ld\n", registry->getNumberOfInstances(node, 1));
    /* Probes and the registry are leaked deliberately; see runCase. */
}

/* ------------------------------------------------------------------------ */
/* A runner-owned device. Records follow srDD.h; every call is counted. */

class ProbeDD : public srDD {
public:
    ProbeDD()
    {
        /* The first entry stays ARGB4444 (createNewTexture's starting format);
           the rest let format.match pick 16-bit, 32-bit and indexed storage. */
        static const srPixelConvert::e_surfaceType types[TEXTURE_FORMATS] = {
            srPixelConvert::SURFACE_ARGB4444, srPixelConvert::SURFACE_RGB565,
            srPixelConvert::SURFACE_BGRA32, srPixelConvert::SURFACE_P8};
        int index;
        memset(calls, 0, sizeof(calls));
        memset(formats, 0, sizeof(formats));
        texture_events = 0;
        dump_texture_rows = 0;
        for (index = 0; index < TEXTURE_FORMATS; ++index) {
            srPixelConvert::PixelFormat format;
            srPixelConvert::mapPixelFormat(types[index], format);
            /* srDD::PixelFormat is the converter format without its FourCC. */
            memcpy(&formats[index], &format, sizeof(formats[index]));
        }
        modes[0].width = 640;
        modes[0].height = 480;
        modes[0].depth = 16;
        modes[1].width = 800;
        modes[1].height = 600;
        modes[1].depth = 32;
        last_projection_type = -1;
    }

    virtual ~ProbeDD() {}
    virtual void getInfo(Info& info)
    {
        ++calls[0];
        info.max_back_buffer_width = 2048;
        info.max_back_buffer_height = 2048;
    }
    virtual void getWindowList(WindowInfoList& list)
    {
        ++calls[1];
        list.entries = modes;
        list.count = 2;
    }
    virtual void getTextureFormats(PixelFormatList& list)
    {
        ++calls[2];
        list.formats = formats;
        list.count = TEXTURE_FORMATS;
    }
    virtual void getStatistics(Statistics& stats)
    {
        (void)stats;
    }
    virtual void resetStatistics() {}
    virtual void extCommand(unsigned long command, void* data, unsigned long size)
    {
        (void)command;
        (void)data;
        (void)size;
    }
    virtual int isBusy()
    {
        return 0;
    }
    virtual e_error openWindow(const OpenInfo& info, OpenResult& result)
    {
        (void)info;
        result.back_buffer_type = 0;
        return ERROR_NONE;
    }
    virtual void closeWindow() {}
    virtual void beginFrame() {}
    virtual void endFrame() {}
    virtual void flushFrame() {}
    virtual void flipFrame(const Scissor* first, const Scissor* second, unsigned long value)
    {
        (void)first;
        (void)second;
        (void)value;
    }
    virtual void clearBuffers(const srFlags<e_buffer>& buffers)
    {
        (void)buffers;
    }
    virtual e_error bufferOp(const BufferCommand& command)
    {
        (void)command;
        return ERROR_NONE;
    }
    virtual void update(const Update& values)
    {
        (void)values;
    }
    virtual void setScissor(const Scissor& scissor)
    {
        (void)scissor;
    }
    virtual void setViewPort(const ViewPort& viewport)
    {
        (void)viewport;
    }
    virtual void setClearValues(const ClearValues& values)
    {
        (void)values;
    }
    virtual void setFogColor(const srVector4T<float>& color)
    {
        (void)color;
    }
    virtual void setShader(const srShader& shader)
    {
        (void)shader;
    }
    virtual void setTextureParameters(unsigned long stage, const TexParms& parms)
    {
        (void)stage;
        (void)parms;
    }
    virtual void bindTexture(unsigned long stage, Texture& texture)
    {
        recordTexture("bindTexture", stage, texture);
    }
    virtual void deleteTexture(Texture& texture)
    {
        (void)texture;
    }
    virtual void texImage(Texture& texture, unsigned long level)
    {
        recordTexture("texImage", level, texture);
    }
    virtual void texSubImage(Texture& texture, unsigned long a, unsigned long b, unsigned long c,
                             unsigned long d, unsigned long e)
    {
        (void)texture;
        (void)a;
        (void)b;
        (void)c;
        (void)d;
        (void)e;
    }
    virtual void setGlobalPalette(unsigned long* palette, unsigned long count)
    {
        (void)palette;
        (void)count;
    }
    virtual void bindPalette(Palette& palette)
    {
        (void)palette;
    }
    virtual void deletePalette(Palette& palette)
    {
        (void)palette;
    }
    virtual e_error createContext(unsigned long window)
    {
        (void)window;
        ++calls[3];
        return ERROR_NONE;
    }
    virtual void deleteContext() {}
    virtual void getDriverInfo(DriverInfo& info)
    {
        ++calls[4];
        info.dd_api_version = info.api_version;
        info.driver_id = 0x7f;
        strcpy(info.name, "difftest");
        strcpy(info.api_name, "difftest");
    }
    virtual void fence() {}
    virtual void getBufferPixelFormat(PixelFormat& format)
    {
        format = formats[0];
    }
    virtual void preBindTexture(unsigned long stage, Texture& texture)
    {
        (void)stage;
        (void)texture;
    }
    virtual void setPolygonMode(e_polygonMode mode)
    {
        (void)mode;
    }
    virtual void setCullMode(e_cullMode mode)
    {
        (void)mode;
    }
    virtual void setProjectionMatrix(const srMatrix4T<float>& matrix,
                                     srMatrix4T<float>::e_type type)
    {
        last_projection = matrix;
        last_projection_type = type;
    }
    virtual void setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info)
    {
        (void)info;
    }
    virtual void drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                              srRendererDefs::e_indexType type, const void* indices)
    {
        (void)primitive;
        (void)count;
        (void)type;
        (void)indices;
    }
    virtual void drawArrays(srRendererDefs::e_primitive primitive, long first, unsigned long count)
    {
        (void)primitive;
        (void)first;
        (void)count;
    }
    virtual void setPolygonOffset(long offset)
    {
        (void)offset;
    }

    /* Values and a per-level checksum of the pixel data the GERD hands over. */
    void recordTexture(const char* call, unsigned long argument, Texture& texture)
    {
        unsigned long level;
        if (texture_events >= 16) {
            return;
        }
        ++texture_events;
        /* texture.format is the source format the GERD asked for; the level
           data is in the matched device format. */
        const PixelFormat& stored =
            formats[texture.format_index < TEXTURE_FORMATS ? texture.format_index : 0];
        printf("dd %s %lu size %lux%lu levels %lu..%lu pixel-size %d stored %lu/%d\n", call,
               argument, texture.width, texture.height, texture.first_level, texture.last_level,
               static_cast<int>(texture.format.pixel_size), texture.format_index,
               static_cast<int>(stored.pixel_size));
        for (level = texture.first_level; level <= texture.last_level && level < 12; ++level) {
            unsigned long width = texture.width >> level;
            unsigned long height = texture.height >> level;
            unsigned long bytes;
            unsigned long sum = 2166136261UL;
            unsigned long offset;
            const unsigned char* data = static_cast<const unsigned char*>(texture.levels[level]);
            if (width == 0) {
                width = 1;
            }
            if (height == 0) {
                height = 1;
            }
            bytes = width * height * (static_cast<unsigned long>(stored.pixel_size) + 1);
            if (data == 0) {
                printf("dd   level %lu %lux%lu null\n", level, width, height);
                continue;
            }
            for (offset = 0; offset < bytes; ++offset) {
                sum = (sum ^ data[offset]) * 16777619UL;
            }
            printf("dd   level %lu %lux%lu fnv %08lx\n", level, width, height, sum);
            if (dump_texture_rows && level == texture.first_level) {
                unsigned long row_bytes = bytes / height;
                unsigned long row;
                for (row = 0; row < height && row < 8; ++row) {
                    printf("dd   row %lu ", row);
                    for (offset = 0; offset < row_bytes && offset < 64; ++offset) {
                        printf("%02x", data[row * row_bytes + offset]);
                    }
                    printf("\n");
                }
            }
        }
    }

    unsigned long calls[8];
    int texture_events;
    /* Set by gerdTextureCase: also print the first rows of the top level. */
    int dump_texture_rows;
    enum { TEXTURE_FORMATS = 4 };
    PixelFormat formats[TEXTURE_FORMATS];
    WindowInfo modes[2];
    srMatrix4T<float> last_projection;
    int last_projection_type;
};

/* The VC6 SDK headers predate message-only windows. */
#ifndef HWND_MESSAGE
#define HWND_MESSAGE (reinterpret_cast<HWND>(-3))
#endif

static srGERD* g_gerd;
static ProbeDD* g_dd;
static HWND g_window;

static int ensureRenderer()
{
    if (g_gerd != 0) {
        return 1;
    }
    if (!ensureInit()) {
        printf("error srInit failed\n");
        return 0;
    }
    g_dd = new ProbeDD;
    g_gerd = new srGERD(g_dd, 0, "difftest");
    g_window = CreateWindowA("STATIC", "sr-difftest", 0, 0, 0, 64, 64, HWND_MESSAGE, 0,
                             GetModuleHandleA(0), 0);
    printf("window %s\n", g_window != 0 ? "created" : "failed");
    return 1;
}

static int g_context_result = -2;

static void driverListProbe()
{
    unsigned long code = 0;
    unsigned long count = 0;
    int result = -1;
    unsigned long index;
    if (!ensureRenderer()) {
        return;
    }
    __try {
        result = g_gerd->createContext(reinterpret_cast<unsigned long>(g_window));
    } __except (filterProbe(GetExceptionCode(), &code)) {
        printf("crash createContext %08lx\n", code);
        return;
    }
    g_context_result = result;
    printf("createContext %d\n", result);
    printf("dd-calls getInfo %lu getWindowList %lu getTextureFormats %lu\n", g_dd->calls[0],
           g_dd->calls[1], g_dd->calls[2]);
    count = g_gerd->getTextureFormatCount();
    printf("texture-format-count %lu\n", count);
    count = g_gerd->getDisplayModeCount();
    printf("display-mode-count %lu\n", count);
    for (index = 0; index < 2; ++index) {
        printf("display-mode %lux%lux%lu index %ld\n", g_dd->modes[index].width,
               g_dd->modes[index].height, g_dd->modes[index].depth,
               g_gerd->getDisplayMode(g_dd->modes[index].width, g_dd->modes[index].height,
                                      g_dd->modes[index].depth));
    }
    printf("display-mode 1024x768x16 index %ld\n", g_gerd->getDisplayMode(1024, 768, 16));
}

/* Opening a window builds the default texture (64x64, seven mipmap levels
   with the GERD's default minimum dimension of 1) through
   srGERD::createNewTexture's MultiRequest; binding uploads it. */
static void textureProbe()
{
    long result;
    if (g_context_result == -2) {
        driverListProbe();
    }
    if (g_context_result != 0) {
        printf("error no context\n");
        return;
    }
    {
        RECT client;
        GetClientRect(g_window, &client);
        printf("window-size %ldx%ld\n", client.right, client.bottom);
    }
    result = g_gerd->openWindow(32, 32);
    printf("openWindow %ld\n", result);
    printf("texture-events %d\n", g_dd->texture_events);
    g_gerd->setTexture(srCore.getTexture(), 0);
    printf("setTexture done, texture-events %d\n", g_dd->texture_events);
}

/* Binds a runner-built texture through the GERD; ProbeDD records every
   uploaded level. */
typedef void* ExportAddress;

static ExportAddress g_apply_state;

static void callMember0(ExportAddress function, void* self)
{
    __asm {
        mov ecx, self
        call function
    }
}

void gerdTextureCase(srTextureIFace* texture)
{
    if (g_context_result == -2) {
        driverListProbe();
    }
    if (g_context_result != 0) {
        printf("error no context\n");
        return;
    }
    static int window_open = 0;
    if (!window_open) {
        g_gerd->openWindow(32, 32);
        window_open = 1;
    }
    if (g_apply_state == 0) {
        g_apply_state = reinterpret_cast<ExportAddress>(
            GetProcAddress(GetModuleHandleA("sr.dll"), "?applyDrawStateChanges@srGERD@@AAEXXZ"));
    }
    g_dd->texture_events = 0;
    g_dd->dump_texture_rows = 1;
    g_gerd->setTexture(texture, 0);
    /* setTexture only builds the GERD copy; the bind happens when the next
       draw applies the dirty state. */
    callMember0(g_apply_state, g_gerd);
    printf("texture-events %d\n", g_dd->texture_events);
    srGERD::TextureInfo info;
    memset(&info, 0, sizeof(info));
    int known = g_gerd->getTextureInfo(texture, info);
    printf("texture-info %d %lux%lu last %lu format %d/%d %d/%d %d/%d %d/%d size %d cached %d\n",
           known, info.width, info.height, info.last_level, info.pixel_format.red_bits,
           info.pixel_format.red_shift, info.pixel_format.green_bits, info.pixel_format.green_shift,
           info.pixel_format.blue_bits, info.pixel_format.blue_shift, info.pixel_format.alpha_bits,
           info.pixel_format.alpha_shift, static_cast<int>(info.pixel_format.pixel_size),
           g_gerd->isTextureCached(texture));
    g_dd->dump_texture_rows = 0;
    g_gerd->setTexture(srCore.getTexture(), 0);
    callMember0(g_apply_state, g_gerd);
    g_gerd->invalidateTexture(texture);
}

/* ------------------------------------------------------------------------ */
/* Camera: processPush loads the view matrix into the renderer. */

static ExportAddress g_process_push;
static ExportAddress g_process_pop;

static void callMember1(ExportAddress function, void* self, void* argument)
{
    __asm {
        mov ecx, self
        push argument
        call function
    }
}

static void printDoubleMatrix(const char* label, const srMatrix4T<double>& matrix)
{
    int row;
    for (row = 0; row < 4; ++row) {
        const double* values = &matrix.vectors[row].x;
        int column;
        for (column = 0; column < 4; ++column) {
            unsigned long words[2];
            memcpy(words, &values[column], 8);
            printf("out %s[%d].%d %08lx%08lx %.17g\n", label, row, column, words[1], words[0],
                   values[column]);
        }
    }
}

static void pushAndRead(srCamera* camera)
{
    srMatrix4T<double> view;
    callMember1(g_process_push, camera, g_gerd);
    g_gerd->getMatrix(srGERD::MATRIX_MODELVIEW, view);
    printDoubleMatrix("modelview", view);
    callMember1(g_process_pop, camera, g_gerd);
}

static int protectedPush(srCamera* camera, unsigned long* code)
{
    __try {
        pushAndRead(camera);
    } __except (filterProbe(GetExceptionCode(), code)) {
        return 0;
    }
    return 1;
}

static void cameraProbe(const char* tag, double x, double y, double z, double angle,
                        const srVector3T<double>& axis)
{
    srCamera* camera;
    unsigned long code = 0;
    if (!ensureRenderer()) {
        return;
    }
    if (g_process_push == 0) {
        HMODULE module = GetModuleHandleA("sr.dll");
        g_process_push = reinterpret_cast<ExportAddress>(
            GetProcAddress(module, "?processPush@srCamera@@IAEXPAVsrGERD@@@Z"));
        g_process_pop = reinterpret_cast<ExportAddress>(
            GetProcAddress(module, "?processPop@srCamera@@IAEXPAVsrGERD@@@Z"));
    }
    camera = new srCamera(0);
    camera->setLocation(x, y, z);
    if (angle != 0.0) {
        camera->rotate(angle, axis);
    }
    printf("camera %s location %.17g %.17g %.17g angle %.17g\n", tag, x, y, z, angle);
    if (!protectedPush(camera, &code)) {
        printf("crash %08lx\n", code);
    }
}

/* ------------------------------------------------------------------------ */

void runProbe(const char* name, void (*probe)())
{
    unsigned long code = 0;
    if (!probeSelected(name)) {
        return;
    }
    if (probeListOnly()) {
        printf("%s\n", name);
        return;
    }
    printf("case %s\n", name);
    if (!ensureInit()) {
        printf("error srInit failed\n");
    } else {
        __try {
            probe();
        } __except (filterProbe(GetExceptionCode(), &code)) {
            printf("crash %08lx\n", code);
        }
    }
    printf("end %s\n", name);
    fflush(stdout);
}

static void cameraIdentity()
{
    cameraProbe("identity", 0, 0, 0, 0, srVector3T<double>(0, 1, 0));
}

static void cameraTranslated()
{
    cameraProbe("translated", 1, 2, 3, 0, srVector3T<double>(0, 1, 0));
}

static void cameraRotated()
{
    cameraProbe("rotated", 0, 0, 0, 0.5, srVector3T<double>(0, 1, 0));
}

static void cameraTranslatedRotated()
{
    cameraProbe("translated-rotated", -4, 0.5, 10, 1.25, srVector3T<double>(1, 0, 0));
}

void surfaceCases();
void huffmanCases();
void miscCases();
void streamCases();
void modelCases();
extern const char* g_blob_file;

void probeCases()
{
    runProbe("probe.registry.grow-shrink", registryProbe);
    runProbe("probe.gerd.driver-lists", driverListProbe);
    runProbe("probe.gerd.default-texture", textureProbe);
    runProbe("probe.camera.identity", cameraIdentity);
    runProbe("probe.camera.translated", cameraTranslated);
    runProbe("probe.camera.rotated", cameraRotated);
    runProbe("probe.camera.translated-rotated", cameraTranslatedRotated);
    surfaceCases();
    huffmanCases();
    miscCases();
    streamCases();
    modelCases();
}
