// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include<map>
#include "entity.h"
#include "protocol.h"

struct Snapshot
{
    float x;
    float y;
    float ori;
    uint32_t time;
};

struct Input
{
    float thr;
    float steer;
    uint32_t time;
};


static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;
//for local phisics
static std::vector<Input> inp_history;
static std::vector<Snapshot> my_snapshot_history;



const uint32_t time_delta = 16;
float fixed_dt = 0.001f * time_delta;

uint32_t time_to_tick(uint32_t time) { return time / time_delta; }

void on_new_entity_packet(ENetPacket* packet)
{
    Entity newEntity;
    deserialize_new_entity(packet, newEntity);
    // TODO: Direct adressing, of course!
    for (const Entity& e : entities)
        if (e.eid == newEntity.eid)
            return; // don't need to do anything, we already have entity
    entities.push_back(newEntity);
}

void on_set_controlled_entity(ENetPacket* packet)
{
    deserialize_set_controlled_entity(packet, my_entity);
}

int on_snapshot(ENetPacket* packet)
{
    uint16_t eid = invalid_entity;
    float x = 0.f; float y = 0.f; float ori = 0.f;
    uint32_t tick;
    deserialize_snapshot(packet, eid, x, y, ori, tick);
    // TODO: Direct adressing, of course!
    for (Entity& e : entities)
        if (e.eid == eid)
        {
            e.x = x;
            e.y = y;
            e.ori = ori;
            e.tick = tick;
            if (e.eid == my_entity)
            {
                int i = 0;
                for (; i < my_snapshot_history.size(); ++i)
                {
                    Snapshot& s = my_snapshot_history[i];
                    if (s.time > tick)
                        break;

                    if (s.time == tick && (s.x != x || s.y != y || s.ori != ori))
                    {
                        for (Entity& e : entities)
                            if (e.eid == my_entity)
                            {
                                e.x = x;
                                e.y = y;
                                e.ori = ori;
                                uint32_t old_tick = e.tick;
                                e.tick = tick;
                                auto it_erase = inp_history.begin();
                                for (auto it =inp_history.begin(); (it < inp_history.end() && it->time < old_tick); ++it)
                                {
                                    if (it->time >= tick)
                                    {
                                        if (it + 1 < inp_history.end())
                                        {
                                            Entity& my_ent = e;
                                            my_ent.thr = it->thr;
                                            my_ent.steer = it->steer;
                                            for (uint32_t t = 0; t < (it + 1)->time - it->time; ++t)
                                            {
                                                simulate_entity(my_ent, fixed_dt);
                                                my_ent.tick++;
                                                my_snapshot_history.push_back({ my_ent.x, my_ent.y, my_ent.ori, my_ent.tick });
                                            }
                                        }
                                        else
                                        {
                                            Entity& my_ent = e;
                                            my_ent.thr = it->thr;
                                            my_ent.steer = it->steer;
                                            for (uint32_t t = 0; t < old_tick - it->time; ++t)
                                            {
                                                simulate_entity(my_ent, fixed_dt);
                                                my_ent.tick++;
                                                my_snapshot_history.push_back({ my_ent.x, my_ent.y, my_ent.ori, my_ent.tick });
                                            }
                                        }
                                    }
                                    else
                                    {
                                        it_erase = it;
                                    }
                                }

                               inp_history.erase(inp_history.begin(), it_erase);
                               i = my_snapshot_history.size();
                               break;
                            }
                    }
                }
                my_snapshot_history.erase(my_snapshot_history.begin(), my_snapshot_history.begin() + i);
            }
            
        }
    return eid;
}

