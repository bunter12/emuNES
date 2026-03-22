# emuNES

Эмулятор Nintendo Entertainment System (NES) на C++17 с графикой/вводом через SDL2 и звуком через SDL2_mixer.

## Текущее состояние

Реализовано:
- CPU 6502 (основной набор официальных инструкций, IRQ/NMI, стек, addressing modes).
- PPU (фон/спрайты, скролл, sprite 0 hit, NMI на VBlank).
- APU (Pulse/Triangle/Noise/DMC, frame counter, микширование, фильтры).
- Контроллеры NES (strobe + последовательное чтение кнопок).
- Cartridge: iNES + базовая поддержка NROM (Mapper 0), mirroring.

Ограничения:
- Практически поддерживается Mapper 0 (NROM).
- `src/tests.cpp` сейчас не входит в сборку и не соответствует текущему API некоторых модулей.
- Рекомендуется всегда передавать ROM через аргумент командной строки.

## Управление

Controller 1:
- `Z` -> `A`
- `X` -> `B`
- `A` -> `SELECT`
- `S` -> `START`
- `Arrow keys` -> D-Pad

## Зависимости

- CMake `>= 3.15`
- C++17 compiler
- SDL2
- SDL2_mixer

`CMakeLists.txt` уже кроссплатформенный: поддерживает и современные CMake targets (`SDL2::SDL2`, `SDL2_mixer::SDL2_mixer`), и fallback на legacy-переменные (`SDL2_LIBRARIES`, `SDL2_MIXER_LIBRARIES`).

## Сборка и запуск (macOS/Linux, без Ninja)

### 1) Установить зависимости

macOS:

```bash
brew install cmake sdl2 sdl2_mixer
```

Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake libsdl2-dev libsdl2-mixer-dev
```

### 2) Сконфигурировать, собрать, запустить

```bash
cmake -S . -B build -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/emuNES ./nestest.nes
```

## Сборка и запуск (Windows)

### Вариант A: MSYS2 + MinGW Makefiles (без Ninja)

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-SDL2 \
  mingw-w64-ucrt-x86_64-SDL2_mixer
```

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/emuNES.exe ./nestest.nes
```

### Вариант B: Visual Studio + vcpkg

```powershell
vcpkg install sdl2 sdl2-mixer
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-vs --config Debug
```

Запуск:

```powershell
./build-vs/Debug/emuNES.exe ./nestest.nes
```

## Где искать бинарник

- Single-config генераторы (Unix Makefiles/MinGW Makefiles): `build/emuNES` или `build/emuNES.exe`
- Multi-config генераторы (Visual Studio): `build-*/Debug/emuNES` или `build-*/Debug/emuNES.exe`

## Полезные файлы

- `src/main.cpp` — SDL-цикл, синхронизация CPU/PPU/APU.
- `src/cpu.cpp`, `src/ppu.cpp`, `src/apu.cpp` — ядро эмулятора.
- `src/bus.cpp` — шина и маршрутизация памяти/прерываний.
- `src/cartridge.cpp` — загрузка iNES и mapper logic.
- `make_apu_test_rom.py` — генерация тестового APU ROM.

## Проверка после запуска

1. Запустите `nestest.nes`.
2. Убедитесь, что появляется окно эмулятора и есть аудио.
3. Проверьте ввод (`Z`, `X`, `A`, `S`, стрелки).
