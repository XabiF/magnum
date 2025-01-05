package com.xabifdez.magnum.app

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.ListView
import androidx.appcompat.app.AppCompatActivity
import java.util.Stack

class MainActivity : AppCompatActivity() {
    lateinit var ctx: NetContext
    lateinit var tree: SlideTree
    var slide_id_stack: Stack<SlideId> = Stack();

    companion object {

        lateinit var Main: MainActivity

        fun init(activity: MainActivity) {
            Main = activity;
        }

    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_test);

        init(this);

        val ip_text = this.findViewById<EditText>(R.id.ip_text);

        val connect_button = this.findViewById<Button>(R.id.connect_button);
        connect_button.setOnClickListener {
            val ip = ip_text.text.toString();
            ctx = NetContext(ip);
            ctx.start();

            connect_button.isEnabled = false;
        }

        val req_full_button = this.findViewById<Button>(R.id.req_full_button);
        req_full_button.setOnClickListener {
            ctx.requestToggleFullscreen();
        }

        val req_exit_button = this.findViewById<Button>(R.id.req_exit_button);
        req_exit_button.setOnClickListener {
            ctx.requestExit();
        }
    }

    fun loadStatus(tree: SlideTree, slide_id_stack: Stack<SlideId>) {
        this.tree = tree;
        this.slide_id_stack = slide_id_stack;
        runOnUiThread {
            this.reloadList();
        }
    }

    fun reloadList() {
        val cur_slide = this.tree.getSlideById(this.slide_id_stack.peek())!!;
        val test_list = this.findViewById<ListView>(R.id.test_list);
        test_list.adapter = SlideActionButtonListAdapter(this, cur_slide);
    }

    fun notifyDisconnected() {
        runOnUiThread {
            val connect_button = this.findViewById<Button>(R.id.connect_button);
            connect_button.isEnabled = true;

            val test_list = this.findViewById<ListView>(R.id.test_list);
            test_list.adapter = null;
        }
    }

    fun prevSlide() {
        this.ctx.requestMovePreviousSlide();
        this.slide_id_stack.pop();
        this.reloadList();
    }

    fun nextSlide(next_slide_id: SlideId, next_slide_i: Int) {
        this.ctx.requestMoveNextSlide(next_slide_i);
        this.slide_id_stack.push(next_slide_id);
        this.reloadList();
    }
}