#include "raylib.h"
#include <enet/enet.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include "GameEvent.h"

enum ClientState
{
    DISCONNECTED,
    LOBBY,
    CONNECTING_TO_GAME,
    GAME
};

ENetHost* client;
ClientState clientState = DISCONNECTED;
ENetPeer* gameServer = nullptr;
std::unordered_map<std::string, int> playerPings;

void gameServerConnect(std::string addressData)
{
    int d = addressData.find(':');

    ENetAddress gameAddress;
    enet_address_set_host(&gameAddress, addressData.substr(0, d).c_str());
    gameAddress.port = std::stoi(addressData.substr(d + 1).c_str());

    gameServer = enet_host_connect(client, &gameAddress, 2, 0);
    if (!gameServer)
    {
        return;
    }
}

std::string stateToString()
{
    switch (clientState)
    {
    case DISCONNECTED:
        return "DISCONNECTED";
    case LOBBY:
        return "LOBBY";
    case CONNECTING_TO_GAME:
        return "CONNECTING_TO_GAME";
    case GAME:
        return "GAME";
    default:
        return "UNDEFINED";
    }
}

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

void processPlayerList(std::string data)
{
    int sp = 0;
    int d = data.find(',', sp);
    while (d <= data.length() - 1)
    {
        playerPings.emplace(data.substr(sp, d - sp), 0);
        sp += d - sp + 1;
        d = data.find(',', sp);
    }
}

void logPacket(GameEvent header, std::string data)
{
    std::cout << "Event type: ";

    switch (header)
    {
    case GameEvent::GAME_ADDRESS_BROADCAST:
        std::cout << "GAME_ADDRESS_BROADCAST" << std::endl;
        std::cout << data << std::endl;
        break;

    case GameEvent::GAME_ADDRESS_SEND:
        std::cout << "GAME_ADDRESS_SEND" << std::endl;
        std::cout << data << std::endl;
        break;
    case GameEvent::PLAYER_LIST_SEND:
        std::cout << "PLAYER_LIST_SEND" << std::endl;
        std::cout << data << std::endl;
        break;
    case GameEvent::PLAYER_JOIN_SEND:
        std::cout << "PLAYER_JOIN_SEND" << std::endl;
        std::cout << data << std::endl;
        break;
    case GameEvent::PING_LIST_BROADCAST:
        std::cout << "PING_LIST_BROADCAST" << std::endl;
        std::cout << data << std::endl;
        break;
    }
}

void processPingList(std::string data)
{
    int sp = 0;
    int d = data.find(',', sp);
    while (d <= data.length() - 1)
    {
        std::string data1 = data.substr(sp, d - sp);
        playerPings.at(data1.substr(0, data1.find(':'))) = std::atoi(data1.substr(data1.find(':') + 1).data());
        sp += d - sp + 1;
        d = data.find(',', sp);
    }
}

void processPacket(GameEvent header, std::string data)
{
    logPacket(header, data);

    switch (header)
    {
    case GameEvent::GAME_ADDRESS_BROADCAST:
        clientState = CONNECTING_TO_GAME;
        gameServerConnect(data);
        break;
    case GameEvent::GAME_ADDRESS_SEND:
        clientState = CONNECTING_TO_GAME;
        gameServerConnect(data);
        break;
    case GameEvent::PLAYER_LIST_SEND:
        processPlayerList(data);
        break;
    case GameEvent::PLAYER_JOIN_SEND:
        playerPings.emplace(data, 0.0f);
        break;
    case GameEvent::PING_LIST_BROADCAST:
        processPingList(data);
        break;
    }
}

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

void send_player_position(float x, float y)
{
  std::string msg = buildMessage(GameEvent::PLAYER_POSITION_SEND, std::string(sizeof(float) * 2, '\0'));
  std::memcpy(msg.data() + sizeof(GameEvent), &x, sizeof(float));
  std::memcpy(msg.data() + sizeof(GameEvent) + sizeof(float), &y, sizeof(float));
  ENetPacket *packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_UNSEQUENCED);
  enet_peer_send(gameServer, 1, packet);
}

