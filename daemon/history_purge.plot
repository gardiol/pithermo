plot  "history.dat" using 1:2 title 'Temp' with linespoint, \
      "history.dat" using 1:4 title 'Humi' with linespoint, \
      "history.dat" using 1:3 title "ExtTemp" with linespoint, \
      "history.dat" using 1:5 title "ExtHumi" with linespoint;
