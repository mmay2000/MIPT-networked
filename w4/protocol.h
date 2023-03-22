#pragma once
#include <cstdint>
#include <enet/enet.h>
#include "entity.h"
#include "CustomBitstream.h"

enum MessageType : uint8_t
{
  E_CLIENT_TO_SERVER_JOIN = 0,
  E_SERVER_TO_CLIENT_NEW_ENTITY,
  E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
  E_CLIENT_TO_SERVER_STATE,
  E_SERVER_TO_CLIENT_SNAPSHOT,
  E_SERVER_TO_CLIENT_CONTROLLED_ENTITY_EATEN,
  E_SERVER_TO_CLIENT_CONTROLLED_ENTITY_EAT
};

void send_join(ENetPeer *peer);
void send_new_entity(ENetPeer *peer, const Entity &ent);
void send_set_controlled_entity(ENetPeer *peer, uint16_t eid);
void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y);
void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y, float bodySize);
void send_eaten_entity(ENetPeer* peer, uint16_t eid, float x, float y, float bodySize);
void send_entity_eat(ENetPeer* peer, uint16_t eid, float bodySize);

MessageType get_packet_type(ENetPacket *packet);

void deserialize_new_entity(ENetPacket *packet, Entity &ent);
void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid);
void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y);
void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y, float &bodySize);
void deserialize_eaten_entity(ENetPacket* packet, uint16_t& eid, float& x, float& y, float& bodySize);
void deserialize_entity_eat(ENetPacket* packet, uint16_t& eid, float& bodySize);

