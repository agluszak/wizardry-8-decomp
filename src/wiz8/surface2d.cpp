#include "wiz8/surface2d.h"

#include "surrender/srCore.h"
#include "surrender/srGERD.h"

stTexture2D::stTexture2D()
    : srClassSupport<stTexture2D, srTexture, false, 0x1000f>(),
      left(0), top(0), right(128), bottom(128),
      frame_handle(getNewFrameHandle()), surface(0)
{
    setMipmap((e_mipmap)0);
    enableHint((e_hint)3);
    enableHint((e_hint)6);
    enableHint((e_hint)1);
    texture_dimensions_.width = 128;
    texture_dimensions_.height = 128;
}

// TEMPLATE: WIZ8 0x0047E7D0
// srClassSupport<stTexture2D,srTexture,0,65551>::getClassID

// TEMPLATE: WIZ8 0x0047E7E0
// srClassSupport<stTexture2D,srTexture,0,65551>::getClassName

// TEMPLATE: WIZ8 0x0047EBE0
// srClassSupport<stTexture2D,srTexture,0,65551>::getClassNode

// FUNCTION: WIZ8 0x0047E600
stTexture2D::~stTexture2D()
{
    invalidateFrameHandle(frame_handle);
}

// FUNCTION: WIZ8 0x0047DE60
srClass* stTexture2D::vInstance() { return new stTexture2D; }

// TEMPLATE: WIZ8 0x0047E7F0
// srClassSupport<stTexture2D,srTexture,0,65551>::clone

// TEMPLATE: WIZ8 0x0047E830
// srClassSupport<stTexture2D,srTexture,0,65551>::~srClassSupport<stTexture2D,srTexture,0,65551>

// FUNCTION: WIZ8 0x0047DE50
unsigned long stTexture2D::getTextureFrameHandle() { return frame_handle; }

// FUNCTION: WIZ8 0x0047E710
void stTexture2D::getMipmapData(MultiRequest& request)
{
    request.destinations[request.mipmap_level]->blit(
        0, 0, *surface, left, top, right, bottom);
}

// FUNCTION: WIZ8 0x0047E740
void stTexture2D::getMipmapLevelPartial(PartialRequest& request)
{
    request.destination->blit(
        request.destination_x, request.destination_y, *surface,
        left + request.destination_x, top + request.destination_y,
        left + request.source_right, top + request.source_bottom);
}

// FUNCTION: WIZ8 0x0047DE40
void stTexture2D::invalidate() { invalidateFrameHandle(frame_handle); }

// FUNCTION: WIZ8 0x0047E790
void stTexture2D::setupDefaultValues()
{
    if (surface) {
        surface->getPixelFormat(surface_format_);
        if (texture_filter_) {
            texture_filter_->release();
            texture_filter_ = 0;
        }
    }
    texture_flags_ &= ~2UL;
}

// FUNCTION: WIZ8 0x0047DAE0
stSurface2D::stSurface2D(srColorSurfaceIFace* source, int source_width,
                         int source_height, srNode* parent, int tile_extent)
    : srClassSupport<stSurface2D, srNode, false, 0x1000e>(
          static_cast<srNode*>(0)),
      source_surface(source), state(0x10), flags(0x100a017),
      tile_size(tile_extent),
      columns((source_width + tile_extent - 1) / tile_extent),
      rows((source_height + tile_extent - 1) / tile_extent),
      tile_count(columns * rows), width(source_width), height(source_height),
      tiles(new stTexture2D*[tile_count]),
      tile_u((float)tile_extent / (float)source_width),
      tile_v((float)tile_extent / (float)source_height), field_168(0),
      field_16c(1.0f), scale(0.0f), field_194(0)
{
    int row;
    int column;
    int index = 0;

    setParent(parent, 1);
    for (row = 0; row != rows; ++row) {
        for (column = 0; column != columns; ++column) {
            stTexture2D* texture = new stTexture2D;
            texture->surface = source;
            texture->texture_dimensions_.width = tile_extent;
            texture->texture_dimensions_.height = tile_extent;
            texture->left = column * tile_extent;
            texture->top = row * tile_extent;
            texture->right = texture->left + tile_extent;
            texture->bottom = texture->top + tile_extent;
            texture->setWrapS((srTextureIFace::e_wrap)1);
            texture->setWrapT((srTextureIFace::e_wrap)1);
            tiles[index++] = texture;
        }
    }
    coordinates[0] = 0.0f;
    coordinates[1] = 0.0f;
    coordinates[2] = 1.0f;
    coordinates[3] = 0.0f;
    coordinates[4] = 0.0f;
    coordinates[5] = 1.0f;
    coordinates[6] = 1.0f;
    coordinates[7] = 1.0f;
}

// TEMPLATE: WIZ8 0x0047E930
// srClassSupport<stSurface2D,srNode,0,65550>::getClassID

// TEMPLATE: WIZ8 0x0047E940
// srClassSupport<stSurface2D,srNode,0,65550>::getClassName

// TEMPLATE: WIZ8 0x0047E950
// srClassSupport<stSurface2D,srNode,0,65550>::getClassNode

// FUNCTION: WIZ8 0x0047DFF0
stSurface2D::~stSurface2D()
{
    int index;
    for (index = 0; index != tile_count; ++index) tiles[index]->release();
    delete[] tiles;
}

