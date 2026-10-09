#include "surrender/srDebugDD.h"

#include "surrender/srCore.h"

// GLOBAL: SURRENDER 0x10099028
const char* srDebugDD::funcName[0x2c] = {
    "dummy command",
    "getInfo()",
    "getWindowList()",
    "getTextureFormat()",
    "getStatistics()",
    "resetStatistics()",
    "closeWindow()",
    "beginFrame()",
    "endFrame()",
    "flushFrame()",
    "flipFrame()",
    "clearBuffers()",
    "update()",
    "setScissor()",
    "setViewPort()",
    "setClearValues()",
    "setFogColor()",
    "setShader()",
    "deleteTexture()",
    "deletePalette()",
    "deleteContext()",
    "getDriverInfo()",
    "openWindow()",
    "isbusy()",
    "bufferOp()",
    "createContext()",
    "extCommand()",
    "bindTexture()",
    "setTextureParameters()",
    "texImage()",
    "texSubImage()",
    "setGlobalPalette()",
    "bindPalette()",
    "fence()",
    "getBufferPixelFormat()",
    "preBindTexture()",
    "setPolygonMode()",
    "setCullMode()",
    "setProjectionMatrix()",
    "setVertexArrayInfo()",
    "drawElements()",
    "drawArrays()",
    "setPolygonOffset()",
    "CMDMAX",
};

// FUNCTION: SURRENDER 0x10016CC0
void srDebugDD::resetInternalStatistics()
{
    int command;

    for (command = 0; command < 0x2b; ++command) {
        call_times[command] = 0.0;
        call_counts[command] = 0;
    }
}

// FUNCTION: SURRENDER 0x10016CF0
srDebugDD::srDebugDD(srDD* device)
{
    int iteration;

    this->device = device;
    unknown_08 = 1;
    call_times[0] = 0.0;
    for (iteration = 0; iteration < 0x2710; ++iteration) {
        ScopeTimer timer(this, COMMAND_DUMMY);
    }
    time_scale = call_times[0] * 0.0001;
    resetInternalStatistics();
}

// FUNCTION: SURRENDER 0x10016D70
srDebugDD::ScopeTimer::ScopeTimer(srDebugDD* owner, e_command command)
{
    this->owner = owner;
    this->command = command;
    start_time = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT);
}

// FUNCTION: SURRENDER 0x10016DA0
srDebugDD::ScopeTimer::~ScopeTimer()
{
    double elapsed = srCore.getTimer()->getTime(srTimer::TIMER_READ_DEFAULT) - start_time;
    owner->call_times[command] += elapsed;
    ++owner->call_counts[command];
}

// FUNCTION: SURRENDER 0x10016DD0
void srDebugDD::getInfo(Info& info)
{
    ScopeTimer timer(this, COMMAND_GET_INFO);
    device->getInfo(info);
}

// FUNCTION: SURRENDER 0x10016E00
void srDebugDD::getWindowList(WindowInfoList& list)
{
    ScopeTimer timer(this, COMMAND_GET_WINDOW_LIST);
    device->getWindowList(list);
}

// FUNCTION: SURRENDER 0x10016E30
void srDebugDD::getTextureFormats(PixelFormatList& list)
{
    ScopeTimer timer(this, COMMAND_GET_TEXTURE_FORMAT);
    device->getTextureFormats(list);
}

// FUNCTION: SURRENDER 0x10016E60
void srDebugDD::getStatistics(Statistics& stats)
{
    ScopeTimer timer(this, COMMAND_GET_STATISTICS);
    device->getStatistics(stats);
}

// FUNCTION: SURRENDER 0x10016E90
void srDebugDD::resetStatistics()
{
    ScopeTimer timer(this, COMMAND_RESET_STATISTICS);
    device->resetStatistics();
    resetInternalStatistics();
}

// FUNCTION: SURRENDER 0x10016EC0
void srDebugDD::closeWindow()
{
    ScopeTimer timer(this, COMMAND_CLOSE_WINDOW);
    device->closeWindow();
}

// FUNCTION: SURRENDER 0x10016EF0
void srDebugDD::beginFrame()
{
    ScopeTimer timer(this, COMMAND_BEGIN_FRAME);
    device->beginFrame();
}

// FUNCTION: SURRENDER 0x10016F20
void srDebugDD::endFrame()
{
    ScopeTimer timer(this, COMMAND_END_FRAME);
    device->endFrame();
}

// FUNCTION: SURRENDER 0x10016F50
void srDebugDD::flushFrame()
{
    ScopeTimer timer(this, COMMAND_FLUSH_FRAME);
    device->flushFrame();
}

