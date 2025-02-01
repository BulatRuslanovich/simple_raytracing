# Минимальная версия CMake
CMAKE_MIN_VERSION := 3.10
# Имя проекта
PROJECT_NAME := simple_raytracing
# Директория сборки
BUILD_DIR := build
# Определение типа видеокарты
GPU_TYPE := $(shell (lspci | grep -q "NVIDIA" && echo "nvidia") || (lspci | grep -q "AMD/ATI" && echo "amd") || echo "none")

.PHONY: all build clean run

all: build

build:
	@echo "Checking CMake version..."
	@cmake_ver=$$(cmake --version | awk '/cmake version/ {print $$3}'); \
	if [ "$$(printf '%s\n' "$(CMAKE_MIN_VERSION)" "$$cmake_ver" | sort -V | head -n1)" != "$(CMAKE_MIN_VERSION)" ]; then \
		echo "CMake $${cmake_ver} is older than required $(CMAKE_MIN_VERSION)"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..
	@$(MAKE) -C $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Build files removed"

run: build
	@echo "Detected GPU type: $(GPU_TYPE)"
	@case "$(GPU_TYPE)" in \
		("nvidia") \
			echo "Using NVIDIA GPU"; \
			__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./$(BUILD_DIR)/$(PROJECT_NAME); \
			;; \
		("amd") \
			echo "Using AMD GPU"; \
			DRI_PRIME=1 ./$(BUILD_DIR)/$(PROJECT_NAME); \
			;; \
		(*) \
			echo "Using default graphics"; \
			./$(BUILD_DIR)/$(PROJECT_NAME); \
			;; \
	esac
run-nvidia: build
	@__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./$(BUILD_DIR)/$(PROJECT_NAME)

run-amd: build
	@DRI_PRIME=1 ./$(BUILD_DIR)/$(PROJECT_NAME)
run-process:
	./$(BUILD_DIR)/$(PROJECT_NAME)

help:
	@echo "Available targets:"
	@echo "  build    - Compile the project (default)"
	@echo "  clean    - Remove build files"
	@echo "  run      - Build and run the program"
	@echo "  help     - Show this help message"