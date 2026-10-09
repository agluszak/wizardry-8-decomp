#include "surrender/srModel.h"

#include "surrender/srCore.h"

#include <ostream>

// FUNCTION: SURRENDER 0x1003C2F0
srModel::Client::Client()
{
    model = 0;
    previous = 0;
    next = 0;
}

// FUNCTION: SURRENDER 0x1003C350
srModel::Client::~Client()
{
    setModel(0);
}

// FUNCTION: SURRENDER 0x1003C3B0
void srModel::Client::setModel(srModel* model)
{
    srModel* old = this->model;
    if (old != model) {
        if (old != 0) {
            if (previous != 0) {
                previous->next = next;
            }
            if (next != 0) {
                next->previous = previous;
            }
            if (this == old->first_client) {
                old->first_client = next;
            }
        }
        this->model = model;
        if (model != 0) {
            previous = 0;
            next = model->first_client;
            if (next != 0) {
                next->previous = this;
            }
            model->first_client = this;
        }
    }
}

// FUNCTION: SURRENDER 0x1003C430
void srModel::Client::updateClient(e_update update) {}

// FUNCTION: SURRENDER 0x1003C6C0
srModel* srModel::Client::getModel() const
{
    return model;
}

// FUNCTION: SURRENDER 0x1003C6D0
srModel::Client* srModel::Client::getNextClient() const
{
    return next;
}

// FUNCTION: SURRENDER 0x1003C6E0
srModel::Client* srModel::Client::getPrevClient() const
{
    return previous;
}

// FUNCTION: SURRENDER 0x1003C520
srModel::srModel()
{
    first_client = 0;
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
    w8_long flags = stream.flags();
    stream.flags((flags & 0xfffffe7fL) | 0x40);
    if (first_client != 0) {
        stream.width(0x20);
        stream << "  First client: " << first_client << '\n';
    }
    stream.flags(flags & 0x7fff);
}

// FUNCTION: SURRENDER 0x1003C440
void srModel::updateAllClients(Client::e_update update)
{
    for (Client* client = first_client; client != 0; client = client->next) {
        client->updateClient(update);
    }
}

// FUNCTION: SURRENDER 0x1003C700
srModel::Client* srModel::getFirstClient() const
{
    return first_client;
}
