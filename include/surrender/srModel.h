#pragma once

#include "srMath.h"
#include "srPtr.h"
#include "srTypeRegistry.h"

class srGERD;

// VTABLE: SURRENDER 0x10076CE8
// class srClassSupport<srModel, srClass, 1, 8192>

// VTABLE: SURRENDER 0x10076D08 srModel
// class srModel
class SR_DLL_IMPORT SR_DLL_EXPORT srModel : public srClassSupport<srModel, srClass, true, 0x2000> {
public:
    // VTABLE: SURRENDER 0x10076CD8 Client
    // class Client
    class SR_DLL_IMPORT SR_DLL_EXPORT Client {
    public:
        /* srMeshModel sends 0 after bounds invalidation/recalculation;
           srModelInstance forwards it as NOTIFY_BOUNDS_DIRTY. */
        enum e_update { UPDATE_BOUNDS = 0 };

        Client();
        /* Copy construction/assignment are plain memberwise srPtr/links copies;
           the custom destructor remains because it detaches from the model. */

        virtual ~Client();
        virtual void setModel(srModel* model);
        virtual void updateClient(e_update update);
        virtual srModel* getModel() const;
        Client* getNextClient() const;
        Client* getPrevClient() const;

    private:
        friend class srModel;
        srPtr<srModel> model;
        Client* previous;
        Client* next;
    };

    srModel();

    srModel& operator=(const srModel& other);
    friend class Client;

    // FUNCTION: SURRENDER 0x1003C6F0
    static const char* sGetClassName()
    {
        return "srModel";
    }

    virtual void dump(std::ostream& stream) override;

protected:
    virtual ~srModel() override;

public:
    virtual int getBoundingSphere(srVector3T<float>& center, float& radius) = 0;
    virtual int getBoundingBox(srVector3T<float>& minimum, srVector3T<float>& maximum) = 0;
    virtual void render(class srGERD& renderer) = 0;
    virtual void updateAllClients(Client::e_update update);

    Client* getFirstClient() const;

protected:
    Client* first_client;
};

W8_ABI_ASSERT((sizeof(srModel::Client) == 0x10), "srModelClient_must_be_0x10");
W8_ABI_ASSERT((sizeof(srModel) == 0x1c), "srModel_must_be_0x1c");
