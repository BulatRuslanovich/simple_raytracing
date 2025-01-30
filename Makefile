# Минимальная версия CMake
CMAKE_MIN_VERSION := 3.10
# Имя проекта
PROJECT_NAME := simple_raytracing
# Директория сборки
BUILD_DIR := build

.PHONY: all build clean run

all: build

build:
	@echo "Checking CMake version..."
	@cmake --version | grep -q $(CMAKE_MIN_VERSION) || { echo "CMake $(CMAKE_MIN_VERSION)+ required";  }
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..
	@$(MAKE) -C $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build files removed"

run: build
	@./$(BUILD_DIR)/$(PROJECT_NAME)

help:
	@echo "Available targets:"
	@echo "  build    - Compile the project (default)"
	@echo "  clean    - Remove build files"
	@echo "  run      - Build and run the program"
	@echo "  help     - Show this help message"