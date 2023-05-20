#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include "mathUtils.h"
#include <stdlib.h>
#include <vector>
#include <map>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
static std::vector<Snapshot> snapshot_history;
static std::map<uint16_t, uint16_t> approved_snap_id_map; // eid - ac snap_id

void on_join(ENetPacket* packet, ENetPeer* peer, ENetHost* host)
{
    // send all entities
    for (const Entity& ent : entities)
        send_new_entity(peer, ent);

    // find max eid
    uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
    for (const Entity& e : entities)
        maxEid = std::max(maxEid, e.eid);
    uint16_t newEid = maxEid + 1;
    uint32_t color = 0xff000000 +
        0x00ff0000 * (rand() % 5) +
        0x00004400 * (rand() % 5) +
        0x00000044 * (rand() % 5);
    float x = (rand() % 4) * 5.f;
    float y = (rand() % 4) * 5.f;
    Entity ent = { color, x, y, 0.f, (rand() / RAND_MAX) * 3.141592654f, 0.f, 0.f, newEid };
    entities.push_back(ent);

    controlledMap[newEid] = peer;
    approved_snap_id_map[newEid] = 0;

    // send info about new entity to everyone
    for (size_t i = 0; i < host->peerCount; ++i)
        send_new_entity(&host->peers[i], ent);
    // send info about controlled entity
    send_set_controlled_entity(peer, newEid);
}

void on_input(ENetPacket* packet, ENetPeer* peer)
{
    uint16_t eid = invalid_entity;
    float thr = 0.f; float steer = 0.f;
    uint16_t new_approved_id = 0;
    deserialize_entity_input(packet, eid, thr, steer, new_approved_id);
    for (Entity& e : entities)
        if (e.eid == eid)
        {
            e.thr = thr;
            e.steer = steer;
        }

    send_input_ac(peer, new_approved_id);
}

void on_snapshot_id_ac(ENetPacket* packet)
{
    uint16_t eid, new_id;
    deserialize_general_snapshot_ac(packet, eid, new_id);
    approved_snap_id_map[eid] = new_id;
    //std::cout << approved_snap_id_map[eid] << std::endl;
}

int main(int argc, const char** argv)
{
    Snapshot snap;
    snap.entities = entities;
    snap.id = 0;
    
    std::map<uint16_t, uint8_t> headers;

    srand(time(NULL));
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10131;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    uint32_t lastTime = enet_time_get();
    while (true)
    {
        uint32_t curTime = enet_time_get();
        float dt = (curTime - lastTime) * 0.001f;
        lastTime = curTime;
        ENetEvent event;
        while (enet_host_service(server, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (get_packet_type(event.packet))
                {
                case E_CLIENT_TO_SERVER_JOIN:
                    on_join(event.packet, event.peer, server);
                    break;
                case E_CLIENT_TO_SERVER_INPUT:
                    on_input(event.packet, event.peer);
                    break;
                case E_CLIENT_TO_SERVER_AC_SNAPSHOT_ID:
                    on_snapshot_id_ac(event.packet);
                    break;
                };
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }
        static int t = 0;
        for (Entity& e : entities)
        {
            // simulate
            simulate_entity(e, dt); 
        }

        snap.entities = entities;
        snap.id++;
        snapshot_history.push_back(snap);
        

        if(!entities.empty())
        {
            for (Entity& ent : entities)
            {
                headers.clear();
                ENetPeer* peer = controlledMap[ent.eid];
                
                for (Entity& e : entities)
                {
                    // fill headers
                    uint8_t header = 0;

                    for (Entity& se : snapshot_history.at(approved_snap_id_map[ent.eid]).entities)
                    {
                        if (se.eid == e.eid)
                        {
                            if (e.x != se.x || e.y != se.y)
                            {
                                header = header | 0b11000000;
                            }

                            if (e.ori != se.ori)
                            {
                                header = header | 0b10100000;
                            }

                            break;
                        }
                    }
                    headers.insert(std::pair<uint16_t, uint8_t>(e.eid, header));
                }

                send_general_snapshot(peer, entities, headers, snap.id, approved_snap_id_map[ent.eid]);
            }
        }

        

        Sleep(100);
    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}
