#!/bin/bash

set -u

TEST_ROOT="/tmp/ossp_final_test_$$"
LOG_DIR="$TEST_ROOT/logs"

cleanup()
{
    chmod -R u+rwX "$TEST_ROOT" 2>/dev/null || true
    rm -rf "$TEST_ROOT"
}

fail()
{
    echo
    echo "[FAIL] $1"
    echo
    echo "Test data kept only until script exits."
    exit 1
}

pass()
{
    echo "[PASS] $1"
}

trap cleanup EXIT

echo "========================================"
echo " OSSP Final Automated Regression Suite"
echo "========================================"

mkdir -p "$LOG_DIR" || fail "Cannot create test directory"

command -v flock >/dev/null 2>&1 ||
    fail "flock command is required"

make backup_sync >/dev/null 2>&1 ||
    fail "backup_sync failed to compile"

# ============================================================
# TEST 1 - Initial recursive backup
# ============================================================

echo
echo "[TEST 1] Initial recursive backup"

SOURCE="$TEST_ROOT/basic/source"
BACKUP="$TEST_ROOT/basic/backup"

mkdir -p "$SOURCE/docs"

printf 'Original File\n' > "$SOURCE/file1.txt"
printf 'Nested File\n' > "$SOURCE/docs/file2.txt"

./backup_sync "$SOURCE" "$BACKUP" \
    > "$LOG_DIR/test1.txt" 2>&1 ||
    fail "Initial synchronization failed"

cmp "$SOURCE/file1.txt" "$BACKUP/file1.txt" ||
    fail "file1.txt does not match"

cmp "$SOURCE/docs/file2.txt" "$BACKUP/docs/file2.txt" ||
    fail "Nested file does not match"

grep -Fq "New files copied    : 2" "$LOG_DIR/test1.txt" ||
    fail "Initial copy count is incorrect"

grep -Fq "Errors              : 0" "$LOG_DIR/test1.txt" ||
    fail "Initial backup reported errors"

pass "Initial backup and recursive copy"


# ============================================================
# TEST 2 - Unchanged files
# ============================================================

echo
echo "[TEST 2] Unchanged-file detection"

./backup_sync "$SOURCE" "$BACKUP" \
    > "$LOG_DIR/test2.txt" 2>&1 ||
    fail "Second synchronization failed"

grep -Fq "Unchanged skipped   : 2" "$LOG_DIR/test2.txt" ||
    fail "Unchanged files were not skipped"

grep -Fq "Bytes copied        : 0" "$LOG_DIR/test2.txt" ||
    fail "Unchanged backup copied unexpected bytes"

pass "Unchanged files skipped"


# ============================================================
# TEST 3 - Nanosecond modification detection
# ============================================================

echo
echo "[TEST 3] Nanosecond modification detection"

printf 'AAAAAAAA' > "$SOURCE/rapid.txt"

./backup_sync "$SOURCE" "$BACKUP" \
    > "$LOG_DIR/test3a.txt" 2>&1 ||
    fail "Could not create rapid.txt backup"

printf 'BBBBBBBB' > "$SOURCE/rapid.txt"

touch -d '2030-01-01 00:00:00.123456789' \
    "$SOURCE/rapid.txt"

touch -d '2030-01-01 00:00:00.987654321' \
    "$BACKUP/rapid.txt"

SOURCE_SIZE=$(stat -c '%s' "$SOURCE/rapid.txt")
BACKUP_SIZE=$(stat -c '%s' "$BACKUP/rapid.txt")

SOURCE_SEC=$(stat -c '%Y' "$SOURCE/rapid.txt")
BACKUP_SEC=$(stat -c '%Y' "$BACKUP/rapid.txt")

[ "$SOURCE_SIZE" = "$BACKUP_SIZE" ] ||
    fail "Nanosecond test files are not the same size"

[ "$SOURCE_SEC" = "$BACKUP_SEC" ] ||
    fail "Nanosecond test files are not in the same timestamp second"

./backup_sync "$SOURCE" "$BACKUP" \
    > "$LOG_DIR/test3b.txt" 2>&1 ||
    fail "Nanosecond synchronization failed"

