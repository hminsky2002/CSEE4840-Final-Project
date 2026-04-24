# Kernel build & deploy — DE1-SoC (Cyclone V, ARMv7-A)

Cross-build of `linux-socfpga` 4.19 with USB-MIDI / ALSA support, on a modern Ubuntu host (24.04, GCC 13).

## TL;DR — happy path

```bash
sudo apt install -y gcc-arm-linux-gnueabihf bison flex libssl-dev libelf-dev bc lz4
cd kernel
make kernel-download         # clones linux-socfpga at the 4.19 tag
make kernel-config           # applies defconfig + the disable/enable tweaks
make kernel-menuconfig       # toggle SND_USB_AUDIO, USB_MON, debug flags (see below)
make -j$(nproc) zimage       # → linux-socfpga/arch/arm/boot/zImage
```

Result: `kernel/linux-socfpga/arch/arm/boot/zImage`, ~5 MB, ALSA + USB-MIDI built in.

---

## Host packages

| Package | Why |
|---|---|
| `gcc-arm-linux-gnueabihf` | the cross-compiler — `arm-linux-gnueabihf-gcc` |
| `bison`, `flex` | Kconfig parser/lexer |
| `libssl-dev` | needed even with `CONFIG_MODULE_SIG=n` for cert tooling |
| `libelf-dev` | `objtool` and other build helpers |
| `bc`, `lz4` | kernel build scripts |

You do **not** need to be inside SoC EDS's embedded shell for the kernel build itself. (You do need it for `sopc2dts` / `dtc` when generating the device tree from `.sopcinfo`, but that's a separate flow handled in [hw/quartus/wave-table-synth/Makefile](../hw/quartus/wave-table-synth/Makefile).)

---

## menuconfig settings

Use `/SYMBOL` in menuconfig to jump directly to each option. `Y` = built into zImage, `M` = loadable module, `N` = not built.

### Required (USB-MIDI through ALSA)

| Symbol | Set | Why |
|---|---|---|
| `SOUND` | y | parent gate for SND_*; without this, the ALSA tree is invisible |
| `SND_USB_AUDIO` | y | the actual driver; auto-selects `SND`, `SND_PCM`, `SND_RAWMIDI`, `SND_HWDEP`, `SND_USB`. Carries USB-MIDI support too, despite the name. |

### Recommended (debug / observability)

| Symbol | Set | Why |
|---|---|---|
| `USB_MON` | m | `usbmon` traces under `/sys/kernel/debug/usb/usbmon/` |
| `USB_ANNOUNCE_NEW_DEVICES` | y | descriptor dump in `dmesg` when a USB device plugs in |
| `DYNAMIC_DEBUG` | y | runtime `pr_debug()` toggling via `/sys/kernel/debug/dynamic_debug/control` |
| `SND_VERBOSE_PRINTK` | y | nicer ALSA log lines |
| `SND_DEBUG` | y | extra ALSA sanity checks |

### Already handled by `make kernel-config`

Set automatically via `scripts/config` after defconfig is applied — don't toggle in menuconfig:

- `CONFIG_LOCALVERSION_AUTO=n` (so `uname -r` stays clean: `4.19.0`)
- `CONFIG_LBDAF=y` (large block device support — needed for ext4 RW root)
- `CONFIG_XFS_FS=n`, `CONFIG_GFS2_FS=n`, `CONFIG_TEST_KMOD=n`

---

## Deploy

1. Mount the DE1-SoC's SD card on the dev box (or `scp` to the running board if `/boot` is writable in-place).
2. Replace the existing `zImage` on the FAT/boot partition with the new one.
3. Reboot. U-Boot picks up the new image automatically.

### Verification on the board

```
uname -r                       # 4.19.0
cat /proc/asound/cards         # should list the MPKmini2
amidi -l                       # hardware MIDI port, e.g. hw:1,0,0
dmesg | grep -iE 'snd|usbcore' # confirm snd-usb-audio attached
```

Plug in the MPKmini2 — `dmesg` should now log a full descriptor dump (thanks to `USB_ANNOUNCE_NEW_DEVICES`).

### Userspace consequences

With ALSA in the kernel, the existing libusb-based [sw/midi.c](../sw/midi.c) becomes unnecessary; the simpler path is:

```c
int fd = open("/dev/snd/midiC1D0", O_RDONLY);
read(fd, buf, sizeof(buf));   // standard 3-byte MIDI messages, hardware-clocked
```

No libusb scaffolding, no manual interface claiming, and timing is driven by the host controller's URB scheduling rather than userspace polling.

---

## Gotchas hit during initial setup

If anything in the Makefile looks weird, this is why.

### 1. `socfpga-4.19` branch was renamed

