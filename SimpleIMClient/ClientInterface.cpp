#include "ClientInterface.h"

ClientInterface::ClientInterface()
{}

ClientInterface::~ClientInterface()
{}

bool ClientInterface::run()
{

    if(!m_clientInterfaceThread) {
        m_clientInterfaceThread.reset(new std::thread([this] ()->void {

            while(!m_terminate) {

            }
        }));
        return true;
    }
    return false;
}

void ClientInterface::stop()
{
}
