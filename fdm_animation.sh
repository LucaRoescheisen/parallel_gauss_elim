readonly frame_rate=30
readonly video_length=10

frame_count=$((30 * 10))

OUTPUT_DIR="frames"
if [ -n "$OUTPUT_DIR" ]; then
    rm -rf "${OUTPUT_DIR:?}"/*
fi
voltage=0
voltage_increment=$(echo "5 / $frame_count" | bc -l)

for i in $(seq 1 $frame_count)
do
  ./assignment 0.5 8 $voltage
    gnuplot -e "filename='${OUTPUT_DIR}/frame_${i}.png'" fdm.gp
  voltage=$(echo "$voltage + $voltage_increment" | bc -l)
done


ffmpeg -framerate "$frame_rate" -i "${OUTPUT_DIR}/frame_%d.png" \
    -c:v libx264 -pix_fmt yuv420p output.mp4
