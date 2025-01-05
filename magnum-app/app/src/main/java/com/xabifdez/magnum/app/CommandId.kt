package com.xabifdez.magnum.app

class CommandId {
    companion object {
        const val CommandAttachClient = 100;
        const val CommandMovePreviousSlide = CommandAttachClient + 1;
        const val CommandMoveNextSlide = CommandMovePreviousSlide + 1;
        const val CommandToggleFullscreen = CommandMoveNextSlide + 1;
        const val CommandExit = CommandToggleFullscreen + 1;
    }
}