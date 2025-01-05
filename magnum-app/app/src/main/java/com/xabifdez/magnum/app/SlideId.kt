package com.xabifdez.magnum.app

data class SlideId(val x: Int, val y: Int) {
    companion object {
        var NullId = SlideId(Int.MAX_VALUE, Int.MAX_VALUE);
    }
}
