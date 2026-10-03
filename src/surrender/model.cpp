#include "surrender/srModel.h"

#include "surrender/srCore.h"

#include <ostream>

// FUNCTION: SURRENDER 0x1003C2F0
srModel::Client::Client()
{
    model_04 = 0;
    previous_08 = 0;
    next_0c = 0;
}

// FUNCTION: SURRENDER 0x1003C350
srModel::Client::~Client()
{
    setModel(0);
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
    first_client_18 = 0;
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
srModel::~srModel() {}

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
