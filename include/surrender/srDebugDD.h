#pragma once

#include "srDD.h"

// VTABLE: SURRENDER 0x100765f0 srDebugDD
class srDebugDD : public srDD {
public:
    SR_DLL_IMPORT srDebugDD(srDD* device);
    SR_DLL_IMPORT srDebugDD(const srDebugDD& other);
    SR_DLL_IMPORT srDebugDD& operator=(const srDebugDD& other);
    virtual ~srDebugDD() override;

    virtual SR_DLL_IMPORT void getInfo(Info& info) override;
    virtual SR_DLL_IMPORT void getWindowList(WindowInfoList& list) override;
    virtual SR_DLL_IMPORT void getTextureFormats(PixelFormatList& list) override;
    virtual SR_DLL_IMPORT void getStatistics(Statistics& stats) override;
    virtual SR_DLL_IMPORT void resetStatistics() override;
    virtual SR_DLL_IMPORT void extCommand(unsigned long command, void* data,
                                          unsigned long size) override;
    virtual SR_DLL_IMPORT int isBusy() override;
    virtual SR_DLL_IMPORT e_error openWindow(const OpenInfo& info, OpenResult& result) override;
    virtual SR_DLL_IMPORT void closeWindow() override;
    virtual SR_DLL_IMPORT void beginFrame() override;
    virtual SR_DLL_IMPORT void endFrame() override;
    virtual SR_DLL_IMPORT void flushFrame() override;
    virtual SR_DLL_IMPORT void flipFrame(const Scissor* first, const Scissor* second,
                                         unsigned long value) override;
    virtual SR_DLL_IMPORT void clearBuffers(const srFlags<e_buffer>& buffers) override;
    virtual SR_DLL_IMPORT e_error bufferOp(const BufferCommand& command) override;
    virtual SR_DLL_IMPORT void update(const Update& values) override;
    virtual SR_DLL_IMPORT void setScissor(const Scissor& scissor) override;
    virtual SR_DLL_IMPORT void setViewPort(const ViewPort& viewport) override;
    virtual SR_DLL_IMPORT void setClearValues(const ClearValues& values) override;
    virtual SR_DLL_IMPORT void setFogColor(const srVector4T<float>& color) override;
    virtual SR_DLL_IMPORT void setShader(const srShader& shader) override;
    virtual SR_DLL_IMPORT void setTextureParameters(unsigned long stage,
                                                    const TexParms& parms) override;
    virtual SR_DLL_IMPORT void bindTexture(unsigned long stage, Texture& texture) override;
    virtual SR_DLL_IMPORT void deleteTexture(Texture& texture) override;
    virtual SR_DLL_IMPORT void texImage(Texture& texture, unsigned long level) override;
    virtual SR_DLL_IMPORT void texSubImage(Texture& texture, unsigned long a, unsigned long b,
                                           unsigned long c, unsigned long d,
                                           unsigned long e) override;
    virtual SR_DLL_IMPORT void setGlobalPalette(unsigned long* palette,
                                                unsigned long count) override;
    virtual SR_DLL_IMPORT void bindPalette(Palette& palette) override;
    virtual SR_DLL_IMPORT void deletePalette(Palette& palette) override;
    virtual SR_DLL_IMPORT e_error createContext(unsigned long window) override;
    virtual SR_DLL_IMPORT void deleteContext() override;
    virtual SR_DLL_IMPORT void getDriverInfo(DriverInfo& info) override;
    virtual SR_DLL_IMPORT void fence() override;
    virtual SR_DLL_IMPORT void getBufferPixelFormat(PixelFormat& format) override;
    virtual SR_DLL_IMPORT void preBindTexture(unsigned long stage, Texture& texture) override;
    virtual SR_DLL_IMPORT void setPolygonMode(e_polygonMode mode) override;
    virtual SR_DLL_IMPORT void setCullMode(e_cullMode mode) override;
    virtual SR_DLL_IMPORT void setProjectionMatrix(const srMatrix4T<float>& matrix,
                                                   srMatrix4T<float>::e_type type) override;
    virtual SR_DLL_IMPORT void
    setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info) override;
    virtual SR_DLL_IMPORT void drawElements(srRendererDefs::e_primitive primitive,
                                            unsigned long count, srRendererDefs::e_indexType type,
                                            const void* indices) override;
    virtual SR_DLL_IMPORT void drawArrays(srRendererDefs::e_primitive primitive, long first,
                                          unsigned long count) override;
    virtual SR_DLL_IMPORT void setPolygonOffset(long offset) override;

    unsigned long getFunctionCallCount(e_command command) const;
    double getFunctionCallTime(e_command command) const;
    SR_DLL_IMPORT const char* getFunctionName(e_command command) const;
    void increaseCallCount(e_command command);
    SR_DLL_IMPORT void increaseCallTime(e_command command, double time);

protected:
    void resetInternalStatistics();

    srDD* device_04;
    unsigned long unknown_08;
    unsigned char unknown_0c_[4];
    double time_scale_10;
    double call_times_18[0x2b];
    unsigned long call_counts_170[0x2b];
};
