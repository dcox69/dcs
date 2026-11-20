include <boss.scad>

module pcb_four_boss(boss_type, x, y, length, width, height, z_offset, x_pitch, y_pitch, col = "Red", txt = "PCB") {     
    spec = get_boss_spec(boss_type);
    d = screw_hole_diameter(spec);
    z = boss_height(spec) + z_offset;    
    color(col, 0.5) {
        difference() {
            // pcb
            translate([x - length / 2, y - width / 2, z])
                cube([length, width, height]);
            // holes            
            translate([x - x_pitch / 2, y - y_pitch / 2, z]) 
                cylinder(h=height * 2, d=d);
            translate([x + x_pitch / 2, y - y_pitch / 2, z]) 
                cylinder(h=height * 2, d=d);
            translate([x - x_pitch / 2, y + y_pitch / 2, z]) 
                cylinder(h=height * 2, d=d);
            translate([x + x_pitch / 2, y + y_pitch / 2, z]) 
                cylinder(h=height * 2, d=d); 
                
        }
        // text
        translate([x, y, z + height]) 
            linear_extrude(height=2)
                text(txt, size=8, halign="center");
   
    }
}


module pcb_only(x, y, length, width, height, z, col = "Red", txt = "PCB") {
    color(col, 0.5) {
        // pcb
        translate([x - length / 2, y - width / 2, z])
            cube([length, width, height]);         
        // text            
        translate([x, y, z + height]) 
            linear_extrude(height=2)
                text(txt, size=8, halign="center");   
    } 
}