grep -Fq "Files updated       : 1" "$LOG_DIR/test3b.txt" ||
    fail "Nanosecond-only modification was not detected"

cmp "$SOURCE/rapid.txt" "$BACKUP/rapid.txt" ||
    fail "Nanosecond-modified file does not match"

pass "Nanosecond timestamp change detected"


# ============================================================
# TEST 4 - Deep recursion
# ============================================================

echo
echo "[TEST 4] Deep recursive directory"

mkdir -p "$SOURCE/college/semester4/project"

printf 'OSSP Final Project\n' \
    > "$SOURCE/college/semester4/project/report.txt"

./backup_sync "$SOURCE" "$BACKUP" \
    > "$LOG_DIR/test4.txt" 2>&1 ||
    fail "Deep recursive synchronization failed"

cmp \
    "$SOURCE/college/semester4/project/report.txt" \
    "$BACKUP/college/semester4/project/report.txt" ||
    fail "Deep recursive file was not copied"

pass "Deep recursive synchronization"


# ============================================================
# TEST 5 - Configurable worker threads and statistics
# ============================================================

echo
echo "[TEST 5] Worker threads and byte statistics"

STAT_SOURCE="$TEST_ROOT/stats/source"
STAT_BACKUP="$TEST_ROOT/stats/backup"

mkdir -p "$STAT_SOURCE/sub"

printf '12345' > "$STAT_SOURCE/a.txt"
printf 'abcdefghij' > "$STAT_SOURCE/b.txt"
printf '12345678901234567890' > "$STAT_SOURCE/sub/c.txt"

./backup_sync \
    "$STAT_SOURCE" \
    "$STAT_BACKUP" \
    --threads 3 \
    > "$LOG_DIR/test5.txt" 2>&1 ||
    fail "Configurable-thread backup failed"

grep -Fq "Worker threads      : 3" "$LOG_DIR/test5.txt" ||
    fail "Configured thread count not reported correctly"

grep -Fq "Bytes copied        : 35" "$LOG_DIR/test5.txt" ||
    fail "Copied-byte count should be 35"

pass "Configurable threads and byte statistics"


# ============================================================
# TEST 6 - Invalid thread counts
# ============================================================

echo
echo "[TEST 6] Invalid thread-count rejection"

if ./backup_sync \
    "$STAT_SOURCE" \
    "$STAT_BACKUP" \
    --threads 0 \
    > "$LOG_DIR/test6a.txt" 2>&1
then
    fail "--threads 0 was incorrectly accepted"
fi

if ./backup_sync \
    "$STAT_SOURCE" \
    "$STAT_BACKUP" \
    --threads 17 \
    > "$LOG_DIR/test6b.txt" 2>&1
then
    fail "--threads 17 was incorrectly accepted"
fi

if ./backup_sync \
    "$STAT_SOURCE" \
    "$STAT_BACKUP" \
    --threads abc \
    > "$LOG_DIR/test6c.txt" 2>&1
then
    fail "Non-numeric thread count was incorrectly accepted"
fi

pass "Invalid thread counts rejected"


# ============================================================
# TEST 7 - Optional deletion mode
# ============================================================

echo
echo "[TEST 7] Safe optional --delete mode"

DELETE_SOURCE="$TEST_ROOT/delete/source"
DELETE_BACKUP="$TEST_ROOT/delete/backup"

mkdir -p "$DELETE_SOURCE"

printf 'Keep me\n' > "$DELETE_SOURCE/keep.txt"
printf 'Remove me\n' > "$DELETE_SOURCE/stale.txt"

./backup_sync \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    > "$LOG_DIR/test7a.txt" 2>&1 ||
    fail "Deletion setup backup failed"

rm "$DELETE_SOURCE/stale.txt"

./backup_sync \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    > "$LOG_DIR/test7b.txt" 2>&1 ||
    fail "Normal non-delete backup failed"

[ -f "$DELETE_BACKUP/stale.txt" ] ||
    fail "Normal mode incorrectly deleted stale.txt"

./backup_sync \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    --delete \
    > "$LOG_DIR/test7c.txt" 2>&1 ||
    fail "--delete synchronization failed"

[ ! -e "$DELETE_BACKUP/stale.txt" ] ||
    fail "--delete did not remove stale.txt"

