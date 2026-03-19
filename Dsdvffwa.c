# --- BƯỚC 1: Thu thập dữ liệu ---
while {[gets $fp line] >= 0} {
    set fields [split $line]
    set master_clock [lindex $fields 3]
    set gl_sinks     [lindex $fields 6]
    set gl_latency   [lindex $fields 7]
    set sinks        [lindex $fields 10]
    set latency      [lindex $fields 11]

    # Khởi tạo mảng nếu gặp Master Clock mới
    if {![info exists total_bnd_sinks($master_clock)]} {
        set total_bnd_sinks($master_clock) 0
        set total_bnd_lat($master_clock)   0.0
        set has_bnd($master_clock)         0
        # Lưu sẵn giá trị Global để dùng nếu cần
        set saved_gl_sinks($master_clock)   $gl_sinks
        set saved_gl_latency($master_clock) $gl_latency
    }

    # Cộng dồn tất cả Bnd vào biến tạm
    set total_bnd_sinks($master_clock) [expr {$total_bnd_sinks($master_clock) + $sinks}]
    set total_bnd_lat($master_clock)   [expr {$total_bnd_lat($master_clock) + ($sinks * $latency)}]

    # Nếu chỉ cần 1 dòng có sinks > 0, đánh dấu Master này dùng Bnd
    if {$sinks != 0} {
        set has_bnd($master_clock) 1
    }
}
close $fp

# --- BƯỚC 2: Tính toán và In kết quả ---
foreach m_clk [array names total_bnd_sinks] {
    if {$has_bnd($m_clk) == 1} {
        # TH1: Có ít nhất 1 sink khác 0 -> Dùng tổng Bnd
        set final_sinks $total_bnd_sinks($m_clk)
        set final_lat   $total_bnd_lat($m_clk)
    } else {
        # TH2: Tất cả sinks đều bằng 0 -> Dùng giá trị Global
        set final_sinks $saved_gl_sinks($m_clk)
        set final_lat   [expr {$saved_gl_sinks($m_clk) * $saved_gl_latency($m_clk)}]
    }

    # Tránh lỗi chia cho 0
    if {$final_sinks != 0} {
        set result [expr {($final_lat / $final_sinks) * -1}]
        # In kết quả theo format bạn muốn
        puts "set_clock_latency -source [format %.4f $result] \[get_ports ...] -clock $m_clk"
    }
}
