// initial skeleton is a clone from https://github.com/jpcy/bgfx-minimal-example
//
#include <functional>
#include "raylib.h"
#include <enet/enet.h>
#include <math.h>

#include <vector>
#include "entity.h"
#include "protocol.h"


static std::vector<Input> input_history;
static std::vector<Entity> entities;
static uint16_t my_entity = invalid_entity;

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

void on_snapshot(ENetPacket* packet)
{
    uint16_t eid = invalid_entity;
    float x = 0.f; float y = 0.f; float ori = 0.f;
    deserialize_snapshot(packet, eid, x, y, ori);
    // TODO: Direct adressing, of course!
    for (Entity& e : entities)
        if (e.eid == eid)
        {
            e.x = x;
            e.y = y;
            e.ori = ori;
        }
}

uint16_t on_input_ac(ENetPacket* packet)
{
    uint16_t new_approved_id = 0;
    deserialize_input_ac(packet, new_approved_id);
    return new_approved_id;
}

void on_general_snapshot(ENetPacket* packet, ENetPeer* peer)
{
    //std::vector<Entity> entities_snapshot;
    uint16_t new_id;
    deserialize_general_snapshot(packet, entities, new_id);

    send_general_snapshot_ac(peer, my_entity, new_id);
}

int main(int argc, const char** argv)
{
    uint16_t inp_id = 0;
    uint16_t approved_id = 0;
    Input inp;
    inp.id = inp_id;
    inp.steer = 0;
    inp.thr = 0;
    input_history.push_back(inp);

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
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
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
                    on_snapshot(event.packet);
                    break;
                case E_SERVER_TO_CLIENT_AC_INPUT_ID:
                    approved_id = on_input_ac(event.packet);
                    while (input_history.at(0).id != approved_id) //clear history
                        input_history.erase(input_history.begin());
                    break;
                case E_SERVER_TO_CLIENT_GENERAL_SNAPSHOT:
                    on_general_snapshot(event.packet, event.peer);
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
                    int thr = 0;
                    int steer = 0;
                    inp.thr = (up ? 1.f : 0.f) + (down ? -1.f : 0.f);
                    inp.steer = (left ? -1.f : 0.f) + (right ? 1.f : 0.f);
                    inp.id = ++inp_id;
                    input_history.push_back(inp);
                    uint8_t header = 0;
                    for (Input& input : input_history)
                    {
                        if (input.id == approved_id)
                        {
                            if (input.steer != inp.steer)
                            {
                                steer = inp.steer;
                                header = header | (0b11110000);
                            }
                            if (input.thr != inp.thr)
                            {
                                thr = inp.thr;
                                header = header | (0b00001111);
                            }
                            
                            break;
                        }
                    }

                    /*uint8_t val = header;
                    int arr[8] = {0};
                    for (int i = 0; i < 8; ++i)
                    {
                        arr[i] = val % 2;
                        val /= 2;
                        std::cout << arr[i] << " ";
                    }
                    std::cout << std::endl; */
                    // Send
                    send_entity_input(serverPeer, my_entity, inp.thr, inp.steer, header, inp_id, approved_id);
                    break;
                }
        }

        BeginDrawing();
        ClearBackground(GRAY);
        BeginMode2D(camera);
        DrawRectangleLines(-16, -16, 32, 32, GetColor(0xff00ffff));
        for (const Entity& e : entities)
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