#!/bin/bash

case $# in
0)
  runs=3
  step=0.5
  ;;
1)
  runs=$1
  step=0.5
  ;;
2)
  runs=$1
  step=$2
  ;;
*)
  echo "syntax: performance_analysis.sh [runs] [step_size]"
  exit 1
  ;;
esac

voltage=5
results=results

proc_list="1 2 4 6 8 10 12"
chunk_list="512 1024 4096 16384 65536"
chunk=4096
fixed_procs=4

elapsed()
{
  sed -n 's/.*[ ]\([0-9:.]*\)elapsed.*/\1/p' |
  awk -F: '{ if (NF == 2) print $1 * 60 + $2; else print $1 }'
}

measure()
{
  total=0

  for i in $(seq 1 $runs)
  do
    t=$(/usr/bin/time $* 2>&1 >/dev/null | elapsed)
    total=$(awk -v a=$total -v b=$t 'BEGIN { print a + b }')
  done

  awk -v s=$total -v n=$runs 'BEGIN { printf("%.2f", s / n) }'
}

if [ ! -d $results ]
then
  mkdir $results
fi

echo "building"
make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [ ! -x ./fdm_fork ]
then
  echo "error: fdm_fork did not build"
  exit 1
fi

if [ ! -x ./assignment ]
then
  echo "error: assignment did not build"
  exit 1
fi


./fdm_fork $step 1 $voltage > /dev/null 2>&1
./assignment $step 1 $voltage > /dev/null 2>&1

echo "averaging $runs runs at step size $step"
echo

echo "# procs fork pthreads" > $results/procs.dat
echo "# procs pipe socket" > $results/ipc.dat
echo "# procs cyclic block" > $results/layout.dat

echo "sweeping process and thread counts"

for p in $proc_list
do
  pipe_cyc=$(measure ./fdm_fork $step $p $voltage $chunk cyclic pipe)
  sock_cyc=$(measure ./fdm_fork $step $p $voltage $chunk cyclic socket)
  pipe_blk=$(measure ./fdm_fork $step $p $voltage $chunk block pipe)
  threads=$(measure ./assignment $step $p $voltage)

  echo "$p $pipe_cyc $threads" >> $results/procs.dat
  echo "$p $pipe_cyc $sock_cyc" >> $results/ipc.dat
  echo "$p $pipe_cyc $pipe_blk" >> $results/layout.dat

  echo "  procs=$p  pipe=$pipe_cyc  socket=$sock_cyc  block=$pipe_blk  threads=$threads"
done

echo

echo "# chunk pipe socket" > $results/chunk.dat

echo "sweeping chunk sizes at $fixed_procs processes"

for c in $chunk_list
do
  a=$(measure ./fdm_fork $step $fixed_procs $voltage $c cyclic pipe)
  b=$(measure ./fdm_fork $step $fixed_procs $voltage $c cyclic socket)

  echo "$c $a $b" >> $results/chunk.dat
  echo "  chunk=$c  pipe=$a  socket=$b"
done

echo
echo "data written to $results/"

echo
echo "=================================================================="
echo " PERFORMANCE REPORT"
echo "=================================================================="

echo
echo "--- execution time vs processes / threads (analyses C and D) ---"
printf "%6s %9s %8s %7s %11s %8s %7s\n" procs fork speedup eff threads speedup eff

awk '
NR == 2 {
  base_fork = $2;
  base_thr = $3;
}
NR > 1 {
  fs = 0; fe = 0; ts = 0; te = 0;

  if ($2 > 0) {
    fs = base_fork / $2;
    fe = fs / $1 * 100;
  }
  if ($3 > 0) {
    ts = base_thr / $3;
    te = ts / $1 * 100;
  }

  printf("%6d %8.2fs %7.2fx %6.0f%% %10.2fs %7.2fx %6.0f%%\n", $1, $2, fs, fe, $3, ts, te);
}
' $results/procs.dat

echo
echo "--- pipes vs socketpairs (analysis B) ---"
printf "%6s %9s %9s %8s %7s\n" procs pipe socket faster diff

awk '
NR > 1 {
  if ($2 <= $3) {
    faster = "pipe";
    slow = $3;
  }
  else {
    faster = "socket";
    slow = $2;
  }

  diff = 0;
  if (slow > 0) {
    diff = ($2 - $3) / slow * 100;
    if (diff < 0) {
      diff = -diff;
    }
  }

  printf("%6d %8.2fs %8.2fs %8s %6.1f%%\n", $1, $2, $3, faster, diff);
}
' $results/ipc.dat

echo
echo "--- cyclic vs block row allocation (analysis E) ---"
printf "%6s %9s %9s %8s %7s\n" procs cyclic block faster diff

awk '
NR > 1 {
  if ($2 <= $3) {
    faster = "cyclic";
    slow = $3;
  }
  else {
    faster = "block";
    slow = $2;
  }

  diff = 0;
  if (slow > 0) {
    diff = ($2 - $3) / slow * 100;
    if (diff < 0) {
      diff = -diff;
    }
  }

  printf("%6d %8.2fs %8.2fs %8s %6.1f%%\n", $1, $2, $3, faster, diff);
}
' $results/layout.dat

echo
echo "--- chunk size at $fixed_procs processes (analysis B) ---"
printf "%7s %9s %9s\n" chunk pipe socket

