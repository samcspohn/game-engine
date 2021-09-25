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
	pacman -S --noconfirm cmake
	pacman -S --noconfirm openal
	pacman -S --noconfirm freealut
	pacman -S --noconfirm boost
	pacman -S --noconfirm libxxf86dga
	# pacman -S --noconfirm clang

getDebDeps:
	apt-get install libglfw3-dev -y
	apt install libsoil-dev -y
	apt install libassimp-dev  -y
	apt install libglm-dev -y
	# apt install libgdb -y
	apt install cmake -y
	apt install libopenal-dev -y
	apt install libalut-dev -y
	apt install libxi-dev -y
	apt install libxxf86vm-dev -y
	apt install g++ -y
	apt install libglew-dev -y
	apt install libtbb-dev -y
	apt install libxxf86dga-dev -y
	apt install libasio-dev -y
	apt install libyaml-cpp-dev -y