int main(int argc, const char** argv)
{

    std::map< uint32_t, std::vector<Entity>> snapshot_history; //time - snapshot for interpolation
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }

    ENetHost* client = enet_host_create(nullptr, 1, 2, 0, 0);
    if (!client)
    {
        printf("Cannot create ENet client\n");
        return 1;
    }

    ENetAddress address;
    enet_address_set_host(&address, "localhost");
    address.port = 10131;

    ENetPeer* serverPeer = enet_host_connect(client, &address, 2, 0);
    if (!serverPeer)
    {
        printf("Cannot connect to server");
        return 1;
    }

    int width = 600;
    int height = 600;

    InitWindow(width, height, "w5 networked MIPT");

    const int scrWidth = GetMonitorWidth(0);
    const int scrHeight = GetMonitorHeight(0);
    if (scrWidth < width || scrHeight < height)
    {
        width = std::min(scrWidth, width);
        height = std::min(scrHeight - 150, height);
        SetWindowSize(width, height);
    }

    Camera2D camera = { {0, 0}, {0, 0}, 0.f, 1.f };
    camera.target = Vector2{ 0.f, 0.f };
    camera.offset = Vector2{ width * 0.5f, height * 0.5f };
    camera.rotation = 0.f;
    camera.zoom = 10.f;


    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

    bool connected = false;
    uint32_t time_offset = 80; // for interpolation
    uint32_t checkTime;
    std::vector<uint32_t> time_marks;
    uint32_t lastTime = enet_time_get();
    while (!WindowShouldClose())
    {
        uint32_t curTime = enet_time_get();
        ENetEvent event;
        while (enet_host_service(client, &event, 0) > 0)
        {
            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                send_join(serverPeer);
                connected = true;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                switch (get_packet_type(event.packet))
                {
                case E_SERVER_TO_CLIENT_NEW_ENTITY:
                    on_new_entity_packet(event.packet);
                    break;
                case E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY:
                    on_set_controlled_entity(event.packet);
                    break;
                case E_SERVER_TO_CLIENT_SNAPSHOT:
                    if (!entities.empty())
                    {
                        if (on_snapshot(event.packet) == entities[entities.size() - 1].eid)
                        {
                            checkTime = enet_time_get() + time_offset;
                            snapshot_history[checkTime] = entities;
                            time_marks.push_back(checkTime);

                            if (time_marks.size() > 2)
                            {
                                auto it = snapshot_history.find(time_marks[0]);
                                if (it != snapshot_history.end())
                                    snapshot_history.erase(it);
                                time_marks.erase(time_marks.begin());
                            }
                        }
                    }
                    else
                    {
                        on_snapshot(event.packet);
                    }
                    break;
                };
                break;
            default:
                break;
            };
        }
        if (my_entity != invalid_entity)
        {
            bool left = IsKeyDown(KEY_LEFT);
            bool right = IsKeyDown(KEY_RIGHT);
            bool up = IsKeyDown(KEY_UP);
            bool down = IsKeyDown(KEY_DOWN);
            // TODO: Direct adressing, of course!
            for (Entity& e : entities)
                if (e.eid == my_entity)
                {
                    // Update
                    float thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
                    float steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);

                    
                    Entity &my_entity_state = e;
                    my_entity_state.steer = steer; //local physics
                    my_entity_state.thr = thr;
                    uint32_t old_tick = my_entity_state.tick;
                    Input input(thr, steer, my_entity_state.tick);

                    //simulate
                    for (uint32_t t = 0; t < time_to_tick(curTime-lastTime); ++t)
                    {
                        simulate_entity(my_entity_state, fixed_dt);
                        my_entity_state.tick++;
                        Snapshot s(my_entity_state.x, my_entity_state.y, my_entity_state.ori, my_entity_state.tick);
                        my_snapshot_history.push_back(s);
                    }
                    lastTime += time_to_tick(curTime - lastTime) * time_delta;

                    if (old_tick != my_entity_state.tick)
                    { 
                        // Send
                        send_entity_input(serverPeer, my_entity, thr, steer);
                        inp_history.push_back(input);
                    }
                }
        }

        
         int toDelete = inp_history.size() - 100;
         if (toDelete > 0)
              inp_history.erase(std::begin(inp_history), std::begin(inp_history) + toDelete);
       
        //interpolation
        {
            if (time_marks.size() > 1)
            {
                uint32_t delta = time_marks[1] - time_marks[0];
                if (curTime < time_marks[1])
                {
                    for (const Entity& e0 : snapshot_history[time_marks[0]])
                    {
                        for (const Entity& e1 : snapshot_history[time_marks[1]])
                        {
                            if (e0.eid == e1.eid && e0.eid != my_entity)
                            {
                                float x = e0.x + (e1.x - e0.x) * (curTime - time_marks[0]) / (delta);
                                float y = e0.y + (e1.y - e0.y) * (curTime - time_marks[0]) / (delta);
                                float ori = e0.ori + (e1.ori - e0.ori) * (curTime - time_marks[0]) / (delta);
                                for (Entity& e : entities)
                                {
                                    if (e.eid == e0.eid)
                                    {
                                        e.x = x;
                                        e.y = y;
                                        e.ori = ori;
                                    }
                                } 
                            }
                            
                        }
                    }
                }
                
            }
        }


        BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(camera);

        for (Entity& e : entities)
        {
            const Rectangle rect = { e.x, e.y, 3.f, 1.f };
            DrawRectanglePro(rect, { 0.f, 0.5f }, e.ori * 180.f / PI, GetColor(e.color));
        }

        EndMode2D();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}