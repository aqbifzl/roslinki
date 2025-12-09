# FQBN for your board
FQBN ?= rp2040:rp2040:rpipico

PROJECT = roslinki

# Serial port
PORT ?= /dev/ttyACM0

# Serial baud rate
BAUD ?= 115200

# Build dirs
BUILD_DEBUG := build/debug
BUILD_RELEASE := build/release

# Arduino CLI executable
ARDUINO_CLI := arduino-cli

# Default build release
all: release

debug:
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD_DEBUG) \
		--optimize-for-debug \
		--build-property build.preferred_out_format=elf \
		--jobs 0 \
		--verbose

release:
	$(ARDUINO_CLI) compile \
		--fqbn $(FQBN) \
		--build-path $(BUILD_RELEASE) \
		--jobs 0 \
	    --build-property compiler.optimization.levels.release=3 \
	    --build-property build.preferred_out_format=elf \
	    --build-property compiler.debug_level=none \
		--verbose

upload-debug: debug
	openocd \
	  -f interface/cmsis-dap.cfg \
	  -f target/rp2040.cfg \
	  -c "adapter speed 5000" \
	  -c "program $(BUILD_DEBUG)/$(PROJECT).ino.elf verify reset exit"

upload-release: release
	openocd \
	  -f interface/cmsis-dap.cfg \
	  -f target/rp2040.cfg \
	  -c "adapter speed 5000" \
	  -c "program $(BUILD_RELEASE)/$(PROJECT).ino.elf verify reset exit"

monitor:
	$(ARDUINO_CLI) monitor -p $(PORT) --config baudrate=$(BAUD)

clean:
	rm -rf build

.PHONY: all debug release upload-debug upload-release clean monitor
