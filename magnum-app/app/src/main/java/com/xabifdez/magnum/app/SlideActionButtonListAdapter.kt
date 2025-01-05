package com.xabifdez.magnum.app

import android.content.Context
import android.view.View
import android.view.ViewGroup
import android.widget.ArrayAdapter
import android.widget.Button

class SlideActionButtonListAdapter(context: Context, cur_slide: SlideTreeEntry) : ArrayAdapter<SlideId>(context, 0, cur_slide.next_slide_ids.toMutableList().apply { add(SlideId.NullId) }) {
    override fun getView(position: Int, convertView: View?, parent: ViewGroup): View {
        if(position == 0) {
            val button_view = convertView ?: Button(context).apply {
                setText("...");
                setOnClickListener {
                    MainActivity.Main.prevSlide();
                };
            };

            return button_view;
        }

        val next_slide_id = getItem(position - 1)!!;
        val next_slide = MainActivity.Main.tree.getSlideById(next_slide_id)!!;

        val button_view = convertView ?: Button(context).apply {
            setText(next_slide.name);
            setOnClickListener {
                MainActivity.Main.nextSlide(next_slide_id, position - 1);
            };
        };

        return button_view;
    }
}