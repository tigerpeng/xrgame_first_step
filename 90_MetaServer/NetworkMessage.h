#pragma once
#include "../Urho3DAll.h"

// Identifier for the chat network messages

const int MSG_SUBCRIBE_SOUNDER  = MSG_USER + 0;     //订阅发声节点
const int MSG_ERROR             = MSG_USER + 1;

const int MSG_PLAYER_ENTER      = MSG_USER + 2;
const int MSG_PLAYER_LEAVE      = MSG_USER + 3;
const int MSG_VOICE             = MSG_USER + 4;


// UDP port we will use
static const unsigned short SERVER_PORT = 2345;
// Identifier for our custom remote event we use to tell the client which object they control
static const StringHash E_CLIENTOBJECTID("ClientObjectID");
// Identifier for the node ID parameter in the event data
static const StringHash P_ID("ID");
static const StringHash HASH_ID("HASH");

