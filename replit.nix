{ pkgs }: {
    deps = [
        # ld, as
        pkgs.binutils
        # clang compiler
        pkgs.clang
        # misc deps for the build process
        pkgs.python3
        pkgs.cdrkit
        # emulator to run the kernel
        pkgs.qemu
    ];
}