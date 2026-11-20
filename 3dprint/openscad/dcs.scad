/*
 * DCS (3D Printer Filament Dryer System)  
 *
 * Basic enclosure & panel
 *
 * David GOMEZ (c) 2026
 *
 * V1.0 July, 3 2026
 *
 */

// Global resolution
$fs = 0.1;  // Don't generate smaller facets than 0.1 mm
$fn = 200; //results in smooth shape

/* [General] */

show_mode                                   = 4;        // [0:box 3d, 1:box 2d, 2:lid 3d, 3:lid 2d, 4:panel 3d, 5:panel 2d]
show_pcbs                                   = false; 

/* [PCB - Global] */

pcb_thickness                               = 1.6;
pcb_clearance                               = 20;

/* [AC Box] */

acbox_size                                  = [150.0, 104.0, 30.0]; // Length, Width, Height
acbox_radius                                = 10;   
acbox_doubleradius                          = 2 * acbox_radius; 
acbox_wall_thickness                        = 1.5;
acbox_lid_clearance                         = 0.3;
acbox_lid_height                            = 14.0;
acbox_pg_connector                          = "PG11";           // [PG7, PG9, PG11, PG13]

/* [AC Board] */

acboard_bosses_pitch                        = [70.0, 45.0]; // Length, Width               
acboard_pos                                 = [65.0, 53.0]; // X, Y 
acboard_boss_type                           = "M2.5";   // [M2, M2.5, M3, M4]     

/* [Opto Board] */ 

optoboard_bosses_pitch                      = [40.4,  4.0]; // Length, Width
optoboard_pos                               = [40.0, 94.0]; // X, Y                         
optoboard_boss_type                         = "M2.5";   // [M2, M2.5, M3, M4]   

/* [Panel] */

panel_ext_length                            = 140;    
panel_int_length                            = 105;
panel_ext_width                             = 113;
panel_int_width                             = 78; 
panel_int_clearance                         = 0.8;                         
panel_thickness                             = 2.0;
panel_radius                                = 10.0; 
panel_int_depth                             = 6.0;    
panel_fixing_hole_diameter                  = 3.0;

panel_offset_x                              = panel_radius;
panel_offset_y                              = panel_radius;

panel_int_ajusted_length                    = panel_int_length - (panel_int_clearance / 2);
panel_int_ajusted_width                     = panel_int_width - (panel_int_clearance / 2);
panel_int_x                                 = (panel_ext_length - panel_int_ajusted_length) / 2;       
panel_int_y                                 = (panel_ext_width - panel_int_ajusted_width) / 2;     

/* [Screen] */

screen_length                               = 24;       // [30.0:0.1:100.0] 
screen_width                                = 14.0;     // [30.0:0.1:100.0] 
screen_panel_width                          = 26.0;     // [50.0:0.1:100.0] 
screen_panel_height                         = 26.0;     // [30.0:0.1:100.0] 
screen_offset_x                             = -25;      // [-10:0.1:10]
screen_offset_y                             = 0;
screen_tray_depth                           = 1.0;      // [0.0:0.1:5.0] 
screen_tray_margin                          = 3.0;      // [0.0:0.1:10.0] 
screen_offset_tray_x                        = screen_offset_x;      // [-10.0:0.1:10.0] 
screen_offset_tray_y                        = screen_offset_y;      // [-10.0:0.1:10.0] 
screen_boss_type                            = "M2";     //
screen_bosses_follow_tray                   = true; 
screen_boss_pitch_x                         = 22.0;     // [20.0:0.1:150.0] 
screen_boss_pitch_y                         = 22.0;     // [20.0:0.1:100.0]
screen_boss_height                          = 4.0;      // [0.0:0.1:10.0] 

screen_boss_offset_x                        = screen_offset_x;      // [-10.0:0.1:10.0] 
screen_boss_offset_y                        = screen_offset_y - 1.5;      // [-10.0:0.1:10.0] 

/* [EC11 hole] */

ec11_hole_diameter                          = 7.5;

/* [USB connector] */

usb_connector_opening_length                = 11.2;
usb_connector_opening_width                 = 4.8;

/* [logic board] */

logicboard_enabled                          = true;
logicboard_boss_type                        = "M2.5";    
logicboard_pos_x                            = 95;
logicboard_pos_y                            = 48;
logicboard_center_x                         = -8;
logicboard_center_y                         = 8;
logicboard_pitch_x                          = 40;
logicboard_pitch_y                          = 40;
logicboard_width                            = 47;
logicboard_length                           = 47;
logicboard_height                           = 30; 

