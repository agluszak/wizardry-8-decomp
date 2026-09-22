#include "surrender/srModel.h"

#include "surrender/srCore.h"

// FUNCTION: SURRENDER 0x1003C2F0
srModel::Client::Client()
{
    model_04 = 0;
    previous_08 = 0;
    next_0c = 0;
}

// FUNCTION: SURRENDER 0x1003C710
srModel::Client::Client(const Client& other) : model_04(other.model_04)
{
    previous_08 = other.previous_08;
    next_0c = other.next_0c;
}

// FUNCTION: SURRENDER 0x1003C350
srModel::Client::~Client()
{
    setModel(0);
}

// FUNCTION: SURRENDER 0x1003C750
srModel::Client& srModel::Client::operator=(const Client& other)
{
    model_04 = other.model_04;
    previous_08 = other.previous_08;
    next_0c = other.next_0c;
    return *this;
}

// FUNCTION: SURRENDER 0x1003C3B0
void srModel::Client::setModel(srModel* model)
{
    srModel* old = model_04;
    if (old != model) {
        if (old != 0) {
            if (previous_08 != 0) {
                previous_08->next_0c = next_0c;
            }
            if (next_0c != 0) {
                next_0c->previous_08 = previous_08;
            }
            if (this == old->first_client_18) {
                old->first_client_18 = next_0c;
            }
        }
        model_04 = model;
        if (model != 0) {
            previous_08 = 0;
            next_0c = model->first_client_18;
            if (next_0c != 0) {
                next_0c->previous_08 = this;
            }
            model->first_client_18 = this;
        }
    }
}

// FUNCTION: SURRENDER 0x1003C430
void srModel::Client::updateClient(e_update update) {}

// FUNCTION: SURRENDER 0x1003C6C0
srModel* srModel::Client::getModel() const
{
    return model_04;
}

// FUNCTION: SURRENDER 0x1003C6D0
srModel::Client* srModel::Client::getNextClient() const
{
    return next_0c;
}

// FUNCTION: SURRENDER 0x1003C6E0
srModel::Client* srModel::Client::getPrevClient() const
{
    return previous_08;
}

// FUNCTION: SURRENDER 0x1003C520
srModel::srModel()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2000);
    if (node == 0) {
        node = registry->registerClass(sGetClassName(), srClass::sGetClassNode(), 0x2000, 1);
    }
    registry->registerInstance(node, this);
    first_client_18 = 0;
}

// FUNCTION: SURRENDER 0x1003C7B0
srModel::srModel(const srModel& other)
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2000);
    if (node == 0) {
        node = registry->registerClass(sGetClassName(), srClass::sGetClassNode(), 0x2000, 1);
    }
    registry->registerInstance(node, this);
    *this = other;
    first_client_18 = other.first_client_18;
}

// FUNCTION: SURRENDER 0x1003C500
srModel& srModel::operator=(const srModel& other)
{
    if (this != &other) {
        srClass::operator=(other);
    }
    return *this;
}

// FUNCTION: SURRENDER 0x1003C470
srModel::~srModel()
{
    srRegistry* registry = srCore.getRegistry();
    srRegistry::ClassNode* node = registry->getClassNode(0x2000);
    if (node == 0) {
        node = registry->registerClass(sGetClassName(), srClass::sGetClassNode(), 0x2000, 1);
    }
    registry->unregisterInstance(node, this);
}

// FUNCTION: SURRENDER 0x1003C5B0
void srModel::dump(std::ostream& stream)
{
    srClass::dump(stream);
    long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    if (first_client_18 != 0) {
        stream.width(0x20);
        stream << "  First client: " << first_client_18 << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1003C440
void srModel::updateAllClients(Client::e_update update)
{
    for (Client* client = first_client_18; client != 0; client = client->next_0c) {
        client->updateClient(update);
    }
}

// FUNCTION: SURRENDER 0x1003C700
srModel::Client* srModel::getFirstClient() const
{
    return first_client_18;
}

#if defined(SURRENDER_BUILD)
// FUNCTION: SURRENDER 0x1003C6F0
const char* srModel::sGetClassName()
{
    return "srModel";
}
#endif
