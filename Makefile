# Top level makefile used to make building from the command line simple.

.PHONY: all mdi tel drd hvc mst str clean help debug release utest

debug release Debug Release:
	@:

BUILD_DIR := build
MODE := Debug  # default

# Each selected board must have a Ceedling project.yml.
UTEST_BOARDS ?= drd mdi tel str

help:
	@echo "Solar v4 Firmware Build"
	@echo "Targets:"
	@echo "  make all debug      - Build all modules (Debug)"
	@echo "  make all release    - Build all modules (Release)"
	@echo "  make mdi debug      - Build MDI in Debug"
	@echo "  make mdi release    - Build MDI in Release"
	@echo "  make utest          - Run host SIL tests (DRD, MDI, TEL, STR)"
	@echo "  make utest UTEST_BOARDS=drd - Select configured SIL boards"
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

all: mdi tel drd hvc mst str

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

hvc:
	@echo "=== Building HVC ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/hvc \
		-B firmware/components/hvc/$(BUILD_DIR)
	cmake --build firmware/components/hvc/$(BUILD_DIR)

mst:
	@echo "=== Building Masterboard ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/mst \
		-B firmware/components/mst/$(BUILD_DIR)
	cmake --build firmware/components/mst/$(BUILD_DIR)

str:
	@echo "=== Building Masterboard ($(MODE)) ==="
	cmake --preset $(MODE) \
		-S firmware/components/str \
		-B firmware/components/str/$(BUILD_DIR)
	cmake --build firmware/components/str/$(BUILD_DIR)

utest:
	@test -n "$(strip $(UTEST_BOARDS))" || { echo "Select at least one SIL board." >&2; exit 1; }
	@set -e; \
	if [ "$$(uname -s)" = Darwin ] && [ -z "$$SDKROOT" ]; then \
		SDKROOT="$$(/usr/bin/xcrun --sdk macosx --show-sdk-path)"; export SDKROOT; \
	fi; \
	for board in $(UTEST_BOARDS); do \
		echo "Running $$board SIL tests"; \
		(cd "firmware/components/$$board" && \
			bundle exec ceedling clobber test:all && \
			ruby -rjson -e 's = JSON.parse(File.read(ARGV.fetch(0))).fetch("Summary"); abort "No passing tests" unless s.fetch("passed") > 0' \
			build_sil/artifacts/test/tests_report.json); \
	done


clean:
	@echo "Cleaning all build folders..."
	rm -rf firmware/components/mdi/$(BUILD_DIR)
	rm -rf firmware/components/tel/$(BUILD_DIR)
	rm -rf firmware/components/drd/$(BUILD_DIR)
	rm -rf firmware/components/hvc/$(BUILD_DIR)
	rm -rf firmware/components/mst/$(BUILD_DIR)
	rm -rf firmware/components/str/$(BUILD_DIR)
	@echo "Clean complete."
