#include "surrender/srDebugDD.h"

#include "surrender/srCore.h"

// GLOBAL: SURRENDER 0x10099028
const char* srDebugDD::funcName[0x2c] = {
    "dummy command",          "getInfo()",
    "getWindowList()",        "getTextureFormat()",
    "getStatistics()",        "resetStatistics()",
    "closeWindow()",          "beginFrame()",
    "endFrame()",             "flushFrame()",
    "flipFrame()",            "clearBuffers()",
    "update()",               "setScissor()",
    "setViewPort()",          "setClearValues()",
    "setFogColor()",          "setShader()",
    "deleteTexture()",        "deletePalette()",
    "deleteContext()",        "getDriverInfo()",
    "openWindow()",           "isbusy()",
    "bufferOp()",             "createContext()",
    "extCommand()",           "bindTexture()",
    "setTextureParameters()", "texImage()",
    "texSubImage()",          "setGlobalPalette()",
    "bindPalette()",          "fence()",
    "getBufferPixelFormat()", "preBindTexture()",
    "setPolygonMode()",       "setCullMode()",
    "setProjectionMatrix()",  "setVertexArrayInfo()",
    "drawElements()",         "drawArrays()",
    "setPolygonOffset()",     "CMDMAX",
};

// FUNCTION: SURRENDER 0x10016CC0
void srDebugDD::resetInternalStatistics()
{
    int command;

    for (command = 0; command < 0x2b; ++command) {
        call_times_18[command] = 0.0;
        call_counts_170[command] = 0;
    }
}

// FUNCTION: SURRENDER 0x10016CF0
srDebugDD::srDebugDD(srDD* device)
{
    int iteration;

    device_04 = device;
    unknown_08 = 1;
    call_times_18[0] = 0.0;
    for (iteration = 0; iteration < 0x2710; ++iteration) {
        ScopeTimer timer(this, COMMAND_DUMMY);
    }
    time_scale_10 = call_times_18[0] * 0.0001;
    resetInternalStatistics();
}

// FUNCTION: SURRENDER 0x10016D60
srDebugDD::~srDebugDD() {}

