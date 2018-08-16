# This is a very simple bed leveling routine. It generates Gcode that should be
# suitable for any machine.
import sys

# Set the overlap of the tool cuts. Step should be about 50% of the tool diameter.
# If you get recursion errors, try making the tolerance closer to zero.
def get_step(step, offset, tolerance, size):
    trial = (size / step) - int(size / step)
    if tolerance < abs(trial):
        return get_step(step+offset, offset, tolerance, size)
    else:
        return round(step, 6)

# constant values. edit these.
# all dims are in mm
work_x = 400.0  # X dimention of the work piece
work_y = 600.0  # Y dimention of the work piece
lev_x = 395.0   # X size to cut
lev_y = 595.0   # Y size to cut
tool_dia = 6.0  # Diameter of the cutting tool
feed = 1200.0   # Cutting feed rate in mm/minute
depth = -0.5    # Depth to cut to
safe_height = 10.0  # Height to raise the tool to when not cutting

# calculated values. don't edit
start_x = (work_x - lev_x) / 2.0
start_y = (work_y - lev_y) / 2.0
end_x = lev_x - start_x
end_y = lev_y - start_y
curnt_x = start_x

# Calculate the step size so that 1) it's between 50% and 60% of the tool diameter, 
# and 2) so that it comes out evenly to the correct size. Requiring an integer number
# of passes.
step_size = get_step(tool_dia * 0.5, 0.001, 0.001, lev_x)

# preamble
sys.stdout.write("\n(max X = %f)\n"%lev_x)
sys.stdout.write("(max Y = %f)\n"%lev_y)
sys.stdout.write("(tool diameter = %f)\n"%tool_dia)
sys.stdout.write("(step_size = %f)\n"%step_size)
sys.stdout.write("(cut depth = %f)\n"%depth)
sys.stdout.write("(safe height = %f)\n\n"%safe_height)

# cutting instructions
sys.stdout.write("M3\n")
sys.stdout.write("G0 X%f Y%f Z%f\n"%(start_x, start_y, safe_height))
sys.stdout.write("G1 Z%f F%f\n"%(depth, feed / 2))
sys.stdout.write("F%f\n"%(feed))

sys.stdout.write("G1 X%f Y%f\n"%(start_x, start_y))

sys.stdout.write("G1 X%f Y%f\n"%(start_x, end_y))
sys.stdout.write("G1 X%f Y%f\n"%(end_x, end_y))
sys.stdout.write("G1 X%f Y%f\n"%(end_x, start_y))
sys.stdout.write("G1 X%f Y%f\n"%(start_x, start_y))

start_y += step_size
end_y -= step_size

while True:
    curnt_x += step_size
    if curnt_x > end_x:
        break
    sys.stdout.write("G1 X%f\n"%(curnt_x))
    sys.stdout.write("G1 Y%f\n"%(end_y))
    curnt_x += step_size
    if curnt_x > end_x:
        break
    sys.stdout.write("G1 X%f\n"%(curnt_x))
    sys.stdout.write("G1 Y%f\n"%(start_y))

# finished
sys.stdout.write("G0 Z%f\n"%(safe_height))
sys.stdout.write("G0 X%f Y%f\n"%(start_x, start_y))
sys.stdout.write("M5\n")

