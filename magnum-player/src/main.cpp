#include <slide.hpp>
#include <comm.hpp>
#include <rt.hpp>
#include <asset_Font.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

void OnCriticalError(const std::string &msg) {
    std::cerr << "CRITICAL ERROR: " << msg << std::endl;
    exit(EXIT_FAILURE);
}

SDL_Window *g_Window;
SDL_Renderer *g_Renderer;
bool g_Running = true;
bool g_SlidePlayFlag = false;
bool g_GoalFrameReached = false;
bool g_FullscreenFlag = false;
SwsContext *g_SwsContext;
AVFrame *g_Frame;
AVFrame *g_RgbFrame;
slide::SlideshowContext g_Context;

void StartPlay() {
    if(!g_SlidePlayFlag) {
        // Start initial play
        g_SlidePlayFlag = true;
    }
}

void AttachClient() {
    StartPlay();

    const auto res_cmd = comm::CommandResponse {
        .id = comm::CommandId::AttachClient
    };
    comm::PushResponse(res_cmd);
}

void Exit() {
    g_Running = false;
}

void RewindCurrentSlideVideo() {
    if(av_seek_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).fmt_ctx, g_Context.slides.at(rt::GetCurrentSlideIndex()).video_stream_idx, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        OnCriticalError("Error seeking to the start of the video");
    }

    // Flush the codec buffers
    avcodec_flush_buffers(g_Context.slides.at(rt::GetCurrentSlideIndex()).codec_ctx);
}

bool g_RenderBackwardFlag = false;
AVPacket g_RenderBackwardPacket;
std::vector<AVFrame*> g_BackwardRenderFrameBuffer;
size_t g_BackwardRenderFrameBufferIndex;

void PreRenderBackward() {
    g_BackwardRenderFrameBufferIndex = 0;

    // Seek to the previous keyframe
    if(av_seek_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).fmt_ctx, g_Context.slides.at(rt::GetCurrentSlideIndex()).video_stream_idx, 0, AVSEEK_FLAG_BACKWARD) < 0) {
        OnCriticalError("Error seeking to previous keyframe");
        return;
    }

    // Decode frames forward and store them
    while(av_read_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).fmt_ctx, &g_RenderBackwardPacket) >= 0) {
        if(g_RenderBackwardPacket.stream_index == g_Context.slides.at(rt::GetCurrentSlideIndex()).video_stream_idx) {
            if(avcodec_send_packet(g_Context.slides.at(rt::GetCurrentSlideIndex()).codec_ctx, &g_RenderBackwardPacket) < 0) {
                OnCriticalError("Error sending packet to decoder");
            }

            while(avcodec_receive_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).codec_ctx, g_Frame) == 0) {
                AVFrame *cloned_frame = av_frame_clone(g_Frame);
                g_BackwardRenderFrameBuffer.push_back(cloned_frame);
            }
        }
        av_packet_unref(&g_RenderBackwardPacket);
    }

    g_BackwardRenderFrameBufferIndex = g_BackwardRenderFrameBuffer.size() - 1;
}

void ReplaySlide() {
    RewindCurrentSlideVideo();

    g_RenderBackwardFlag = false;
    g_GoalFrameReached = false;
    comm::UpdateStatusBuffer(g_Context);
}

void MovePreviousSlide() {
    RewindCurrentSlideVideo();

    g_RenderBackwardFlag = true;
    g_GoalFrameReached = false;
    comm::UpdateStatusBuffer(g_Context);
}

void MoveNextSlide(const int slide_i) {
    RewindCurrentSlideVideo();

    try {
        const auto next_slide_i = g_Context.FindNextSlideIndexForSlideIndex(rt::GetCurrentSlideIndex(), slide_i);
        rt::PushSlideIndex(next_slide_i);
        LOG_FMT("Moving to next slide, index " << rt::GetCurrentSlideIndex());

        g_RenderBackwardFlag = false;
        g_GoalFrameReached = false;
        comm::UpdateStatusBuffer(g_Context);
    }
    catch(const std::exception &e) {
        LOG_FMT("Error moving to next slide: " << e.what());
    }
}

void ToggleFullscreen() {
    g_FullscreenFlag = !g_FullscreenFlag;
    if(g_FullscreenFlag) {
        SDL_SetWindowFullscreen(g_Window, SDL_WINDOW_FULLSCREEN);
    }
    else {
        SDL_SetWindowFullscreen(g_Window, 0);
    }
}

SDL_Texture *g_CurrentFrameTexture;
SDL_Texture *g_StatusTextTexture;
SDL_Rect g_ServerIpTextRect;

bool g_ShowIpText = true;