// FUNCTION: SURRENDER 0x10016D70
srDebugDD::ScopeTimer::ScopeTimer(srDebugDD* owner, e_command command)
{
    owner_00 = owner;
    command_04 = command;
    start_08 = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x10016DA0
srDebugDD::ScopeTimer::~ScopeTimer()
{
    double elapsed =
        srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - start_08;
    owner_00->call_times_18[command_04] += elapsed;
    ++owner_00->call_counts_170[command_04];
}

// FUNCTION: SURRENDER 0x10016DD0
void srDebugDD::getInfo(Info& info)
{
    ScopeTimer timer(this, COMMAND_GET_INFO);
    device_04->getInfo(info);
}

// FUNCTION: SURRENDER 0x10016E00
void srDebugDD::getWindowList(WindowInfoList& list)
{
    ScopeTimer timer(this, COMMAND_GET_WINDOW_LIST);
    device_04->getWindowList(list);
}

// FUNCTION: SURRENDER 0x10016E30
void srDebugDD::getTextureFormats(PixelFormatList& list)
{
    ScopeTimer timer(this, COMMAND_GET_TEXTURE_FORMAT);
    device_04->getTextureFormats(list);
}

// FUNCTION: SURRENDER 0x10016E60
void srDebugDD::getStatistics(Statistics& stats)
{
    ScopeTimer timer(this, COMMAND_GET_STATISTICS);
    device_04->getStatistics(stats);
}

// FUNCTION: SURRENDER 0x10016E90
void srDebugDD::resetStatistics()
{
    ScopeTimer timer(this, COMMAND_RESET_STATISTICS);
    device_04->resetStatistics();
    resetInternalStatistics();
}

// FUNCTION: SURRENDER 0x10016EC0
void srDebugDD::closeWindow()
{
    ScopeTimer timer(this, COMMAND_CLOSE_WINDOW);
    device_04->closeWindow();
}

// FUNCTION: SURRENDER 0x10016EF0
void srDebugDD::beginFrame()
{
    ScopeTimer timer(this, COMMAND_BEGIN_FRAME);
    device_04->beginFrame();
}

// FUNCTION: SURRENDER 0x10016F20
void srDebugDD::endFrame()
{
    ScopeTimer timer(this, COMMAND_END_FRAME);
    device_04->endFrame();
}

// FUNCTION: SURRENDER 0x10016F50
void srDebugDD::flushFrame()
{
    ScopeTimer timer(this, COMMAND_FLUSH_FRAME);
    device_04->flushFrame();
}

// FUNCTION: SURRENDER 0x10016F80
void srDebugDD::flipFrame(const Scissor* first, const Scissor* second,
                          unsigned long value)
{
    ScopeTimer timer(this, COMMAND_FLIP_FRAME);
    device_04->flipFrame(first, second, value);
}

// FUNCTION: SURRENDER 0x10016FC0
void srDebugDD::clearBuffers(const srFlags<e_buffer>& buffers)
{
    ScopeTimer timer(this, COMMAND_CLEAR_BUFFERS);
    device_04->clearBuffers(buffers);
}

// FUNCTION: SURRENDER 0x10016FF0
void srDebugDD::update(const Update& values)
{
    ScopeTimer timer(this, COMMAND_UPDATE);
    device_04->update(values);
}

// FUNCTION: SURRENDER 0x10017020
void srDebugDD::setScissor(const Scissor& scissor)
{
    ScopeTimer timer(this, COMMAND_SET_SCISSOR);
    device_04->setScissor(scissor);
}

// FUNCTION: SURRENDER 0x10017050
void srDebugDD::setViewPort(const ViewPort& viewport)
{
    ScopeTimer timer(this, COMMAND_SET_VIEW_PORT);
    device_04->setViewPort(viewport);
}

// FUNCTION: SURRENDER 0x10017080
void srDebugDD::setClearValues(const ClearValues& values)
{
    ScopeTimer timer(this, COMMAND_SET_CLEAR_VALUES);
    device_04->setClearValues(values);
}

// FUNCTION: SURRENDER 0x100170B0
void srDebugDD::setFogColor(const srVector4T<float>& color)
{
    ScopeTimer timer(this, COMMAND_SET_FOG_COLOR);
    device_04->setFogColor(color);
}

// FUNCTION: SURRENDER 0x100170E0
void srDebugDD::setShader(const srShader& shader)
{
    ScopeTimer timer(this, COMMAND_SET_SHADER);
    device_04->setShader(shader);
}

// FUNCTION: SURRENDER 0x10017110
void srDebugDD::deleteTexture(Texture& texture)
{
    ScopeTimer timer(this, COMMAND_DELETE_TEXTURE);
    device_04->deleteTexture(texture);
}

// FUNCTION: SURRENDER 0x10017140
void srDebugDD::deletePalette(Palette& palette)
{
    ScopeTimer timer(this, COMMAND_DELETE_PALETTE);
    device_04->deletePalette(palette);
}

// FUNCTION: SURRENDER 0x10017170
void srDebugDD::deleteContext()
{
    ScopeTimer timer(this, COMMAND_DELETE_CONTEXT);
    device_04->deleteContext();
}

// FUNCTION: SURRENDER 0x100171A0
void srDebugDD::getDriverInfo(DriverInfo& info)
{
    ScopeTimer timer(this, COMMAND_GET_DRIVER_INFO);
    device_04->getDriverInfo(info);
}

// FUNCTION: SURRENDER 0x100171E0
srDD::e_error srDebugDD::openWindow(const OpenInfo& info, OpenResult& result)
{
    ScopeTimer timer(this, COMMAND_OPEN_WINDOW);
    return device_04->openWindow(info, result);
}

// FUNCTION: SURRENDER 0x10017220
int srDebugDD::isBusy()
{
    ScopeTimer timer(this, COMMAND_IS_BUSY);
    return device_04->isBusy();
}

// FUNCTION: SURRENDER 0x10017250
srDD::e_error srDebugDD::bufferOp(const BufferCommand& command)
{
    ScopeTimer timer(this, COMMAND_BUFFER_OP);
    return device_04->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10017290
srDD::e_error srDebugDD::createContext(unsigned long window)
{
    ScopeTimer timer(this, COMMAND_CREATE_CONTEXT);
    return device_04->createContext(window);
}

// FUNCTION: SURRENDER 0x100172D0
void srDebugDD::extCommand(unsigned long command, void* data, unsigned long size)
{
    ScopeTimer timer(this, COMMAND_EXT_COMMAND);
    device_04->extCommand(command, data, size);
}

// FUNCTION: SURRENDER 0x10017310
void srDebugDD::bindTexture(unsigned long stage, Texture& texture)
{
    ScopeTimer timer(this, COMMAND_BIND_TEXTURE);
    device_04->bindTexture(stage, texture);
}

// FUNCTION: SURRENDER 0x10017350
void srDebugDD::setTextureParameters(unsigned long stage, const TexParms& parms)
{
    ScopeTimer timer(this, COMMAND_SET_TEXTURE_PARAMETERS);
    device_04->setTextureParameters(stage, parms);
}

// FUNCTION: SURRENDER 0x10017390
void srDebugDD::texImage(Texture& texture, unsigned long level)
{
    ScopeTimer timer(this, COMMAND_TEX_IMAGE);
    device_04->texImage(texture, level);
}

// FUNCTION: SURRENDER 0x100173D0
void srDebugDD::texSubImage(Texture& texture, unsigned long a, unsigned long b,
                            unsigned long c, unsigned long d, unsigned long e)
{
    ScopeTimer timer(this, COMMAND_TEX_SUB_IMAGE);
    device_04->texSubImage(texture, a, b, c, d, e);
}

// FUNCTION: SURRENDER 0x10017420
void srDebugDD::setGlobalPalette(unsigned long* palette, unsigned long count)
{
    ScopeTimer timer(this, COMMAND_SET_GLOBAL_PALETTE);
    device_04->setGlobalPalette(palette, count);
}

// FUNCTION: SURRENDER 0x10017460
void srDebugDD::bindPalette(Palette& palette)
{
    ScopeTimer timer(this, COMMAND_BIND_PALETTE);
    device_04->bindPalette(palette);
}

// FUNCTION: SURRENDER 0x10017490
void srDebugDD::fence()
{
    ScopeTimer timer(this, COMMAND_FENCE);
    device_04->fence();
}

// FUNCTION: SURRENDER 0x100174C0
void srDebugDD::getBufferPixelFormat(PixelFormat& format)
{
    ScopeTimer timer(this, COMMAND_GET_BUFFER_PIXEL_FORMAT);
    device_04->getBufferPixelFormat(format);
}

// FUNCTION: SURRENDER 0x10017500
void srDebugDD::preBindTexture(unsigned long stage, Texture& texture)
{
    ScopeTimer timer(this, COMMAND_PRE_BIND_TEXTURE);
    device_04->preBindTexture(stage, texture);
}

// FUNCTION: SURRENDER 0x10017540
void srDebugDD::setPolygonMode(e_polygonMode mode)
{
    ScopeTimer timer(this, COMMAND_SET_POLYGON_MODE);
    device_04->setPolygonMode(mode);
}

// FUNCTION: SURRENDER 0x10017580
void srDebugDD::setCullMode(e_cullMode mode)
{
    ScopeTimer timer(this, COMMAND_SET_CULL_MODE);
    device_04->setCullMode(mode);
}

// FUNCTION: SURRENDER 0x100175C0
void srDebugDD::setProjectionMatrix(const srMatrix4T<float>& matrix,
                                    srMatrix4T<float>::e_type type)
{
    ScopeTimer timer(this, COMMAND_SET_PROJECTION_MATRIX);
    device_04->setProjectionMatrix(matrix, type);
}

// FUNCTION: SURRENDER 0x10017600
void srDebugDD::setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info)
{
    ScopeTimer timer(this, COMMAND_SET_VERTEX_ARRAY_INFO);
    device_04->setVertexArrayInfo(info);
}

// FUNCTION: SURRENDER 0x10017640
void srDebugDD::drawElements(srRendererDefs::e_primitive primitive,
                             unsigned long count,
                             srRendererDefs::e_indexType type,
                             const void* indices)
{
    ScopeTimer timer(this, COMMAND_DRAW_ELEMENTS);
    device_04->drawElements(primitive, count, type, indices);
}

/* Retail carries a C++ exception frame (FuncInfo 0x100786F8, dwTryCount=0) on
   this one forwarder while the other 41 have none: the unwindable object is
   the ScopeTimer itself, so the function's original TU context had unwind
   semantics enabled (/GX) while the rest of this file did not. */
// FUNCTION: SURRENDER 0x10017690
void srDebugDD::drawArrays(srRendererDefs::e_primitive primitive, long first,
                           unsigned long count)
{
    ScopeTimer timer(this, COMMAND_DRAW_ARRAYS);
    device_04->drawArrays(primitive, first, count);
}

// FUNCTION: SURRENDER 0x10017700
void srDebugDD::setPolygonOffset(long offset)
{
    ScopeTimer timer(this, COMMAND_SET_POLYGON_OFFSET);
    device_04->setPolygonOffset(offset);
}

// FUNCTION: SURRENDER 0x10017740
unsigned long srDebugDD::getFunctionCallCount(e_command command) const
{
    return call_counts_170[command];
}

// FUNCTION: SURRENDER 0x10017750
const char* srDebugDD::getFunctionName(e_command command) const
{
    return funcName[command];
}

// FUNCTION: SURRENDER 0x10017760
double srDebugDD::getFunctionCallTime(e_command command) const
{
    double time = call_times_18[command] - call_counts_170[command] * time_scale_10;
    if (time <= 0.0) {
        time = 0.0;
    }
    return time;
}

// FUNCTION: SURRENDER 0x100177A0
void srDebugDD::increaseCallCount(e_command command)
{
    ++call_counts_170[command];
}

// FUNCTION: SURRENDER 0x100177B0
void srDebugDD::increaseCallTime(e_command command, double time)
{
    call_times_18[command] += time;
}