Upstream `altera-opensource/linux-socfpga.git` no longer has a `socfpga-4.19` branch — the 4.19 line lives only as tags. The Makefile uses `KERNEL_BRANCH = rel_socfpga-4.19_19.04.01_pr` (the latest 4.19 tag, April 2019). `git clone --branch` accepts tags, gives detached HEAD — fine for our purposes.

### 2. SoC EDS 20.1 doesn't bundle the toolchain

`~/intelFPGA/20.1/embedded/host_tools/linaro/` contains only `install_linaro.sh`, which downloads a *bare-metal* `arm-eabi-` toolchain. That's the wrong flavor for kernel work and an extra hassle. Use Debian's `gcc-arm-linux-gnueabihf` package instead — Linux-targeting, glibc-aware, well-tested for kernel builds.

The Makefile uses `CROSS_COMPILE=arm-linux-gnueabihf-` (Debian naming), not the older `arm-altera-eabi-` (SoC EDS naming).

### 3. `embedded_command_shell.sh` must be run, not sourced

Intel's script uses `${0}` to find its own directory. When sourced, `${0}` becomes the parent shell name (`bash`), so `env.sh` is looked up in `cwd` and silently fails. **Run** it (it spawns a configured subshell). Doesn't matter for the kernel build (we don't use it), but it does matter for the `dtc`/`sopc2dts` flow.

### 4. `.config` getting wiped by `make zimage`

A naive `$(KERNEL_CONFIG) : $(KERNEL_DIR)` dependency triggers `socfpga_defconfig` regeneration every time, clobbering menuconfig changes. The Makefile uses an **order-only prereq** (`| $(KERNEL_DIR)`) so `.config` is only generated when missing or when `make kernel-config` is invoked explicitly.

### 5. Host-side `dtc` link fails with GCC 10+

Symptom: `multiple definition of 'yylloc'` between `dtc-lexer.lex.o` and `dtc-parser.tab.o`. GCC 10 made `-fno-common` the default, exposing duplicate symbol declarations in the in-tree `dtc`. The Makefile passes `HOSTCFLAGS=-fcommon` to keep the old behavior for host tools.

### 6. ARMv7 instructions rejected by the assembler

Symptom: `Error: selected processor does not support 'isb' / 'cpsid i' / 'dmb ish' in ARM mode`.

Root cause: kernel `arch/arm/Makefile` line 67 uses

```makefile
arch-$(CONFIG_CPU_32v7) = -D__LINUX_ARM_ARCH__=7 $(call cc-option,-march=armv7-a,-march=armv5t -Wa$(comma)-march=armv7-a)
```

GCC 13's hard-float toolchain rejects `-march=armv7-a` combined with the kernel's `-msoft-float`, so `cc-option` returns the **fallback** path: `-march=armv5t` for the C compiler, `-march=armv7-a` for the assembler only. The compiler then writes `.arch armv5t` into intermediate `.s` files, which the assembler honors *over* the command-line `-Wa,-march=armv7-a`. The Makefile fixes this by appending `KCFLAGS=-march=armv7-a` (last `-march` wins for gcc).

### 7. `#alloc` / `#execinstr` section flags removed in binutils 2.36+

Symptom: `arch/arm/mm/proc-v7.S:640: Error: junk at end of line, first unrecognized character is '#'`.

Old ARM-specific GAS syntax for ELF section flags. ~28 hand-written `.S` files in `arch/arm/` use it. Mechanical fix:

```bash
grep -rl '\.section.*#alloc\|\.section.*#execinstr' arch/arm/ \
  | xargs sed -i -E '
      s/(\.section[^,]*),[[:space:]]*#alloc,[[:space:]]*#execinstr/\1, "ax"/g;
      s/(\.section[^,]*),[[:space:]]*#alloc/\1, "a"/g;
      s/(\.section[^,]*),[[:space:]]*#execinstr/\1, "x"/g
    '
```

This patch is applied directly to `kernel/linux-socfpga/arch/arm/`. It's not in the Makefile because it's a one-shot fixup against the source tree — re-running `make kernel-download` (which deletes and re-clones) would require re-applying it. Worth automating if it becomes a problem; left manual for now.

### 8. menuconfig terminal size

`mconf` requires ≥ 80×19 chars. Resize the terminal before launching.

---

## Files

- [Makefile](Makefile) — top-level kernel build flow (download / config / menuconfig / zimage)
- [linux-socfpga/.config](linux-socfpga/.config) — current kernel configuration
- [linux-socfpga/arch/arm/boot/zImage](linux-socfpga/arch/arm/boot/zImage) — built kernel image (post-build)
- [linux-headers-4.19.0/](linux-headers-4.19.0/) — headers for the *running* kernel image (currently mismatched: that image was built without `CONFIG_SOUND`, so its `Module.symvers` has zero `snd_*` symbols. Useful only for out-of-tree modules against features the running kernel actually has — not for adding ALSA shortcuts.)
ls -la /dev/snd/
