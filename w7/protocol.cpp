#include "protocol.h"
#include "quantisation.h"
#include <cstring> // memcpy
#include <iostream>

void send_join(ENetPeer* peer)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t), ENET_PACKET_FLAG_RELIABLE);
    *packet->data = E_CLIENT_TO_SERVER_JOIN;

    Snapshot snpsht;
    client_snapshots.snapshot_history.push_back(snpsht);

    enet_peer_send(peer, 0, packet);
}

void send_new_entity(ENetPeer* peer, const Entity& ent)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(Entity),
        ENET_PACKET_FLAG_RELIABLE);
    uint8_t* ptr = packet->data;
    *ptr = E_SERVER_TO_CLIENT_NEW_ENTITY; ptr += sizeof(uint8_t);
    memcpy(ptr, &ent, sizeof(Entity)); ptr += sizeof(Entity);

    enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer* peer, uint16_t eid)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t), //+ sizeof(uint8_t),
        ENET_PACKET_FLAG_RELIABLE);
    CustomBitstream bs(packet->data);
    bs.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
    bs.write(eid);
   // uint32_t a = 123;
    //printf("%d", a);
   // bs.write_packing_uint32(a);
    enet_peer_send(peer, 0, packet);

    std::vector<Input> vec;
    server_inp_history.insert(std::pair<int, std::vector<Input>>(eid, vec));
}

void send_entity_input(ENetPeer* peer, uint16_t eid, float thr, float ori, uint8_t header, uint16_t cur_id, uint16_t approved_id)
{
    size_t packet_size = sizeof(uint8_t) + sizeof(uint16_t) * 3 + sizeof(uint8_t);
    uint8_t check = 128 & header;
    if (check == 128)
    {
        packet_size += sizeof(float);
    }
    check = 8 & header;
    if (check == 8)
    {
        packet_size += sizeof(float);
    }
    ENetPacket* packet = enet_packet_create(nullptr, packet_size,
        ENET_PACKET_FLAG_UNSEQUENCED);
    CustomBitstream bs(packet->data);
    bs.write(E_CLIENT_TO_SERVER_INPUT);
    bs.write(eid);
    bs.write(cur_id);
    bs.write(approved_id);
    bs.write(header);

    check = 128 & header;
    if (check == 128)
    {
        bs.write(ori);
    }
    check = 8 & header;
    if (check == 8)
    {
        bs.write(thr);
    }
    //uint8_t* ptr = packet->data;
    //*ptr = E_CLIENT_TO_SERVER_INPUT; ptr += sizeof(uint8_t);
   // memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);
   /* float4bitsQuantized thrPacked(thr, -1.f, 1.f);
    float4bitsQuantized oriPacked(ori, -1.f, 1.f);
    uint8_t thrSteerPacked = (thrPacked.packedVal << 4) | oriPacked.packedVal;
    memcpy(ptr, &thrSteerPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);
   */
    //memcpy(ptr, &thr, sizeof(float)); ptr += sizeof(float);
   // memcpy(ptr, &ori, sizeof(float)); ptr += sizeof(float);
    

    enet_peer_send(peer, 1, packet);
}

typedef PackedFloat<uint16_t, 11> PositionXQuantized;
typedef PackedFloat<uint16_t, 10> PositionYQuantized;

void send_snapshot(ENetPeer* peer, uint16_t eid, float x, float y, float ori)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) +
        sizeof(uint32_t) +
        sizeof(uint8_t),
        ENET_PACKET_FLAG_UNSEQUENCED);
    uint8_t* ptr = packet->data;
    *ptr = E_SERVER_TO_CLIENT_SNAPSHOT; ptr += sizeof(uint8_t);
    memcpy(ptr, &eid, sizeof(uint16_t)); ptr += sizeof(uint16_t);

    uint8_t oriPacked = pack_float<uint8_t>(ori, -PI, PI, 8);

    PackedVec2<uint32_t, 11, 10> XY(x, y, -8, 8);

    memcpy(ptr, &XY.packedval, sizeof(uint32_t)); ptr += sizeof(uint32_t);
    memcpy(ptr, &oriPacked, sizeof(uint8_t)); ptr += sizeof(uint8_t);

    enet_peer_send(peer, 1, packet);
}

void send_input_ac(ENetPeer* peer, uint16_t approved_id)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t),
        ENET_PACKET_FLAG_UNSEQUENCED);
    CustomBitstream bs(packet->data);
    bs.write(E_SERVER_TO_CLIENT_AC_INPUT_ID);
    bs.write(approved_id);
    enet_peer_send(peer, 1, packet);
}

