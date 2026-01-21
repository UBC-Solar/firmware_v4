.PHONY: all mdi tel drd clean help debug release utest

BUILD_DIR := build
MODE := Debug  # default

help:
	@echo "Solar v4 Firmware Build"
	@echo "Targets:"
	@echo "  make all debug      - Build all modules (Debug)"
	@echo "  make all release    - Build all modules (Release)"
	@echo "  make mdi debug      - Build MDI in Debug"
	@echo "  make mdi release    - Build MDI in Release"
	@echo "  make utest          - Run all unit tests"
	@echo "  make utest mdi      - Run unit tests for MDI only"
	@echo "  make clean          - Remove all build directories"

ifeq (,$(filter debug release,$(MAKECMDGOALS)))
    MODE := Debug
else
    LAST_MODE := $(lastword $(filter debug release,$(MAKECMDGOALS)))
    ifeq ($(LAST_MODE),debug)
        MODE := Debug
    else ifeq ($(LAST_MODE),release)
        MODE := Release
    endif
endif


FILTERED_GOALS := $(filter-out debug release,$(MAKECMDGOALS))

all: mdi tel drd

mdi:
	@echo "=== Building MDI ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/mdi \
		-B firmware/components/mdi/$(BUILD_DIR)
	cmake --build firmware/components/mdi/$(BUILD_DIR)

tel:
	@echo "=== Building TEL ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/tel \
		-B firmware/components/tel/$(BUILD_DIR)
	cmake --build firmware/components/tel/$(BUILD_DIR)

drd:
	@echo "=== Building DRD ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/drd \
		-B firmware/components/drd/$(BUILD_DIR)
	cmake --build firmware/components/drd/$(BUILD_DIR)

utest:
	@echo "=== Running all unit tests ==="
	cd firmware/components/mdi/ && ./ceedling test:all


clean:
	@echo "Cleaning all build folders..."
	rm -rf firmware/components/mdi/$(BUILD_DIR)
	rm -rf firmware/components/tel/$(BUILD_DIR)
	rm -rf firmware/components/drd/$(BUILD_DIR)
	cd firmware/components/mdi/ && ./ceedling clean
	@echo "Clean complete."