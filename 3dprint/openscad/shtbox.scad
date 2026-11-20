/*
 * DCS (3D Printer Filament Dryer System)  
 *
 * SHT sensor cover 
 *
 * David GOMEZ (c) 2026
 *
 * V1.0 July, 3 2026
 *
 * The SHT sensor is placed into the oven, protected & fixed by this small box (printed in ABS or ASA)
 *
 */


/* [Hidden] */
EVENT_HOLE_X = 0;
EVENT_HOLE_Y = 1;
EVENT_HOLE_DIAMETER = 2;
SHT_BOSS_X = 0;
SHT_BOSS_Y = 1;
SHT_BOSS_TYPE = 2;
SHT_BOSS_HEIGHT = 3;

$fn = 120;


/* [Options] */

top_wall = true;

/* [Internal dimensions] */ 

wall_thickness = 2.0;
length = 20;
width = 14;
fix_length = 10; 
fix_hole_diam = 4.8;

/* [Holes] */

event_hole = [8.5, 11, 5];      /* X, Y, Diameter */ 
sht_boss = [8, 6, "M2", 5];     /* X, Y, Type, Height */

include <DCOX/boss.scad>

$BOSS_COLOR = "Red";

/* right face */ 

cube([length + wall_thickness, width + wall_thickness, wall_thickness]);

/* front face */ 
difference()
{
    union() 
    {
        cube([length + wall_thickness, wall_thickness, width + wall_thickness]);
        /* event tower */ 
        translate([event_hole[EVENT_HOLE_X] - (event_hole[EVENT_HOLE_DIAMETER] / 2) , wall_thickness, event_hole[EVENT_HOLE_Y]  - (event_hole[EVENT_HOLE_DIAMETER] / 2)])
        {
            cube([event_hole[EVENT_HOLE_DIAMETER] + 2, sht_boss[SHT_BOSS_HEIGHT] - 0.8, event_hole[EVENT_HOLE_DIAMETER] + 2]);            
        }
    }
    /* event hole */ 
    translate([event_hole[EVENT_HOLE_X] - (event_hole[EVENT_HOLE_DIAMETER] / 2) +1 , 0, event_hole[EVENT_HOLE_Y]  - (event_hole[EVENT_HOLE_DIAMETER] / 2) + 1])
    {
        cube([event_hole[EVENT_HOLE_DIAMETER], sht_boss[SHT_BOSS_HEIGHT] + wall_thickness, event_hole[EVENT_HOLE_DIAMETER]]);            
    }    
}

translate([sht_boss[SHT_BOSS_X], wall_thickness, sht_boss[SHT_BOSS_Y]])
{
    rotate([90, 0, 180])
    {
        boss_for_insert(sht_boss[SHT_BOSS_TYPE], sht_boss[SHT_BOSS_HEIGHT]);
    }
}


/* left face */
translate([0, 0, width + wall_thickness])
{
    cube([length + wall_thickness, width + wall_thickness, wall_thickness]);
}

/* top wall */ 
if (top_wall) 
{
    translate([length + wall_thickness, 0, 0])
    {
        cube([wall_thickness, width + wall_thickness, width + (2 * wall_thickness)]);
    }
}

/* right fixing part */     
translate([0, width, -fix_length - wall_thickness]) 
{
    difference() 
    {
        cube([length + (wall_thickness * 2), wall_thickness, fix_length + wall_thickness]);
        translate([length / 2, wall_thickness + .1, (fix_length + wall_thickness) / 2]) 
        {
            rotate([90, 0, 0])
            {
                cylinder(h=3,d=fix_hole_diam);
            }
        }
    }
}

/* left fixing part */ 
translate([0, width, width + (2 * wall_thickness)]) 
{
    difference() 
    {
        cube([length + (wall_thickness * 2), wall_thickness, fix_length + wall_thickness]);
        translate([length / 2, wall_thickness + .1, (fix_length + wall_thickness) / 2]) 
        {
            rotate([90, 0, 0])
            {
                cylinder(h=3,d=fix_hole_diam);
            }
        }
    }
    
}    
