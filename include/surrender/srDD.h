#pragma once

#include "srFlags.h"
#include "srMath.h"
#include "srPixelConvert.h"
#include "srRendererDefs.h"
#include "srShader.h"

class srARGB;
class srDD;

// Dynamic driver entrypoints.
enum { SR_DD_MIN_API_VERSION = 0x128 };
typedef unsigned long(__cdecl* srDDGetDriverApiVersionFn)();
typedef const char*(__cdecl* srDDGetDriverNameFn)();
typedef void(__cdecl* srDDConfigureDriverFn)(const char* configuration);
typedef unsigned long(__cdecl* srDDGetDeviceCountFn)();
typedef const char*(__cdecl* srDDGetDeviceNameFn)(unsigned long index);
typedef srDD*(__cdecl* srDDInitDeviceFn)(unsigned long index);

/* Empty base class; its original name is unknown. */
class srDDEmptyBase {};

class __declspec(novtable) srDD : public srDDEmptyBase {
public:
    /* Device palette record handed to bindPalette/deletePalette. */
    struct Palette {
        const srARGB* data;
        unsigned long size;
        unsigned long flags;
    };
    /* Device pixel format: srPixelConvert::PixelFormat without its trailing FourCC word. */
    struct PixelFormat {
        unsigned char red_bits;
        unsigned char red_shift;
        unsigned char green_bits;
        unsigned char green_shift;
        unsigned char blue_bits;
        unsigned char blue_shift;
        unsigned char alpha_bits;
        unsigned char alpha_shift;
        /* +8 conversion class of srPixelConvert::PixelFormat, copied
           verbatim by convertPixelFormat. */
        srPixelConvert::e_colorModel color_model;
        srPixelConvert::e_pixelSize pixel_size;
    };
    /* Device texture record handed to bindTexture/deleteTexture; levels holds the per-level data
       pointers. */
    struct Texture {
        /* Shared initialization in GERD allocation/deletion. reset is a
           descriptive name; format, deleted and resident are set separately. */
        void reset()
        {
            flags = 0;
            size = 0;
            last_use = 0;
            priority = 0.5f;
            resident_data = 0;
            width = 0;
            height = 0;
            first_level = 0;
            last_level = 0;
            format_index = 0;
            parameter = 0;
            resident_size = 0;
            for (long level = 0; level < 12; ++level) {
                levels[level] = 0;
            }
        }

        unsigned long flags;
        PixelFormat format;
        float priority;
        unsigned long last_use;
        unsigned long size;
        unsigned long width;
        unsigned long height;
        unsigned long first_level;
        unsigned long last_level;
        unsigned long format_index;
        unsigned long parameter;
        void* levels[12];
        /* Resident device-surface record and its byte size. */
        unsigned long resident_data;
        unsigned long resident_size;
        /* markTextureAsDeleted sets this once the texture is on the
           GERD-side deleted list. */
        unsigned long deleted;
        unsigned long resident;
    };
    /* Six-dword buffer command handed to bufferOp. Opcodes: 0 lock, 1 unlock, 2 read row, 3 write
       row, 4 horizontal fill, 6 read column, 7 write column. Lock and unlock fill only the leading
       dwords. */
    struct BufferCommand {
        unsigned long flags;
        unsigned long opcode;
        void* data;
        long x;
        long y;
        long count;
    };
    /* getDriverInfo block: the caller writes the requested API version, a capability flag and a
       debug callback; the device reports its DD API version, driver id, name and API version
       string. */
    struct DriverInfo {
        DriverInfo() : flags(0) {}

        unsigned long api_version;
        unsigned long flags;
        void (*debug_write)(const char* text);
        unsigned long dd_api_version;
        unsigned long driver_id;
        char name[64];
        char api_name[64];
    };
    static_assert(sizeof(DriverInfo) == 0x94, "srDD_DriverInfo_must_be_0x94");
    /* getInfo output record. The nine trailing strings are the device identity fields. */
    struct Info {
        enum { PIXEL_TEXTURE_STATISTICS = 0x10u, RELEASE_SURFACE_AFTER_BIND = 0x20u };
        Info() : flags(0) {}

