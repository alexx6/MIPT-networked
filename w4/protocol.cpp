#include "protocol.h"
#include "bitstream.h"

void send_join(ENetPeer *peer)
{
    Bitstream pdata;
    pdata.write(E_CLIENT_TO_SERVER_JOIN);

    ENetPacket *packet = enet_packet_create(pdata.get(), pdata.size(), ENET_PACKET_FLAG_RELIABLE);

    enet_peer_send(peer, 0, packet);
    if (packet->referenceCount == 0)
    {
        enet_packet_destroy(packet);
    }
}

void send_new_entity(ENetPeer *peer, const Entity &ent)
{
    Bitstream pdata;
    pdata.write(E_SERVER_TO_CLIENT_NEW_ENTITY);
    pdata.write(ent);

    ENetPacket *packet = enet_packet_create(pdata.get(), pdata.size(), ENET_PACKET_FLAG_RELIABLE);

    enet_peer_send(peer, 0, packet);
}

void send_set_controlled_entity(ENetPeer *peer, uint16_t eid)
{
    Bitstream pdata;
    pdata.write(E_SERVER_TO_CLIENT_SET_CONTROLLED_ENTITY);
    pdata.write(eid);

    ENetPacket *packet = enet_packet_create(pdata.get(), pdata.size(), ENET_PACKET_FLAG_RELIABLE);

    enet_peer_send(peer, 0, packet);
}

void send_entity_state(ENetPeer *peer, uint16_t eid, float x, float y)
{
    Bitstream pdata;
    pdata.write(E_CLIENT_TO_SERVER_STATE);
    pdata.write(eid);
    pdata.write(x);
    pdata.write(y);

    ENetPacket *packet = enet_packet_create(pdata.get(), pdata.size(), ENET_PACKET_FLAG_UNSEQUENCED);

    enet_peer_send(peer, 1, packet);
}

void send_snapshot(ENetPeer *peer, uint16_t eid, float x, float y)
{
    Bitstream pdata;
    pdata.write(E_SERVER_TO_CLIENT_SNAPSHOT);
    pdata.write(eid);
    pdata.write(x);
    pdata.write(y);

    ENetPacket *packet = enet_packet_create(pdata.get(), pdata.size(), ENET_PACKET_FLAG_UNSEQUENCED);

    enet_peer_send(peer, 1, packet);
}

MessageType get_packet_type(ENetPacket *packet)
{
  return (MessageType)*packet->data;
}

void deserialize_new_entity(ENetPacket *packet, Entity &ent)
{
    Bitstream pdata(packet->data + 1, packet->dataLength - 1);
    pdata.read(ent);
}

void deserialize_set_controlled_entity(ENetPacket *packet, uint16_t &eid)
{
    Bitstream pdata(packet->data + 1, packet->dataLength - 1);
    pdata.read(eid);
}

void deserialize_entity_state(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
    Bitstream pdata(packet->data + 1, packet->dataLength - 1);
    pdata.read(eid);
    pdata.read(x);
    pdata.read(y);
}

void deserialize_snapshot(ENetPacket *packet, uint16_t &eid, float &x, float &y)
{
    Bitstream pdata(packet->data + 1, packet->dataLength - 1);
    pdata.read(eid);
    pdata.read(x);
    pdata.read(y);
}