void send_general_snapshot(ENetPeer* peer, std::vector<Entity>& entities, std::map<uint16_t, uint8_t> headers, uint16_t new_id, uint16_t base_id)
{
    size_t packet_size = sizeof(uint8_t) + sizeof(size_t) 
        + headers.size() * sizeof(uint8_t) + (entities.size() + 2) * sizeof(uint16_t);

    for (Entity& ent : entities)
    {
        if (headers[ent.eid] != 0)
        {
            uint8_t check = headers[ent.eid] & 0b01000000;
            if (check == 0b01000000)
            {
                packet_size += sizeof(uint32_t);
            }

            check = headers[ent.eid] & 0b00100000;
            if (check == 0b00100000)
            {
                packet_size += sizeof(uint8_t);
            }
        }
    }

    ENetPacket* packet = enet_packet_create(nullptr, packet_size,
        ENET_PACKET_FLAG_UNSEQUENCED);
    CustomBitstream bs(packet->data);
    bs.write(E_SERVER_TO_CLIENT_GENERAL_SNAPSHOT);
    bs.write(entities.size());
    bs.write(new_id);
    bs.write(base_id);
    for (Entity& ent : entities)
    {
        bs.write(ent.eid);
        bs.write(headers[ent.eid]);
        if (headers[ent.eid] != 0)
        {
            uint8_t check = headers[ent.eid] & 0b01000000;
            if (check == 0b01000000)
            {
                PackedVec2<uint32_t, 11, 10> XY(ent.x, ent.y, -16, 16);
                bs.write(XY.packedval);
            }

            check = headers[ent.eid] & 0b00100000;
            if (check == 0b00100000)
            {
                uint8_t oriPacked = pack_float<uint8_t>(ent.ori, -PI, PI, 8);
                bs.write(oriPacked);
            }
        }       
    }
    enet_peer_send(peer, 1, packet);
}

void send_general_snapshot_ac(ENetPeer* peer, uint16_t eid, uint16_t approved_id)
{
    ENetPacket* packet = enet_packet_create(nullptr, sizeof(uint8_t) + sizeof(uint16_t) * 2,
        ENET_PACKET_FLAG_UNSEQUENCED);
    CustomBitstream bs(packet->data);
    bs.write(E_CLIENT_TO_SERVER_AC_SNAPSHOT_ID);
    bs.write(eid);
    bs.write(approved_id);
    enet_peer_send(peer, 1, packet);
}


MessageType get_packet_type(ENetPacket* packet)
{
    return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket* packet, Entity& ent)
{
    uint8_t* ptr = packet->data; ptr += sizeof(uint8_t);
    ent = *(Entity*)(ptr); ptr += sizeof(Entity);
}

void deserialize_set_controlled_entity(ENetPacket* packet, uint16_t& eid)
{
    MessageType skip;
    CustomBitstream bs(packet->data);
    bs.read(skip);
    bs.read(eid);
   // uint32_t a = bs.read_packing_uint32();
   // printf("\n%d", a);
}

void deserialize_entity_input(ENetPacket* packet, uint16_t& eid, float& thr, float& steer, uint16_t new_id)
{
    uint16_t based_id;
    uint8_t header;
    CustomBitstream bs(packet->data);
    MessageType skip;
    bs.read(skip);
    bs.read(eid);
    bs.read(new_id);
    bs.read(based_id);
    bs.read(header);
    uint8_t check = 128 & header;
    if (check == 128)
    {
        bs.read(steer);
    }
    else
        for (Input& inp : server_inp_history[eid].input_history)
            if (inp.id == based_id)
                steer = inp.steer;
    check = 8 & header;
    if (check == 8)
    {
        bs.read(thr);
    }
    else
        for (Input& inp : server_inp_history[eid].input_history)
            if (inp.id == based_id)
                thr = inp.thr;

    Input new_input;
    new_input.id = new_id;
    new_input.steer = steer;
    new_input.thr = thr;

    server_inp_history[eid].input_history.push_back(new_input);
    if (server_inp_history[eid].last_based_id < based_id)
    {
        while (server_inp_history[eid].input_history.at(0).id != based_id)
            server_inp_history[eid].input_history.erase(server_inp_history[eid].input_history.begin());

        server_inp_history[eid].last_based_id = based_id;
    }

    /* uint8_t* ptr = packet->data; ptr += sizeof(uint8_t);
     eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
     uint8_t thrSteerPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);

     uint8_t thrPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);
     uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);

     static uint8_t neutralPackedValue = pack_float<uint8_t>(0.f, -1.f, 1.f, 4);
     static uint8_t nominalPackedValue = pack_float<uint8_t>(1.f, 0.f, 1.2f, 4);
     float4bitsQuantized thrPacked(thrSteerPacked >> 4);
     float4bitsQuantized steerPacked(thrSteerPacked & 0x0f);
     thr = thrPacked.packedVal == neutralPackedValue ? 0.f : thrPacked.unpack(-1.f, 1.f);
     steer = steerPacked.packedVal == neutralPackedValue ? 0.f : steerPacked.unpack(-1.f, 1.f);
     */
}


