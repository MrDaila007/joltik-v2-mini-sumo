# Joltik V2 Mini Sumo - Zephyr RTOS Makefile
#
# Usage:
#   make              Build firmware
#   make flash        Copy UF2 to RP2040 (hold BOOT + plug USB first)
#   make monitor      Open serial console (USB CDC ACM)
#   make clean        Remove build directory
#   make menuconfig   Interactive Kconfig editor
#   make display      Build with OLED display enabled
#   make rom_report   Show flash usage breakdown
#   make ram_report   Show RAM usage breakdown
#   make size         Show firmware size summary
#   make test         Build and run unit tests (native_sim)

BOARD         ?= rp2040_zero
BUILD_DIR     ?= build
ZEPHYR_BASE   ?= $(HOME)/SDK/zephyrproject-v4.3/zephyr
ZEPHYR_SDK    ?= $(HOME)/SDK/zephyr-sdk-0.17.0
VENV          ?= $(HOME)/SDK/zephyrproject-v4.3/.venv
UF2_MOUNT     ?= /media/$(USER)/RPI-RP2
SERIAL_PORT   ?= /dev/ttyACM0
SERIAL_BAUD   ?= 115200

# Activate venv + set env for all targets
export ZEPHYR_BASE
export ZEPHYR_SDK_INSTALL_DIR := $(ZEPHYR_SDK)
WEST := $(VENV)/bin/west

TEST_BUILD_DIR ?= build_tests

.PHONY: all build flash monitor clean pristine menuconfig guiconfig \
        display rom_report ram_report size debug test

all: build

build:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR)

# Build with OLED display support
display:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) -- -DCONFIG_JOLTIK_DISPLAY=y

# Flash UF2 to RP2040 in BOOTSEL mode
flash: build
	@if [ -d "$(UF2_MOUNT)" ]; then \
		cp $(BUILD_DIR)/zephyr/zephyr.uf2 $(UF2_MOUNT)/ && \
		echo "Flashed to $(UF2_MOUNT)"; \
	else \
		echo "ERROR: $(UF2_MOUNT) not found."; \
		echo "Hold BOOT button, plug USB, then retry."; \
		exit 1; \
	fi

# Open serial monitor (Ctrl+A then K to quit)
monitor:
	@echo "Connecting to $(SERIAL_PORT) at $(SERIAL_BAUD)..."
	@echo "Exit: Ctrl+A then K (screen) or Ctrl+A then X (minicom)"
	@if command -v minicom >/dev/null 2>&1; then \
		minicom -D $(SERIAL_PORT) -b $(SERIAL_BAUD); \
	elif command -v screen >/dev/null 2>&1; then \
		screen $(SERIAL_PORT) $(SERIAL_BAUD); \
	else \
		echo "Install minicom or screen: sudo apt install minicom"; \
	fi

# Flash and immediately open monitor
flash-monitor: flash
	@sleep 2
	@$(MAKE) monitor

# Interactive Kconfig
menuconfig:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) -t menuconfig

guiconfig:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) -t guiconfig

# Reports
rom_report:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) -t rom_report

ram_report:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) -t ram_report

size: build
	@arm-zephyr-eabi-size $(BUILD_DIR)/zephyr/zephyr.elf 2>/dev/null || \
		$(ZEPHYR_SDK)/arm-zephyr-eabi/bin/arm-zephyr-eabi-size $(BUILD_DIR)/zephyr/zephyr.elf
	@echo ""
	@ls -lh $(BUILD_DIR)/zephyr/zephyr.uf2 | awk '{print "UF2: " $$5}'

# Unit tests (native_sim)
test:
	$(WEST) build -b native_sim tests --build-dir $(TEST_BUILD_DIR)
	$(TEST_BUILD_DIR)/zephyr/zephyr.exe

# Clean
clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)

# Full pristine rebuild
pristine:
	$(WEST) build -b $(BOARD) . --build-dir $(BUILD_DIR) --pristine always

# Debug: show resolved DT and generated configs
debug:
	@echo "=== Board: $(BOARD) ==="
	@echo "=== Zephyr: $(ZEPHYR_BASE) ==="
	@echo "=== SDK: $(ZEPHYR_SDK) ==="
	@echo ""
	@echo "--- Devicetree (generated) ---"
	@cat $(BUILD_DIR)/zephyr/zephyr.dts 2>/dev/null | head -50 || echo "(build first)"
	@echo ""
	@echo "--- Autoconf (Kconfig) ---"
	@grep -E "CONFIG_JOLTIK|CONFIG_PWM|CONFIG_ADC|CONFIG_SHELL|CONFIG_SETTINGS|CONFIG_LED_STRIP" \
		$(BUILD_DIR)/zephyr/.config 2>/dev/null || echo "(build first)"
