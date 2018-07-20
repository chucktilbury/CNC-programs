# deparam
CNC post-processor to convert parameters into values.

I had a problem with InkScape in that it would not produce my drawings to proper scale. Also, the CNC controller I am using does not like parameterized g-code. So I wrote this simple filter to change the parameters into values that the controller would accept. I am using a DDCSV2.1 controller, which I like very much except for this one deficiency. 
