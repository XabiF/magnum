# magnum

*magnum* is a [manim](https://docs.manim.community/en/stable/index.html) extension, providing support for dynamic presentations and real-time-controlled math animations.

It consists on a [Python extension](magnum-py) adapting manim's work in order to generate slideshow/presentation contents, along with a [cross-platform slideshow (video) player](magnum-ui) software which is used to play any generated presentation content on Linux and Windows PCs.

Playback can be controlled using the keyboard, or even from an [Android device](magnum-app) via TCP-connectivity with a simple, lightweight dedicated app.

GGGAAA

## Why?

While manim is a wonderful tool for rendering STEM-related animated content, it is not really conceived in a nice way to be able to use it for real-time slideshows.

[Official support for slideshows](https://www.manim.community/plugin/manim-slides/) exists, but it has two design choices that limit possibilities to some extent: slideshows follow a linear structure (like usual Powerpoint presentations, and so on) and (more importantly) slideshows are meant to be run as Python scripts themselves, which requires everything to be set-up (Python, manim and so on) on the target PC.

Dynamic presentations naturally arise in STRM-related contexts: a sub-branch of slides may be used to explain a certain topic in detail (if the audience requires it), afterwards returning to the main branch and continuing the slideshow.

## How it works

*magnum* offers a cross-platform, dependency-free solution by pre-rendering transitions between slides, so that the generated presentation content essentially consists on a bunch of video files and a description of the slide tree. This format is not dependent on Python/manim anymore, and the custom, portable, cross-platform video player is enough to show the presentation.

## Usage

### Python/manim

Check the [examples](examples).

> TODO: make more examples, or proper docs

### Player

The slideshow player is meant to be opened with the generated slide directory (which contains video fragments and a slide tree JSON) as the first argument. This can easily be done by drag-dropping the directory on the executable itself, or manually from command-line.

- Pressing the *SPACE* key will start playback.

- Pressing *0* or *right arrow* keys will advance to the next main (non-branch) slide.

- Pressing *1*, *2* (and so on) keys will advance to the corresponding branch slide (if they exist).

  > TODO: make the player more interactive, showing slide captions like in the Android app

- Pressing the *left arrow* key will go back to the previous slide.

- Pressing the *ESCAPE* key will close the player.

- Pressing the *F* key will toggle full-screen.

- Pressing the *I* key will hide/show the IP/help text.

You may use the Android app to connect to the player. The connecting process is straightforward from the app UI, as well as nicely controlling the presentation flow.

> TODO: make this better documented/organized

## Credits

- [manim](https://docs.manim.community/en/stable/index.html) and its developers, for creating such an amazing tool for STEM-like animations.

- [ffmpeg](https://www.ffmpeg.org/) for cross-platform video playback support.

- [SDL2](https://www.libsdl.org/) for cross-platform UI/graphics support.
