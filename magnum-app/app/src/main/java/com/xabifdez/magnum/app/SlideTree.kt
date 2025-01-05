package com.xabifdez.magnum.app

class SlideTree(val slides: List<SlideTreeEntry>) {
    fun getSlideById(slide_id: SlideId) : SlideTreeEntry? {
        for(slide in slides) {
            if(slide.id == slide_id) {
                return slide;
            }
        }

        return null;
    }
}