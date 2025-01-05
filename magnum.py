from manim import *
import os
import json

# Most of the slide/branch mechanism below was designed to make the final user manim-rendering function to be as simple as possible (to make as much as possible automated)

class SlideWrapper:
    def __init__(self, obj):
        self.obj = obj

    def __enter__(self):
        # Slide pushing was already done in the slideshow object
        pass

    def __exit__(self, exc_type, exc_value, traceback):
        self.obj.pop_slide()

class BranchWrapper:
    def __init__(self, obj):
        self.obj = obj

    def __enter__(self):
        return self.obj.push_branch()

    def __exit__(self, exc_type, exc_value, traceback):
        self.obj.pop_branch()

class SlideIdManager:
    def __init__(self):
        self.ids = []
        self.branch_ys = [0]
        self.cur_id = None
        self.branch_id_stack = []
        self.cur_y_stack = []
        self.branch_path_stack = []

    def push_branch(self):
        next_y = max(self.branch_ys) + 1
        push_id = (self.cur_id[0], next_y)
        self.branch_id_stack.append(self.cur_id)
        # self.ids.append(push_id)
        self.branch_ys.append(next_y)
        self.cur_id = push_id

        path_copy = self.top_path()[:]
        self.branch_path_stack.append(path_copy)

    def top_path(self):
        return self.branch_path_stack[len(self.branch_path_stack) - 1]

    def all_paths(self):
        return self.branch_path_stack

    def get_id(self):
        return self.cur_id
    
    def all_ids(self):
        return self.ids
    
    def pop_branch(self):
        old_id = self.branch_id_stack.pop()
        self.cur_id = old_id

        self.branch_path_stack.insert(0, self.branch_path_stack.pop())

    def push_slide(self):
        if self.cur_id == None:
            self.cur_id = (0, 0)
            self.ids.append(self.cur_id)
            path = [self.cur_id]
            self.branch_path_stack.append(path)
            return
        
        new_id = (self.cur_id[0] + 1, self.cur_id[1])
        self.ids.append(new_id)
        self.top_path().append(new_id)
        self.cur_id = new_id

def id_eq(id_1, id_2):
    return (id_1[0] == id_2[0]) and (id_1[1] == id_2[1])

def fmt_id(id):
    return f"{id[0]}-{id[1]}"

def default_caption(id):
    return f"Slide {id[0] + 1}-{id[1] + 1}"

class ConstructHalt(Exception):
    pass

class Slideshow(Scene):
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.target_id = None
        self.target_path = None
        self.slides = []
        self.reset()

    def reset(self):
        self.id_man = SlideIdManager()

    def show(self):
        pass

    def construct(self):
        try:
            self.show()
            self.target_id = None
        except ConstructHalt:
            pass

    def push_slide(self, caption):
        self.id_man.push_slide()

        if self.target_path != None:
            new_id = self.id_man.get_id()
            if new_id not in self.target_path:
                # We went away from our path: halt rendering
                # Can this even happen?
                raise ConstructHalt()

        # Successfully created a new slide
        cur_id = self.id_man.get_id()
        actual_caption = caption if caption != None else default_caption(cur_id)
        self.slides.append({
            "id": cur_id,
            "caption": actual_caption
        })

    def pop_slide(self):
        if (self.target_id != None) and id_eq(self.id_man.get_id(), self.target_id):
            # Got to our desired target slide, halt rendering
            raise ConstructHalt()
        
        # Done with current slide

    def push_branch(self):
        self.id_man.push_branch()

        if self.target_path != None:
            # Get the actual "next-branch-slide" ID
            branch_pseudo_id = self.id_man.get_id()
            new_id = (branch_pseudo_id[0] + 1, branch_pseudo_id[1])
            if new_id not in self.target_path:
                # We do not want to go through this branch, notify user to not run slides
                return False
        
        # Go through the branch slides
        return True

    def pop_branch(self):
        # Exit current branch
        self.id_man.pop_branch()

    ###############################################################

    def slide(self, caption=None):
        self.push_slide(caption=caption)
        return SlideWrapper(self)
    
    def branch(self):
        return BranchWrapper(self)
    
    def play(self, *args, **kwargs):
        if self.target_id == None:
            # We're doing a dummy rendering aka no rendering at all, thus save time/resources and do nothing :P
            return

        # We only care about the animations of the final slide
        # Thus, only "play" those, and just "add" the other objects unanimated
        # (This probably does not play well with more advanced/3D/etc manim utilities)
        if id_eq(self.id_man.get_id(), self.target_id):
            super().play(*args, **kwargs)
        else:
            self.add(*[anim.mobject for anim in args])

    ###############################################################

    def do_render(self):
        # Reset rendering status, we want these to be completely independent renders
        config.save_last_frame = False
        config.write_to_movie = True
        self.reset()
        super().__init__()

        self.render()

    def gen(self, out_path):
        end_path = out_path
        if not os.path.isabs(end_path):
            current_dir = os.path.dirname(os.path.abspath(__file__))
            end_path = os.path.join(current_dir, end_path)

        if not os.path.isdir(end_path):
            os.mkdir(end_path)

        # Do a full dummy render until the end, just so we can get/generate all slide IDs beforehand
        # Since nothing is rendered, the output will be a blank PNG image
        self.target_id = None
        out_dummy_path = os.path.join(end_path, "dummy")
        config.output_file = out_dummy_path
        self.do_render()
        os.remove(out_dummy_path + ".png")

        all_slide_ids = self.id_man.all_ids()
        all_paths = self.id_man.all_paths()

        slide_tree = {}

        # Perform the rendering up to each slide: this way, the result are individual fragments with the animations of each slide and the already-present objects of previous slides
        # Since each slide can only be reached through a single path, there are no conflicts with the previous slides' objects
        for id in all_slide_ids:
            self.target_id = id
            for path in all_paths:
                # The first ocurrence is fine, duplicates will occur with slides common to various branch paths (thus before the branches happen)
                if self.target_id in path:
                    self.target_path = path
                    break

            out_scene_temp_name = f"{id[0]}-{id[1]}"
            out_scene_path = os.path.join(end_path, out_scene_temp_name)
            config.output_file = out_scene_path

            self.do_render()

            slide_caption = None
            for slide in self.slides:
                if id_eq(slide["id"], id):
                    slide_caption = slide["caption"]
            if slide_caption == None:
                raise RuntimeError("Should not happen")

            slide_entry = {
                "caption": slide_caption
            }

            next_ids = set()

            for path in all_paths:
                if id in path:
                    id_path_idx = path.index(id)
                    if (id_path_idx + 1) < len(path):
                        # The slide has a next slide
                        next_id = path[id_path_idx + 1]
                        next_ids.add(fmt_id(next_id))

            slide_entry["next_slide_ids"] = list(next_ids)

            slide_tree[fmt_id(id)] = slide_entry

        out_tree_json = os.path.join(end_path, "tree.json")
        with open(out_tree_json, "w") as json_file:
            json.dump(slide_tree, json_file, indent=4)