grep -Fq "Files deleted       : 1" "$LOG_DIR/test7c.txt" ||
    fail "Deleted-file statistic is incorrect"

diff -r \
    --exclude=".backup_sync.lock" \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    >/dev/null ||
    fail "Mirror backup differs from source"

pass "Deletion is opt-in and mirror mode works"


# ============================================================
# TEST 8 - Stale directory-tree deletion
# ============================================================

echo
echo "[TEST 8] Stale directory-tree deletion"

mkdir -p "$DELETE_SOURCE/oldtree/a/b"

printf 'Old nested file\n' \
    > "$DELETE_SOURCE/oldtree/a/b/old.txt"

./backup_sync \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    > "$LOG_DIR/test8a.txt" 2>&1 ||
    fail "Directory deletion setup failed"

rm -rf "$DELETE_SOURCE/oldtree"

./backup_sync \
    "$DELETE_SOURCE" \
    "$DELETE_BACKUP" \
    --delete \
    > "$LOG_DIR/test8b.txt" 2>&1 ||
    fail "Directory-tree deletion failed"

[ ! -e "$DELETE_BACKUP/oldtree" ] ||
    fail "Stale directory tree still exists"

pass "Stale directory tree removed"


# ============================================================
# TEST 9 - File/directory type changes
# ============================================================

echo
echo "[TEST 9] File/directory type changes"

TYPE_SOURCE="$TEST_ROOT/type/source"
TYPE_BACKUP="$TEST_ROOT/type/backup"

mkdir -p "$TYPE_SOURCE/change"

printf 'Inside directory\n' \
    > "$TYPE_SOURCE/change/inside.txt"

./backup_sync \
    "$TYPE_SOURCE" \
    "$TYPE_BACKUP" \
    > "$LOG_DIR/test9a.txt" 2>&1 ||
    fail "Directory-to-file setup failed"

rm -rf "$TYPE_SOURCE/change"

printf 'Now a regular file\n' \
    > "$TYPE_SOURCE/change"

./backup_sync \
    "$TYPE_SOURCE" \
    "$TYPE_BACKUP" \
    --delete \
    > "$LOG_DIR/test9b.txt" 2>&1 ||
    fail "Directory-to-file conversion failed"

[ -f "$TYPE_BACKUP/change" ] ||
    fail "Backup path did not become a regular file"

cmp "$TYPE_SOURCE/change" "$TYPE_BACKUP/change" ||
    fail "Directory-to-file content mismatch"

rm "$TYPE_SOURCE/change"

mkdir -p "$TYPE_SOURCE/change/subdir"

printf 'Now inside a directory\n' \
    > "$TYPE_SOURCE/change/subdir/new.txt"

./backup_sync \
    "$TYPE_SOURCE" \
    "$TYPE_BACKUP" \
    --delete \
    > "$LOG_DIR/test9c.txt" 2>&1 ||
    fail "File-to-directory conversion failed"

[ -d "$TYPE_BACKUP/change" ] ||
    fail "Backup path did not become a directory"

cmp \
    "$TYPE_SOURCE/change/subdir/new.txt" \
    "$TYPE_BACKUP/change/subdir/new.txt" ||
    fail "File-to-directory content mismatch"

pass "Directory/file type changes handled safely"


# ============================================================
# TEST 10 - Source/backup relationship protection
# ============================================================

echo
echo "[TEST 10] Dangerous path protection"

PATH_ROOT="$TEST_ROOT/path"
PATH_SOURCE="$PATH_ROOT/source"

mkdir -p "$PATH_SOURCE/nested_backup"
printf 'Safety\n' > "$PATH_SOURCE/file.txt"

if ./backup_sync \
    "$PATH_SOURCE" \
    "$PATH_SOURCE" \
    > "$LOG_DIR/test10a.txt" 2>&1
then
    fail "Source == backup was incorrectly accepted"
fi

if ./backup_sync \
    "$PATH_SOURCE" \
    "$PATH_SOURCE/nested_backup" \
    > "$LOG_DIR/test10b.txt" 2>&1
then
    fail "Backup inside source was incorrectly accepted"
fi

