cmd_/home/sniper/everyfile/kernelmod/vfs_kprobe.ko := ld -r -m elf_x86_64  -z max-page-size=0x200000  --build-id  -T ./scripts/module-common.lds -o /home/sniper/everyfile/kernelmod/vfs_kprobe.ko /home/sniper/everyfile/kernelmod/vfs_kprobe.o /home/sniper/everyfile/kernelmod/vfs_kprobe.mod.o;  true