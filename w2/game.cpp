#include <enet/enet.h>
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include "GameEvent.h"

ENetHost* server;
std::unordered_map<enet_uint16, std::string> gameClients;
std::vector<ENetPeer *> clients;

std::string buildMessage(GameEvent ev, std::string data)
{
    int headerSize = sizeof(GameEvent);
    std::string msg = std::string(headerSize, '\0') + std::string(data);
    std::memcpy(msg.data(), &ev, sizeof(GameEvent));
    return msg;
}

GameEvent getHeader(char* data)
{
    GameEvent ev;
    std::memcpy(&ev, data, 4);
    return ev;
}

std::string getData(char* data)
{
    int headerSize = sizeof(GameEvent);
    std::string res = data + headerSize;
    return res;
}

void logPacket(GameEvent header, std::string data, ENetPeer* ep)
{
    std::cout << "Event type: ";

    switch (header)
    {
    case GameEvent::START_GAME:
        std::cout << "START_GAME ";
        break;
    }

    std::cout << "FROM " << ep->address.host << ":" << ep->address.port << std::endl;

    if (!data.empty())
    {
        std::cout << data << std::endl;
    }
}

void send_player_join(ENetPeer* ep)
{
    for (const auto& c : clients)
    {
        if (c->address.port == ep->address.port)
            continue;

        std::string msg = buildMessage(GameEvent::PLAYER_JOIN_SEND, gameClients.at(ep->address.port));
        ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(c, 0, packet);
    }
}

void send_player_disconnect(ENetPeer* ep)
{
    return;
    for (const auto& c : clients)
    {
        if (c->address.port == ep->address.port)
            continue;

        std::string msg = buildMessage(GameEvent::PLAYER_JOIN_SEND, gameClients.at(ep->address.port));
        ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
        enet_peer_send(c, 0, packet);
    }
}

void send_player_list(ENetPeer* ep)
{
    std::string pl = "";
    for (const auto& [k, v] : gameClients)
    {
        pl += v + ",";
    }

    std::string msg = buildMessage(GameEvent::PLAYER_LIST_SEND, pl);
    ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(ep, 0, packet);
}

void processPacket(GameEvent header, std::string data, ENetPeer* ep)
{
    logPacket(header, data, ep);

    /*switch (header)
    {
    case GameEvent::START_GAME:
        if (gameState == NOT_STARTED)
        {
            broadcast_game_address();
            gameState = STARTED;
        }
        else
        {
            send_game_address(ep);
        }
        break;
    }*/

}

int main(int argc, const char** argv)
{
    if (enet_initialize() != 0)
    {
        printf("Cannot init ENet");
        return 1;
    }
    ENetAddress address;

    address.host = ENET_HOST_ANY;
    address.port = 10888;

    server = enet_host_create(&address, 32, 2, 0, 0);

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
                gameClients.emplace(event.peer->address.port, "Player " + std::to_string(gameClients.size()));
                clients.push_back(event.peer);
                std::cout << gameClients.at(event.peer->address.port) << " JOINED THE GAME" << std::endl;
                send_player_list(event.peer);
                send_player_join(event.peer);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Connection with %x:%u lost\n", event.peer->address.host, event.peer->address.port);
                send_player_disconnect(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                //printf("Packet received '%s'\n", event.packet->data);
                processPacket(getHeader((char*)event.packet->data), getData((char*)event.packet->data), event.peer);
                enet_packet_destroy(event.packet);
                break;
            default:
                break;
            };
        }


    }

    enet_host_destroy(server);

    atexit(enet_deinitialize);
    return 0;
}