void BaseRender() {
    SDL_RenderClear(g_Renderer);
    
    SDL_RenderCopy(g_Renderer, g_CurrentFrameTexture, nullptr, nullptr);

    if(!comm::IsConnected() && g_ShowIpText) {
        SDL_RenderCopy(g_Renderer, g_StatusTextTexture, nullptr, &g_ServerIpTextRect);
    }

    SDL_RenderPresent(g_Renderer);
}

void RenderForward() {
    AVPacket packet;
    if(av_read_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).fmt_ctx, &packet) < 0) {
        // End of video reached
        g_GoalFrameReached = true;
    }

    if(packet.stream_index == g_Context.slides.at(rt::GetCurrentSlideIndex()).video_stream_idx) {
        if(avcodec_send_packet(g_Context.slides.at(rt::GetCurrentSlideIndex()).codec_ctx, &packet) < 0) {
            OnCriticalError("Error sending packet to decoder");
        }

        while(avcodec_receive_frame(g_Context.slides.at(rt::GetCurrentSlideIndex()).codec_ctx, g_Frame) == 0) {
            // Convert the frame to RGB
            sws_scale(g_SwsContext, g_Frame->data, g_Frame->linesize, 0, g_Context.GetHeight(), g_RgbFrame->data, g_RgbFrame->linesize);

            // Update the texture with the YUV frame converted to RGB
            SDL_UpdateTexture(g_CurrentFrameTexture, nullptr, g_RgbFrame->data[0], g_RgbFrame->linesize[0]);

            // Render the texture
            BaseRender();
        }
    }
    av_packet_unref(&packet);
}

void RenderBackward() {
    if(g_BackwardRenderFrameBuffer.empty()) {
        PreRenderBackward();
    }

    if((g_BackwardRenderFrameBufferIndex < 0) || (g_BackwardRenderFrameBufferIndex >= g_BackwardRenderFrameBuffer.size())) {
        return;
    }

    auto frame_to_render = g_BackwardRenderFrameBuffer.at(g_BackwardRenderFrameBufferIndex);

    // Convert the frame to RGB
    sws_scale(g_SwsContext, frame_to_render->data, frame_to_render->linesize, 0, g_Context.GetHeight(), g_RgbFrame->data, g_RgbFrame->linesize);

    // Update the texture
    SDL_UpdateTexture(g_CurrentFrameTexture, nullptr, g_RgbFrame->data[0], g_RgbFrame->linesize[0]);

    // Render the texture
    BaseRender();

    // Free the frame
    av_frame_free(&frame_to_render);

    if(g_BackwardRenderFrameBufferIndex == 0) {
        g_BackwardRenderFrameBuffer.clear();
        g_GoalFrameReached = true;
        RewindCurrentSlideVideo();

        if(!rt::IsInStartSlide()) {
            rt::PopSlideIndex();
            LOG_FMT("Moving back to previous slide, index " << rt::GetCurrentSlideIndex());
        }
        else {
            LOG_FMT("Cannot move back, already in start slide...");
        }
    }
    else {
        g_BackwardRenderFrameBufferIndex--;
    }
}

