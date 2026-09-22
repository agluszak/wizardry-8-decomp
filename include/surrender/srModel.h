#pragma once

#include "srMath.h"
#include "srPtr.h"
#include "srTypeRegistry.h"

class srGERD;

class SR_DLL_IMPORT srModel : public srClassSupport<srModel, srClass, true, 0x2000> {
public:
    class SR_DLL_IMPORT Client {
    public:
        enum e_update {};

        Client();
        Client(const Client& other);
        virtual ~Client();
        Client& operator=(const Client& other);
        virtual void setModel(srModel* model);
        virtual void updateClient(e_update update);
        virtual srModel* getModel() const;
        Client* getNextClient() const;
        Client* getPrevClient() const;

    private:
        friend class srModel;
        srPtr<srModel> model_04;
        Client* previous_08;
        Client* next_0c;
    };

    srModel();
    srModel(const srModel& other);
    srModel& operator=(const srModel& other);
    friend class Client;

#if defined(SURRENDER_BUILD)
    static const char* sGetClassName();
#else
    static const char* sGetClassName()
    {
        return "srModel";
    }
#endif

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
    Client* first_client_18;
};

static_assert((sizeof(srModel::Client) == 0x10), "srModelClient_must_be_0x10");
static_assert((sizeof(srModel) == 0x1c), "srModel_must_be_0x1c");