// TEMPLATE: WIZ8 0x0047E9C0
// srClassSupport<stSurface2D,srNode,0,65550>::clone

// TEMPLATE: WIZ8 0x0047EAC0
// srClassSupport<stSurface2D,srNode,0,65550>::~srClassSupport<stSurface2D,srNode,0,65550>

// FUNCTION: WIZ8 0x004D6540
void stSurface2D::traverse(TraverseInfo& info)
{
    if (nextSibling() != 0) {
        nextSibling()->traverse(info);
    }

    if (!testFlag(FLAG_POSITIONAL_0)) {
        TraverseInfo::Entry& entry = info.entries[info.entry_count];
        entry.node = this;
        entry.value = 0;
        ++info.entry_count;
    }

    if (!testFlag(FLAG_POSITIONAL_1) && firstChild() != 0) {
        firstChild()->traverse(info);
    }
}

// FUNCTION: WIZ8 0x0047E0F0
void stSurface2D::process(const ProcessInfo& info, e_processType)
{
    srGERD* renderer = info.renderer;
    int row;
    int column;
    int index = 0;

    renderer->matrixMode((srGERD::e_matrixMode)1);
    renderer->pushMatrix();
    renderer->loadIdentity();
    renderer->ortho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
    renderer->setVertexArrayMask(
        srFlags<srRendererDefs::e_vertexArray>(state));
    renderer->setCullMode(srGERD::CULL_MODE_POSITIONAL_2);
    srShader shader;
    shader.value = flags;
    renderer->setShader(shader);
    renderer->setTexCoordPointer(
        2, srRendererDefs::TYPE_POSITIONAL_1, 8, coordinates, 0);
    renderer->setClipState(srFlags<srRendererDefs::e_clip>(0x3f));
    renderer->setAntiAlias((srGERD::e_antiAlias)0);

    for (row = 0; row != rows; ++row) {
        for (column = 0; column != columns; ++column) {
            float vertices[12];
            float left = (float)column * tile_u;
            float top = (float)row * tile_v;
            float right = (float)(column + 1) * tile_u;
            float bottom = (float)(row + 1) * tile_v;

            vertices[0] = left;
            vertices[1] = top;
            vertices[2] = -0.1f;
            vertices[3] = right;
            vertices[4] = top;
            vertices[5] = -0.1f;
            vertices[6] = left;
            vertices[7] = bottom;
            vertices[8] = -0.1f;
            vertices[9] = right;
            vertices[10] = bottom;
            vertices[11] = -0.1f;

            renderer->setTexture(tiles[index++], 0);
            renderer->setVertexPointer(
                3, srRendererDefs::TYPE_POSITIONAL_1, 0xc, vertices, 4);
            renderer->drawArrays((srRendererDefs::e_primitive)3, 0, 4);
        }
    }
    renderer->setTexture(0, 0);
    renderer->matrixMode((srGERD::e_matrixMode)1);
    renderer->popMatrix();
}

void stSurface2D::invalidateTiles()
{
    int index;
    for (index = 0; index != tile_count; ++index) {
        tiles[index]->invalidate();
    }
}

/* Retail 0x0047E450. The locked surface and pitch are deliberately retained in
   the ABI even though SurRender obtains the pixels through each stTexture2D's
   source surface. Keeping the source locked brackets the immediate partial
   texture uploads exactly as the caller does. */
// FUNCTION: WIZ8 0x0047E450
void stSurface2D::updateRectangle(srGERD* renderer, void*, long,
                                  int left, int top, int right, int bottom)
{
    int x = left;
    int y = top;

    while (y < bottom) {
        stTexture2D* texture = 0;
        for (int index = 0; index != tile_count; ++index) {
            stTexture2D* candidate = tiles[index];
            if (candidate->left <= x && x < candidate->right
                && candidate->top <= y && y < candidate->bottom) {
                texture = candidate;
                break;
            }
        }
        if (!texture) {
            return;
        }

        int destination_x = x - texture->left;
        int destination_y = y - texture->top;
        int update_width = right - x;
        int update_height = bottom - y;
        if (texture->right - x < update_width) {
            update_width = texture->right - x;
        }
        if (texture->bottom - y < update_height) {
            update_height = texture->bottom - y;
        }
        if (field_194 & 1) {
            destination_x = 0;
            destination_y = 0;
            update_width = tile_size;
            update_height = tile_size;
        }
        renderer->setTextureSubImage(
            texture, 0, destination_x, destination_y,
            update_width, update_height);

        x += tile_size - x % tile_size;
        if (right <= x) {
            y += tile_size - y % tile_size;
            x = left;
        }
    }
}

// FUNCTION: WIZ8 0x0047e5b0
void stSurface2D::enableRendererFlag(unsigned int flag)
{
    field_194 |= flag;
}

// SYNTHETIC: WIZ8 0x0047DFC0
// stSurface2D::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0047E5D0
// stTexture2D::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0047EB80
// srClassSupport<stTexture2D,srTexture,0,65551>::`scalar deleting destructor'

// SYNTHETIC: WIZ8 0x0047EBB0
// srClassSupport<stSurface2D,srNode,0,65550>::`scalar deleting destructor'
