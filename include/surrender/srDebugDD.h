#pragma once

#include "srDD.h"

class srGERD;

// VTABLE: SURRENDER 0x100765f0 srDebugDD
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srDebugDD : public srDD {
public:
    srDebugDD(srDD* device);

    /* The copy constructor stores the vtable last; the assignment
       does not. */

    virtual void getInfo(Info& info) override;
    virtual void getWindowList(WindowInfoList& list) override;
    virtual void getTextureFormats(PixelFormatList& list) override;
    virtual void getStatistics(Statistics& stats) override;
    virtual void resetStatistics() override;
    virtual void extCommand(w8_ulong command, void* data, w8_ulong size) override;
    virtual int isBusy() override;
    virtual e_error openWindow(const OpenInfo& info, OpenResult& result) override;
    virtual void closeWindow() override;
    virtual void beginFrame() override;
    virtual void endFrame() override;
    virtual void flushFrame() override;
    virtual void flipFrame(const Scissor* first, const Scissor* second, w8_ulong value) override;
    virtual void clearBuffers(const srFlags<e_buffer>& buffers) override;
    virtual e_error bufferOp(const BufferCommand& command) override;
    virtual void update(const Update& values) override;
    virtual void setScissor(const Scissor& scissor) override;
    virtual void setViewPort(const ViewPort& viewport) override;
    virtual void setClearValues(const ClearValues& values) override;
    virtual void setFogColor(const srVector4T<float>& color) override;
    virtual void setShader(const srShader& shader) override;
    virtual void setTextureParameters(w8_ulong stage, const TexParms& parms) override;
    virtual void bindTexture(w8_ulong stage, Texture& texture) override;
    virtual void deleteTexture(Texture& texture) override;
    virtual void texImage(Texture& texture, w8_ulong level) override;
    virtual void texSubImage(Texture& texture, w8_ulong a, w8_ulong b, w8_ulong c, w8_ulong d,
                             w8_ulong e) override;
    virtual void setGlobalPalette(w8_ulong* palette, w8_ulong count) override;
    virtual void bindPalette(Palette& palette) override;
    virtual void deletePalette(Palette& palette) override;
    virtual e_error createContext(w8_ulong_ptr window) override;
    virtual void deleteContext() override;
    virtual void getDriverInfo(DriverInfo& info) override;
    virtual void fence() override;
    virtual void getBufferPixelFormat(PixelFormat& format) override;
    virtual void preBindTexture(w8_ulong stage, Texture& texture) override;
    virtual void setPolygonMode(e_polygonMode mode) override;
    virtual void setCullMode(e_cullMode mode) override;
    virtual void setProjectionMatrix(const srMatrix4T<float>& matrix,
                                     srMatrix4T<float>::e_type type) override;
    virtual void setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info) override;
    virtual void drawElements(srRendererDefs::e_primitive primitive, w8_ulong count,
                              srRendererDefs::e_indexType type, const void* indices) override;
    virtual void drawArrays(srRendererDefs::e_primitive primitive, w8_long first,
                            w8_ulong count) override;
    virtual void setPolygonOffset(w8_long offset) override;

    w8_ulong getFunctionCallCount(e_command command) const;
    double getFunctionCallTime(e_command command) const;
    const char* getFunctionName(e_command command) const;
    void increaseCallCount(e_command command);
    void increaseCallTime(e_command command, double time);

    /* RAII timer constructed at the top of every srDebugDD forwarder and
       destroyed after the wrapped call returns. */
    class ScopeTimer {
    public:
        ScopeTimer(srDebugDD* owner, e_command command);
        ~ScopeTimer();

    private:
        srDebugDD* owner;
        e_command command;
        double start_time;
    };
    friend class ScopeTimer;
    friend class srGERD;

private:
    void resetInternalStatistics();

    /* Command names indexed by e_command. */
    static const char* funcName[0x2c];

    srDD* device;
    w8_ulong unknown_08;
    double time_scale;
    double call_times[0x2b];
    w8_ulong call_counts[0x2b];
};
