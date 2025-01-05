#include <comm.hpp>
#include <util.hpp>
#include <rt.hpp>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <queue>
#include <mutex>

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

namespace comm {

    namespace {

        constexpr auto CommunicationTcpPort = 2147;

        std::vector<uint8_t> g_StatusBuffer;

        inline void SetStatusBuffer32(const size_t offset, const int value) {
            g_StatusBuffer[offset] = value & 0xFF;
            g_StatusBuffer[offset + 1] = (value >> 8) & 0xFF;
            g_StatusBuffer[offset + 2] = (value >> 16) & 0xFF;
            g_StatusBuffer[offset + 3] = (value >> 24) & 0xFF;
        }

        inline void PushStatusBuffer32(const int value) {
            g_StatusBuffer.push_back(value & 0xFF);
            g_StatusBuffer.push_back((value >> 8) & 0xFF);
            g_StatusBuffer.push_back((value >> 16) & 0xFF);
            g_StatusBuffer.push_back((value >> 24) & 0xFF);
        }

        void InitializeStatusBuffer(slide::SlideshowContext &ctx) {
            g_StatusBuffer.clear();

            // This first int field will be updated every time we move to another slide, the rest of the buffer doesn't change

            const auto slide_id = ctx.GetSlideIdForSlideIndex(rt::GetCurrentSlideIndex());
            PushStatusBuffer32(slide_id.x);
            PushStatusBuffer32(slide_id.y);

            for(const auto &slide: ctx.slides) {
                PushStatusBuffer32(slide.id.x);
                PushStatusBuffer32(slide.id.y);

                PushStatusBuffer32(slide.caption.length());
                for(int i = 0; i < slide.caption.length(); i++) {
                    g_StatusBuffer.push_back(slide.caption.c_str()[i]);
                }

                PushStatusBuffer32(slide.next_slide_ids.size());
                for(const auto &next_slide_id: slide.next_slide_ids) {
                    PushStatusBuffer32(next_slide_id.x);
                    PushStatusBuffer32(next_slide_id.y);
                }
            }
        }

        std::queue<CommandRequest> g_RequestQueue;
        std::mutex g_RequestQueueLock;

        std::queue<CommandResponse> g_ResponseQueue;
        std::mutex g_ResponseQueueLock;

        std::atomic_bool g_Connected = false;

        ReceivedRequestHandler g_ReceivedRequestHandler = nullptr;

    }

}

namespace comm {

    void UpdateStatusBuffer(slide::SlideshowContext &ctx) {
        const auto slide_id = ctx.GetSlideIdForSlideIndex(rt::GetCurrentSlideIndex());
        SetStatusBuffer32(0, slide_id.x);
        SetStatusBuffer32(0 + sizeof(int), slide_id.y);
    }

    namespace {

        void NetReadThread(tcp::socket &socket) {
            try {
                while(true) {
                    std::array<CommandRequest, 1> data;
                    boost::asio::read(socket, boost::asio::buffer(data));

                    std::lock_guard<std::mutex> lock(g_RequestQueueLock);
                    g_RequestQueue.push(data.front());
                }
            }
            catch(const std::exception &e) {
                LOG_FMT("Assuming disconnection, got error: " << e.what());
                g_Connected = false;
            }
        }

        void NetWriteThread(tcp::socket &socket) {
            try {
                while(true) {
                    {
                        std::lock_guard<std::mutex> lock(g_ResponseQueueLock);
                        while(!g_ResponseQueue.empty()) {
                            auto cmd_res = g_ResponseQueue.front();
                            g_ResponseQueue.pop();

                            if(cmd_res.id == CommandId::AttachClient) {
                                cmd_res.start.status_buf_size = g_StatusBuffer.size();
                            }

                            std::array<CommandResponse, 1> data = { cmd_res };
                            boost::asio::write(socket, boost::asio::buffer(data));

                            if(cmd_res.id == CommandId::AttachClient) {
                                // Send playback status on client attaching
                                boost::asio::write(socket, boost::asio::buffer(g_StatusBuffer));
                            }
                        }
                    }
                }
            }
            catch(const std::exception &e) {
                LOG_FMT("Assuming disconnection, got error: " << e.what());
                g_Connected = false;
            }
        }

        void NetConnectThread() {
            boost::asio::io_context io_context;
            tcp::endpoint endpoint(tcp::v4(), CommunicationTcpPort);

            while(true) {
                try {
                    tcp::acceptor acceptor(io_context, endpoint);

                    LOG_FMT("Waiting for client (app) connection...");
                    tcp::socket socket(io_context);
                    acceptor.accept(socket);

                    LOG_FMT("Client connected from IP " << socket.remote_endpoint().address().to_string() << ", port " << socket.remote_endpoint().port());
                    g_Connected = true;

                    // Spawn read and write threads for this connection

                    std::thread read_thr([&socket]() {
                        NetReadThread(socket);
                    });
                    read_thr.detach();

                    std::thread write_thr([&socket]() {
                        NetWriteThread(socket);
                    });
                    write_thr.detach();

                    // Wait until disconnection (r/w threads will have exited as well)

                    while(g_Connected) {}

                    socket.close();
                }
                catch(const std::exception &e) {
                    LOG_FMT("Got error: " << e.what());
                }
            }
        }

    }

    std::string ListAvailableIpAddresses() {
        struct ifaddrs* ifaddr_ptr = nullptr;
        getifaddrs(&ifaddr_ptr);

        std::string ips;

        for(struct ifaddrs* ifa = ifaddr_ptr; ifa != nullptr; ifa = ifa->ifa_next) {
            if(ifa->ifa_addr && (ifa->ifa_addr->sa_family == AF_INET)) {
                void* addr_ptr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                char addr_str[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, addr_ptr, addr_str, INET_ADDRSTRLEN);

                // Skip loopback (127.x.x.x)
                if(std::strncmp(addr_str, "127", 3) != 0) {
                    ips += addr_str;
                    ips += ", ";
                }
            }
        }

        // Remove extra final ", "
        if(!ips.empty()) {
            ips.pop_back(); ips.pop_back();
        }

        freeifaddrs(ifaddr_ptr);
        return ips;
    }

    void Initialize(slide::SlideshowContext &ctx) {
        InitializeStatusBuffer(ctx);

        std::thread thr(NetConnectThread);
        thr.detach(); 
    }

    bool IsConnected() {
        return g_Connected;
    }

    void SetReceivedRequestHandler(const ReceivedRequestHandler &handler) {
        g_ReceivedRequestHandler = handler;
    }

    void HandleReceivedRequests() {
        std::lock_guard<std::mutex> lock(g_RequestQueueLock);
        while(!g_RequestQueue.empty()) {
            const auto req_cmd = g_RequestQueue.front();
            g_RequestQueue.pop();

            if(g_ReceivedRequestHandler != nullptr) {
                g_ReceivedRequestHandler(req_cmd);
            }
        }
    }

    void PushResponse(const CommandResponse &res_cmd) {
        std::lock_guard<std::mutex> lock(g_ResponseQueueLock);
        g_ResponseQueue.push(res_cmd);
    }

}
