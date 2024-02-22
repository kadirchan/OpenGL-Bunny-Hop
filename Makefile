hw3:
	g++ main.cpp -g -O3 -o main \
        `pkg-config --cflags --libs freetype2` \
        -lglfw -lGLU -lGL -lGLEW 
