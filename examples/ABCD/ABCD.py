from manim import *
from magnum import Slideshow

# Slideshows work similarly to Scenes, just that they must inherit from "Slideshow" (instead of "Scene") and define a "show" method (instead of "construct")

# For comparison with regular manim, check the other script in this example.

class ABCDSlideshow(Slideshow):
    def show(self):
        # Slides are declared using a "with" statement, with an optional (but encouraged) argument of a brief caption/description of the slide (for later identification)
        # Slides esentially work as "separators" of different manim operations

        with self.slide("Show A"):
            text_A = Text("A")
            text_A.shift(DOWN+2*LEFT)
            self.play(FadeIn(text_A, shift=LEFT))
        
        with self.slide("Show B"):
            text_B = Text("B")
            text_B.shift(2*DOWN)
            self.play(FadeIn(text_B, shift=DOWN))

        with self.slide("Show C"):
            text_C = Text("C")
            text_C.shift(DOWN+2*RIGHT)
            self.play(FadeIn(text_C, shift=RIGHT))

        # Branches work similarly, but with the "with" argument being a boolean that indicates whether the branch is taken (this is due to the internals of magnum)

        # The code of different branches (given that a particular branch was followed) will not be executed.
        # Note that magnum will emulate said paths when pre-rendering and generating slide content, but the same idea applies.
        
        # Branches are alternative paths which will eventually end (and you will need to go backwards to the branching point, which is the last, C-letter slide here)
        # Until here, the slideshow was linear (show A, then B, then C) but here there are four possible paths:
        # * Continue over the main path, showing D
        # * One branch, which details the letter A
        # * Another branch, which details the letter B
        # * Yet another branch, which details the letter C

        # On playback, these are presented as four slides which might follow next, at the user's choice.

        with self.branch() as b:
            if b:
                with self.slide("Detail A"):
                    box_A = SurroundingRectangle(text_A, buff=SMALL_BUFF)
                    info_A = Text("Letter A", font="consolas", font_size=18)
                    info_A.next_to(box_A.get_corner(UR), UR, buff=0.1)
                    self.play(Create(box_A), Write(info_A))
            
        with self.branch() as b:
            if b:
                with self.slide("Detail B"):
                    box_B = SurroundingRectangle(text_B, buff=SMALL_BUFF)
                    info_B = Text("Letter B", font="consolas", font_size=18)
                    info_B.next_to(box_B.get_corner(UR), UR, buff=0.1)
                    self.play(Create(box_B), Write(info_B))
            
        with self.branch() as b:
            if b:
                with self.slide("Detail C"):
                    box_C = SurroundingRectangle(text_C, buff=SMALL_BUFF)
                    info_C = Text("Letter C", font="consolas", font_size=18)
                    info_C.next_to(box_C.get_corner(UR), UR, buff=0.1)
                    self.play(Create(box_C), Write(info_C))

        with self.slide("Show D"):
            text_D = Text("D")
            self.play(FadeIn(text_D, shift=UP))

###############

if __name__ == "__main__":
    config.quality = "high_quality"

    # Render and export the contents (video fragments and slide tree data) inside "abcd_slide" dir
    # This directory will be the actual generated slide, the argument to be passed to the player

    ABCDSlideshow().gen("abcd_slide")
