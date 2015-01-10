1000 rem Initialization
1010 rem ----------------------------- 
1020 cls
1030 let i = 0
1040 let x = rn % 40
1050 let y = rn % 4
1060 at x, y
1070 write "*"
1080 let i = i + 1
1090 if i < 40 then goto 1040
2000 rem ----------------------------- 
2010 rem Update world
2020 rem ----------------------------- 
2030 let x = 0
2040 let y = 0
3000 rem ----------------------------- 
3010 rem Calcualte neighbours pos
3020 rem ----------------------------- 
3030 let yt = y + 3
3040 let yt = yt % 4
3050 let yb = y + 5
3060 let yb = yb % 4
3070 let xl = x + 39
3080 let xl = xl % 40
3090 let xr = x + 41
3100 let xr = xr % 40
4000 rem ----------------------------- 
4010 rem Calcualte num neighbours
4020 rem ----------------------------- 
4030 let c = 0
4040 at x, yt, a$
4050 if a$ == "*" then let c = c + 1
4060 at xr, yt, a$
4070 if a$ == "*" then let c = c + 1
4080 at xr, y, a$
4090 if a$ == "*" then let c = c + 1
4100 at xr, yb, a$
4110 if a$ == "*" then let c = c + 1
4120 at x, yb, a$
4130 if a$ == "*" then let c = c + 1
4140 at xl, yb, a$
4150 if a$ == "*" then let c = c + 1
4160 at xl, y, a$
4170 if a$ == "*" then let c = c + 1
4180 at xl, yt, a$
4190 if a$ == "*" then let c = c + 1
5000 rem ----------------------------- 
5010 rem Update cell
5020 rem ----------------------------- 
5030 at x, y, a$
5040 at x, y
5050 if a$ == "*" then goto 5080
5060 if c == 3 then write "*"
5070 goto 6000
5080 if c < 2 then write " "
5090 if c == 2 then write "*"
5100 if c == 3 then wrtie "*"
5110 if c > 3 then write " "
6000 rem ----------------------------- 
6010 rem Next cell pos
6020 rem ----------------------------- 
6030 let x = x + 1
6040 if x < 40 then goto 3000
6050 let x = 0
6060 let y = y + 1
6070 if y < 4 then goto 3000
6080 goto 2000
