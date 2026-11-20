/* small cap to fill the oven holes */ 


button_cap = true;
int_diam = 21.6;
ext_diam = 40;
thickness = 1.0;

button_hole_diam = 22.0;
button_fix_hole_diam = 22.0;
button_fix_diam = 19.6;
button_hole_height = 15.0;

$fn = 200;

if (button_cap)
    button_cap();
else
    fill_cap();
  
module fill_cap()
{
   cylinder(h = 1, d=ext_diam);

    difference()
    {
        cylinder(h = 10, d=int_diam);
        cylinder(h = 10, d=int_diam-2);
        translate([-2.5, -int_diam / 2, 0])
        {
            cube([5, int_diam, 5]);
        }
    } 

    translate([int_diam, -int_diam / 2, 0])
    {
        cube([4.5, int_diam + 10, 3.1]);
        translate([-2,0,0])
        {
            cube([5+4, 4, 3.1]);
        }
    }   
}

module button_cap()
{
    difference() 
    {
        cylinder(h = 2, d=ext_diam);
        translate([0, 0, -1])
        {
           cylinder(h = 10, d = button_hole_diam - 1); 
        } 
    }
       
    /* int side */
    difference()
    {
        union()
        {
            cylinder(h = 8, d=int_diam);
            translate([0, 0, thickness + 1 + 4])
            mirror([0, 0, 1])
            cylinder_snap_lugs(
                ext_diam / 2 + thickness - 0.4,
                0,
                lug_height  = 3,
                lug_radial  = 0.8,
                lug_angle   = 18,
                lead_height = 2.2,
                back_height = 0.2
                );
        }
        cylinder(h = 8, d=int_diam- (2 * thickness));    
    }
    
    /* ext / button side */
    translate([0, 0, -button_hole_height])
    {
        difference()
        {
            cylinder(h = button_hole_height, d=button_fix_hole_diam + (2 * thickness));
            cylinder(h = button_hole_height, d=button_fix_hole_diam);
        }
        difference()
        {
            union() 
            {
                cylinder(h = 1.1, d=button_fix_hole_diam + (2 * thickness));
                cylinder(h = 1.1, d=button_fix_diam);
            }
            cylinder(h = 1.5, d=button_fix_diam);
        } 
    }
}


/*
    Two opposing locking lugs on the outside of a cylinder.

    The cylinder is assumed to be oriented along the Z-axis.
    The lugs follow the curvature of the cylinder, and the slope is axial.

    Parameters:
    - cyl_d         : outer diameter of the support cylinder
    - z_pos         : position of the bottom of the lug
    - lug_height    : total axial height of the lug
    - lug_radial    : maximum radial projection of the lug
    - lug_angle     : angular width of the lug, in degrees
    - lead_height   : axial length of the insertion slope
    - back_height   : axial height of the straight rear face
*/
module cylinder_snap_lugs(
    cyl_d,
    z_pos,
    lug_height  = 3,
    lug_radial  = 1.0,
    lug_angle   = 18,
    lead_height = 2.2,
    back_height = 0.2
) {
    for (a = [0, 180]) {
        rotate([0,0,a])
            cylinder_snap_lug(
                cyl_d        = cyl_d,
                z_pos        = z_pos,
                lug_height   = lug_height,
                lug_radial   = lug_radial,
                lug_angle    = lug_angle,
                lead_height  = lead_height,
                back_height  = back_height
            );
    }
}

/*
    Single curved lug.
    Create a 2D profile in the plane (radius, z), then perform a rotate_extrude
    at a small angle centered around the X+ axis.
*/
module cylinder_snap_lug(
    cyl_d,
    z_pos,
    lug_height  = 3,
    lug_radial  = 1.0,
    lug_angle   = 18,
    lead_height = 2.2,
    back_height = 0.2
) {
    r = cyl_d / 2;

    rotate([0,0,-lug_angle/2])
        rotate_extrude(angle = lug_angle, convexity = 10)
            translate([r, z_pos])
                polygon(points = [
                    [0, 0],                                      // lower inner base
                    [lug_radial, lead_height],                  // end of insertion slope
                    [lug_radial, lug_height - back_height],     // outer apex
                    [0, lug_height]                             // nearly straight rear face
                ]);
}