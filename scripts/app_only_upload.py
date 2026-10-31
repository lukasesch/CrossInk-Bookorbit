"""
PlatformIO post script for boards that keep their vendor bootloader, partition
table, otadata and recovery partition (BOOX Picco). An upload may write only the
app image, at the ota_0 offset of the partition table. Everything else that would
write flash is refused. All checks fail closed.
"""

import csv
import os
import re

from SCons.Script import COMMAND_LINE_TARGETS

Import("env")  # noqa: F821  # type: ignore[name-defined]

REFUSAL = "app_only_upload: refusing target on single-slot board"
BLOCKED_TARGETS = {"erase", "erase_upload", "uploadfs", "uploadfsota"}
# Any other command means another protocol (espota, dfu, openocd, custom), whose
# writes these checks cannot see.
ESPTOOL_UPLOADCMD = "$UPLOADER $UPLOADERFLAGS $ESP32_APP_OFFSET $SOURCE"
OFFSET_TOKEN = re.compile(r"^0x", re.IGNORECASE)

# At load time: a pre-action would run after `uploadfs` has already failed on the
# missing data/ directory, which would look like a refusal.
if BLOCKED_TARGETS & set(COMMAND_LINE_TARGETS):
    print(REFUSAL, flush=True)
    env.Exit(1)  # noqa: F821  # type: ignore[name-defined]

env.Replace(ERASECMD=f'"$PYTHONEXE" -c "raise SystemExit(\'{REFUSAL}\')"')  # noqa: F821  # type: ignore[name-defined]


def _without_offset_pairs(flags):
    # The platform has already appended the extra images to the flags as
    # `<offset> <path>` pairs, so clearing FLASH_EXTRA_IMAGES alone leaves them in.
    kept = []
    tokens = iter(flags)
    for token in tokens:
        if OFFSET_TOKEN.match(str(token)):
            next(tokens, None)
            continue
        kept.append(token)
    return kept


env.Replace(  # noqa: F821  # type: ignore[name-defined]
    FLASH_EXTRA_IMAGES=[],
    UPLOADERFLAGS=_without_offset_pairs(env.get("UPLOADERFLAGS", [])),  # noqa: F821  # type: ignore[name-defined]
)


def _ota0_offset(env):
    partition_file = env.GetProjectOption("board_build.partitions")
    if not os.path.isabs(partition_file):
        partition_file = os.path.join(env["PROJECT_DIR"], partition_file)
    offsets = []
    with open(partition_file, newline="", encoding="utf-8") as fp:
        for row in csv.reader(fp):
            row = [cell.strip() for cell in row]
            if len(row) >= 5 and not row[0].startswith("#") and row[1] == "app" and row[2] == "ota_0":
                offsets.append(int(row[3], 16))
    if len(offsets) != 1:
        raise ValueError(f"expected one ota_0 partition in {partition_file}, found {len(offsets)}")
    return offsets[0]


def _upload_problem(env):
    if env.get("UPLOADCMD") != ESPTOOL_UPLOADCMD:
        return f"upload command is not esptool's: {env.get('UPLOADCMD')}"
    flags = [env.subst(str(flag)) for flag in env.get("UPLOADERFLAGS", [])]
    if "write-flash" not in flags:
        return "esptool flags have no write-flash"
    extra = [flag for flag in flags if OFFSET_TOKEN.match(flag)]
    if extra:
        return f"extra offsets remain in UPLOADERFLAGS: {extra}"
    try:
        app_offset = int(env.subst("$ESP32_APP_OFFSET"), 16)
        ota0_offset = _ota0_offset(env)
    except (OSError, ValueError) as error:
        return f"cannot compare offsets: {error}"
    if app_offset != ota0_offset:
        return f"app offset {hex(app_offset)} is not the ota_0 offset {hex(ota0_offset)}"
    return None


def _refuse_unless_app_only(target, source, env):
    del target, source
    problem = _upload_problem(env)
    if problem:
        print(f"app_only_upload: refusing upload: {problem}", flush=True)
        return 1
    return None


env.AddPreAction("upload", _refuse_unless_app_only)  # noqa: F821  # type: ignore[name-defined]
