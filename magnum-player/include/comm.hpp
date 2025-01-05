
#pragma once
#include <slide.hpp>
#include <string>
#include <functional>

namespace comm {

    enum class CommandId : int {
        AttachClient = 100,
        MovePreviousSlide,
        MoveNextSlide,
        ToggleFullscreen,
        Exit
    };

    struct CommandRequest {
        CommandId id;
        union {
            struct {
                int next_slide_i;
            } move_next_slide;
            struct {
                int px;
                int py;
            } draw_point;
        };
    };

    constexpr auto CommandRequestSize = sizeof(CommandRequest);

    struct CommandResponse {
        CommandId id;
        union {
            struct {
                int status_buf_size;
            } start;
        };
    };

    constexpr auto CommandResponseSize = sizeof(CommandResponse);

    using ReceivedRequestHandler = std::function<void(const comm::CommandRequest&)>;

    std::string ListAvailableIpAddresses();

    void Initialize(slide::SlideshowContext &ctx);

    bool IsConnected();
    void UpdateStatusBuffer(slide::SlideshowContext &ctx);

    void SetReceivedRequestHandler(const ReceivedRequestHandler &hhh);
    void HandleReceivedRequests();

    void PushResponse(const CommandResponse &res);

}