        /* openWindowInternal rejects back-buffer dimensions above these
           maximums. */
        unsigned long max_back_buffer_width;
        unsigned long max_back_buffer_height;
        /* initDDInfo defaults: 4. */
        unsigned long unknown_08_;
        /* initDDInfo defaults: 0x10. */
        unsigned long unknown_0c_;
        /* initDDInfo defaults: 1.0f / 65536.0f. */
        float unknown_10_;
        float unknown_14_;
        /* changeTexture tests bit 5; getDepthBufferType reads bit 3. */
        unsigned long flags;
        /* initDDInfo defaults: 0x100; createRenderer passes it to each
           Renderer as its batch limit. */
        unsigned long renderer_batch_limit;
        /* initDDInfo defaults: 0x3b808081 / 0x200000. */
        unsigned long unknown_20_;
        /* Device texture RAM in bytes; dumpTextureCache prints it in kB and
           treats 0 as "infinite" (no residency percentage). */
        unsigned long texture_ram;
        /* Texture-dimension clamps applied by evaluateTextureDimensions. */
        unsigned long max_texture_stages;
        unsigned long texture_min_dim;
        unsigned long texture_max_dim;
        unsigned long texture_max_aspect;
        /* initDDInfo defaults: 1. getHardwareID result; e_hardwareID is an
           empty enum and cannot be the field type. */
        unsigned long hardware_id;
        /* Device identity strings in getter order: device name, vendor,
           platform, driver name, vendor, version, hardware chipset, name,
           vendor. */
        char text[9][0x40];
    };
    static_assert(sizeof(Info) == 0x27c, "srDD_Info_must_be_0x27c");
    /* getStatistics output record. */
    struct Statistics {
        double texture_transfer;
        double pixels_drawn;
        unsigned long value_10;
        unsigned long value_14;
        unsigned long triangles_received;
        unsigned long vertices_transferred;
        unsigned long vertex_indices;
        unsigned long value_24;
    };
    /* getTextureFormats / getWindowList fill {count, pointer} out-records;
       GERD copies the pointed arrays into its own storage. */
    struct PixelFormatList {
        long count;
        PixelFormat* formats;
    };
    /* getDisplayMode matches {width, height, depth} triples. */
    struct WindowInfo {
        unsigned long width;
        unsigned long height;
        unsigned long depth;
    };
    static_assert(sizeof(WindowInfo) == 0x0c, "WindowInfo_must_be_0x0c");
    struct WindowInfoList {
        long count;
        WindowInfo* entries;
    };
    /* srGERD::openWindowInternal forwards the requested backbuffer size and
       display-mode index; the device reports the back-buffer count in the
       single-dword result. */
    struct OpenInfo {
        unsigned long width;
        unsigned long height;
        long display_mode;
    };
    struct OpenResult {
        unsigned long back_buffer_type;
    };
    /* setClearColor clamps color, accumClear clamps accum to [-1,1] per channel and setClearDepth
       clamps depth to [0,1]. */
#pragma pack(push, 4)
    struct ClearValues {
        srVector4T<float> color;
        srVector4T<float> accum;
        double depth;
        unsigned long stencil;
    };
#pragma pack(pop)
    static_assert(sizeof(ClearValues) == 0x2c, "srDD_ClearValues_must_be_0x2c");
    /* Scissor rectangle {left, top, right, bottom}. */
    struct Scissor {
        unsigned long left, top, right, bottom;
    };
    /* setViewPort receives (x, y, width, height) plus four opaque trailing
       slots applyViewStateChanges copies through unchanged. */
    struct ViewPort {
        long x, y, width, height;
        /* Depth range forwarded by applyViewStateChanges. */
        unsigned long extra[4];
    };
    /* Frame state handed to update(): dirty bits, gamma, a constant 1.0f, swap interval, antialias
       mode and enable bit 0. */
    struct Update {
        enum {
            UPDATE_ENABLE = 0x01u,
            UPDATE_SWAP_INTERVAL = 0x02u,
            UPDATE_GAMMA = 0x04u,
            UPDATE_ANTIALIAS = 0x08u
        };
        unsigned long flags;
        srVector3T<float> gamma;
        float value_10;
        unsigned long swap_interval;
        unsigned long antialias;
        unsigned long enabled;
    };
    /* srGERD::setTextureParameters repacks the texture's Parameters into
       this per-stage record and passes it to the device. */
    struct TexParms {
        unsigned long packed;
        float mipmap_bias;
    };

