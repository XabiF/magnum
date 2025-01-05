#include <slide.hpp>
#include <ext/json.hpp>

namespace slide {

    bool LoadSlideshowContext(SlideshowContext &ctx, const std::string &path) {
        const auto tree_json_path = PathJoin(path, "tree.json");
        if(!ExistsFile(tree_json_path)) {
            LOG_FMT("Unable to load slideshow: tree JSON file not found");
            return false;
        }

        nlohmann::json tree_json_obj;
        try {
            std::ifstream tree_json(tree_json_path);
            tree_json_obj = nlohmann::json::parse(tree_json);
        }
        catch(const std::exception &e) {
            LOG_FMT("Unable to load slideshow: error: " << e.what());
            return false;
        }

        std::unordered_map<SlideId, bool, SlideIdHash> slide_is_next_of_other;

        auto dir = opendir(path.c_str());
        if(dir != nullptr) {
            while(true) {
                auto ent = readdir(dir);
                if(ent == nullptr) {
                    break;
                }

                const auto file_path = PathJoin(path, ent->d_name);
                struct stat st;
                if(stat(file_path.c_str(), &st) == 0) {
                    if(st.st_mode & S_IFREG) {
                        const auto ext = GetFileExtension(ent->d_name);
                        if(ext == "mp4") {
                            try {
                                const auto fmt_slide_id = GetFileName(ent->d_name);
                                SlideId slide_id;
                                if(!SlideId::ParseFrom(fmt_slide_id, slide_id)) {
                                    LOG_FMT("Unable to parse slide ID '" + fmt_slide_id + "'");
                                    return false;
                                }

                                if(slide_is_next_of_other.count(slide_id) == 0) {
                                    slide_is_next_of_other[slide_id] = false;
                                }

                                std::vector<SlideId> next_slide_ids;
                                const auto next_slide_ids_obj = tree_json_obj[fmt_slide_id]["next_slide_ids"];
                                const auto caption = tree_json_obj[fmt_slide_id]["caption"].get<std::string>();

                                LOG_FMT("Loading slide '" << fmt_slide_id << "' with caption '" << caption << "'");

                                for(int i = 0; i < next_slide_ids_obj.size(); i++) {
                                    const auto fmt_next_slide_id = next_slide_ids_obj[i].get<std::string>();
                                    SlideId next_slide_id;
                                    if(!SlideId::ParseFrom(fmt_next_slide_id, next_slide_id)) {
                                        LOG_FMT("Unable to parse next slide ID '" + fmt_next_slide_id + "'");
                                        return false;
                                    }
                                    next_slide_ids.push_back(next_slide_id);
                                    slide_is_next_of_other[next_slide_id] = true;
                                }

                                AVFormatContext *fmt_ctx = nullptr;
                                if(avformat_open_input(&fmt_ctx, file_path.c_str(), nullptr, nullptr) != 0) {
                                    LOG_FMT("Unable to load slideshow: could not open slide '" + fmt_slide_id + "' video");
                                    return false;
                                }

                                if(avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
                                    avformat_close_input(&fmt_ctx);
                                    LOG_FMT("Unable to load slideshow: could not open find slide '" + fmt_slide_id + "' video stream info");
                                    return false;
                                }

                                // Find the first video stream
                                int video_stream_idx = -1;
                                for(unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
                                    if(fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                                        video_stream_idx = i;
                                        break;
                                    }
                                }

                                if(video_stream_idx == -1) {
                                    avformat_close_input(&fmt_ctx);
                                    LOG_FMT("Unable to load slideshow: no video stream found for slide '" + fmt_slide_id + "'");
                                    return false;
                                }

                                AVCodecParameters *codec_params = fmt_ctx->streams[video_stream_idx]->codecpar;
                                const AVCodec *codec = avcodec_find_decoder(codec_params->codec_id);
                                if(codec == nullptr) {
                                    avformat_close_input(&fmt_ctx);
                                    LOG_FMT("Unable to load slideshow: no codec found for slide '" + fmt_slide_id + "' video stream");
                                    return false;
                                }

                                AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
                                if(avcodec_parameters_to_context(codec_ctx, codec_params) < 0) {
                                    avcodec_free_context(&codec_ctx);
                                    avformat_close_input(&fmt_ctx);
                                    LOG_FMT("Unable to load slideshow: failed to copy codec parameters for slide '" + fmt_slide_id + "' video stream");
                                    return false;
                                }

                                if(avcodec_open2(codec_ctx, codec, nullptr) < 0) {
                                    avcodec_free_context(&codec_ctx);
                                    avformat_close_input(&fmt_ctx);
                                    LOG_FMT("Unable to load slideshow: could not open codec for slide '" + fmt_slide_id + "' video stream");
                                    return false;
                                }

                                const SlideAnimationEntry entry = {
                                    slide_id,
                                    caption,
                                    fmt_ctx,
                                    video_stream_idx,
                                    codec_params,
                                    codec,
                                    codec_ctx,
                                    next_slide_ids
                                };
                                ctx.slides.push_back(std::move(entry));
                            }
                            catch(const std::exception &e) {
                                LOG_FMT("Unable to load slideshow: error: " << e.what());
                                return false;
                            }
                        }
                    }
                }
                else {
                    LOG_FMT("Unable to load slideshow: unable to open file: \"" + file_path + "\"");
                    return false;
                }
            }
        }
        else {
            LOG_FMT("Unable to load slideshow: unable to open directory: \"" + path + "\"");
            return false;
        }

        for(const auto &[slide_id, is_next] : slide_is_next_of_other) {
            if(!is_next) {
                ctx.start_slide_id = slide_id;
                LOG_FMT("Start slide: " << slide_id.x << "-" << slide_id.y);
                break;
            }
        }

        return true;
    }

}
