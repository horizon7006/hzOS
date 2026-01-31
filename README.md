# hzOS
Stupid little x86 OS. Thats it. (also this os is wip so expect many bugs lol)

![the hzOS desktop.](https://raw.githubusercontent.com/horizon7006/hzOS/refs/heads/main/github_assets/VirtualBoxVM.png)

## Build Instructions


### Install dependencies
```bash
sudo apt update
sudo apt install qemu-system-x86 qemu-utils build-essential nasm grub-pc-bin mtools xorriso
```

### Clone the repository
```bash
git clone https://github.com/horizon7006/hzOS.git
cd hzOS
```

### Build the kernel and ISO
```bash
make -j$(nproc)
```

### Run the OS
```bash
make run-qemu
```