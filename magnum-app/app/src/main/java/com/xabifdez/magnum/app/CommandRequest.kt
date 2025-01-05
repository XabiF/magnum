package com.xabifdez.magnum.app

import java.nio.ByteBuffer
import java.nio.ByteOrder

class CommandRequest(val id: Int) {
    var move_next_slide_i = 0;

    companion object {
        const val BufferSize = 12;
    }

    fun build() : ByteBuffer {
        val cmd_buf = ByteBuffer.allocate(BufferSize);
        cmd_buf.order(ByteOrder.LITTLE_ENDIAN);

        cmd_buf.putInt(this.id);

        if(this.id == CommandId.CommandMoveNextSlide) {
            cmd_buf.putInt(this.move_next_slide_i);
        }

        return cmd_buf;
    }
}