ln -s \
    "$PATH_SOURCE/nested_backup" \
    "$PATH_ROOT/backup_link"

if ./backup_sync \
    "$PATH_SOURCE" \
    "$PATH_ROOT/backup_link" \
    > "$LOG_DIR/test10c.txt" 2>&1
then
    fail "Symlink path-safety bypass was incorrectly accepted"
fi

pass "Dangerous source/backup layouts rejected"


# ============================================================
# TEST 11 - Exclusive backup locking
# ============================================================

echo
echo "[TEST 11] Exclusive backup locking"

LOCK_SOURCE="$TEST_ROOT/lock/source"
LOCK_BACKUP="$TEST_ROOT/lock/backup"

mkdir -p "$LOCK_SOURCE" "$LOCK_BACKUP"

printf 'Lock test\n' > "$LOCK_SOURCE/file.txt"

flock -x \
    "$LOCK_BACKUP/.backup_sync.lock" \
    -c 'sleep 3' &

LOCK_PID=$!

sleep 1

if ./backup_sync \
    "$LOCK_SOURCE" \
    "$LOCK_BACKUP" \
    > "$LOG_DIR/test11.txt" 2>&1
then
    wait "$LOCK_PID" 2>/dev/null || true
    fail "Concurrent backup was incorrectly accepted"
fi

wait "$LOCK_PID" ||
    fail "Lock-holder process failed"

grep -Fq \
    "Another backup process is already using" \
    "$LOG_DIR/test11.txt" ||
    fail "Expected lock-conflict message not found"

pass "Concurrent backup correctly rejected"


# ============================================================
# TEST 12 - Threaded recoverable copy error
# ============================================================

echo
echo "[TEST 12] Threaded error handling"

ERROR_SOURCE="$TEST_ROOT/error/source"
ERROR_BACKUP="$TEST_ROOT/error/backup"

mkdir -p "$ERROR_SOURCE" "$ERROR_BACKUP"

printf 'Good file\n' > "$ERROR_SOURCE/good.txt"
printf 'Cannot read me\n' > "$ERROR_SOURCE/unreadable.txt"

chmod 000 "$ERROR_SOURCE/unreadable.txt"

if [ "$(id -u)" -eq 0 ]
then
    echo "[SKIP] Permission-denied test skipped for root user"
else
    if ./backup_sync \
        "$ERROR_SOURCE" \
        "$ERROR_BACKUP" \
        --threads 4 \
        > "$LOG_DIR/test12.txt" 2>&1
    then
        chmod 644 "$ERROR_SOURCE/unreadable.txt"
        fail "Unreadable file did not produce failure status"
    fi

    [ -f "$ERROR_BACKUP/good.txt" ] ||
        fail "Good file was not copied during partial failure"

    [ ! -e "$ERROR_BACKUP/unreadable.txt" ] ||
        fail "Unreadable file was unexpectedly created"

    grep -Fq "Errors              : 1" "$LOG_DIR/test12.txt" ||
        fail "Worker error was not counted"

    grep -Fq "SYNC_COMPLETED_WITH_ERRORS" "$LOG_DIR/test12.txt" ||
        fail "Failure IPC status was not reported"

    pass "Threaded copy error handled without stopping good files"
fi

chmod 644 "$ERROR_SOURCE/unreadable.txt"


# ============================================================
# TEST 13 - Atomic-copy temporary-file cleanup
# ============================================================

echo
echo "[TEST 13] Atomic-copy cleanup"

if find "$TEST_ROOT" \
    -name '*.tmp.*' \
    -print -quit |
    grep -q .
then
    fail "Temporary atomic-copy files were left behind"
fi

pass "No temporary copy files left behind"


# ============================================================
# TEST 14 - Invalid source
# ============================================================

echo
echo "[TEST 14] Invalid source directory"

if ./backup_sync \
    "$TEST_ROOT/does_not_exist" \
    "$TEST_ROOT/invalid_backup" \
    > "$LOG_DIR/test14.txt" 2>&1
then
    fail "Invalid source directory was incorrectly accepted"
fi

pass "Invalid source correctly rejected"


echo
echo "========================================"
echo " ALL FINAL REGRESSION TESTS PASSED"
echo "========================================"
