from manim import *

# This is ther 

class ABCDScene(Scene):
    def construct(self):
        text_A = Text("A")
        text_A.shift(DOWN+2*LEFT)
        self.play(FadeIn(text_A, shift=LEFT))

        text_B = Text("B")
        text_B.shift(2*DOWN)
        self.play(FadeIn(text_B, shift=DOWN))

        text_C = Text("C")
        text_C.shift(DOWN+2*RIGHT)
        self.play(FadeIn(text_C, shift=RIGHT))

        box_A = SurroundingRectangle(text_A, buff=SMALL_BUFF)
        info_A = Text("Letter A", font="consolas", font_size=18)
        info_A.next_to(box_A.get_corner(UR), UR, buff=0.1)
        self.play(Create(box_A), Write(info_A))

        box_B = SurroundingRectangle(text_B, buff=SMALL_BUFF)
        info_B = Text("Letter B", font="consolas", font_size=18)
        info_B.next_to(box_B.get_corner(UR), UR, buff=0.1)
        self.play(Create(box_B), Write(info_B))

        box_C = SurroundingRectangle(text_C, buff=SMALL_BUFF)
        info_C = Text("Letter C", font="cambria math", font_size=18)
        info_C.next_to(box_C.get_corner(UR), UR, buff=0.1)
        self.play(Create(box_C), Write(info_C))

        text_D = Text("D")
        self.play(FadeIn(text_D, shift=UP))

        self.wait()
