.PHONY: init all clean

all :
	@echo Building...
	@mkdir build/ 2>/dev/null; exit 0
	@cd build; cmake ..
	@cd build; make

init :
	@echo Initializing git submodules...
	@git submodule update --init
	@cd pico-sdk; git submodule update --init

clean :
	@echo Removing build output for clean build...
	@rm -rf build/
