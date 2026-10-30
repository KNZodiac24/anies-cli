WIN_ARGS = 

ifeq ($(OS),Windows_NT)
	CLEAN_BUILD = (if exist build rmdir /s /q build)
	STRIP_EXE = strip build/anies-cli.exe
else
	CLEAN_BUILD = rm -rf build
	STRIP_EXE = strip build/anies-cli
endif

.PHONY: setup build

setup:
	$(CLEAN_BUILD) && cmake -G Ninja -B build -S . -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(WIN_ARGS)

build:
	cmake --build build && $(STRIP_EXE)
