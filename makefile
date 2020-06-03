build:
	clang++ -w -fpermissive -std=c++17 -c main.cpp 
	clang++ -std=c++17 main.o -o main.exec -lGL -lGLU -lGLEW -lglfw -lX11 -lXxf86vm -lXrandr -lpthread -lXi -lSOIL -lassimp

run:
	./main.exec
getDeps:
	pacman -S --noconfirm glfw-x11
	pacman -S --noconfirm soil
	pacman -S --noconfirm assimp
	pacman -S --noconfirm glm
	pacman -S --noconfirm gdb
	pacman -S --noconfirm clang
