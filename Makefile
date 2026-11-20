ifeq ($(OS),Windows_NT)
	CLEAN_BUILD = (if exist build rmdir /s /q build)
	STRIP_EXE = strip --strip-unneeded --strip-all --strip-dwo --strip-debug --strip-section-headers build/anies-cli.exe
	WIN_ARGS = -DCMAKE_CXX_COMPILER=g++ -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DVCPKG_HOST_TRIPLET=x64-mingw-static
else
	CLEAN_BUILD = rm -rf build
	STRIP_EXE = strip --strip-unneeded --strip-all --strip-dwo --strip-debug --strip-section-headers build/anies-cli
endif

.PHONY: setup build setup-android build-android

setup:
	$(CLEAN_BUILD)
	cmake -G Ninja -B build -S . \
		-DCMAKE_MAKE_PROGRAM=ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
		$(WIN_ARGS)

build:
	cmake --build build && $(STRIP_EXE)

setup-android:
	rm -rf build-android
	cmake -G Ninja -B build-android -S . \
		-DCMAKE_MAKE_PROGRAM=ninja \
		-DCMAKE_TOOLCHAIN_FILE=$(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake \
		-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$(ANDROID_NDK_HOME)/build/cmake/android.toolchain.cmake \
		-DVCPKG_TARGET_TRIPLET=arm64-android \
		-DVCPKG_HOST_TRIPLET=$(HOST_TRIPLET) \
		-DANDROID_ABI=arm64-v8a \
		-DANDROID_PLATFORM=android-24 \
		-DCMAKE_CXX_SCAN_FOR_MODULES=OFF \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=ON

build-android:
	cmake --build build-android
	$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/bin/llvm-strip --strip-unneeded --strip-all --strip-debug build-android/anies-cli
