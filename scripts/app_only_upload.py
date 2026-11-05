"""Keep vendor recovery intact: upload only the selected application slot.

For boards that keep their vendor bootloader, partition table, otadata and
recovery partition (BOOX Picco). Everything else that would write flash is
refused, and the upload itself is checked before it runs. All checks fail closed.
"""
import csv
import re
from pathlib import Path

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

partition_file = Path(env['PROJECT_DIR']) / env.GetProjectOption('board_build.partitions')  # noqa: F821
app_label = env.GetProjectOption('custom_firmware_app_partition', 'app')  # noqa: F821
with partition_file.open(newline='', encoding='utf-8') as table:
    slots = [row for row in csv.reader(table)
             if len(row) >= 5 and row[0].strip() == app_label and row[1].strip() == 'app']
if len(slots) != 1 or not slots[0][3].strip():
    raise RuntimeError('App-only upload requires one selected app slot with an explicit offset')
app_offset = int(slots[0][3].strip(), 0)

# PlatformIO can already have copied these pairs into UPLOADERFLAGS before a
# post script runs. Clearing FLASH_EXTRA_IMAGES alone does not remove them.
flags = list(env.get('UPLOADERFLAGS', []))  # noqa: F821
for offset, image in env.get('FLASH_EXTRA_IMAGES', []):  # noqa: F821
    pair = [env.subst(str(offset)), env.subst(str(image))]  # noqa: F821
    index = 0
    while index + 1 < len(flags):
        if [env.subst(str(item)) for item in flags[index:index + 2]] == pair:  # noqa: F821
            del flags[index:index + 2]
        else:
            index += 1

env.Replace(FLASH_EXTRA_IMAGES=[], UPLOADERFLAGS=flags, ESP32_APP_OFFSET=hex(app_offset))  # noqa: F821


def _upload_problem(env):
    if env.get("UPLOADCMD") != ESPTOOL_UPLOADCMD:
        return f"upload command is not esptool's: {env.get('UPLOADCMD')}"
    upload_flags = [env.subst(str(flag)) for flag in env.get("UPLOADERFLAGS", [])]
    if "write-flash" not in upload_flags:
        return "esptool flags have no write-flash"
    extra = [flag for flag in upload_flags if OFFSET_TOKEN.match(flag)]
    if extra:
        return f"extra offsets remain in UPLOADERFLAGS: {extra}"
    try:
        offset = int(env.subst("$ESP32_APP_OFFSET"), 16)
    except ValueError as error:
        return f"cannot read the app offset: {error}"
    if offset != app_offset:
        return f"app offset {hex(offset)} is not the {app_label} slot offset {hex(app_offset)}"
    return None


def _refuse_unless_app_only(target, source, env):
    del target, source
    problem = _upload_problem(env)
    if problem:
        print(f"app_only_upload: refusing upload: {problem}", flush=True)
        return 1
    return None


env.AddPreAction("upload", _refuse_unless_app_only)  # noqa: F821  # type: ignore[name-defined]
