# deparam

deparam: CNC post-processor to convert parameters into values.

I had a problem with InkScape in that it would not produce my drawings to proper scale. Also, the CNC controller I am using does not like parameterized g-code. So I wrote this simple filter to change the parameters into values that the controller would accept. I am using a DDCSV2.1 controller, which I like very much except for this one deficiency. 

This code should run on any machine that has python installed.

This program depends on the InkScape g-code format. Other formats, especially ones with named parameters will not work at all. Other formats might have X and Y statements on single lines. Those formats will not work either.

level_bed: I needed a program to cut the sacrificial bed to be level. This is a simple python script to generate the bed level program g-code from parameters. It should work on any g-code interpreter on any operating system. use: "python level_bed.py > file.nc"