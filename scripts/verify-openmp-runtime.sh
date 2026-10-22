#!/usr/bin/env bash
set -uo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
run_id=${LOMP_VERIFY_RUN_ID:-${GITHUB_RUN_ID:-local-$(date +%s%N)}}
[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ && "$run_id" != . && "$run_id" != .. ]] || exit 2
build="$root/build/verify-openmp-runtime/$run_id"
evidence="$root/artifacts/native-runs/$run_id"
mkdir -p "$evidence"
terminal="$evidence/result.json"
clang_bin=${LOMP_CLANG:-clang-18}
clangxx_bin=${LOMP_CLANGXX:-clang++-18}
injected_control=false

run_step() {
  local name=$1
  shift
  local status=0
  if "$@" 2>&1 | tee -a "$evidence/$name.log"; then
    status=0
  else
    status=$?
  fi
  printf '%s\t%s\n' "$name" "$status" >> "$evidence/status.tsv"
  return "$status"
}

doctor() {
  test "$(uname -s)" = Linux || return 2
  command -v "$clang_bin" >/dev/null || return 2
  command -v "$clangxx_bin" >/dev/null || return 2
  command -v cmake >/dev/null || return 2
  command -v ninja >/dev/null || return 2
  test -f "$build/.lomp-verify-owned" || return 2
  test -f "$build/src/libomp.so" || return 2
  test -x "$build/tests/fac2_benchmark" || return 2
  test -x "$build/system-fac2-benchmark"
}

launch() {
  if [[ -e "$build" && ! -f "$build/.lomp-verify-owned" ]]; then
    printf 'refusing to reuse a build directory without the driver marker: %s\n' "$build" >&2
    return 2
  fi
  mkdir -p "$build"
  touch "$build/.lomp-verify-owned"
  run_step configure cmake -S "$root" -B "$build" -G Ninja \
    -DCMAKE_C_COMPILER="$clang_bin" -DCMAKE_CXX_COMPILER="$clangxx_bin" \
    -DLOMP_BUILD_MICROBM=OFF || return $?
  run_step build cmake --build "$build" --parallel || return $?
  run_step system-benchmark-build "$clangxx_bin" -std=c++17 -O3 -DNDEBUG -fopenmp \
    -I "$root/include" "$root/tests/bench_fac2.cc" \
    -o "$build/system-fac2-benchmark"
}

drive() {
  run_step ctest env OMP_NUM_THREADS=4 ctest --test-dir "$build" \
    --output-on-failure || return $?
  for workers in 2 4; do
    for repeat in 1 2 3 4 5; do
      run_step "benchmark-lomp-$workers-$repeat" \
        "$build/tests/fac2_benchmark" "$workers" 5 || return $?
      run_step "benchmark-system-$workers-$repeat" \
        "$build/system-fac2-benchmark" "$workers" 5 --standard-only || return $?
    done
  done
}

capture() {
  run_step consumer-linkage ldd "$build/tests/test_fac2" || return $?
  grep -F "$build/src/" "$evidence/consumer-linkage.log" >/dev/null || return $?
  run_step lomp-benchmark-linkage ldd "$build/tests/fac2_benchmark" || return $?
  grep -F "$build/src/" "$evidence/lomp-benchmark-linkage.log" >/dev/null || return $?
  run_step system-benchmark-linkage ldd "$build/system-fac2-benchmark" || return $?
  grep -F libomp "$evidence/system-benchmark-linkage.log" >/dev/null || return $?
  ! grep -F "$build/src/" "$evidence/system-benchmark-linkage.log" >/dev/null
}

cleanup() {
  local expected="$root/build/verify-openmp-runtime/$run_id"
  test "$build" = "$expected" || return 2
  if test -d "$build"; then
    test -f "$build/.lomp-verify-owned" || return 2
    rm -rf "$build"
  fi
}

write_result() {
  local passed=$1 cleanup_ok=$2 interrupted=$3 injected=$4
  local tmp="$terminal.tmp"
  printf '{"run_id":"%s","passed":%s,"cleanup_succeeded":%s,"interrupted":%s,"injected_failure":%s,"evidence_dir":"%s"}\n' \
    "$run_id" "$passed" "$cleanup_ok" "$interrupted" "$injected" "$evidence" > "$tmp"
  mv -f "$tmp" "$terminal"
}

finalize() {
  local status=$1 interrupted=$2 injected=$3 cleanup_ok=true passed=false
  trap - INT TERM
  if cleanup; then
    cleanup_ok=true
  else
    cleanup_ok=false
    status=2
  fi
  if [[ $status -eq 0 && $injected == false ]]; then passed=true; fi
  write_result "$passed" "$cleanup_ok" "$interrupted" "$injected"
  return "$status"
}

handle_signal() {
  local signal=$1 status=143
  [[ $signal == INT ]] && status=130
  printf 'verification interrupted by %s; preserving evidence in %s\n' "$signal" "$evidence" >&2
  finalize "$status" true "$injected_control"
  exit "$status"
}

run_all() {
  trap 'handle_signal INT' INT
  trap 'handle_signal TERM' TERM
  local status=0
  launch && doctor && drive && capture || status=$?
  finalize "$status" false false
}

run_negative_control() {
  trap 'handle_signal INT' INT
  trap 'handle_signal TERM' TERM
  local status=0
  launch && doctor || status=$?
  if [[ $status -eq 0 ]]; then
    injected_control=true
    if run_step injected-failure bash -c 'exit 73'; then
      status=1
    else
      status=$?
    fi
  fi
  finalize "$status" false "$injected_control"
}

case "${1:-all}" in
  launch) launch ;;
  doctor) doctor ;;
  drive) drive ;;
  evidence) capture ;;
  cleanup) cleanup ;;
  all) run_all ;;
  negative-control) run_negative_control ;;
  *)
    printf 'usage: %s [all|negative-control|launch|doctor|drive|evidence|cleanup]\n' "$0" >&2
    exit 2
    ;;
esac
