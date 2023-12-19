//brauverein@andrebetz.de
itemsShown="both";
wallThickness=2;
bottomThickness=2;
boxLength=41+2*wallThickness;
boxWidth=28+2*wallThickness;
boxHeight=39+2*bottomThickness;
cornerRadius=5;
lidThickness=2;
lidClearance=0.2;
lidEdgeThickness=0.5;
// openings
openUSBPos = 0;
openUSBh = 6.5;
openUSBb = 11.5;
openTempPos = 16;
openTemph = 6.5;
openTempb = 13;
openSwitchPos = 0;
openSwitchb = 5;
knobLength  = wallThickness*3;
withNotch=true;
showWhat = 3;
$fn = 128;


if (0 == showWhat ) { // box
    showBoxAll();
} else if (1 == showWhat ) { // lid
    showLid();
} else if (2 == showWhat ) { // knob
    knob();
} else if (3 == showWhat ) { // all
    showBoxAll();
    translate ([0, 0,-3.2]) showLid();
    translate ([-openSwitchb, 0,-3.2]) knob();
}

module knob() {
    knobD = openSwitchb-0.5;
    cylinder(knobLength,knobD/2,knobD/2);
    translate ([-openSwitchb/2, -openSwitchb/2, 0]) 
        cube([openSwitchb,openSwitchb,1]);
}
module showBoxAll() {
    difference() 
    {
        showBox();
        translate ([-0.1, boxWidth/2-openUSBb/2,bottomThickness])
            cube([wallThickness*2,openUSBb,openUSBh]);
        translate ([boxLength-wallThickness-.1, boxWidth/2-openTempb/2,bottomThickness+openTempPos])
            cube([wallThickness*2,openTempb,openTemph]);            
        rotate([90,0,0])
            translate ([10,wallThickness+openSwitchb/2,-boxWidth-wallThickness])
                cylinder(wallThickness*3,openSwitchb/2,openSwitchb/2);
    }
    translate ([-1.5, 0,-3.2])
        brick(3,2);
    //bar floor
    translate ([wallThickness*4,wallThickness,bottomThickness])        
        cube([2,(boxWidth-knobLength-1),1]);
    
}
module showLid(){
	translate ([0, -2*wallThickness, 0]) 
	roundBoxLid(l=boxLength-wallThickness,
				w=boxWidth-2*wallThickness-lidClearance,
				h=lidThickness,
				et=lidEdgeThickness,
				r=cornerRadius-wallThickness,
				notch=withNotch);
    translate ([2, -26,0]) brick(2,1);
}

module showBox(){
	round_box(l=boxLength,
			  w=boxWidth,
			  h=boxHeight,
			  bt=bottomThickness,
			  wt=wallThickness,
			  lt=lidThickness,
			  r=cornerRadius);
}

module round_box(l=40,w=30,h=30,bt=2,wt=2,lt=2,r=5,){
	difference() { 
		round_cube(l=l,w=w,h=h-lt,r=r);
		translate ([wt, wt, bt]) 
		round_cube(l=l-wt*2,w=w-wt*2,h=h,r=r-wt);
	}
	roundBoxRim();
	translate ([0, 0, -wt]) roundBoxRim();
}

module roundBoxRim(l=boxLength,
				   w=boxWidth,
				   h=boxHeight,
				   et=lidEdgeThickness,
				   r=cornerRadius,
				   wt=wallThickness,
				   lt=lidThickness){
	difference() { 
		translate ([0, 0, h-lt]) 
		round_cube(l=l,w=w,h=lt,r=r);
		translate ([wt+lt,wt+lt-et*2,h-lt-0.1]) 
		round_cube(l=l*2,w=w-2*(wt+lt)+4*et,h=lt+0.2,r=r-wt+lt);

		//subtract out a lid to make the ledge
		translate ([wt, w-wt, h-lt-0.1])
		roundBoxLid(l=l*2,w=w-2*wt,h=lt+0.1,wt=wt,t=lt,et=0.5,r=r-wt,notch=false);
	}
}

