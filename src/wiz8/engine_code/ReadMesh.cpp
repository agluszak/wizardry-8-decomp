#include "wiz8/engine_code/ReadLevel.h"
#include "wiz8/virtual_file.h"

#include "FileMan.h"

#include <stdlib.h>

/* ReadMesh.cpp owns four parallel scratch allocations. Their element types are
   not established yet; this cleanup proves only the shared lifetime and the
   trailing count. */
static void *g_read_mesh_scratch_65b9e8;
static void *g_read_mesh_scratch_65b9ec;
static void *g_read_mesh_scratch_65b9f0;
static void *g_read_mesh_scratch_65b9f4;
static int g_read_mesh_scratch_count_65b9f8;

// FUNCTION: WIZ8 0x004881d0
void ReleaseReadMeshScratch004881D0() {
  if (g_read_mesh_scratch_65b9e8 != 0) {
    free(g_read_mesh_scratch_65b9e8);
    g_read_mesh_scratch_65b9e8 = 0;
  }
  if (g_read_mesh_scratch_65b9ec != 0) {
    free(g_read_mesh_scratch_65b9ec);
    g_read_mesh_scratch_65b9ec = 0;
  }
  if (g_read_mesh_scratch_65b9f0 != 0) {
    free(g_read_mesh_scratch_65b9f0);
    g_read_mesh_scratch_65b9f0 = 0;
  }
  if (g_read_mesh_scratch_65b9f4 != 0) {
    free(g_read_mesh_scratch_65b9f4);
    g_read_mesh_scratch_65b9f4 = 0;
  }
  g_read_mesh_scratch_count_65b9f8 = 0;
}

// FUNCTION: WIZ8 0x00487bd0
unsigned char SkipSingleLevelMesh00487BD0(W8ReadLevelInfo *info) {
  int version;
  int vertex_count;
  int face_count;
  unsigned char flags = 0;
  unsigned char count;
  short item_count;
  short index;
  unsigned char success = 1;

  if (info == 0 || info->world == 0 || info->hFile == 0) {
    return 0;
  }
  if (!ReadVirtualFile(info->hFile, &version, 4, 0) ||
      !ReadVirtualFile(info->hFile, &vertex_count, 4, 0) ||
      !ReadVirtualFile(info->hFile, &face_count, 4, 0)) {
    return 0;
  }
  if (vertex_count < 1 || face_count < 1) {
    return 0;
  }
  if (version > 2) {
    success = ReadVirtualFile(info->hFile, &flags, 1, 0);
  }
  if (version > 1) {
    FileSeek(info->hFile, 0x28, FILE_SEEK_FROM_CURRENT);
  }
  if (version > 3) {
    if (success == 0 || !ReadVirtualFile(info->hFile, &count, 1, 0)) {
      success = 0;
    }
    if (count != 0) {
      FileSeek(info->hFile,
               static_cast<int>(static_cast<signed char>(count)) * 4,
               FILE_SEEK_FROM_CURRENT);
    }
  }
  if ((flags & 1) == 0) {
    vertex_count *= 0xc;
  } else {
    unsigned char ignored;
    short group_count;

    ReadVirtualFile(info->hFile, &ignored, 1, 0);
    ReadVirtualFile(info->hFile, &group_count, 2, 0);
    if ((flags & 2) == 0) {
      vertex_count = group_count * vertex_count * 0xc;
    } else {
      vertex_count = group_count * vertex_count * 6;
    }
  }
  FileSeek(info->hFile, vertex_count, FILE_SEEK_FROM_CURRENT);
  if ((flags & 4) == 0) {
    face_count *= 0x29;
  } else {
    face_count *= 0x21;
  }
  FileSeek(info->hFile, face_count, FILE_SEEK_FROM_CURRENT);
  if (ReadVirtualFile(info->hFile, &item_count, 2, 0) && item_count > 0) {
    for (index = 0; index < item_count; ++index) {
      ReadVirtualFile(info->hFile, &count, 1, 0);
      FileSeek(info->hFile, 0x119, FILE_SEEK_FROM_CURRENT);
      if (count > 3) {
        FileSeek(info->hFile, 0x10, FILE_SEEK_FROM_CURRENT);
      }
    }
  }
  if ((flags & 1) != 0) {
    success = 2;
  }
  return success;
}
