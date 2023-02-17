#include <enet/enet.h>
#include <iostream>
#include <thread>
#include <string>

void send_fragmented_packet(ENetPeer *peer)
{
  const char *baseMsg = "Stay awhile and listen. ";
  const size_t msgLen = strlen(baseMsg);

  const size_t sendSize = 2500;
  char *hugeMessage = new char[sendSize];
  for (size_t i = 0; i < sendSize; ++i)
    hugeMessage[i] = baseMsg[i % msgLen];
  hugeMessage[sendSize-1] = '\0';

  ENetPacket *packet = enet_packet_create(hugeMessage, sendSize, ENET_PACKET_FLAG_RELIABLE);
  enet_peer_send(peer, 0, packet);

  delete[] hugeMessage;
}

void send_micro_packet(ENetPeer *peer)
{
  const char *msg = "dv/dt";
  ENetPacket *packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(peer, 1, packet);
}


void InputStart(ENetPeer* serv, ENetPeer* lob)
{
    while (!serv) {
        std::string input;
        std::getline(std::cin, input);
        if (strcmp(input.data(), "start") == 0)
        {
            printf("Starting the game...\n");
            const char* msg = "start";
            ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(lob, 0, packet);
            break;
        }
    }
}


int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  ENetHost *client = enet_host_create(nullptr, 2, 2, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  

  ENetAddress address_lobby;
  enet_address_set_host(&address_lobby, "localhost");
  address_lobby.port = 10887;

  ENetPeer* serverPeer = nullptr;
  ENetPeer* lobbyPeer = enet_host_connect(client, &address_lobby, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  std::thread inputThread(InputStart, serverPeer, lobbyPeer);
  inputThread.detach();


  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  bool inGame = false;
  
  while (true)
  {
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
          case ENET_EVENT_TYPE_CONNECT:
            printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
            connected = true;
            break;
          case ENET_EVENT_TYPE_RECEIVE:
            printf("Packet received '%s'\n", event.packet->data);
            if (event.packet->data[0] == '!')
            {
                
                ENetAddress address_server;
                enet_address_set_host(&address_server, "localhost");
                address_server.port = std::atoi((const char*)(event.packet->data + 1));

                serverPeer = enet_host_connect(client, &address_server, 2, 0);
                if (!serverPeer)
                {
                    printf("Cannot connect to server");
                    return 1;
                }
                inGame = true;
            }
            enet_packet_destroy(event.packet);
            break;
          default:
            break;
      };
    }
    if (inGame && serverPeer != nullptr)
    {
      uint32_t curTime = enet_time_get();
      srand(time(0));
      std::string msg = "My time = " + std::to_string(curTime + rand());
      ENetPacket* packet = enet_packet_create(msg.c_str(), strlen(msg.c_str()) + 1, ENET_PACKET_FLAG_RELIABLE);
      enet_peer_send(serverPeer, 0, packet);
    }
  }

  inputThread.join();
  return 0;
}
