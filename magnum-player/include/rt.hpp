
#pragma once

namespace rt {

    int GetCurrentSlideIndex();

    void PushSlideIndex(const int slide_i);
    void PopSlideIndex();

    bool IsInStartSlide();

}
