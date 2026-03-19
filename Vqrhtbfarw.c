# Init array
array set has_bnd {}
array set sum_bnd_sinks {}
array set sum_bnd_latency {}
array set sum_gl_sinks {}
array set sum_gl_latency {}

set fp [open "input.txt" r]

while {[gets $fp line] >= 0} {

    set fields [split $line]

    set master_clock [lindex $fields 3]
    set sinks        [lindex $fields 10]
    set latency      [lindex $fields 11]

    set gl_sinks     [lindex $fields 6]
    set gl_latency   [lindex $fields 7]

    # Init nếu chưa có
    if {![info exists has_bnd($master_clock)]} {
        set has_bnd($master_clock) 0
        set sum_bnd_sinks($master_clock) 0
        set sum_bnd_latency($master_clock) 0.0
        set sum_gl_sinks($master_clock) 0
        set sum_gl_latency($master_clock) 0.0
    }

    # Detect boundary != 0
    if {$latency != 0} {
        set has_bnd($master_clock) 1
    }

    # Luôn accumulate cả 2
    set sum_bnd_sinks($master_clock) \
        [expr {$sum_bnd_sinks($master_clock) + $sinks}]

    set sum_bnd_latency($master_clock) \
        [expr {$sum_bnd_latency($master_clock) + $sinks * $latency}]

    set sum_gl_sinks($master_clock) \
        [expr {$sum_gl_sinks($master_clock) + $gl_sinks}]

    set sum_gl_latency($master_clock) \
        [expr {$sum_gl_latency($master_clock) + $gl_sinks * $gl_latency}]
}

close $fp

# Final result
foreach master_clock [array names has_bnd] {

    if {$has_bnd($master_clock)} {
        if {$sum_bnd_sinks($master_clock) != 0} {
            set result [expr {
                $sum_bnd_latency($master_clock) / double($sum_bnd_sinks($master_clock))
            }]
        } else {
            set result 0
        }
        set mode "BOUNDARY"
    } else {
        if {$sum_gl_sinks($master_clock) != 0} {
            set result [expr {
                $sum_gl_latency($master_clock) / double($sum_gl_sinks($master_clock))
            }]
        } else {
            set result 0
        }
        set mode "GLOBAL"
    }

    puts "Clock: $master_clock | Mode: $mode | Latency: $result"
}
