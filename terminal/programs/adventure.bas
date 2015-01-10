100 let s=0
110 print "You are in an empty room."
120 print "There are dors in each wall."
130 put "You can go n,s,w,e: "
140 input d$
150 if d$=="n" then goto 300
160 if d$=="s" then goto 300
170 if d$=="w" then goto 400
180 if d$=="e" then goto 500
190 print "Sorry, i don't understand you."
200 goto 130
300 if d$=="n" then let s = s + 1
310 if d$=="s" then let s = s - 1
320 if s == 0 then goto 110
330 print "You're walking through a long corridor."
340 print "There are doors to the west and east."
350 goto 130
400 print "You're in the lab of the mad scientist."
410 print "You're dead!"
420 end 
500 print "Hell, yeah! You escaped!"
