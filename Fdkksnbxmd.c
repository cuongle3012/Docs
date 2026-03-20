# Define the source file path
set file "/work/cuongle/cuongle_scripts/func2C_hold9.csv"
set fp [open $file r]

# Initialize arrays to store accumulated values
array set sum_sinks {}
array set sum_latency {}
array set sum_gl_sinks {}
array set sum_gl_latency {}
array set master {}
array set is_port {}
array set has_bnd {}

# Read file line by line
while {[gets $fp line] >= 0} {
    set fields [split $line]
    
    # Extract values based on column index
    set is_gen       [lindex $fields 2]
    set master_clock [lindex $fields 3]
    set clk_src      [lindex $fields 4]
    set is_port_src  [lindex $fields 5]
    set gl_sinks     [lindex $fields 6]
    set gl_latency   [lindex $fields 7]
    set sinks        [lindex $fields 10]
    set latency      [lindex $fields 11]

    # Process only if generated clock status is valid
    if {$is_gen eq "true" || $is_gen eq "false"} {
        
        # Initialize clock entry if seen for the first time
        if {![info exists sum_sinks($master_clock)]} {
            set sum_sinks($master_clock) 0
            set sum_latency($master_clock) 0.0
            set sum_gl_sinks($master_clock) 0
            set sum_gl_latency($master_clock) 0.0
            set has_bnd($master_clock) 0
        }

        # 1. Always accumulate Global data as a fallback
        set sum_gl_sinks($master_clock) [expr {$sum_gl_sinks($master_clock) + $gl_sinks}]
        set sum_gl_latency($master_clock) [expr {$sum_gl_latency($master_clock) + ($gl_sinks * $gl_latency)}]

        # 2. If valid BND data exists (sinks != 0), accumulate it and set the flag
        if {$sinks != 0} {
            set has_bnd($master_clock) 1
            set sum_sinks($master_clock) [expr {$sum_sinks($master_clock) + $sinks}]
            set sum_latency($master_clock) [expr {$sum_latency($master_clock) + ($sinks * $latency)}]
        }

        # Store metadata (updated by the last occurrence in file)
        set master($master_clock) $clk_src
        set is_port($master_clock) $is_port_src
    }
}
close $fp

# --- Output Generation Phase ---
foreach m_clock [array names sum_gl_sinks] {
    
    # Logic: Prioritize BND if the flag was tripped; otherwise, use Global
    if {$has_bnd($m_clock)} {
        set final_sum $sum_sinks($m_clock)
        set final_acc_latency $sum_latency($m_clock)
    } else {
        set final_sum $sum_gl_sinks($m_clock)
        set final_acc_latency $sum_gl_latency($m_clock)
    }

    # Calculate weighted average and apply negative sign for latency adjustment
    if {$final_sum != 0} {
        set result [expr {($final_acc_latency / $final_sum) * (-1)}]
        
        # Output to terminal
        echo "#Clock: $m_clock"
        
        # Format the SDC command based on whether it is a Port or a Pin
        if {$is_port($m_clock) eq "true"} {
            set cmd [format "set_clock_latency -source %7.4f \[get_ports {%s}\] -clock %s" $result $m_clock $master($m_clock)]
        } else {
            set cmd [format "set_clock_latency -source %7.4f \[get_pins {%s}\] -clock %s" $result $m_clock $master($m_clock)]
        }
        
        # Write to the external timing file
        echo $cmd >> STIMING(new_latency)
    }
}
