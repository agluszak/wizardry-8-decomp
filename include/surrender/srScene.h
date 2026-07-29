#pragma once

#include "srNode.h"

class srGERD;
class srModeler;

class SR_DLL_IMPORT srScene : public srNode {
public:
    srScene(srNode* parent);
    srScene& operator=(const srScene& other);

    virtual void dump(std::ostream& stream) override;
    virtual ~srScene() override;
    /* Emitted by Wiz8.exe at 0x00423EB0, freeing through the renderer's heap
       the way srNode's does. */
    srScene* scalar_deleting_destructor(unsigned char flags);
    virtual srClass* vInstance() override;
    virtual void traverse(TraverseInfo& info) override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void render(srGERD& renderer, class srCamera* camera);

protected:
    unsigned char unknown_138_[0x3c];
    unsigned long overlay_state_[6];       /* 0x174 */
    unsigned char unknown_18c_[4];
};

class SR_DLL_IMPORT srCamera : public srNode {
public:
    struct Rect {
        double left;
        double bottom;
        double right;
        double top;
    };

    srCamera(srNode* parent);
    srCamera& operator=(const srCamera& other);

    virtual void dump(std::ostream& stream) override;
    virtual ~srCamera() override;
    /* Emitted by Wiz8.exe at 0x00423E80. */
    srCamera* scalar_deleting_destructor(unsigned char flags);
    virtual srClass* vInstance() override;
    virtual void process(const ProcessInfo& info, e_processType type) override;

    void setClipRange(double near_plane, double far_plane);
    /* The reader for the pair setClipRange writes. Both planes come back
       through out-parameters, which is why the caller at 0x0046E440 keeps two
       doubles on its frame and returns only the far one. */
    void getClipRange(double* near_plane, double* far_plane);
    void setViewPlane(const Rect& rectangle, double distance);
    void setViewPlane(double width, double height);
    void getViewPlane(Rect& rectangle, double& distance) const;
    void setEnvironmentRange(float near_range, float far_range);

private:
    unsigned char unknown_138_[0x50];
};

typedef char srScene_must_be_0x190[(sizeof(srScene) == 0x190) ? 1 : -1];
typedef char srCamera_must_be_0x188[(sizeof(srCamera) == 0x188) ? 1 : -1];