void deserialize_snapshot(ENetPacket* packet, uint16_t& eid, float& x, float& y, float& ori)
{
    uint8_t* ptr = packet->data; ptr += sizeof(uint8_t);
    eid = *(uint16_t*)(ptr); ptr += sizeof(uint16_t);
    uint32_t xyPacked = *(uint32_t*)(ptr); ptr += sizeof(uint32_t);
    PackedVec2<uint32_t, 11, 10> XY(xyPacked);
    uint8_t oriPacked = *(uint8_t*)(ptr); ptr += sizeof(uint8_t);

    vec2 coords = XY.unpack(-8, 8);
    x = coords.x;
    y = coords.y;

    ori = unpack_float<uint8_t>(oriPacked, -PI, PI, 8);
}

void deserialize_input_ac(ENetPacket* packet, uint16_t& approved_id)
{
    CustomBitstream bs(packet->data);
    MessageType skip;
    bs.read(skip);
    bs.read(approved_id);
}

void deserialize_general_snapshot(ENetPacket* packet, std::vector<Entity>& entities, uint16_t& new_id)
{
    CustomBitstream bs(packet->data);
    MessageType skip;
    bs.read(skip);
    size_t entities_num;
    bs.read(entities_num);
    uint8_t header;
    uint16_t base_id;
    bs.read(new_id);
    bs.read(base_id);
    Snapshot snap;
    snap.id = new_id;
    for (size_t i = 0; i < entities_num; ++i)
    {
        uint16_t eid;
        bs.read(eid);
        bs.read(header);
        
        for (Entity& ent : entities)
            if (ent.eid == eid)
            {
                if (header != 0)
                {
                    uint8_t check = header & 0b01000000;
                    if (check == 0b01000000)
                    {
                        uint32_t xyPacked;
                        bs.read(xyPacked);
                        PackedVec2<uint32_t, 11, 10> XY(xyPacked);
                        vec2 coords = XY.unpack(-16, 16);
                        ent.x = coords.x;
                        ent.y = coords.y;
                    }
                    else
                    {
                        for (Snapshot& bs : client_snapshots.snapshot_history)
                            if (bs.id == base_id)
                            {
                                for (Entity& e : bs.entities)
                                if (ent.eid == e.eid)
                                    {
                                        ent.x = e.x;
                                        ent.y = e.y;
                                        break;
                                    }
                                break;
                            }
                    }

                    check = header & 0b00100000;
                    if (check == 0b00100000)
                    {
                        uint8_t oriPacked;
                        bs.read(oriPacked);
                        ent.ori = unpack_float<uint8_t>(oriPacked, -PI, PI, 8);
                    }
                    else
                    {
                        for (Snapshot& bs : client_snapshots.snapshot_history)
                            if (bs.id == base_id)
                            {
                                for (Entity& e : bs.entities)
                                    if (ent.eid == e.eid)
                                    {
                                        ent.ori = e.ori;
                                        break;
                                    }
                                break;
                            }
                    }
                }
                else
                {
                    for (Snapshot& bs : client_snapshots.snapshot_history)
                        if (bs.id == base_id)
                        {
                            for (Entity& e : bs.entities)
                                if (ent.eid == e.eid)
                                {
                                    ent.x = e.x;
                                    ent.y = e.y;
                                    ent.ori = e.ori;
                                    break;
                                }
                            break;
                        }
                }

                snap.entities.push_back(ent);
            }
    }

    client_snapshots.snapshot_history.push_back(snap);

    if (client_snapshots.last_based_id < base_id)
    {
        while (client_snapshots.snapshot_history.at(0).id != base_id)
            client_snapshots.snapshot_history.erase(client_snapshots.snapshot_history.begin());
        client_snapshots.last_based_id = base_id;
    }
}

void deserialize_general_snapshot_ac(ENetPacket* packet, uint16_t& eid, uint16_t& approved_id)
{
    CustomBitstream bs(packet->data);
    MessageType skip;
    bs.read(skip);
    bs.read(eid);
    bs.read(approved_id);
}
