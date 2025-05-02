#pragma once
#include "SinglePortModule.h"

/**
 * A simple example module that just replies with "Message received" to any message it receives.
 */
class AutoResponseModule : public SinglePortModule, public Observable<const meshtastic_MeshPacket *>
{
  public:
    /** Constructor
     * name is for debugging output
     */
     AutoResponseModule() : SinglePortModule("XXXXMod", meshtastic_PortNum_TEXT_MESSAGE_APP) {}

  //virtual ~AutoResponseModule() {}

  protected:
    /** For reply module we do all of our processing in the (normally optional)
     * want_replies handling
     */

    virtual bool wantPacket(const meshtastic_MeshPacket *p) override;
    virtual ProcessMessage handleReceived(const meshtastic_MeshPacket &mp) override;


};

extern AutoResponseModule *autoResponseModule;
