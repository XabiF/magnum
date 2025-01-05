
#pragma once
#include <vector>
#include <sstream>
#include <util.hpp>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
}

namespace slide {

    struct SlideId {
        int x;
        int y;

        bool operator==(const SlideId &other) const {
            return (this->x == other.x) && (this->y == other.y);
        }

        inline static bool ParseFrom(const std::string &fmt_id, SlideId &out_id) {
            int x;
            int y;
            char dash;

            std::stringstream ss(fmt_id);
            if(ss >> x >> dash >> y && dash == '-') {
                out_id.x = x;
                out_id.y = y;
                return true;
            }
            else {
                return false;
            }
        }

    };

    struct SlideIdHash {
        std::size_t operator()(const SlideId &id) const {
            // Combine hashes of x and y
            std::size_t h1 = std::hash<int>{}(id.x);
            std::size_t h2 = std::hash<int>{}(id.y);
            return h1 ^ (h2 << 1);
        }
    };

    struct SlideAnimationEntry {
        SlideId id;
        std::string caption;
        AVFormatContext *fmt_ctx;
        int video_stream_idx;
        AVCodecParameters *codec_params;
        const AVCodec *codec;
        AVCodecContext *codec_ctx;
        std::vector<SlideId> next_slide_ids;

        inline void Dispose() {
            avcodec_free_context(&this->codec_ctx);
            avformat_close_input(&this->fmt_ctx);
        }
    };

    struct SlideshowContext {
        SlideId start_slide_id;
        std::vector<SlideAnimationEntry> slides;

        inline SlideId GetSlideIdForSlideIndex(const int slide_i) {
            return this->slides.at(slide_i).id;
        }
        
        int GetSlideIndexForSlideId(const SlideId id) {
            for(int i = 0; i < this->slides.size(); i++) {
                if(this->slides.at(i).id == id) {
                    return i;
                }
            }

            return -1;
        }

        inline int GetStartSlideIndex() {
            return GetSlideIndexForSlideId(this->start_slide_id);
        }
        
        SlideId FindNextSlideIdForSlideId(const SlideId cur_slide_id, const int next_slide_i) {
            for(const auto &slide: this->slides) {
                if(slide.id == cur_slide_id) {
                    return slide.next_slide_ids.at(next_slide_i);
                }
            }

            return {};
        }

        inline int FindNextSlideIndexForSlideId(const SlideId cur_slide_id, const int next_slide_i) {
            return GetSlideIndexForSlideId(FindNextSlideIdForSlideId(cur_slide_id, next_slide_i));
        }

        SlideId FindNextSlideIdForSlideIndex(const int cur_slide_i, const int next_slide_i) {
            return this->slides.at(cur_slide_i).next_slide_ids.at(next_slide_i);
        }

        inline int FindNextSlideIndexForSlideIndex(const int cur_slide_i, const int next_slide_i) {
            return GetSlideIndexForSlideId(FindNextSlideIdForSlideIndex(cur_slide_i, next_slide_i));
        }

        inline int GetWidth() {
            return this->slides.front().codec_params->width;
        }

        inline int GetHeight() {
            return this->slides.front().codec_params->height;
        }

        inline AVPixelFormat GetPixelFormat() {
            return this->slides.front().codec_ctx->pix_fmt;
        }
    };

    bool LoadSlideshowContext(SlideshowContext &ctx, const std::string &path);

}
