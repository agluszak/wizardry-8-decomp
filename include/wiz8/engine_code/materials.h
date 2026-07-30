#pragma once

class srMeshModel;
class srModelInstance;

unsigned char MeshHasAnimatedTexture004B9AA0(srMeshModel* model);
void SetModelAnimatedTextureFrame004B9B00(
    srModelInstance* instance, int frame);
