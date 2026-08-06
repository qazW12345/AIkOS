# env.sh — platform-aware toolchain paths (ADR-011).
# Windows git-bash (MINGW): absolute install paths. Linux (CI): package names.
# Source this from build.sh and test.sh.

case "$(uname -s)" in
    MINGW*|MSYS*)
        NASM="/c/Users/marce/AppData/Local/bin/NASM/nasm.exe"
        CLANG="/c/Program Files/LLVM/bin/clang.exe"
        LLD="/c/Program Files/LLVM/bin/ld.lld.exe"
        OBJCOPY="/c/Program Files/LLVM/bin/llvm-objcopy.exe"
        QEMU="C:/Program Files/qemu/qemu-system-x86_64.exe"
        PYTHON="python"          # never python3 on this machine (see Guides)
        ;;
    Linux)
        NASM="nasm"
        CLANG="clang"
        LLD="ld.lld"
        OBJCOPY="llvm-objcopy"
        QEMU="qemu-system-x86_64"
        PYTHON="python3"
        ;;
    *)
        echo "env.sh: unsupported platform $(uname -s)" >&2
        exit 1
        ;;
esac
