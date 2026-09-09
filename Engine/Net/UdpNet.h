#pragma once
#include "Math/Vector.h"
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#ifdef MUK_PLATFORM_WINDOWS
#include <Winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

namespace Muk {

enum class NetMsgType : uint8_t {
    Hello = 1,
    Transform = 2,
    Chat = 3,
    Ping = 4,
    Pong = 5
};

#pragma pack(push, 1)
struct NetPacket {
    uint32_t Magic = 0x4D554B31; // MUK1
    NetMsgType Type = NetMsgType::Ping;
    uint32_t PlayerId = 0;
    float X = 0, Y = 0, Z = 0;
    char Name[32] = {};
};
#pragma pack(pop)

class UdpNet {
public:
    bool StartHost(uint16_t port = 7777) {
#ifdef MUK_PLATFORM_WINDOWS
        WSADATA wsa; if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return false;
        m_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Sock == INVALID_SOCKET) return false;
        sockaddr_in addr{}; addr.sin_family = AF_INET;
        addr.sin_port = htons(port); addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(m_Sock, (sockaddr*)&addr, sizeof(addr)) != 0) return false;
        u_long non = 1; ioctlsocket(m_Sock, FIONBIO, &non);
        m_Hosting = true; m_Port = port; m_LocalId = 1;
        return true;
#else
        (void)port; return false;
#endif
    }

    bool Connect(const std::string& ip, uint16_t port = 7777) {
#ifdef MUK_PLATFORM_WINDOWS
        WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
        m_Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (m_Sock == INVALID_SOCKET) return false;
        m_Remote.sin_family = AF_INET;
        m_Remote.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &m_Remote.sin_addr);
        u_long non = 1; ioctlsocket(m_Sock, FIONBIO, &non);
        m_Hosting = false; m_LocalId = 2;
        NetPacket hello; hello.Type = NetMsgType::Hello; hello.PlayerId = m_LocalId;
        strncpy(hello.Name, "Client", 31);
        sendto(m_Sock, (char*)&hello, sizeof(hello), 0, (sockaddr*)&m_Remote, sizeof(m_Remote));
        return true;
#else
        (void)ip; (void)port; return false;
#endif
    }

    void SendTransform(const Vec3& pos) {
#ifdef MUK_PLATFORM_WINDOWS
        if (m_Sock == INVALID_SOCKET) return;
        NetPacket p; p.Type = NetMsgType::Transform; p.PlayerId = m_LocalId;
        p.X = pos.x; p.Y = pos.y; p.Z = pos.z;
        if (m_Hosting) {
            // broadcast to last remote
            if (m_HasRemote)
                sendto(m_Sock, (char*)&p, sizeof(p), 0, (sockaddr*)&m_Remote, sizeof(m_Remote));
        } else {
            sendto(m_Sock, (char*)&p, sizeof(p), 0, (sockaddr*)&m_Remote, sizeof(m_Remote));
        }
#else
        (void)pos;
#endif
    }

    void Poll(std::vector<NetPacket>& out) {
#ifdef MUK_PLATFORM_WINDOWS
        if (m_Sock == INVALID_SOCKET) return;
        for (;;) {
            NetPacket p{};
            sockaddr_in from{}; int fl = sizeof(from);
            int n = recvfrom(m_Sock, (char*)&p, sizeof(p), 0, (sockaddr*)&from, &fl);
            if (n <= 0) break;
            if (p.Magic != 0x4D554B31) continue;
            m_Remote = from; m_HasRemote = true;
            out.push_back(p);
        }
#else
        (void)out;
#endif
    }

    void Shutdown() {
#ifdef MUK_PLATFORM_WINDOWS
        if (m_Sock != INVALID_SOCKET) { closesocket(m_Sock); m_Sock = INVALID_SOCKET; }
        WSACleanup();
#endif
    }

    bool IsActive() const {
#ifdef MUK_PLATFORM_WINDOWS
        return m_Sock != INVALID_SOCKET;
#else
        return false;
#endif
    }
    bool IsHosting() const { return m_Hosting; }
    uint32_t LocalId() const { return m_LocalId; }
    uint16_t Port() const { return m_Port; }

private:
#ifdef MUK_PLATFORM_WINDOWS
    SOCKET m_Sock = INVALID_SOCKET;
    sockaddr_in m_Remote{};
    bool m_HasRemote = false;
#endif
    bool m_Hosting = false;
    uint32_t m_LocalId = 0;
    uint16_t m_Port = 7777;
};

} // namespace Muk