/* PG diameter */ 

acbox_pg_connector_diameter =
    acbox_pg_connector == "PG7"     ? 13.0 :
    acbox_pg_connector == "PG9"     ? 16.0 :
    acbox_pg_connector == "PG11"    ? 19.2 :
    acbox_pg_connector == "PG13"    ? 23.0 :
    19.2; // PG11 by default


include <DCOX/boss.scad>
include <DCOX/pcb.scad>

if (show_mode == 0)
{
    box_3d();
} 
else if (show_mode == 1)
{
    box_2d();
}
else if (show_mode == 2)
{
    lid_3d();
}
else if (show_mode == 3)
{
    lid_2d();
}
else if (show_mode == 4)
{
    panel_3d();
}
else if (show_mode == 5)
{
    panel_2d();
}

module pcb_and_bosses(enabled, pos_x, pos_y, center_x, center_y, pitch_x, pitch_y, boss_type, length, width, col, txt, pcb_height, boss_height = 0) {
    x_base = pos_x + center_x;
    y_base = pos_y + center_y;
    if (enabled) 
    {
        translate([x_base, y_base, panel_thickness]) 
        {
            four_boss_for_insert(boss_type, pitch_x, pitch_y, boss_height);      
        }
        if (show_pcbs) 
        {
            pcb_four_boss(  boss_type, 
                            x_base, 
                            y_base, 
                            length, 
                            width, 
                            pcb_thickness + pcb_height, 
                            panel_thickness, 
                            pitch_x,      
                            pitch_y, 
                            col, 
                            txt);        
        }
    }
}

/* panel */ 
module panel_3d()
{
    difference()
    {
        translate([panel_offset_x, panel_offset_y, 0])
        {
            rounded_cube(panel_ext_length, panel_ext_width, panel_thickness, panel_radius);        
            translate([panel_int_x, panel_int_y, 0])
            {
                rounded_box(panel_int_ajusted_length, panel_int_ajusted_width, panel_int_depth, panel_radius, panel_thickness);
            }
        }

        /* fixing holes */

        /* top */
        translate([panel_ext_length / 2, panel_ext_width - panel_offset_y, -.1])
        {
            cylinder(h=panel_thickness + .2, d=panel_fixing_hole_diameter); 
        }
        /* bottom */
        translate([panel_ext_length / 2, panel_offset_y, -.1])
        {
            cylinder(h=panel_thickness + .2, d=panel_fixing_hole_diameter); 
        }
        /* left */
        translate([panel_ext_length - panel_offset_x, panel_ext_width / 2, 0])
        {
            cylinder(h=panel_thickness + .2, d=panel_fixing_hole_diameter); 
        }        
        /* right */
        translate([panel_offset_x, panel_ext_width / 2, -.1])
        {
            cylinder(h=panel_thickness + .2, d=panel_fixing_hole_diameter); 

        }
            
        /* screen opening - centered */         
        translate([((panel_ext_length - screen_length)/2) + screen_offset_x, (panel_ext_width - screen_width)/2 + screen_offset_y, -.1])
        {
            cube([screen_length, screen_width, panel_thickness + 0.2]);
        }
        
        /* EC11 opening */        
        translate([panel_ext_length / 2 + screen_offset_x, panel_ext_width / 2 - 25, -.1])
        {
            cylinder(h=panel_thickness + .2, d=ec11_hole_diameter); 
        }
        
        /* USB connector opening */
        translate([panel_ext_length / 2 + screen_offset_x - (usb_connector_opening_length / 2), panel_ext_width / 2 + 20, 0])
        {
            cube([usb_connector_opening_length, usb_connector_opening_width, panel_thickness + 0.2]);
        }
                      
    }
    
    /* screen frame */
    
    start_x = ((panel_ext_length - screen_length)/2) + screen_offset_x - 1;
    start_y = ((panel_ext_width - screen_width)/2) + screen_offset_y - 1;
    
    difference() 
    {
        translate([start_x - 2, start_y - 2, panel_thickness - screen_tray_depth])
        {
            cube([screen_length + 6, screen_width + 4, 2.2]);
        }
        translate([start_x + 1, start_y + 1, panel_thickness - screen_tray_depth])
        {
            cube([screen_length, screen_width, 2.4]);
        }
    } 
        
    /* screen bosses */
    