// FUNCTION: SURRENDER 0x10016F80
void srDebugDD::flipFrame(const Scissor* first, const Scissor* second, w8_ulong value)
{
    ScopeTimer timer(this, COMMAND_FLIP_FRAME);
    device->flipFrame(first, second, value);
}

// FUNCTION: SURRENDER 0x10016FC0
void srDebugDD::clearBuffers(const srFlags<e_buffer>& buffers)
{
    ScopeTimer timer(this, COMMAND_CLEAR_BUFFERS);
    device->clearBuffers(buffers);
}

// FUNCTION: SURRENDER 0x10016FF0
void srDebugDD::update(const Update& values)
{
    ScopeTimer timer(this, COMMAND_UPDATE);
    device->update(values);
}

// FUNCTION: SURRENDER 0x10017020
void srDebugDD::setScissor(const Scissor& scissor)
{
    ScopeTimer timer(this, COMMAND_SET_SCISSOR);
    device->setScissor(scissor);
}

// FUNCTION: SURRENDER 0x10017050
void srDebugDD::setViewPort(const ViewPort& viewport)
{
    ScopeTimer timer(this, COMMAND_SET_VIEW_PORT);
    device->setViewPort(viewport);
}

// FUNCTION: SURRENDER 0x10017080
void srDebugDD::setClearValues(const ClearValues& values)
{
    ScopeTimer timer(this, COMMAND_SET_CLEAR_VALUES);
    device->setClearValues(values);
}

// FUNCTION: SURRENDER 0x100170B0
void srDebugDD::setFogColor(const srVector4T<float>& color)
{
    ScopeTimer timer(this, COMMAND_SET_FOG_COLOR);
    device->setFogColor(color);
}

// FUNCTION: SURRENDER 0x100170E0
void srDebugDD::setShader(const srShader& shader)
{
    ScopeTimer timer(this, COMMAND_SET_SHADER);
    device->setShader(shader);
}

// FUNCTION: SURRENDER 0x10017110
void srDebugDD::deleteTexture(Texture& texture)
{
    ScopeTimer timer(this, COMMAND_DELETE_TEXTURE);
    device->deleteTexture(texture);
}

// FUNCTION: SURRENDER 0x10017140
void srDebugDD::deletePalette(Palette& palette)
{
    ScopeTimer timer(this, COMMAND_DELETE_PALETTE);
    device->deletePalette(palette);
}

// FUNCTION: SURRENDER 0x10017170
void srDebugDD::deleteContext()
{
    ScopeTimer timer(this, COMMAND_DELETE_CONTEXT);
    device->deleteContext();
}

// FUNCTION: SURRENDER 0x100171A0
void srDebugDD::getDriverInfo(DriverInfo& info)
{
    ScopeTimer timer(this, COMMAND_GET_DRIVER_INFO);
    device->getDriverInfo(info);
}

// FUNCTION: SURRENDER 0x100171E0
srDD::e_error srDebugDD::openWindow(const OpenInfo& info, OpenResult& result)
{
    ScopeTimer timer(this, COMMAND_OPEN_WINDOW);
    return device->openWindow(info, result);
}

// FUNCTION: SURRENDER 0x10017220
int srDebugDD::isBusy()
{
    ScopeTimer timer(this, COMMAND_IS_BUSY);
    return device->isBusy();
}

// FUNCTION: SURRENDER 0x10017250
srDD::e_error srDebugDD::bufferOp(const BufferCommand& command)
{
    ScopeTimer timer(this, COMMAND_BUFFER_OP);
    return device->bufferOp(command);
}

// FUNCTION: SURRENDER 0x10017290
srDD::e_error srDebugDD::createContext(w8_ulong_ptr window)
{
    ScopeTimer timer(this, COMMAND_CREATE_CONTEXT);
    return device->createContext(window);
}

// FUNCTION: SURRENDER 0x100172D0
void srDebugDD::extCommand(w8_ulong command, void* data, w8_ulong size)
{
    ScopeTimer timer(this, COMMAND_EXT_COMMAND);
    device->extCommand(command, data, size);
}

// FUNCTION: SURRENDER 0x10017310
void srDebugDD::bindTexture(w8_ulong stage, Texture& texture)
{
    ScopeTimer timer(this, COMMAND_BIND_TEXTURE);
    device->bindTexture(stage, texture);
}

// FUNCTION: SURRENDER 0x10017350
void srDebugDD::setTextureParameters(w8_ulong stage, const TexParms& parms)
{
    ScopeTimer timer(this, COMMAND_SET_TEXTURE_PARAMETERS);
    device->setTextureParameters(stage, parms);
}