    enum e_error { ERROR_NONE = 0 };
    /* srDD_OpenGL clearBuffers: bit 0 clears GL_COLOR_BUFFER_BIT, bit 1
       GL_DEPTH_BUFFER_BIT and bit 2 GL_STENCIL_BUFFER_BIT. */
    enum e_buffer { BUFFER_COLOR = 1, BUFFER_DEPTH = 2, BUFFER_STENCIL = 4 };
    /* Commands profiled by srDebugDD, in funcName table order; the names are descriptive. */
    enum e_command {
        COMMAND_DUMMY,
        COMMAND_GET_INFO,
        COMMAND_GET_WINDOW_LIST,
        COMMAND_GET_TEXTURE_FORMAT,
        COMMAND_GET_STATISTICS,
        COMMAND_RESET_STATISTICS,
        COMMAND_CLOSE_WINDOW,
        COMMAND_BEGIN_FRAME,
        COMMAND_END_FRAME,
        COMMAND_FLUSH_FRAME,
        COMMAND_FLIP_FRAME,
        COMMAND_CLEAR_BUFFERS,
        COMMAND_UPDATE,
        COMMAND_SET_SCISSOR,
        COMMAND_SET_VIEW_PORT,
        COMMAND_SET_CLEAR_VALUES,
        COMMAND_SET_FOG_COLOR,
        COMMAND_SET_SHADER,
        COMMAND_DELETE_TEXTURE,
        COMMAND_DELETE_PALETTE,
        COMMAND_DELETE_CONTEXT,
        COMMAND_GET_DRIVER_INFO,
        COMMAND_OPEN_WINDOW,
        COMMAND_IS_BUSY,
        COMMAND_BUFFER_OP,
        COMMAND_CREATE_CONTEXT,
        COMMAND_EXT_COMMAND,
        COMMAND_BIND_TEXTURE,
        COMMAND_SET_TEXTURE_PARAMETERS,
        COMMAND_TEX_IMAGE,
        COMMAND_TEX_SUB_IMAGE,
        COMMAND_SET_GLOBAL_PALETTE,
        COMMAND_BIND_PALETTE,
        COMMAND_FENCE,
        COMMAND_GET_BUFFER_PIXEL_FORMAT,
        COMMAND_PRE_BIND_TEXTURE,
        COMMAND_SET_POLYGON_MODE,
        COMMAND_SET_CULL_MODE,
        COMMAND_SET_PROJECTION_MATRIX,
        COMMAND_SET_VERTEX_ARRAY_INFO,
        COMMAND_DRAW_ELEMENTS,
        COMMAND_DRAW_ARRAYS,
        COMMAND_SET_POLYGON_OFFSET,
        COMMAND_MAX
    };
    /* srDD_OpenGL setCullMode: 0 disables GL_CULL_FACE, 1 culls GL_BACK and
       2 culls GL_FRONT. */
    enum e_cullMode { CULL_NONE = 0, CULL_BACK = 1, CULL_FRONT = 2 };
    /* srDD_OpenGL setPolygonMode: glPolygonMode(GL_FRONT_AND_BACK, ...) with
       GL_POINT, GL_LINE and GL_FILL. */
    enum e_polygonMode { POLYGON_POINT = 0, POLYGON_LINE = 1, POLYGON_FILL = 2 };
    enum e_driverID {};
    enum e_hardwareID {};

    virtual ~srDD() = 0;
    virtual void getInfo(Info& info) = 0;
    virtual void getWindowList(WindowInfoList& list) = 0;
    virtual void getTextureFormats(PixelFormatList& list) = 0;
    virtual void getStatistics(Statistics& stats) = 0;
    virtual void resetStatistics() = 0;
    virtual void extCommand(unsigned long command, void* data, unsigned long size) = 0;
    virtual int isBusy() = 0;
    virtual e_error openWindow(const OpenInfo& info, OpenResult& result) = 0;
    virtual void closeWindow() = 0;
    virtual void beginFrame() = 0;
    virtual void endFrame() = 0;
    virtual void flushFrame() = 0;
    virtual void flipFrame(const Scissor* first, const Scissor* second, unsigned long value) = 0;
    virtual void clearBuffers(const srFlags<e_buffer>& buffers) = 0;
    virtual e_error bufferOp(const BufferCommand& command) = 0;
    virtual void update(const Update& values) = 0;
    virtual void setScissor(const Scissor& scissor) = 0;
    virtual void setViewPort(const ViewPort& viewport) = 0;
    virtual void setClearValues(const ClearValues& values) = 0;
    virtual void setFogColor(const srVector4T<float>& color) = 0;
    virtual void setShader(const srShader& shader) = 0;
    virtual void setTextureParameters(unsigned long stage, const TexParms& parms) = 0;
    virtual void bindTexture(unsigned long stage, Texture& texture) = 0;
    virtual void deleteTexture(Texture& texture) = 0;
    virtual void texImage(Texture& texture, unsigned long level) = 0;
    virtual void texSubImage(Texture& texture, unsigned long a, unsigned long b, unsigned long c,
                             unsigned long d, unsigned long e) = 0;
    virtual void setGlobalPalette(unsigned long* palette, unsigned long count) = 0;
    virtual void bindPalette(Palette& palette) = 0;
    virtual void deletePalette(Palette& palette) = 0;
    virtual e_error createContext(unsigned long window) = 0;
    virtual void deleteContext() = 0;
    virtual void getDriverInfo(DriverInfo& info) = 0;
    virtual void fence() = 0;
    virtual void getBufferPixelFormat(PixelFormat& format) = 0;
    virtual void preBindTexture(unsigned long stage, Texture& texture) = 0;
    virtual void setPolygonMode(e_polygonMode mode) = 0;
    virtual void setCullMode(e_cullMode mode) = 0;
    virtual void setProjectionMatrix(const srMatrix4T<float>& matrix,
                                     srMatrix4T<float>::e_type type) = 0;
    virtual void setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info) = 0;
    virtual void drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                              srRendererDefs::e_indexType type, const void* indices) = 0;
    virtual void drawArrays(srRendererDefs::e_primitive primitive, long first,
                            unsigned long count) = 0;
    virtual void setPolygonOffset(long offset) = 0;
};

inline srDD::~srDD() {}
