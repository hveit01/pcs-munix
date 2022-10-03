/* Include file to define the possible types of console. 
	If con_type is set to unknown then the kernel will chose
	one according to the which of the following it finds:
	dl port 0, sl port 0, dh port 0 , bit map 0
	Append new devices at the end
*/

#define unknown 0
#define kl 1
#define dz 2
#define dh 3
#define sl 4
#define bitmap 5
#define dl 6
#define icc 7
#define con 8
#define colour 9

int  con_type;
