#pragma once
#include <enet/enet.h>
#include <cstdint>
#include "entity.h"
#include "CustomBitstream.h"
#include <vector>
#include <map>

enum MessageType : uint8_t
{
	E_CLIENT_TO_SERVER_JOIN = 0,
	E_SERVER_TO_CLIENT_NEW_ENTITY,
	E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY,
	E_CLIENT_TO_SERVER_INPUT,
	E_SERVER_TO_CLIENT_SNAPSHOT,
	E_SERVER_TO_CLIENT_AC_INPUT_ID,
	E_CLIENT_TO_SERVER_AC_SNAPSHOT_ID,
	E_SERVER_TO_CLIENT_GENERAL_SNAPSHOT,
};

struct EntityInputInfo
{
	std::vector<Input> input_history;
	uint16_t last_based_id = 0;

};


struct Snapshot
{
	std::vector<Entity> entities;
	uint16_t id = 0;
};

struct SnapshotInfo
{
	std::vector<Snapshot> snapshot_history;
	uint16_t last_based_id = 0;
};

static std::map<uint16_t, EntityInputInfo> server_inp_history;

static SnapshotInfo client_snapshots;

void send_join(ENetPeer* peer);
void send_new_entity(ENetPeer* peer, const Entity& ent);
void send_set_controlled_entity(ENetPeer* peer, uint16_t eid);
void send_entity_input(ENetPeer* peer, uint16_t eid, float thr, float steer, uint8_t header, uint16_t cur_id, uint16_t approved_id);
void send_snapshot(ENetPeer* peer, uint16_t eid, float x, float y, float ori);
void send_input_ac(ENetPeer* peer, uint16_t approved_id);
void send_general_snapshot(ENetPeer* peer, std::vector<Entity>& entities, std::map<uint16_t, uint8_t> headers, uint16_t new_id, uint16_t base_id);
void send_general_snapshot_ac(ENetPeer* peer, uint16_t eid, uint16_t approved_id);

MessageType get_packet_type(ENetPacket* packet);

void deserialize_new_entity(ENetPacket* packet, Entity& ent);
void deserialize_set_controlled_entity(ENetPacket* packet, uint16_t& eid);
void deserialize_entity_input(ENetPacket* packet, uint16_t& eid, float& thr, float& steer, uint16_t new_id);
void deserialize_snapshot(ENetPacket* packet, uint16_t& eid, float& x, float& y, float& ori);
void deserialize_input_ac(ENetPacket* packet, uint16_t& approved_id);
void deserialize_general_snapshot(ENetPacket* packet, std::vector<Entity>& entities, uint16_t& new_id);
void deserialize_general_snapshot_ac(ENetPacket* packet, uint16_t& eid, uint16_t& approved_id);

void check_uint32_compression();
