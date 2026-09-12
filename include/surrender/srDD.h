#pragma once

#include "srFlags.h"
#include "srMath.h"
#include "srRendererDefs.h"
#include "srShader.h"

class srDD;

// Dynamic driver entrypoints, proven by DirectX7 and the srGERD loader.
// This is a reconstructed header grouping, not a recovered original filename.
// Index/count/API values occupy one 32-bit word. The loader compares them
// unsigned; surviving exports do not distinguish original int/long spelling.
enum { SR_DD_MIN_API_VERSION = 0x128 };
typedef unsigned long(__cdecl* srDDGetDriverApiVersionFn)();
typedef const char*(__cdecl* srDDGetDriverNameFn)();
typedef void(__cdecl* srDDConfigureDriverFn)(const char* configuration);
typedef unsigned long(__cdecl* srDDGetDeviceCountFn)();
typedef const char*(__cdecl* srDDGetDeviceNameFn)(unsigned long index);
typedef srDD*(__cdecl* srDDInitDeviceFn)(unsigned long index);

// Argument-bearing DirectX7 entrypoints and srGERD call sites prove caller
// stack cleanup. No-argument functions alone cannot prove cdecl vs stdcall.
// Nested records are incomplete: DebugDD's virtuals pass them by reference.
class __declspec(novtable) srDD {
public:
    struct Palette;
    struct Texture;
    struct BufferCommand;
    struct PixelFormat;
    struct DriverInfo;
    struct Info;
    struct Statistics;
    struct PixelFormatList;
    struct WindowInfoList;
    struct OpenInfo;
    struct OpenResult;
    struct ClearValues;
    struct Scissor;
    struct ViewPort;
    struct Update;
    struct TexParms;

    enum e_error {};
    enum e_buffer {};
    enum e_command {};
    enum e_cullMode {};
    enum e_polygonMode {};
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
