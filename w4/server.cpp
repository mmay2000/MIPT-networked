#include <enet/enet.h>
#include <iostream>
#include "entity.h"
#include "protocol.h"
#include <stdlib.h>
#include <vector>
#include <map>

static std::vector<Entity> entities;
static std::map<uint16_t, ENetPeer*> controlledMap;
const int AInumber = 10;

void on_join(ENetPacket *packet, ENetPeer *peer, ENetHost *host)
{
  // send all entities
  for (const Entity &ent : entities)
    send_new_entity(peer, ent);

  // find max eid
  uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
  for (const Entity &e : entities)
    maxEid = std::max(maxEid, e.eid);
  uint16_t newEid = maxEid + 1;
  uint32_t color = 0xffff0000 +
                   0x00004400 * (rand() % 5) +
                   0x00000044 * (rand() % 5);
  float x = (rand() % 4) * 2.f;
  float y = (rand() % 4) * 2.f;
  float bodySize = (rand() % 5 + 2) * 10 + 5;
  Entity ent = {color, x, y, newEid, bodySize};
  //entities.push_back(ent);

  controlledMap[newEid] = peer;


  // send info about new entity to everyone
  for (size_t i = 0; i < host->peerCount; ++i)
    send_new_entity(&host->peers[i], ent);
  // send info about controlled entity
  send_set_controlled_entity(peer, newEid);
  
  ent.type = EntityType::PLAYER;
  entities.push_back(ent);
}

void on_state(ENetPacket *packet)
{
  uint16_t eid = invalid_entity;
  float x = 0.f; float y = 0.f;
  deserialize_entity_state(packet, eid, x, y);
  for (Entity &e : entities)
    if (e.eid == eid)
    {
      e.x = x;
      e.y = y;
    }
}

int main(int argc, const char **argv)
{
  uint32_t t0 = enet_time_get();
  
  srand(time(NULL));
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10131;

  ENetHost *server = enet_host_create(&address, 32, 2, 0, 0);

  if (!server)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  for (int i = 0; i < AInumber; ++i)
  {
      uint16_t maxEid = entities.empty() ? invalid_entity : entities[0].eid;
      for (const Entity& e : entities)
          maxEid = std::max(maxEid, e.eid);
      uint16_t newEid = maxEid + 1;
      uint32_t color = 0xff000000 +
          0x00440000 * (rand() % 4 + 1) +
          0x00004400 * (rand() % 4 + 1) +
          0x00000044 * (rand() % 4 + 1);
      float x = rand() % 600 - 300;
      float y = rand() % 600 - 300;
      Entity ent = { color, x, y, newEid };
      ent.destinationX = rand() % 600 - 300;
      ent.destinationY = rand() % 600 - 300;
      ent.type = EntityType::AI;
      ent.bodySize = (rand() % 5 + 1) * 10;
      entities.push_back(ent);
  }



  while (true)
  {
    
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
          case E_CLIENT_TO_SERVER_STATE:
            on_state(event.packet);
            break;
        };
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }

    uint32_t t1 = enet_time_get();
    for (Entity& e : entities)
    {
        if (e.type != EntityType::AI)
            break;
        float dir[2] = {e.destinationX - e.x, e.destinationY - e.y};
        float l2 = dir[0] * dir[0] + dir[1] * dir[1];
        float l = sqrt(l2);
        dir[0] /= l * 1000;
        dir[1] /= l * 1000;
        float delta[2] = { dir[0] * (t1 - t0) , dir[1] * (t1 - t0) };
        if (abs(e.x + delta[0] - e.destinationX) > 10 && abs(e.y + delta[1] - e.destinationY) > 10)
        {
            e.x += delta[0];
            e.y += delta[1];
        }
        else
        {
            e.destinationX = rand() % 600 - 300;
            e.destinationY = rand() % 600 - 300;
        }
    }

    for(int i = 0; i < entities.size() - 1; ++i)
        for(int j = i + 1; j < entities.size(); ++j)
            if((abs(entities[i].x - entities[j].x) < entities[i].bodySize + entities[j].bodySize) 
                && (abs(entities[i].y - entities[j].y) < entities[i].bodySize + entities[j].bodySize))
                if (entities[i].bodySize > entities[j].bodySize)
                {
                    entities[i].bodySize += entities[j].bodySize / 2;
                    if (entities[i].bodySize > 150)
                        entities[i].bodySize = 150;

                    entities[j].bodySize /= 2;

                    if (entities[j].bodySize < 10)
                        entities[j].bodySize = 10;

                    entities[j].x = rand() % 600 - 300;
                    entities[j].y = rand() % 600 - 300;

                    if (entities[j].type == EntityType::PLAYER)
                    {
                        entities[j].bodySize += 5;
                        send_eaten_entity(controlledMap[entities[j].eid], entities[j].eid, 
                            entities[j].x, entities[j].y, entities[j].bodySize);
                    }
                    if (entities[i].type == EntityType::PLAYER)
                    {
                        send_entity_eat(controlledMap[entities[i].eid], entities[i].eid, entities[i].bodySize);
                    }
                }
                else
                {
                    if (entities[i].bodySize < entities[j].bodySize)
                    {
                        entities[j].bodySize += entities[i].bodySize / 2;
                        if (entities[j].bodySize > 150)
                            entities[j].bodySize = 150;

                        entities[i].bodySize /= 2;

                        if (entities[i].bodySize < 10)
                            entities[i].bodySize = 10;
                        entities[i].x = rand() % 600 - 300;
                        entities[i].y = rand() % 600 - 300;

                        if (entities[i].type == EntityType::PLAYER)
                        {
                            entities[i].bodySize += 5;
                            send_eaten_entity(controlledMap[entities[i].eid], entities[i].eid, 
                                entities[i].x, entities[i].y, entities[i].bodySize);
                        }
                        if (entities[j].type == EntityType::PLAYER)
                        {
                            send_entity_eat(controlledMap[entities[j].eid], entities[j].eid, entities[j].bodySize);
                        }
                    }
                }

    //static int t = 0;
    for (const Entity &e : entities)
        for (size_t i = 0; i < server->peerCount; ++i)
        {
        ENetPeer *peer = &server->peers[i];
        if (controlledMap[e.eid] != peer)
            send_snapshot(peer, e.eid, e.x, e.y, e.bodySize);
        }
    Sleep(100);
  }

  enet_host_destroy(server);

  atexit(enet_deinitialize);
  return 0;
}


