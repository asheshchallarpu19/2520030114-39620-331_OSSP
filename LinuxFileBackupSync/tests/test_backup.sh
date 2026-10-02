#!/bin/bash

set -u

TEST_ROOT="/tmp/ossp_automated_test_$$"
SOURCE="$TEST_ROOT/source"
BACKUP="$TEST_ROOT/backup"

cleanup()
{
    rm -rf "$TEST_ROOT"
}

fail()
{
    echo "[FAIL] $1"
    exit 1
}

trap cleanup EXIT

echo "========================================"
echo " OSSP Automated Backup Test"
echo "========================================"

mkdir -p "$SOURCE/docs"

echo "Original File" > "$SOURCE/file1.txt"
echo "Nested File" > "$SOURCE/docs/file2.txt"

echo
echo "[TEST 1] Initial backup"

./backup_sync "$SOURCE" "$BACKUP" > "$TEST_ROOT/run1.txt" ||
    fail "Initial synchronization failed"

cmp "$SOURCE/file1.txt" "$BACKUP/file1.txt" ||
    fail "file1.txt does not match"

cmp "$SOURCE/docs/file2.txt" "$BACKUP/docs/file2.txt" ||
    fail "Nested file does not match"

echo "[PASS] Initial backup and recursive copy"

echo
echo "[TEST 2] Unchanged files"

./backup_sync "$SOURCE" "$BACKUP" > "$TEST_ROOT/run2.txt" ||
    fail "Second synchronization failed"

grep -Fq "Unchanged skipped   : 2" "$TEST_ROOT/run2.txt" ||
    fail "Unchanged files were not skipped"

echo "[PASS] Unchanged files skipped"

echo
echo "[TEST 3] Modified file"

sleep 1
echo "Modified Content" >> "$SOURCE/file1.txt"

./backup_sync "$SOURCE" "$BACKUP" > "$TEST_ROOT/run3.txt" ||
    fail "Modified-file synchronization failed"

grep -Fq "Files updated       : 1" "$TEST_ROOT/run3.txt" ||
    fail "Modified file was not detected"

cmp "$SOURCE/file1.txt" "$BACKUP/file1.txt" ||
    fail "Modified backup does not match source"

echo "[PASS] Modified file detected and updated"

echo
echo "[TEST 4] Deep recursive directory"

mkdir -p "$SOURCE/college/semester4/project"
echo "OSSP Final Project" > \
    "$SOURCE/college/semester4/project/report.txt"

./backup_sync "$SOURCE" "$BACKUP" > "$TEST_ROOT/run4.txt" ||
    fail "Recursive synchronization failed"

cmp \
    "$SOURCE/college/semester4/project/report.txt" \
    "$BACKUP/college/semester4/project/report.txt" ||
    fail "Deep recursive file was not copied"

echo "[PASS] Recursive directory synchronization"

echo
echo "[TEST 5] Invalid source directory"

if ./backup_sync \
    "$TEST_ROOT/does_not_exist" \
    "$BACKUP" \
    > /dev/null 2>&1
then
    fail "Invalid source was incorrectly accepted"
fi

echo "[PASS] Invalid source correctly rejected"

echo
echo "========================================"
echo " ALL AUTOMATED TESTS PASSED"
echo "========================================"
