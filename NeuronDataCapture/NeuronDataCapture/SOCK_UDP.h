
class SOCK_UDP
{
public:

    SOCK_UDP()
    {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Creates UDP socket
        if (sock == INVALID_SOCKET)
            throw std::system_error(WSAGetLastError(), std::system_category(), "Error opening socket");
    }

    ~SOCK_UDP()
    {
        closesocket(sock);
    }

    void SendTo(const std::string& address, unsigned short port, const char* buffer, int len, int flags = 0)
    {
        sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_addr.s_addr = inet_addr(address.c_str());
        add.sin_port = htons(port);
        int ret = sendto(sock, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&add), sizeof(add));
        if (ret < 0)
            throw std::system_error(WSAGetLastError(), std::system_category(), "sendto failed");
    }

    void SendTo(sockaddr_in& address, const char* buffer, int len, int flags = 0)
    {
        int ret = sendto(sock, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&address), sizeof(address));
        if (ret < 0)
            throw std::system_error(WSAGetLastError(), std::system_category(), "sendto failed");
    }

    sockaddr_in RecvFrom(char* buffer, int len, int flags = 0)
    {
        sockaddr_in from;
        int size = sizeof(from);
        int ret = recvfrom(sock, buffer, len, flags, reinterpret_cast<SOCKADDR *>(&from), &size);
		if (ret != len) {
			printf("Wrong recv lenght %d expected %d\n", ret, len);
			throw std::system_error(WSAGetLastError(), std::system_category(), "recvfrom failed wrong record length");
		}

        // make the buffer zero terminated used for testing
        //buffer[ret] = 0;
        return from;
    }

    void Bind(unsigned short port)
    {
        sockaddr_in add;
        add.sin_family = AF_INET;
        add.sin_addr.s_addr = htonl(INADDR_ANY);
        add.sin_port = htons(port);

        int ret = bind(sock, reinterpret_cast<SOCKADDR *>(&add), sizeof(add));
        if (ret < 0)
            throw std::system_error(WSAGetLastError(), std::system_category(), "Bind failed");
    }

private:
    SOCKET sock;
};