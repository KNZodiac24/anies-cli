WIN_ARGS = 

ifdef WIN_ARGS
	CLEAN_BUILD = rmdir /s /q "build"
else
	CLEAN_BUILD = rm -rf build
endif

.PHONY: setup build

setup:
	$(CLEAN_BUILD) && cmake -G Ninja -B build -S . -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(WIN_ARGS)

build:
	cmake --build build && strip build/anies-cli
