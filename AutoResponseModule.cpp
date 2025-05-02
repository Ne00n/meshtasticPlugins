#include "AutoResponseModule.h"
#include "MeshService.h"
#include "configuration.h"
#include "main.h"
#include <unordered_map>
#include <chrono>
#include <string>

bool isAwayModeEnabled = false;
std::string awayMessage = "";
std::unordered_map<uint32_t, std::chrono::steady_clock::time_point> lastReplyTime;
const std::chrono::seconds replyCooldown(500);

ProcessMessage AutoResponseModule::handleReceived(const meshtastic_MeshPacket &currentRequest)
{
    auto &p = currentRequest.decoded;
    char messageRequest[250];
    for (size_t i = 0; i < p.payload.size; ++i)
    {
        messageRequest[i] = static_cast<char>(p.payload.bytes[i]);
    }
    messageRequest[p.payload.size] = '\0';
    
    if (currentRequest.from == nodeDB->getNodeNum()) {
        if (strncmp(messageRequest, "!away ", 6) == 0) {
            isAwayModeEnabled = true;
            awayMessage = messageRequest + 6;
            
            char confirmMessage[250];
            snprintf(confirmMessage, sizeof(confirmMessage), "Away mode enabled. Message: %s", awayMessage.c_str());
            LOG_INFO("AutoResponseModule: %s", confirmMessage);
            
            return ProcessMessage::STOP;
        } else if (strcmp(messageRequest, "!away") == 0) {
            isAwayModeEnabled = true;
            awayMessage = "I am currently away and will respond when I return.";
            
            LOG_INFO("AutoResponseModule: Away mode enabled with default message");
            
            return ProcessMessage::STOP;
        } else if (strcmp(messageRequest, "!back") == 0) {
            isAwayModeEnabled = false;
            awayMessage = "";
            
            LOG_INFO("AutoResponseModule: Away mode disabled");
            
            return ProcessMessage::STOP;
        }
    }
    
    if (isAwayModeEnabled && currentRequest.from != nodeDB->getNodeNum() && 
        currentRequest.to == nodeDB->getNodeNum()) {
        
        auto now = std::chrono::steady_clock::now();
        if (lastReplyTime.find(currentRequest.from) != lastReplyTime.end() && 
            (now - lastReplyTime[currentRequest.from]) < replyCooldown) {
            notifyObservers(&currentRequest);
            return ProcessMessage::CONTINUE;
        }
        
        lastReplyTime[currentRequest.from] = now;
        
        auto reply = allocDataPacket();
        reply->decoded.portnum = meshtastic_PortNum_TEXT_MESSAGE_APP;
        reply->decoded.payload.size = awayMessage.length();
        reply->from = getFrom(&currentRequest);
        reply->to = currentRequest.from;
        reply->channel = currentRequest.channel;
        reply->want_ack = (currentRequest.from != 0) ? currentRequest.want_ack : false;
        if (currentRequest.priority == meshtastic_MeshPacket_Priority_UNSET) {
            reply->priority = meshtastic_MeshPacket_Priority_RELIABLE;
        }
        reply->id = generatePacketId();
        memcpy(reply->decoded.payload.bytes, awayMessage.c_str(), reply->decoded.payload.size);
        service->handleToRadio(*reply);
        
        meshtastic_NodeInfoLite *nodeSender = nodeDB->getMeshNode(currentRequest.from);
        const char *username = nodeSender->has_user ? nodeSender->user.short_name : std::to_string(currentRequest.from).c_str();
        LOG_INFO("AutoResponseModule: Sent away message to %s", username);
    }
    
    notifyObservers(&currentRequest);
    return ProcessMessage::CONTINUE;
}

bool AutoResponseModule::wantPacket(const meshtastic_MeshPacket *p)
{
    return MeshService::isTextPayload(p);
}