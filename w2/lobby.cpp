#include <enet/enet.h>
#include <iostream>
#include "GameEvent.h"

enum GameState
{
    NOT_STARTED,
    STARTED
};

GameState gameState = NOT_STARTED;
ENetHost* server;
std::string gameAddress = "localhost:10888";

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

void broadcast_game_address() 
{
    std::cout << "BROADCASTING GAME ADDRESS" << std::endl;
    std::string msg = buildMessage(GameEvent::GAME_ADDRESS_BROADCAST, gameAddress.data());
    ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, packet);
}

void send_game_address(ENetPeer* ep)
{
    std::string msg = buildMessage(GameEvent::GAME_ADDRESS_SEND, gameAddress.data());
    ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(ep, 0, packet);
}

void processPacket(GameEvent header, std::string data, ENetPeer* ep)
{
    logPacket(header, data, ep);

    switch (header)
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
    }

}

int main(int argc, const char **argv)
{
  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }
  ENetAddress address;

  address.host = ENET_HOST_ANY;
  address.port = 10887;

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
        if (gameState == STARTED)
        {
            std::cout << "SENDING GAME ADDRESS TO NEW CLIENT" << std::endl;
            send_game_address(event.peer);
        }
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

