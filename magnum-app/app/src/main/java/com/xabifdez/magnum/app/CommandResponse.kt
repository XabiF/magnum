package com.xabifdez.magnum.app

import java.nio.ByteBuffer

class CommandResponse(val id: Int) {
    var status_buf_size = 0;

    companion object {
        const val BufferSize = 8;

        fun parse(buffer: ByteBuffer) : CommandResponse {
            val id = buffer.getInt();
            val res = CommandResponse(id);

            if(id == CommandId.CommandAttachClient) {
                res.status_buf_size = buffer.getInt();
            }

            return res;
        }
    }
}