# Create folders
mkdir -p build/linux
# Configure Make
cmake -S . -B build/linux/
# Build
cmake --build build/linux/
# Run
cd build/linux/ && ./exec