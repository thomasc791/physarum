CPP = clang++
C = clang

ANT_FILES = ant-colony

LIBFILES = glad

FLAGS = -std=c++20 \
				-lglfw \
				-L./lib/\
				-lglad \
				-lGL \
				-lX11 \
				-lpthread \
				-lXrandr \
				-lXi \
				-ldl \
				-Wunused-command-line-argument \
				-march=native \
				-O3

SRC_DIR = src
BUILD_DIR = build
OPENGL_DIR = opengl

BD_O = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(FILES)))
ANT_O = $(addprefix $(BUILD_DIR)/, $(addsuffix .o, $(ANT_FILES)))
ANT_CPP = $(addprefix $(SRC_DIR)/, $(addsuffix .cpp, $(ANT_FILES)))

all: ant

$(BUILD_DIR)/%.o: %.cpp
	$(CPP) -g -c $^ -std=c++20 -o $@

$(ANT_O): $(ANT_CPP)
	mkdir -p build
	$(CPP) -g -c $^ -std=c++20 -o $@

ant: $(ANT_O) $(BD_O)
	$(CPP) $^ $(IMGUI_O)  $(FLAGS) -o $(BUILD_DIR)/$@



clean:
	rm -rf ./$(BUILD_DIR)
	rm -f core.*
	rm -rf ./$(LIB_DIR)