    z_start = panel_thickness - (screen_bosses_follow_tray ? screen_tray_depth : 0);
    translate([ panel_ext_length / 2 + screen_boss_offset_x, 
                panel_ext_width / 2 + screen_boss_offset_y, 
                z_start])
        four_boss_for_insert(screen_boss_type, screen_boss_pitch_x, screen_boss_pitch_y, screen_boss_height);
    
    
    /* mainboard bosses */
    
     pcb_and_bosses(logicboard_enabled, 
                    logicboard_pos_x, 
                    logicboard_pos_y, 
                    logicboard_center_x, 
                    logicboard_center_y, 
                    logicboard_pitch_x, 
                    logicboard_pitch_y, 
                    logicboard_boss_type,
                    logicboard_length,
                    logicboard_width,
                    "Cyan",
                    "LOGIC",
                    logicboard_height,
                    4); 
    
}

module panel_2d()
{
    projection(cut = true) 
    {
        difference() 
        {
        translate([0, 0, -panel_thickness])
        {
            panel_3d();
        }            
        translate([0, 0, -panel_thickness - .2])
        {
            panel_3d();
        } 
        }        
    }
}


module box_3d()
{
    /* box */
    difference() 
    {
        translate([acbox_radius, acbox_radius, 0])
        {
            rounded_box(acbox_size[0], acbox_size[1], acbox_size[2], acbox_radius, acbox_wall_thickness);
        }
        
        translate([acbox_radius, acbox_radius, 0])
        {
            /* PGXX opening */ 
            translate([acbox_size[0] - acbox_doubleradius + 1, (acbox_size[1] - acbox_doubleradius) / 2, acbox_size[2] / 2])
            {
                rotate([90, 0, 90]) 
                {
                    cylinder(h = 10, d = acbox_pg_connector_diameter);
                }
            }
            
            /* JST to Logic Board opening */ 
            translate([-acbox_radius - 1, (acbox_size[1] - acbox_doubleradius - 30) / 2, (acbox_size[2] - 10) / 2])
            {
                cube([10, 30, 10]);
            }
        }
    }
        
    /* bosses for AC PCB */
    translate([acboard_pos[0], acboard_pos[1], acbox_wall_thickness])
        four_boss_for_insert(   acboard_boss_type, 
                                acboard_bosses_pitch[0], 
                                acboard_bosses_pitch[1],
                                5); 

    /* bosses for Opto PCB */ 
    translate([optoboard_pos[0], optoboard_pos[1], acbox_wall_thickness])
    {
        boss_for_insert(optoboard_boss_type, 5);
        translate([optoboard_bosses_pitch[0], -optoboard_bosses_pitch[1], 0])
            boss_for_insert(optoboard_boss_type, 5);
    }

}




module lid_3d()
{
    translate([acbox_size[0] - acbox_radius, acbox_radius, 0]) 
    {
        mirror([1,0,0])
        {
            rounded_cube(acbox_size[0], acbox_size[1], acbox_wall_thickness, acbox_radius);
            difference()
            {
                translate([acbox_wall_thickness + acbox_lid_clearance, acbox_wall_thickness + acbox_lid_clearance, 0])
                {
                    rounded_cube(acbox_size[0]-acbox_lid_clearance-(acbox_wall_thickness*2), acbox_size[1]-acbox_lid_clearance-(acbox_wall_thickness *2), acbox_lid_height, acbox_radius);
                }           
                translate([acbox_wall_thickness * 2, acbox_wall_thickness * 2,0])
                {   
                    rounded_cube(acbox_size[0]-acbox_lid_clearance-(acbox_wall_thickness * 4), acbox_size[1]-acbox_lid_clearance-(acbox_wall_thickness * 4), acbox_lid_height, acbox_radius);
                }           
            }
        }
    }

}

module lid_2d()
{
    projection(cut = true) 
    {
        /*translate([0, 0, 0])
        {
            lid_3d();
        }*/           
        translate([0, 0, -acbox_wall_thickness - .2])
        {
            lid_3d();
        }            
    }
}

module box_2d() {
    projection(cut = true) 
    {
        translate([0, 0, -1])
        {
            box_3d();
        }            
    }
}
   

module rounded_cube(length, width, height, radius)
{
    minkowski()
    {
        cube (size=[length - acbox_doubleradius, width-acbox_doubleradius, height]);
        cylinder(r=radius, h = 0.01);
    }
}


module rounded_box(length, width, height, radius, thickness)
{
    difference()
    {   
        rounded_cube(length, width, height, radius);
        translate([thickness,thickness,1])
        {
            //box wall thickness parameters    
            rounded_cube(length-(thickness*2), width-(thickness*2), height-1, radius);
        }   
        
    }    
}