void send_start_packet(ENetPeer* peer)
{
    std::string msg = buildMessage(GameEvent::START_GAME, "");
    ENetPacket* packet = enet_packet_create(msg.data(), msg.length() + 1, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(peer, 1, packet);
    std::cout << "START REQUEST SENT" << std::endl;
}

int main(int argc, const char **argv)
{
  int width = 800;
  int height = 600;

  InitWindow(width, height, "w6 AI MIPT");

  const int scrWidth = GetMonitorWidth(0);
  const int scrHeight = GetMonitorHeight(0);
  if (scrWidth < width || scrHeight < height)
  {
    width = std::min(scrWidth, width);
    height = std::min(scrHeight - 150, height);
    SetWindowSize(width, height);
  }

  SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

  if (enet_initialize() != 0)
  {
    printf("Cannot init ENet");
    return 1;
  }

  client = enet_host_create(nullptr, 2, 4, 0, 0);
  if (!client)
  {
    printf("Cannot create ENet client\n");
    return 1;
  }

  ENetAddress address;
  enet_address_set_host(&address, "localhost");
  address.port = 10887;

  ENetPeer *lobbyPeer = enet_host_connect(client, &address, 2, 0);
  if (!lobbyPeer)
  {
    printf("Cannot connect to lobby");
    return 1;
  }

  uint32_t timeStart = enet_time_get();
  uint32_t lastFragmentedSendTime = timeStart;
  uint32_t lastMicroSendTime = timeStart;
  bool connected = false;
  float posx = rand() % width;
  float posy = rand() % height;
  Color color = { (uint8_t)(rand() % 255), (uint8_t)(rand() % 255), (uint8_t)(rand() % 255), 255 };
  while (!WindowShouldClose())
  {
    const float dt = GetFrameTime();
    ENetEvent event;
    while (enet_host_service(client, &event, 10) > 0)
    {
      switch (event.type)
      {
      case ENET_EVENT_TYPE_CONNECT:
        printf("Connection with %x:%u established\n", event.peer->address.host, event.peer->address.port);
        connected = true;
        if (event.peer->address.host == address.host && event.peer->address.port == address.port)
        {
            clientState = LOBBY;
        }

        if (gameServer)
        {
            if (event.peer->address.host == gameServer->address.host && event.peer->address.port == gameServer->address.port)
            {
                clientState = GAME;
            }
        }
        break;
      case ENET_EVENT_TYPE_RECEIVE:
        processPacket(getHeader((char*)event.packet->data), getData((char*)event.packet->data));
        enet_packet_destroy(event.packet);
        break;
      default:
        break;
      };
    }
    if (connected)
    {
      if (clientState == LOBBY && IsKeyDown(KEY_ENTER))
      {
          send_start_packet(lobbyPeer);
      }
    }
    bool left = IsKeyDown(KEY_LEFT);
    bool right = IsKeyDown(KEY_RIGHT);
    bool up = IsKeyDown(KEY_UP);
    bool down = IsKeyDown(KEY_DOWN);
    constexpr float spd = 50.f;

    posx += ((left ? -1.f : 0.f) + (right ? 1.f : 0.f)) * dt * spd;
    posy += ((up ? -1.f : 0.f) + (down ? 1.f : 0.f)) * dt * spd;

    if (gameServer && (left || right || up || down))
    {
        send_player_position(posx, posy);
    }

    BeginDrawing();
      ClearBackground(BLACK);
      DrawRectangle(posx, posy, 30, 30, color);
      DrawText(TextFormat("Current status: %s", stateToString().c_str()), 20, 20, 20, WHITE);
      DrawText(TextFormat("My position: (%d, %d)", (int)posx, (int)posy), 20, 40, 20, WHITE);
      int c = 0;
      for (const auto& [k, v] : playerPings)
      {
          DrawText((k + " Ping: " + std::to_string(v) + " ms").c_str(), 20, 80 + c * 20, 20, WHITE);
          ++c;
      }
      DrawText("List of players:", 20, 60, 20, WHITE);
    EndDrawing();
  }
  return 0;
}
