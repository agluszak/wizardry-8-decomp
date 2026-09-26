#pragma once

#include "srDD.h"

class srGERD;

// VTABLE: SURRENDER 0x100765f0 srDebugDD
/* Provider-side debug wrapper. Retail exports the entire member surface,
   including the implicit copy constructor/assignment and the vtable, so the
   class is dllexport-ed when building the provider (the same convention as
   srDummyStreamBuf). The implicit copy operations emit the srDD empty-base
   byte copy and store the vtable last, matching retail. No known
   Wizardry/JPEG/ZIP consumer imports srDebugDD symbols; its exported methods
   are provider ABI only. */
class
#if defined(SURRENDER_BUILD)
    __declspec(dllexport)
#endif
    srDebugDD : public srDD {
public:
    srDebugDD(srDD* device);
    virtual ~srDebugDD() override;

    /* Implicit copy constructor/assignment emitted via the class-level
       dllexport. The copy constructor stores the vtable last; the assignment
       does not. */
    // SYNTHETIC: SURRENDER 0x100177D0
    // ??0srDebugDD@@QAE@ABV0@@Z
    // SYNTHETIC: SURRENDER 0x10017830
    // ??4srDebugDD@@QAEAAV0@ABV0@@Z

    virtual void getInfo(Info& info) override;
    virtual void getWindowList(WindowInfoList& list) override;
    virtual void getTextureFormats(PixelFormatList& list) override;
    virtual void getStatistics(Statistics& stats) override;
    virtual void resetStatistics() override;
    virtual void extCommand(unsigned long command, void* data, unsigned long size) override;
    virtual int isBusy() override;
    virtual e_error openWindow(const OpenInfo& info, OpenResult& result) override;
    virtual void closeWindow() override;
    virtual void beginFrame() override;
    virtual void endFrame() override;
    virtual void flushFrame() override;
    virtual void flipFrame(const Scissor* first, const Scissor* second,
                           unsigned long value) override;
    virtual void clearBuffers(const srFlags<e_buffer>& buffers) override;
    virtual e_error bufferOp(const BufferCommand& command) override;
    virtual void update(const Update& values) override;
    virtual void setScissor(const Scissor& scissor) override;
    virtual void setViewPort(const ViewPort& viewport) override;
    virtual void setClearValues(const ClearValues& values) override;
    virtual void setFogColor(const srVector4T<float>& color) override;
    virtual void setShader(const srShader& shader) override;
    virtual void setTextureParameters(unsigned long stage, const TexParms& parms) override;
    virtual void bindTexture(unsigned long stage, Texture& texture) override;
    virtual void deleteTexture(Texture& texture) override;
    virtual void texImage(Texture& texture, unsigned long level) override;
    virtual void texSubImage(Texture& texture, unsigned long a, unsigned long b, unsigned long c,
                             unsigned long d, unsigned long e) override;
    virtual void setGlobalPalette(unsigned long* palette, unsigned long count) override;
    virtual void bindPalette(Palette& palette) override;
    virtual void deletePalette(Palette& palette) override;
    virtual e_error createContext(unsigned long window) override;
    virtual void deleteContext() override;
    virtual void getDriverInfo(DriverInfo& info) override;
    virtual void fence() override;
    virtual void getBufferPixelFormat(PixelFormat& format) override;
    virtual void preBindTexture(unsigned long stage, Texture& texture) override;
    virtual void setPolygonMode(e_polygonMode mode) override;
    virtual void setCullMode(e_cullMode mode) override;
    virtual void setProjectionMatrix(const srMatrix4T<float>& matrix,
                                     srMatrix4T<float>::e_type type) override;
    virtual void setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info) override;
    virtual void drawElements(srRendererDefs::e_primitive primitive, unsigned long count,
                              srRendererDefs::e_indexType type, const void* indices) override;
    virtual void drawArrays(srRendererDefs::e_primitive primitive, long first,
                            unsigned long count) override;
    virtual void setPolygonOffset(long offset) override;

    unsigned long getFunctionCallCount(e_command command) const;
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
        srDebugDD* owner_00;
        e_command command_04;
        double start_08;
    };
    /* VC6 does not grant a nested class access to the enclosing class's
       protected members, so the statistics arrays stay reachable through an
       explicit friend declaration. */
    friend class ScopeTimer;
    /* srGERD::dump (0x1001E9A6) reads call_counts_170 and funcName directly,
       so srGERD was a friend. */
    friend class srGERD;

private:
    void resetInternalStatistics();

    /* Command names indexed by e_command: funcName[0] is "dummy command",
       funcName[42] is "setPolygonOffset()", funcName[43] is "CMDMAX".
       Mangles private static (@@0PAPBDA). */
    static const char* funcName[0x2c];

    srDD* device_04;
    unsigned long unknown_08;
    /* +0x0c..+0x0f is alignment padding before time_scale_10, not a member:
       the implicit copy operations skip it. */
    double time_scale_10;
    double call_times_18[0x2b];
    unsigned long call_counts_170[0x2b];
};
