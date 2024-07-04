//brauverein@andrebetz.de

dHold       = 0.9;
distance    = 5;
long        = 20;
bright      = 10;
thick       = 3;

hight = thick + dHold + thick + distance + thick + dHold + thick;
cube([thick,bright,hight]);
cube([long,bright,thick]);
translate([0,0,thick+dHold]) cube([long,bright,thick]);
translate([0,0,thick+dHold+thick+distance]) cube([long,bright,thick]);
translate([0,0,thick+dHold+thick+distance+thick+dHold]) cube([long,bright,thick]);