module roundBoxLid(l=40,w=30,h=3,wt=2,t=2,et=0.5,r=5,notch=true){
	translate ([l, 0, 0]) 
	rotate (a = [0, 0, 180]) 
	difference(){
		round_cube(l=l,w=w,h=h,t=t,r=r);

		translate ([-1, 0, et]) rotate (a = [45, 0, 0])  cube (size = [l+2, h*2, h*2]); 
		translate ([-1, w, et]) rotate (a = [45, 0, 0])  cube (size = [l+2, h*2, h*2]); 
		translate ([l, -1, et]) rotate (a = [45, 0, 90]) cube (size = [w+2, h*2, h*2]); 
		if (notch==true){
			translate([2,w/2,h+0.001]) thumbNotch(10/2,72,t);
		}
	}
}

module thumbNotch(
	thumbR=12/2,
	angle=72,
	notchHeight=2){

	size=10*thumbR;

	rotate([0,0,90])
	difference(){
		translate([0,
					(thumbR*sin(angle)-notchHeight)/tan(angle),
					 thumbR*sin(angle)-notchHeight])
		rotate([angle,0,0])
		cylinder(r=thumbR,h=size,$fn=30);

		translate([-size,-size,0])
		cube(size*2);
	}
}

module round_cube(l=40,w=30,h=20,r=5,t=0,$fn=30){
	hull(){ 
		translate ([r, r, 0]) cylinder (h = h, r=r);
		translate ([r, w-r, 0]) cylinder (h = h, r=r);
		translate ([l-r,w-r, 0]) cylinder (h = h, r=r);
		translate ([l-r, r, 0]) cylinder (h = h, r=r);
	}
}

module brick(w,l){
  DETAIL_SCALE=0.1; 
  WALL_THICKNESS=16;
  SQUARE_WIDTH=80; 
  SQUARE_HEIGHT=96; 
  PLATE_HEIGHT=32; 
  PEG_RADIUS=24; 
  PEG_HEIGHT=18; 
  ANTI_PEG_RADIUS=32; 

  my_height = PLATE_HEIGHT; 

  scale(DETAIL_SCALE){
    difference(){
      cube([w*(SQUARE_WIDTH*2),l*(SQUARE_WIDTH*2),my_height]);
      translate([WALL_THICKNESS,WALL_THICKNESS,-WALL_THICKNESS]){
        cube([w*(2*SQUARE_WIDTH) - (2*WALL_THICKNESS), l*(2*SQUARE_WIDTH) - (2*WALL_THICKNESS), my_height]);
      }
    }    
    for(i=[0:w-1]){
      for(j=[0:l-1]){
        for(x=[SQUARE_WIDTH*0.5,SQUARE_WIDTH*1.5]) {
          for(y=[SQUARE_WIDTH*0.5, SQUARE_WIDTH*1.5]) {
            translate([(i * SQUARE_WIDTH * 2) + x, (j * SQUARE_WIDTH * 2) + y,my_height]){
              difference(){
                cylinder(h=PEG_HEIGHT, r=PEG_RADIUS);
				 cylinder(h=PEG_HEIGHT+0.1, r=(PEG_RADIUS - WALL_THICKNESS/2));
              }
            }
          }
        }
      }
    }
    for(i=[0:(w-1)*2]){
      for(j=[0:(l-1)*2]){
        translate([SQUARE_WIDTH + (SQUARE_WIDTH * i), SQUARE_WIDTH + (SQUARE_WIDTH * j), 0]){
          difference(){
            cylinder(r=ANTI_PEG_RADIUS, h=my_height);
            translate([0,0,-1]){
              cylinder(r=(ANTI_PEG_RADIUS - (WALL_THICKNESS/2)), h=my_height+0.5);
            }
          }
        }
      }
    }
  }
}