void HandleCommandRequest(const comm::CommandRequest &req_cmd) {
    switch(req_cmd.id) {
        case comm::CommandId::AttachClient: {
            LOG_FMT("AttachClient");
            AttachClient();
            break;
        }
        case comm::CommandId::MovePreviousSlide: {
            LOG_FMT("MovePreviousSlide");
            MovePreviousSlide();
            break;
        }
        case comm::CommandId::MoveNextSlide: {
            LOG_FMT("MoveNextSlide");
            MoveNextSlide(req_cmd.move_next_slide.next_slide_i);
            break;
        }
        case comm::CommandId::ToggleFullscreen: {
            LOG_FMT("ToggleFullscreen");
            ToggleFullscreen();
            break;
        }
        case comm::CommandId::Exit: {
            LOG_FMT("Exit");
            Exit();
            break;
        }
        default: {
            LOG_FMT("Received unknown command: , ID " << (int)req_cmd.id);
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <slideshow_dir_path>" << std::endl;
        return -1;
    }

    LOG_FMT("Hello world!");

    const char *slideshow_path = argv[1];

    const auto res = slide::LoadSlideshowContext(g_Context, slideshow_path);
    if(!res) {
        OnCriticalError("Unable to load slideshow");
    }

    rt::PushSlideIndex(g_Context.GetStartSlideIndex());

    const auto width = g_Context.GetWidth();
    const auto height = g_Context.GetHeight();

    if(SDL_Init(SDL_INIT_VIDEO) != 0) {
        OnCriticalError("SDL_Init error: " + std::string(SDL_GetError()));
    }

    g_Window = SDL_CreateWindow("magnum: manim slideshow player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);
    if(g_Window == nullptr) {
        OnCriticalError("SDL_CreateWindow error: " + std::string(SDL_GetError()));
    }

    g_Renderer = SDL_CreateRenderer(g_Window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(g_Renderer == nullptr) {
        OnCriticalError("SDL_CreateRenderer error: " + std::string(SDL_GetError()));
    }

    g_CurrentFrameTexture = SDL_CreateTexture(g_Renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, width, height);
    if(g_CurrentFrameTexture == nullptr) {
        OnCriticalError("SDL_CreateTexture error: " + std::string(SDL_GetError()));
    }

    if(TTF_Init() != 0) {
        OnCriticalError("TTF_Init error: " + std::string(TTF_GetError()));
    }

    comm::Initialize(g_Context);

    auto font = TTF_OpenFontRW(SDL_RWFromConstMem(asset::FontData, asset::FontData_Size), 1, 24);
    if(font == nullptr) {
        OnCriticalError("TTF_OpenFont error: " + std::string(TTF_GetError()));
    }

    const auto ips = comm::ListAvailableIpAddresses();
    LOG_FMT("Available IP addresses: " << ips);
    SDL_Color textColor = {255, 255, 255};

    const std::string status_text = ips + "\nPress X for controls help";

    SDL_Surface * ip_srf = TTF_RenderText_Solid(font, status_text.c_str(), textColor);
    if(ip_srf == nullptr) {
        OnCriticalError("TTF_RenderText_Solid error: " + std::string(TTF_GetError()));
    }

    g_StatusTextTexture = SDL_CreateTextureFromSurface(g_Renderer, ip_srf);
    if(g_StatusTextTexture == nullptr) {
        OnCriticalError("SDL_CreateTextureFromSurface error: " + std::string(SDL_GetError()));
    }

    g_ServerIpTextRect = {100, 100, ip_srf->w, ip_srf->h};
    SDL_FreeSurface(ip_srf);
    ip_srf = nullptr;

    g_Frame = av_frame_alloc();
    g_RgbFrame = av_frame_alloc();

    // Allocate memory for Y, U, and V planes
    const auto y_size = width * height;
    const auto uv_size = (width / 2) * (height / 2);
    auto buffer = (uint8_t*)av_malloc(y_size + 2 * uv_size);
    if(buffer == nullptr) {
        OnCriticalError("Could not allocate memory for the YUV420P frame");
    }

    // Set the data for the frame manually (is there any better way to automate this, some ffmpeg helper fn...?)
    g_RgbFrame->data[0] = buffer;  // Y plane
    g_RgbFrame->data[1] = buffer + y_size;  // U plane
    g_RgbFrame->data[2] = buffer + y_size + uv_size;  // V plane

    g_RgbFrame->linesize[0] = width;  // Y plane line size
    g_RgbFrame->linesize[1] = width / 2;  // U plane line size
    g_RgbFrame->linesize[2] = width / 2;  // V plane line size

    g_SwsContext = sws_getContext(width, height, g_Context.GetPixelFormat(), width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

    comm::SetReceivedRequestHandler(HandleCommandRequest);

    SDL_SetRenderDrawColor(g_Renderer, 0, 0, 0, 0);
    while(g_Running) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                Exit();
            }
            else if(event.type == SDL_KEYDOWN) {
                if(event.key.keysym.sym == SDLK_SPACE) {
                    StartPlay();
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE) {
                    Exit();
                }
                else if(event.key.keysym.sym == SDLK_f) {
                    ToggleFullscreen();
                }
                else if(event.key.keysym.sym == SDLK_i) {
                    g_ShowIpText = !g_ShowIpText;
                }
                else if((event.key.keysym.sym >= SDLK_0) && (event.key.keysym.sym <= SDLK_9)) {
                    const auto next_slide_i = event.key.keysym.sym - SDLK_0;
                    MoveNextSlide(next_slide_i);
                }
                else if(event.key.keysym.sym == SDLK_LEFT) {
                    MovePreviousSlide();
                }
                else if(event.key.keysym.sym == SDLK_RIGHT) {
                    MoveNextSlide(0);
                }
            }
        }

        comm::HandleReceivedRequests();

        if(g_SlidePlayFlag && !g_GoalFrameReached) {
            if(g_RenderBackwardFlag) {
                RenderBackward();
            }
            else {
                RenderForward();
            }
        }
        else {
            // Keep showing the last frame
            BaseRender();
        }
    }

    // Clean up
    sws_freeContext(g_SwsContext);
    av_freep(&buffer);
    av_frame_free(&g_RgbFrame);
    av_frame_free(&g_Frame);

    for(auto &slide: g_Context.slides) {
        slide.Dispose();
    }

    SDL_DestroyTexture(g_StatusTextTexture);
    SDL_DestroyTexture(g_CurrentFrameTexture);
    SDL_DestroyRenderer(g_Renderer);
    SDL_DestroyWindow(g_Window);
    SDL_Quit();

    return 0;
}

#ifdef _WIN32

#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main(__argc, __argv);
}

#endif
