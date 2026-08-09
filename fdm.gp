# ============================================
# WIREFRAME WITH DATA POINTS
# ============================================

set terminal wxt size 1200,500 font "Arial,12"

# --- 3D Surface: Wireframe + Points ---
set title '3D Wireframe with Data Points'
set xlabel 'X'
set ylabel 'Y'
set zlabel 'Potential'
set view 60, 30
set grid

unset pm3d
splot 'output.dat' with linespoints lw 2 pt 7 ps 1.5 lc rgb 'blue' title 'Potential'

unset multiplot
pause -1 "Press enter to close plot"
