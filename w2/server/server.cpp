#include <enet/enet.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>


struct Client
{
    ENetPeer* peer;
    std::string name;
    int id;
};

int main(int argc, const char** argv)
{
    std::map<int, Client> connectedClients;
    std::map<ENetPeer*, int> idTable;
    std::vector<std::string> namesPool = {"Yngvi Tresca", "Susy Gaertner", "Decimus Russo", "Phryne Wurtz", 
                                        "Anya Erdelyi", "Timo Mindlin", "Uschi Brown", "Patrizia Batchelor",
                                        "Balskjald Dawson", "Margarete Pabst", "Poppaea Eisenberg", "Philiskos Zhukovski",
                                        "Aleksandra Giles", "Gregory Le Verrier", "Kelly Zariski", "Sasha Bootman",
                                        "Sylke Cotes", "Hiroshi Roberts", "Melina Guinar", "Vespasianus Raman",
                                        "Voggur Egger", "Eldar Farrell", "Nikola Poschl", "Jeannette Arwen",
                                        "Rini Henstock", "Angus Dantzig", "Francka Brioschi", "Louisa Brinell",
                                        "Hurriya Seidel", "Otto Besikovitsch", "Takako Kodaira", "Kirsten Hamilton-Cayley"};
    
    int vacantId = 0;


    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10888;

    ENetHost* server = enet_host_create(&address, 32, 2, 0, 0);

    if (!server)
    {
        printf("Cannot create ENet server\n");
        return 1;
    }

    while (true)
    {
        ENetEvent event;
        while (enet_host_service(server, &event, 10) > 0)
        {
            switch (event.type)
            {
                case ENET_EVENT_TYPE_CONNECT:
                    printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
                    
                    {
                        Client newClient;
                        newClient.name = namesPool[vacantId];
                        newClient.id = vacantId;
                        newClient.peer = event.peer;
                        
                        std::string str = newClient.name.c_str() + ' ' + std::to_string(vacantId);
                        const char* msg = str.c_str();
                        ENetPacket* packet = enet_packet_create(msg, strlen(msg) + 1, ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 1, packet);

                        if (!connectedClients.empty())
                        {
                            std::string playerList = "";
                            for (int i = 0; i < vacantId; ++i)
                            {
                                playerList += connectedClients[i].name + '\n';
                            }

                            ENetPacket* packet = enet_packet_create(playerList.c_str(), 
                                                                    strlen(playerList.c_str()) + 1, 
                                                                    ENET_PACKET_FLAG_RELIABLE);
                            enet_peer_send(event.peer, 0, packet);
                        }

                        connectedClients[vacantId] = newClient;
                        idTable[event.peer] = vacantId;
                        vacantId++;
                    }
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    printf("Packet received: '%s' from %s\n", event.packet->data, 
                                                            connectedClients[idTable[event.peer]].name);
                    enet_packet_destroy(event.packet);
                    break;
                default:
                    break;
            };
        }

        if (!connectedClients.empty())
        {
            std::string serverTime = std::to_string(time(0)) + " server time\n"; //server time
            for (int i = 0; i < vacantId; ++i)
            {
                ENetPacket* packet = enet_packet_create(serverTime.c_str(), 
                                                        strlen(serverTime.c_str()) + 1, 
                                                        ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(connectedClients[i].peer, 0, packet);

                std::string strPing = std::to_string(connectedClients[i].peer->roundTripTime) //player ping
                                        + connectedClients[i].name; 

                for (int j = 0; j < vacantId; ++j)
                {
                    if (i == j)
                        continue;

                    ENetPacket* packetPing = enet_packet_create(strPing.c_str(),
                        strlen(strPing.c_str()) + 1,
                        ENET_PACKET_FLAG_UNSEQUENCED);
                    enet_peer_send(connectedClients[j].peer, 0, packetPing);
                }
            }

        }

    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}