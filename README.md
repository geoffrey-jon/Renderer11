# 3D Game Programming with DirectX 11

This is a repository of demos and exercises from "Introduction to 3D Game Progamming with DirectX 11" by Frank D. Luna. Each project in the solution is based on a demo outlined in the book, so a lot of this code is not my own. Most of the credit goes to Luna.

The major differences between the book code and my code are:
1. I have updated the demos to run on Win10 with VS2015 by replacing deprecated libraries and functions with their modern equivalents. Based on Luna's guide, found here: http://www.d3dcoder.net/Data/Book4/d3d11Win10.htm
2. I have removed all references to the Effects library, choosing instead to compile shaders directly, build constant buffers manually, etc.
3. I have done my best to refactor some of Luna's code into re-usable functions and classes in order to make this repository of demos more closely resemble a rendering engine. Classes prefaced with 'G' are written by me, such as 'GObject'. There is a lot more I can and will do to improve the architecture of this engine moving forward. 
