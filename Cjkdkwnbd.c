# Define the source file path
set file "/work/cuongle/cuongle_scripts/func2C_hold9.csv"

if {![file exists $file]} {
    puts "Error: File $file not found."
    return
}

set fp [open $file r]

# Initialize arrays
array set sum_sinks {}
array set sum_latency {}
array set sum_gl_sinks {}
array set sum_gl_latency {}
array set master {}
array set is_port {}
array set has_bnd {}

# Read file line by line
while {[gets $fp line] >= 0} {
    # Trim potential whitespace and split by comma or space (adjust if your CSV uses commas)
    set line [string trim $line]
    if {$line eq ""} continue ;# Skip empty lines
    
    set fields [split $line]
    
    # Check if there are enough columns to avoid "index out of range"
    if {[llength $fields] < 12} continue

    set is_gen       [lindex $fields 2]
    set master_clock [lindex $fields 3]
    set clk_src      [lindex $fields 4]
    set is_port_src  [lindex $fields 5]
    set gl_sinks     [lindex $fields 6]
    set gl_latency   [lindex $fields 7]
    set sinks        [lindex $fields 10]
    set latency      [lindex $fields 11]

    # VALIDATION: Ensure numeric values before calculation
    if {![string is double -strict $gl_sinks] || ![string is double -strict $gl_latency] || \
        ![string is double -strict $sinks] || ![string is double -strict $latency]} {
        continue ;# Skip header or corrupted rows
    }

    if {$is_gen eq "true" || $is_gen eq "false"} {
        
        if {![info exists sum_sinks($master_clock)]} {
            set sum_sinks($master_clock) 0
            set sum_latency($master_clock) 0.0
            set sum_gl_sinks($master_clock) 0
            set sum_gl_latency($master_clock) 0.0
            set has_bnd($master_clock) 0
        }

        # 1. Accumulate Global data
        set sum_gl_sinks($master_clock) [expr {$sum_gl_sinks($master_clock) + $gl_sinks}]
        set sum_gl_latency($master_clock) [expr {$sum_gl_latency($master_clock) + ($gl_sinks * $gl_latency)}]

        # 2. Logic: If sinks > 0, we flag this clock to use BND data
        if {$sinks != 0} {
            set has_bnd($master_clock) 1
            set sum_sinks($master_clock) [expr {$sum_sinks($master_clock) + $sinks}]
            set sum_latency($master_clock) [expr {$sum_latency($master_clock) + ($sinks * $latency)}]
        }

        set master($master_clock) $clk_src
        set is_port($master_clock) $is_port_src
    }
}
close $fp

# --- Output Generation ---
foreach m_clock [array names sum_gl_sinks] {
    
    # Selection: If at least one row had sinks > 0, use BND. Else use Global.
    if {$has_bnd($m_clock)} {
        set final_sum $sum_sinks($m_clock)
        set final_acc_latency $sum_latency($m_clock)
    } else {
        set final_sum $sum_gl_sinks($m_clock)
        set final_acc_latency $sum_gl_latency($m_clock)
    }

    if {$final_sum != 0} {
        set result [expr {($final_acc_latency / $final_sum) * (-1)}]
        
        # Determine command type
        set target_type "get_pins"
        if {$is_port($m_clock) eq "true"} { set target_type "get_ports" }
        
        set cmd [format "set_clock_latency -source %7.4f \[%s {%s}\] -clock %s" \
                 $result $target_type $m_clock $master($m_clock)]
        
        puts "#Clock: $m_clock"
        # Using 'puts' to write to file for better compatibility
        set out_file [open "STIMING_new_latency.tcl" a]
        puts $out_file $cmd
        close $out_file
    }
}
