package com.xabifdez.magnum.app

import android.util.Log
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch
import java.net.Socket
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.Stack
import java.util.concurrent.ConcurrentLinkedQueue

class NetContext(val ip: String) {
    val req_queue = ConcurrentLinkedQueue<CommandRequest>()
    val res_queue = ConcurrentLinkedQueue<CommandResponse>()

    fun writeThread(socket: Socket) {
        val output = socket.getOutputStream();

        while(true) {
            if(!this.req_queue.isEmpty()) {
                val cmd = this.req_queue.poll()!!;

                output.write(cmd.build().array());
                output.flush();
            }
        }
    }

    fun readThread(socket: Socket) {
        val input = socket.getInputStream();

        while(true) {
            val res_buf = ByteBuffer.allocate(CommandResponse.BufferSize);
            res_buf.order(ByteOrder.LITTLE_ENDIAN);

            val temp = ByteArray(CommandResponse.BufferSize);
            val read_size = input.read(temp);

            if(read_size == CommandResponse.BufferSize) {
                res_buf.put(temp, 0, CommandResponse.BufferSize);
                res_buf.rewind();

                val res = CommandResponse.parse(res_buf);
                this.res_queue.offer(res);

                if(res.id == CommandId.CommandAttachClient) {
                    // Receive slide id map buffer

                    Log.d("attach", "attach with ${res.status_buf_size}")

                    val status_buf_data = ByteArray(res.status_buf_size);
                    val read_size_2 = input.read(status_buf_data);
                    if(read_size_2 == res.status_buf_size) {
                        val status_buf = ByteBuffer.allocate(res.status_buf_size);
                        status_buf.order(ByteOrder.LITTLE_ENDIAN);

                        status_buf.put(status_buf_data, 0, res.status_buf_size);
                        status_buf.rewind();

                        val cur_slide_id_x = status_buf.getInt();
                        val cur_slide_id_y = status_buf.getInt();
                        val cur_slide_id = SlideId(cur_slide_id_x, cur_slide_id_y);

                        val slide_map: MutableList<SlideTreeEntry> = mutableListOf();
                        val slide_is_next_of_other: MutableMap<SlideId, Boolean> = mutableMapOf();

                        while(status_buf.hasRemaining()) {
                            val slide_id_x = status_buf.getInt();
                            val slide_id_y = status_buf.getInt();
                            val slide_id = SlideId(slide_id_x, slide_id_y);
                            if(!slide_is_next_of_other.containsKey(slide_id)) {
                                slide_is_next_of_other[slide_id] = false;
                            }

                            val name_length = status_buf.getInt();
                            var name = "";
                            for(i in 0 until name_length) {
                                val chr = status_buf.get();
                                name += chr.toInt().toChar();
                            }

                            val next_slide_id_count = status_buf.getInt();
                            val next_slide_ids: MutableList<SlideId> = mutableListOf();
                            for(i in 0 until next_slide_id_count) {
                                val next_slide_id_x = status_buf.getInt();
                                val next_slide_id_y = status_buf.getInt();
                                val next_slide_id = SlideId(next_slide_id_x, next_slide_id_y);
                                slide_is_next_of_other[next_slide_id] = true;
                                next_slide_ids.add(next_slide_id);
                            }

                            val slide_entry = SlideTreeEntry(slide_id, name, next_slide_ids);
                            slide_map.add(slide_entry);
                        }

                        var start_slide_id = SlideId.NullId;
                        for((slide_id, is_next) in slide_is_next_of_other) {
                            if(!is_next) {
                                start_slide_id = slide_id;
                                break;
                            }
                        }

                        var next_check_slide_id = cur_slide_id;
                        val slide_id_list: MutableList<SlideId> = mutableListOf();
                        slide_id_list.add(cur_slide_id);
                        while(true) {
                            var found = false;
                            for(slide in slide_map) {
                                if(slide.next_slide_ids.contains(next_check_slide_id)) {
                                    next_check_slide_id = slide.id;
                                    slide_id_list.add(next_check_slide_id);
                                    found = true;
                                    break;
                                }
                            }

                            if(!found) {
                                break;
                            }
                        }

                        val slide_id_stack = Stack<SlideId>().apply {
                            addAll(slide_id_list.asReversed());
                        };

                        MainActivity.Main.loadStatus(SlideTree(slide_map), slide_id_stack);
                        Log.d("dg", "got map $slide_map with stack $slide_id_stack");
                    }
                    else {
                        if(read_size_2 == -1) {
                            return;
                        }
                        Log.d("read", "Read invalid data of size $read_size_2");
                    }
                }
            }
            else {
                if(read_size == -1) {
                    return;
                }
                Log.d("read", "Read invalid data of size $read_size");
            }
        }
    }

    fun start() {
        this.sendRequest(CommandRequest(CommandId.CommandAttachClient));

        GlobalScope.launch {
            try {
                val socket = Socket(ip, 2147);

                GlobalScope.launch {
                    writeThread(socket);
                }

                GlobalScope.launch {
                    readThread(socket);
                    MainActivity.Main.notifyDisconnected();
                }
            }
            catch(e: Exception) {
                Log.d("conn", "Exception: ${e}");
                MainActivity.Main.notifyDisconnected();
            }
        }
    }

    private fun sendRequest(cmd: CommandRequest) {
        Log.d("read", "sending request ${cmd.id}")
        this.req_queue.offer(cmd);
    }

    fun requestMovePreviousSlide() {
        this.sendRequest(CommandRequest(CommandId.CommandMovePreviousSlide));
    }

    fun requestMoveNextSlide(next_slide_i: Int) {
        val req = CommandRequest(CommandId.CommandMoveNextSlide);
        req.move_next_slide_i = next_slide_i;
        this.sendRequest(req);
    }

    fun requestToggleFullscreen() {
        this.sendRequest(CommandRequest(CommandId.CommandToggleFullscreen));
    }

    fun requestExit() {
        this.sendRequest(CommandRequest(CommandId.CommandExit));
    }
}