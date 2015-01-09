100 print "Guess my number (1..100)!"
110 let n = rn % 100 + 1
120 let i = 0
130 put "Your gess: "
140 input x
150 let i = i + 1
160 if n == x then goto 220
170 if n > x then goto 200
180 print "No, my number is smaller."
190 goto 130
200 print "No, my number is bigger."
210 goto 130
220 print "Congratulations, ", n, " was my number!"
230 print "You needed ", i, " guesses."
