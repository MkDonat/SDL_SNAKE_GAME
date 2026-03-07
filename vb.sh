# Create folders
mkdir -p build/vita
# Configure
cmake -S. -B build/vita -DCMAKE_TOOLCHAIN_FILE=${VITASDK}/share/vita.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DVIDEO_VITA_PVR=ON -DVIDEO_VITA_PIB=ON
# Build .elf
cmake --build build/vita
# ???
cmake --install build/vita
# Create param.sfo file
vita-mksfoex -s TITLE_ID=SNAKE0001 "Snake" ./build/vita/Release/param.sfo
# Generate .velf
vita-elf-create ./build/vita/Release/exec ./build/vita/Release/snake.velf
# Generate .fself
vita-make-fself ./build/vita/Release/snake.velf ./build/vita/Release/eboot.bin
# Generate .vpk homebrew
vita-pack-vpk -s ./build/vita/Release/param.sfo -b ./build/vita/Release/eboot.bin -a sce_sys=sce_sys ./build/vita/Release/Snake.vpk