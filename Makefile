WIN_ARGS = 

.PHONY: setup build

setup:
	cmake -G Ninja -B build -S . -DCMAKE_MAKE_PROGRAM=ninja -DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON $(WIN_ARGS)

build:
	cmake --build build