// FUNCTION: SURRENDER 0x10017390
void srDebugDD::texImage(Texture& texture, w8_ulong level)
{
    ScopeTimer timer(this, COMMAND_TEX_IMAGE);
    device->texImage(texture, level);
}

// FUNCTION: SURRENDER 0x100173D0
void srDebugDD::texSubImage(Texture& texture, w8_ulong a, w8_ulong b, w8_ulong c, w8_ulong d,
                            w8_ulong e)
{
    ScopeTimer timer(this, COMMAND_TEX_SUB_IMAGE);
    device->texSubImage(texture, a, b, c, d, e);
}

// FUNCTION: SURRENDER 0x10017420
void srDebugDD::setGlobalPalette(w8_ulong* palette, w8_ulong count)
{
    ScopeTimer timer(this, COMMAND_SET_GLOBAL_PALETTE);
    device->setGlobalPalette(palette, count);
}

// FUNCTION: SURRENDER 0x10017460
void srDebugDD::bindPalette(Palette& palette)
{
    ScopeTimer timer(this, COMMAND_BIND_PALETTE);
    device->bindPalette(palette);
}

// FUNCTION: SURRENDER 0x10017490
void srDebugDD::fence()
{
    ScopeTimer timer(this, COMMAND_FENCE);
    device->fence();
}

// FUNCTION: SURRENDER 0x100174C0
void srDebugDD::getBufferPixelFormat(PixelFormat& format)
{
    ScopeTimer timer(this, COMMAND_GET_BUFFER_PIXEL_FORMAT);
    device->getBufferPixelFormat(format);
}

// FUNCTION: SURRENDER 0x10017500
void srDebugDD::preBindTexture(w8_ulong stage, Texture& texture)
{
    ScopeTimer timer(this, COMMAND_PRE_BIND_TEXTURE);
    device->preBindTexture(stage, texture);
}

// FUNCTION: SURRENDER 0x10017540
void srDebugDD::setPolygonMode(e_polygonMode mode)
{
    ScopeTimer timer(this, COMMAND_SET_POLYGON_MODE);
    device->setPolygonMode(mode);
}

// FUNCTION: SURRENDER 0x10017580
void srDebugDD::setCullMode(e_cullMode mode)
{
    ScopeTimer timer(this, COMMAND_SET_CULL_MODE);
    device->setCullMode(mode);
}

// FUNCTION: SURRENDER 0x100175C0
void srDebugDD::setProjectionMatrix(const srMatrix4T<float>& matrix, srMatrix4T<float>::e_type type)
{
    ScopeTimer timer(this, COMMAND_SET_PROJECTION_MATRIX);
    device->setProjectionMatrix(matrix, type);
}

// FUNCTION: SURRENDER 0x10017600
void srDebugDD::setVertexArrayInfo(const srRendererDefs::VertexArrayInfo* info)
{
    ScopeTimer timer(this, COMMAND_SET_VERTEX_ARRAY_INFO);
    device->setVertexArrayInfo(info);
}

// FUNCTION: SURRENDER 0x10017640
void srDebugDD::drawElements(srRendererDefs::e_primitive primitive, w8_ulong count,
                             srRendererDefs::e_indexType type, const void* indices)
{
    ScopeTimer timer(this, COMMAND_DRAW_ELEMENTS);
    device->drawElements(primitive, count, type, indices);
}

// FUNCTION: SURRENDER 0x10017690
void srDebugDD::drawArrays(srRendererDefs::e_primitive primitive, w8_long first, w8_ulong count)
{
    ScopeTimer timer(this, COMMAND_DRAW_ARRAYS);
    device->drawArrays(primitive, first, count);
}

// FUNCTION: SURRENDER 0x10017700
void srDebugDD::setPolygonOffset(w8_long offset)
{
    ScopeTimer timer(this, COMMAND_SET_POLYGON_OFFSET);
    device->setPolygonOffset(offset);
}

// FUNCTION: SURRENDER 0x10017740
w8_ulong srDebugDD::getFunctionCallCount(e_command command) const
{
    return call_counts[command];
}

// FUNCTION: SURRENDER 0x10017750
const char* srDebugDD::getFunctionName(e_command command) const
{
    return funcName[command];
}

// FUNCTION: SURRENDER 0x10017760
double srDebugDD::getFunctionCallTime(e_command command) const
{
    double time = call_times[command] - call_counts[command] * time_scale;
    if (time <= 0.0) {
        time = 0.0;
    }
    return time;
}

// FUNCTION: SURRENDER 0x100177A0
void srDebugDD::increaseCallCount(e_command command)
{
    ++call_counts[command];
}

// FUNCTION: SURRENDER 0x100177B0
void srDebugDD::increaseCallTime(e_command command, double time)
{
    call_times[command] += time;
}
