#include <rt.hpp>
#include <stack>

namespace rt {

    namespace {

        std::stack<int> g_SlideIndexStack;
        
    }

    int GetCurrentSlideIndex() {
        return g_SlideIndexStack.top();
    }

    void PushSlideIndex(const int slide_i) {
        g_SlideIndexStack.push(slide_i);
    }

    void PopSlideIndex() {
        g_SlideIndexStack.pop();
    }

    bool IsInStartSlide() {
        return g_SlideIndexStack.size() == 1;
    }

}
