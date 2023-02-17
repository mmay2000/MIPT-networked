#include <enet/enet.h>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, const char **argv)
{

  std::vector<ENetPeer*> connectedClients;

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

  ENetHost *lobby = enet_host_create(&address, 32, 2, 0, 0);

  if (!lobby)
  {
    printf("Cannot create ENet server\n");
    return 1;
  }

  /*
  ENetAddress address_server;
  enet_address_set_host(&address_server, "localhost");
  address_server.port = 10888;

  ENetPeer* serverPeer = enet_host_connect(lobby, &address_server, 2, 0);
  if (!serverPeer)
  {
      printf("Cannot connect to server");
      return 1;
  }
  */
  bool sessionStarted = false;

  while (true)
  {
    ENetEvent event;
    while (enet_host_service(lobby, &event, 10) > 0)
    {
      switch (event.type)
      {
          case ENET_EVENT_TYPE_CONNECT:
            printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
           
                connectedClients.push_back(event.peer);

                if (sessionStarted)
                {
                    char server_data[] = "!10888";
                    ENetPacket* packet = enet_packet_create(server_data, strlen(server_data) + 1, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(event.peer, 0, packet);
                }
           

            break;
          case ENET_EVENT_TYPE_RECEIVE:
            printf("Packet received '%s'\n", event.packet->data);
            if (std::string((char*)(event.packet->data)) == "start")
            {
                char server_data[] = "!10888";
                for (ENetPeer* p : connectedClients)
                {
                    ENetPacket* packet = enet_packet_create(server_data, strlen(server_data)+1, ENET_PACKET_FLAG_RELIABLE);
                    enet_peer_send(p, 0, packet);
                }
                sessionStarted = true;
            }
            enet_packet_destroy(event.packet);
            break;
          default:
            break;
      };
    }
  }

  enet_host_destroy(lobby);

  atexit(enet_deinitialize);
  return 0;
}

