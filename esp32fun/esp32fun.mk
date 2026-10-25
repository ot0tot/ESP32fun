# Optional GNU Make frontend. Python also works without Make.
ifndef ESP32FUN
ESP32FUN := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)
endif
ifeq ($(OS),Windows_NT)
PYTHON ?= python
else
PYTHON ?= python3
endif
PROJECT ?= $(CURDIR)
PORT ?=
BAUD ?= 460800
DEFINES ?=
FLASH_SIZE ?= 4MB
CHIP ?= esp32c3
FUN_CMD = "$(PYTHON)" "$(ESP32FUN)/tools/esp32fun.py"
FUN_ARGS = --project "$(PROJECT)" --chip $(CHIP) --flash-size $(FLASH_SIZE) $(foreach D,$(DEFINES),-D$(D))

.PHONY: all build flash ram monitor clean size doctor targets
all: build
build clean size:
	$(FUN_CMD) $@ $(FUN_ARGS)
flash:
	$(FUN_CMD) flash $(FUN_ARGS) --port "$(PORT)" --baud $(BAUD)
ram:
	$(FUN_CMD) ram $(FUN_ARGS) --port "$(PORT)"
monitor:
	$(FUN_CMD) monitor --project "$(PROJECT)" --chip $(CHIP) --port "$(PORT)"
doctor:
	$(FUN_CMD) doctor --chip $(CHIP)
targets:
	$(FUN_CMD) targets