awk '
BEGIN {
  best_pipe = 1e10;
  best_sock = 1e10;
}
NR > 1 {
  printf("%7d %8.2fs %8.2fs\n", $1, $2, $3);

  if ($2 < best_pipe) {
    best_pipe = $2;
    pipe_chunk = $1;
  }
  if ($3 < best_sock) {
    best_sock = $3;
    sock_chunk = $1;
  }
}
END {
  printf("\nfastest pipe chunk:   %d bytes (%.2fs)\n", pipe_chunk, best_pipe);
  printf("fastest socket chunk: %d bytes (%.2fs)\n", sock_chunk, best_sock);
}
' $results/chunk.dat

echo
echo "--- Amdahl's law, estimated serial fraction (analysis A) ---"
printf "%6s %13s %13s\n" procs "fork f" "pthreads f"

awk -v out=$results/amdahl.dat '
NR == 2 {
  base_fork = $2;
  base_thr = $3;
}
NR > 2 {
  p = $1;
  fork_txt = "-";
  thr_txt = "-";

  if ($2 > 0 && base_fork > 0) {
    sf = base_fork / $2;
    ff = (1 / sf - 1 / p) / (1 - 1 / p);
    sum_fork += ff;
    n_fork++;
    fork_txt = sprintf("%.3f", ff);
  }
  if ($3 > 0 && base_thr > 0) {
    st = base_thr / $3;
    ft = (1 / st - 1 / p) / (1 - 1 / p);
    sum_thr += ft;
    n_thr++;
    thr_txt = sprintf("%.3f", ft);
  }

  printf("%6d %13s %13s\n", p, fork_txt, thr_txt);
}
END {
  mf = 0;
  mt = 0;

  if (n_fork > 0) {
    mf = sum_fork / n_fork;
  }
  if (n_thr > 0) {
    mt = sum_thr / n_thr;
  }

  printf("\n");

  if (n_fork > 0) {
    if (mf > 1) {
      printf("fork:     serial fraction %.3f, above 1: slower than 1 process, overhead dominates so Amdahl does not apply\n", mf);
    }
    else if (mf < 0) {
      printf("fork:     serial fraction %.3f, below 0: faster than ideal, timer noise on a tiny problem\n", mf);
    }
    else {
      printf("fork:     serial fraction %.3f (%.1f%%)", mf, mf * 100);
      if (mf > 0) {
        printf(", Amdahl limit %.2fx", 1 / mf);
      }
      printf("\n");
    }
  }
  else {
    printf("fork:     not measurable, timings too small to resolve\n");
  }

  if (n_thr > 0) {
    if (mt > 1) {
      printf("pthreads: serial fraction %.3f, above 1: slower than 1 process, overhead dominates so Amdahl does not apply\n", mt);
    }
    else if (mt < 0) {
      printf("pthreads: serial fraction %.3f, below 0: faster than ideal, timer noise on a tiny problem\n", mt);
    }
    else {
      printf("pthreads: serial fraction %.3f (%.1f%%)", mt, mt * 100);
      if (mt > 0) {
        printf(", Amdahl limit %.2fx", 1 / mt);
      }
      printf("\n");
    }
  }
  else {
    printf("pthreads: not measurable, timings too small to resolve\n");
  }

  if (mf < 0) {
    mf = 0;
  }
  if (mf > 1) {
    mf = 1;
  }
  if (mt < 0) {
    mt = 0;
  }
  if (mt > 1) {
    mt = 1;
  }
  print mf, mt > out;
}
' $results/procs.dat

read f_fork f_thr < $results/amdahl.dat

awk '
NR == 2 {
  base_fork = $2;
  base_thr = $3;
}
NR > 1 {
  fs = 0;
  ts = 0;

  if ($2 > 0) {
    fs = base_fork / $2;
  }
  if ($3 > 0) {
    ts = base_thr / $3;
  }

  print $1, fs, ts;
}
' $results/procs.dat > $results/speedup.dat

echo
echo "generating plots"

gnuplot << EOF
set terminal svg size 800,500 font "Arial,12"
set grid
set key top left

set output "$results/time.svg"
set title "Execution time vs processes / threads"
set xlabel "processes / threads"
set ylabel "time (s)"
plot "$results/procs.dat" using 1:2 with linespoints title "fork", \
     "$results/procs.dat" using 1:3 with linespoints title "pthreads"

amdahl(p, f) = 1 / (f + (1 - f) / p)

set output "$results/speedup.svg"
set title "Speedup vs processes / threads, against Amdahl law"
set ylabel "speedup"
plot "$results/speedup.dat" using 1:2 with linespoints title "fork", \
     "$results/speedup.dat" using 1:3 with linespoints title "pthreads", \
     x with lines dashtype 2 title "ideal", \
     amdahl(x, $f_fork) with lines dashtype 3 title sprintf("Amdahl, fork f = %.3f", $f_fork), \
     amdahl(x, $f_thr) with lines dashtype 3 title sprintf("Amdahl, pthreads f = %.3f", $f_thr)

set output "$results/ipc.svg"
set title "Pipes vs socketpairs"
set ylabel "time (s)"
plot "$results/ipc.dat" using 1:2 with linespoints title "pipe", \
     "$results/ipc.dat" using 1:3 with linespoints title "socketpair"

set output "$results/layout.svg"
set title "Cyclic vs block row allocation"
plot "$results/layout.dat" using 1:2 with linespoints title "cyclic", \
     "$results/layout.dat" using 1:3 with linespoints title "block"

set output "$results/chunk.svg"
set title "Chunk size at $fixed_procs processes"
set xlabel "chunk size (bytes)"
set logscale x 2
plot "$results/chunk.dat" using 1:2 with linespoints title "pipe", \
     "$results/chunk.dat" using 1:3 with linespoints title "socketpair"

unset output
EOF

echo "plots written to $results/"
ls $results/*.svg
